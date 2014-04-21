//  Created by Emil Ernerfeldt on 2014-04-17.

#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cmath>
#include <numeric> // iota

using namespace std;
using namespace chrono;

using Clock = chrono::steady_clock;

using Int = uint64_t;

struct Node {
	Int payload; // ignored; just for plausability.
	Node* next = nullptr;
};

static_assert(sizeof(Node) == 16, "Not 64-bit? That's OK too.");


// Returns nanoseconds per element.
double time(Int N, Int iters) {
	// Allocate all the memory continuously so we aren't affected by the particulars of the system allocator:
	vector<Node> memory(N);

	// Initialize the node pointers:
	vector<Node*> nodes(N);
	for (Int i=0; i<N; ++i) {
		nodes[i] = &memory[i];
	}
	//std::iota(begin(nodes), end(nodes), memory.data());

	// Randomize so emulate a list that has been shuffled around a bit.
	// This is so that each memory acces is a *random* memory access.
	// This is the worst case scenario for a linked list, which is exactly what we want to measure.
	// Without the random_shuffle we get O(N) because it enables the pre-fetcher to do its job.
	// Without a prefetcher, a random memory access in N bytes of RAM costs O(N^0.5) due to the Bekenstein bound.
	// This means we get O(N^1.5) complexity of a linked list traversal.
	random_shuffle(begin(nodes), end(nodes));

	// Link up the nodes:
	for (Int i=0; i<N-1; ++i) {
		nodes[i]->next = nodes[i+1];
	}

	Node* start_node = nodes[0];

	nodes.clear(); nodes.shrink_to_fit(); // Free up unused memory before meassuring:

	// Do the actual measurements:

	auto start = Clock::now();

	for (Int it=0; it<iters; ++it) {
		// Run through all the nodes:
		Node* node = start_node;
		while (node) {
			node = node->next;
		}
	}

	auto dur = Clock::now() - start;
	auto ns = duration_cast<nanoseconds>(dur).count();

	return ns / double(N * iters);
}


int main(int argc, const char * argv[])
{
	// Outputs data in gnuplot friendly .data format
	cout << "#bytes    ns/elem" << endl;

	try {
		Int stopsPerFactor = 4; // For every power of 2, how many measurements do we do?
		Int minElemensFactor = 6;  // First measurement is 2^this number of elements.
		Int maxElemsFactor = 30; // Last measurement is 2^this number of elements. 30 == 16GB of memory
		//Int elemsPerMeasure = Int(1) << 28; // measure enough times to process this many elements (to get a good average)

		Int min = stopsPerFactor * minElemensFactor;
		Int max = stopsPerFactor * maxElemsFactor;

		for (Int ei=min; ei<=max; ++ei) {
			Int N = (Int)round(pow(2.0, double(ei) / stopsPerFactor));
			//Int reps = elemsPerMeasure / N;
			Int reps = (Int)round(2e10 / pow(N, 1.5));
			if (reps<1) reps = 1;
			auto ans = time(N, reps);
			cout << (N*sizeof(Node)) << "   " << ans << "   # (N=" << N << ", reps=" << reps << ") " << (ei-min+1) << "/" << (max-min+1) << endl;
		}
	} catch (exception& e) {
		cout << "# stopped due to exception: " << e.what() << endl;
	}
}
