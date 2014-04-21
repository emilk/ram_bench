//  Created by Emil Ernerfeldt on 2014-04-17.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

typedef uint64_t Int;

typedef struct Node Node;

struct Node {
	Int payload; // ignored; just for plausability.
	Node* next;
};

void random_shuffle(Node** list, Int N)
{
	for (Int i=0; i<N-1; ++i) {
		Int swap_ix = i + rand() % (N-i);
		Node* tmp = list[swap_ix];
		list[swap_ix] = list[i];
		list[i] = tmp;
	}
}

// Returns nanoseconds per element.
double bench(Int N, Int iters) {
	// Allocate all the memory continuously so we aren't affected by the particulars of the system allocator:
	Node* memory = (Node*)malloc(N * sizeof(Node));

	// Initialize the node pointers:
	Node** nodes = (Node**)malloc(N * sizeof(Node*));
	for (Int i=0; i<N; ++i) {
		nodes[i] = &memory[i];
	}

	// Randomize so emulate a list that has been shuffled around a bit.
	// This is so that each memory acces is a *random* memory access.
	// This is the worst case scenario for a linked list, which is exactly what we want to measure.
	// Without the random_shuffle we get O(N) because it enables the pre-fetcher to do its job.
	// Without a prefetcher, a random memory access in N bytes of RAM costs O(N^0.5) due to the Bekenstein bound.
	// This means we get O(N^1.5) complexity of a linked list traversal.
	random_shuffle(nodes, N);

	// Link up the nodes:
	for (Int i=0; i<N-1; ++i) {
		nodes[i]->next = nodes[i+1];
	}
	nodes[N-1]->next = NULL;

	Node* start_node = nodes[0];

	free(nodes); // Free up unused memory before meassuring:

	// Do the actual measurements:

	Int start = clock();

	for (Int it=0; it<iters; ++it) {
		// Run through all the nodes:
		Node* node = start_node;
		while (node) {
			node = node->next;
		}
	}

	Int dur = clock() - start;
	double ns = 1e9 * dur / CLOCKS_PER_SEC;

	return ns / (N * iters);
}


int main(int argc, const char * argv[])
{
	// Outputs data in gnuplot friendly .data format
	printf("#bytes    ns/elem\n");

	Int stopsPerFactor = 4; // For every power of 2, how many measurements do we do?
	Int minElemensFactor = 6;  // First measurement is 2^this number of elements.
	Int maxElemsFactor = 30; // Last measurement is 2^this number of elements. 30 == 16GB of memory
	//Int elemsPerMeasure = Int(1) << 28; // measure enough times to process this many elements (to get a good average)

	Int min = stopsPerFactor * minElemensFactor;
	Int max = stopsPerFactor * maxElemsFactor;

	for (Int ei=min; ei<=max; ++ei) {
		Int N = (Int)floor(pow(2.0, (double)ei / stopsPerFactor) + 0.5);
		//Int reps = elemsPerMeasure / N;
		Int reps = (Int)floor(1e9 / pow(N, 1.5) + 0.5);
		if (reps<1) reps = 1;
		double ans = bench(N, reps);
		printf("%llu   %f   # (N=%llu, reps=%llu) %llu/%llu\n", N*sizeof(Node), ans, N, reps, ei-min+1, max-min+1);
	}
}
