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

#include <immintrin.h>

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

typedef struct visitmsg {
	//both vertexes are VERTEX_LOCAL components as we know src and dest PEs to reconstruct VERTEX_GLOBAL
	int vloc;
	int vfrom;
} visitmsg;


// variable for state machine.
double alpha = 20, beta = 100;


// handler for `top-down` sending edge.
// !!! cause we change the `local visited` to global, minor changes needed in reference top-down.
void visithndl(int from,void* data,int sz) {
	visitmsg *m = data;
	// (u, v) means u -> v. u is the predecessor.
	int64_t u = VERTEX_TO_GLOBAL(from, m -> vfrom);
	int64_t v = VERTEX_TO_GLOBAL(rank, m -> vloc);
	if (!GET_GLB_VISIT(v)) {
		SET_GLB_VISIT(v);
		q2[q2c++] = m->vloc;
	 	pred_glob[m->vloc] = u;
	}
}
inline void send_visit(int64_t glob, int from) {
	visitmsg m = {VERTEX_LOCAL(glob), from};
	aml_send(&m, 1, sizeof(visitmsg), VERTEX_OWNER(glob));
}

// handler for `bottom-up` sending bitset.
void glb_visit_syn(int from, void* data, int sz) {
	sz /= 8;
	unsigned long* vi = data;
	int offset = vi[sz - 1];
	int i = 0;

	// unsigned long addr1 = vi;
	// unsigned long addr2 = visited + offset;
	// if ((addr1 & 0x1Full) == (addr2 & 0x1Full)) {
	// 	for (; addr1 & 0x1Full;) {
	// 		visited[i + offset] |= vi[i];
	// 		++i;
	// 		addr1 = vi + i;
	// 		addr2 = visited + offset + i;
	// 	}
	// 	for (; i + 4 < sz - 1; i += 4) {
	// 		_mm256_storeu_si256(visited + offset + i,
	// 							_mm256_or_si256(_mm256_load_si256(vi + i), _mm256_load_si256(visited + offset + i)));
	// 	}
	// }

	for (; i < sz - 1; ++i) visited[i + offset] |= vi[i]; // SIMD??? wrong!!!
}
inline void send_glb_visit() {
	int seg = 1023;
	for (int l = 0; l < visited_size; l += seg) {
		int r = MIN(visited_size - 1, l + seg - 1);
		unsigned long tmp = visited[r + 1];
		visited[r + 1] = l;
		for (int i = 0; i < size; ++i) if (i != rank) {
			aml_send(visited + l, 2, (r - l + 2) * 8, i);
		}
		visited[r + 1] = tmp;
	}
	// printf("in bottom-up\n");
	aml_barrier(); // 里面必须每一个depth栅栏同步一下.
}


//user should provide this function which would be called once to do kernel 1: graph convert
void make_graph_data_structure(const tuple_graph* const tg) {
	//graph conversion, can be changed by user by replacing oned_csr.{c,h} with new graph format 
	convert_graph_to_oned_csr(tg, &g);
	column=g.column;
	rowstarts=g.rowstarts;

	// 直接冲global, 乘个8.
	visited_size = (g.nglobalverts + ulong_bits - 1) / ulong_bits;
	visited = xmalloc((visited_size + 5)*sizeof(unsigned long));
	//user code to allocate other buffers for bfs
	aml_register_handler(visithndl,1);
	aml_register_handler(glb_visit_syn,2); // call-back function, 接收到data就调用这个函数.
	q1 = xmalloc(g.nlocalverts*sizeof(int)); //100% of vertexes
	q2 = xmalloc(g.nlocalverts*sizeof(int));
	for(int i = 0; i < g.nlocalverts;i++) q1[i]=0,q2[i]=0; //touch memory
}



void top_down_naive() {
	for(int i = 0; i < qc; i++)
		for(int j = rowstarts[q1[i]]; j < rowstarts[q1[i]+1]; j++)
			send_visit(COLUMN(j), q1[i]);
	// printf("in top-down\n");
	aml_barrier();
}

void bottom_up_naive() {
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
}

//user should provide this function which would be called several times to do kernel 2: breadth first search
//pred[] should be root for root, -1 for unrechable vertices
//prior to calling run_bfs pred is set to -1 by calling clean_pred
void run_bfs(int64_t root, int64_t* pred) {
	pred_glob=pred;
	aml_register_handler(visithndl,1);
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

	int state = 0; // 维护状态的转化.
	int round = 0;
	while (sum) {

		double start_time = aml_time();

		if (state == 0) { // beginning top-down.
			top_down_naive();
		} else if (state == 1) { // bottom-up.
			send_glb_visit();
			// if you want to use pure bottom-up, just set `state = 1` and don't change.
			bottom_up_naive();
		} else { // ending top-down.
			assert(state == 2);
			top_down_naive();
		}
		qc = q2c, q2c = 0;
		int *tmp = q1; q1 = q2; q2 = tmp;
		sum = qc;
		aml_long_allsum(&sum); // 通信取得这一层的和


		if(rank == 0) {
			printf("Round: %d, state: %d, time: %.3f\n", round++, state, aml_time() - start_time);
		}

		// switch_state
		if (state == 0) {
			// syn the parameter for unify state.
			int64_t front_edge = 0;
			int64_t last_edge = 0;
			for (int i = 0; i < qc; ++i) front_edge += rowstarts[q1[i] + 1] - rowstarts[q1[i]];
			for (int i = 0; i < g.nlocalverts; ++i) {
				if (!GET_GLB_VISIT(VERTEX_TO_GLOBAL(rank, i))) last_edge += rowstarts[i + 1] - rowstarts[i];
			}
			aml_long_allsum(&front_edge);
			aml_long_allsum(&last_edge);
			state = front_edge * alpha > last_edge? 1: 0;
		} else if (state == 1) {
			state = sum * beta < g.nglobalverts? 2: 1;
		}
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
