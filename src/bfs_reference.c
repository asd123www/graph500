/* Copyright (c) 2011-2017 Graph500 Steering Committee
   All rights reserved.
   Developed by:		Anton Korzh anton@korzh.us
				Graph500 Steering Committee
				http://www.graph500.org
   New code under University of Illinois/NCSA Open Source License
   see license.txt or https://opensource.org/licenses/NCSA
*/

// Graph500: Kernel 2: BFS
// Simple level-synchronized BFS with visits as Active Messages

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

#ifdef DEBUGSTATS
extern int64_t nbytes_sent,nbytes_rcvd;
#endif
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

// convert之后保存的该process需要的图的信息, 包括localnode个数???????
oned_csr_graph g;

// 每一次visit需要发送的信息.
typedef struct visitmsg {
	//both vertexes are VERTEX_LOCAL components as we know src and dest PEs to reconstruct VERTEX_GLOBAL
	int vloc;
	int vfrom;
} visitmsg;

// 统计一下边访问的
int null_edge_visit;
int valid_edge_visit;


//AM-handler for check&visit
void visithndl(int from,void* data,int sz) {
	visitmsg *m = data;
	++null_edge_visit; // test
	if (!TEST_VISITEDLOC(m->vloc)) {
		++ valid_edge_visit; // test
		SET_VISITEDLOC(m->vloc);
		q2[q2c++] = m->vloc;
		pred_glob[m->vloc] = VERTEX_TO_GLOBAL(from,m->vfrom);
	}
}

inline void send_visit(int64_t glob, int from) {
	visitmsg m = {VERTEX_LOCAL(glob),from};
	aml_send(&m,1,sizeof(visitmsg),VERTEX_OWNER(glob));
}

// 输入生成的testing graph, 让我们对input进行转换.
void make_graph_data_structure(const tuple_graph* const tg) {
	int i,j,k;
	convert_graph_to_oned_csr(tg, &g); // 我应该也得调这个... 接口有点麻烦.
	column=g.column;
	rowstarts=g.rowstarts;

	// 这里g.nlocalverts似乎就是 total_verts / n_process.
	printf("process: %d vertical: %d edge: %d\n", rank, g.nlocalverts, g.nlocaledges);

	visited_size = (g.nlocalverts + ulong_bits - 1) / ulong_bits;
	aml_register_handler(visithndl,1); // call-back function, 接收到data就调用这个函数.
	q1 = xmalloc(g.nlocalverts*sizeof(int)); //100% of vertexes
	q2 = xmalloc(g.nlocalverts*sizeof(int));
	for(i=0;i<g.nlocalverts;i++) q1[i]=0,q2[i]=0; //touch memory
	visited = xmalloc(visited_size*sizeof(unsigned long));
}

void run_bfs(int64_t root, int64_t* pred) {
	int64_t nvisited;
	long sum;
	unsigned int i,j,k,lvl=1;
	pred_glob=pred;
	aml_register_handler(visithndl,1); // 第二个参数是type, 我们可以区分不同的message类型AML调用不同的call-back函数.

	// `memset`, 清空visit数组.
	CLEAN_VISITED();

	qc=0; sum=1; q2c=0;

	nvisited=1; // only `root`.
	if(VERTEX_OWNER(root) == rank) { // 如果root分配给了我这个process.
		pred[VERTEX_LOCAL(root)]=root;
		SET_VISITED(root);
		q1[0]=VERTEX_LOCAL(root);
		qc=1;
	}

	// While there are vertices in current level

	int round = 0;

	while(sum) {

		double start_time = aml_time();
		null_edge_visit = 0;
		valid_edge_visit = 0;


#ifdef DEBUGSTATS
		double t0=aml_time();
		nbytes_sent=0; nbytes_rcvd=0;
#endif
		//for all vertices in current level send visit AMs to all neighbours
		for(i=0;i<qc;i++) // 这里访存我是不是能用SIMD优化一下?
			for(j=rowstarts[q1[i]];j<rowstarts[q1[i]+1];j++) // csr存储的前缀.
				send_visit(COLUMN(j),q1[i]); // 这里的边访问直接调用进程间通信? 太恐怖了卧槽.
		aml_barrier(); // 里面必须每一个depth栅栏同步一下.

		qc=q2c;
		int *tmp=q1;q1=q2;q2=tmp;
		
		sum=qc;
		aml_long_allsum(&sum); // 通信取得这一层的和
		aml_long_allsum(&null_edge_visit);
		aml_long_allsum(&valid_edge_visit);

		nvisited+=sum;
		
		// 输出一些统计信息.
		if(rank == 0) {
			printf("Round: %d, pts: %d, time: %.3f\n", round++, sum, aml_time() - start_time);
			printf("      valid/all: %d/%d \n", valid_edge_visit, null_edge_visit);
		}

		q2c=0;
#ifdef DEBUGSTATS
		aml_long_allsum(&nbytes_sent);
		t0-=aml_time();
		if(!my_pe()) printf (" --lvl%d : %lld(%lld,%3.2f) visited in %5.2fs, network aggr %5.2fGb/s\n",lvl++,sum,nvisited,((double)nvisited/(double)g.notisolated)*100.0,-t0,-(double)nbytes_sent*8.0/(1.e9*t0));
#endif
	}
	aml_barrier();

}



// 助教说不用管.
//we need edge count to calculate teps. Validation will check if this count is correct
void get_edge_count_for_teps(int64_t* edge_visit_count) {
	long i,j;
	long edge_count=0;
	for(i=0;i<g.nlocalverts;i++)
		if(pred_glob[i]!=-1) {
			for(j=rowstarts[i];j<rowstarts[i+1];j++)
				if(COLUMN(j)<=VERTEX_TO_GLOBAL(my_pe(),i))
					edge_count++;

		}

	aml_long_allsum(&edge_count);
	*edge_visit_count=edge_count;
}

void clean_pred(int64_t* pred) {
	int i;
	for(i=0;i<g.nlocalverts;i++) pred[i]=-1;
}
void free_graph_data_structure(void) {
	int i; 
	free_oned_csr_graph(&g);
	free(q1); free(q2); free(visited);
}

size_t get_nlocalverts_for_pred(void) {
	return g.nlocalverts;
}
