//Stub for custom BFS implementations

#include "common.h"
#include "aml.h"
#include "csr_reference.h"
#include "bitmap_reference.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

// unsigned long居然是8byte...
#define SET_GLB_VISIT(x) (visited[x >> 6] |= (1ll << (x & 63)))
#define GET_GLB_VISIT(x) (visited[x >> 6] & (1ll << (x & 63)))

// two arrays holding visited VERTEX_LOCALs for current and next level
// we swap pointers each time
int *q1,*q2;
int qc,q2c; //pointer to first free element


//VISITED bitmap parameters
unsigned long *visited;
int64_t visited_size;

//global variables of CSR graph to be used inside of AM-handlers
int64_t *column;
int64_t *pred_glob;
int * rowstarts;
oned_csr_graph g;




void glb_visit_syn(int from, void* data, int sz) {
	sz /= 8;
	unsigned long* vi = data;
	int offset = vi[sz - 1];
	for (int i = 0; i < sz - 1; ++i) {
		visited[i + offset] |= vi[i]; // SIMD??? wrong!!!
	}
}
inline void send_glb_visit() {
	int seg = 1200;
	for (int l = 0; l < visited_size; l += seg) {
		int r = MIN(visited_size - 1, l + seg - 1);
		unsigned long tmp = visited[r + 1];
		visited[r + 1] = l;
		for (int i = 0; i < size; ++i) if (i != rank) {
			aml_send(visited + l, 2, (r - l + 2) * 8, i);
		}
		visited[r + 1] = tmp;
	}
}


//user should provide this function which would be called once to do kernel 1: graph convert
void make_graph_data_structure(const tuple_graph* const tg) {
	//graph conversion, can be changed by user by replacing oned_csr.{c,h} with new graph format 
	convert_graph_to_oned_csr(tg, &g);
	column=g.column;
	rowstarts=g.rowstarts;

	// 直接冲global, 乘个8.
	// printf("glb: %d, local: %d\n", g.nglobalverts, g.nlocalverts);
	visited_size = (g.nglobalverts + ulong_bits - 1) / ulong_bits;
	visited = xmalloc((visited_size + 5)*sizeof(unsigned long));
	//user code to allocate other buffers for bfs
	aml_register_handler(glb_visit_syn,2); // call-back function, 接收到data就调用这个函数.
	q1 = xmalloc(g.nlocalverts*sizeof(int)); //100% of vertexes
	q2 = xmalloc(g.nlocalverts*sizeof(int));
	for(int i = 0; i < g.nlocalverts;i++) q1[i]=0,q2[i]=0; //touch memory
}




//user should provide this function which would be called several times to do kernel 2: breadth first search
//pred[] should be root for root, -1 for unrechable vertices
//prior to calling run_bfs pred is set to -1 by calling clean_pred
void run_bfs(int64_t root, int64_t* pred) {
	pred_glob=pred;
	aml_register_handler(glb_visit_syn,2);
	//user code to do bfs
	long sum;


	// `memset`, 清空visit数组.
	CLEAN_VISITED();

	qc=0; sum=1; q2c=0;
	SET_GLB_VISIT(root);
	if(VERTEX_OWNER(root) == rank) { // 如果root分配给了我这个process.
		pred[VERTEX_LOCAL(root)] = root;
		q1[0] = VERTEX_LOCAL(root);
		qc=1;
	}

	int state = 1; // 维护状态的转化.
	int round = 0;
	while (sum) {
		if (state == 0) { // beginning top-down.

		} else if (state == 1) { // bottom-up.
			for (int i = 0; i < g.nlocalverts; ++i) {
				if (GET_GLB_VISIT(VERTEX_TO_GLOBAL(rank, i))) continue;
				for (int j = rowstarts[i]; j < rowstarts[i+1]; j++) { // 有重边...
					if (GET_GLB_VISIT(COLUMN(j))) { // COLUMN(j)才是另一个点.
						pred_glob[i] = COLUMN(j); // local points' father is j(glb).
						q2[q2c++] = i; // 加入local访问的点.
						break;
					}
				}
			}
			for (int i = 0; i < q2c; ++i) SET_GLB_VISIT(VERTEX_TO_GLOBAL(rank, q2[i]));
			send_glb_visit();
		} else { // ending top-down.
			assert(state == 3);

		}
		aml_barrier(); // 里面必须每一个depth栅栏同步一下.
		qc = q2c, q2c = 0;
		int *tmp = q1; q1 = q2; q2 = tmp;
		sum = qc;
		aml_long_allsum(&sum); // 通信取得这一层的和
	}
	aml_barrier();
}
// mpirun -np 1 ./graph500_custom_bfs 16 16
// mpirun -np 2 ./graph500_custom_bfs 10 16

//we need edge count to calculate teps. Validation will check if this count is correct
//user should change this function if another format (not standart CRS) used
void get_edge_count_for_teps(int64_t* edge_visit_count) {
	long i,j;
	long edge_count=0;
	for(i=0;i<g.nlocalverts;i++)
		if(pred_glob[i]!=-1) {
			for(j=g.rowstarts[i];j<g.rowstarts[i+1];j++)
				if(COLUMN(j)<=VERTEX_TO_GLOBAL(my_pe(),i))
					edge_count++;
		}
	aml_long_allsum(&edge_count);
	*edge_visit_count=edge_count;
}

//user provided function to initialize predecessor array to whatevere value user needs
void clean_pred(int64_t* pred) {
	int i;
	for(i=0;i<g.nlocalverts;i++) pred[i]=-1;
}

//user provided function to be called once graph is no longer needed
void free_graph_data_structure(void) {
	free_oned_csr_graph(&g);
	free(visited);
	free(q1); free(q2);
}

//user should change is function if distribution(and counts) of vertices is changed
size_t get_nlocalverts_for_pred(void) {
	return g.nlocalverts;
}
