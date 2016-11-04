
#include <stdlib.h>
#include <stdio.h>

unsigned int L1size = 16;
unsigned int bsize = 32;
unsigned int L1assoc = 4;
unsigned int L2size = 0;
unsigned int L2assoc = 0;
unsigned int L2_hit_latency = 6;
unsigned int mem_latency = 100;

struct cache_blk_t {
	unsigned long tag;
	char valid;
	char dirty;
	unsigned long long ts;	//a timestamp that may be used to implement LRU replacement
	// To guarantee that L2 is inclusive of L1, you may need an additional flag
	// in L2 to indicate that the block is cached in L1
};

struct cache_t {
	// The cache is represented by a 2-D array of blocks.
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
	int nsets;				// # sets
	int blocksize;			// block size
	int assoc;				// associativity
	int hit_latency;			// latency in case of a hit
	struct cache_blk_t **blocks;		// the array of cache blocks
};

struct cache_t *
cache_create(int size, int blocksize, int assoc, int latency)
{
	int i;
	int nblocks = 1;			// number of blocks in the cache
	int nsets = 1;			// number of sets (entries) in the cache

	// YOUR JOB: calculate the number of sets and blocks in the cache
	//
	nblocks = size/blocksize;
	nsets = nblocks/assoc;

	struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

	C->nsets = nsets;
	C->assoc = assoc;
	C->hit_latency = latency;

	C->blocks = (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t));

	for (i = 0; i < nsets; i++) {
		C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
	}

	return C;
}

int cache_access(struct cache_t *cp, unsigned long address,
				 char access_type, unsigned long long now, struct cache_t *next_cp)
{
	//
	// Based on address, determine the set to access in cp and examine the blocks
	// in the set to check hit/miss and update the golbal hit/miss statistics
	// If a miss, determine the victim in the set to replace (LRU). Replacement for
	// L2 blocks should observe the inclusion property.
	//
	// The function should return the hit_latency in case of a hit. In case
	// of a miss, you need to figure out a way to return the time it takes to service
	// the request, which includes writing back the replaced block, if dirty, and bringing
	// the requested block from the lower level (from L2 in case of L1, and from memory in case of L2).
	// This will require one or two calls to cache_access(L2, ...) for misses in L1.
	// It is recommended to start implementing and testing a system with only L1 (no L2). Then add the
	// complexity of an L2.
	// return(cp->hit_latency);

	int latency = cp->hit_latency;

	int idx = get_cache_idx(cp, address);
	int tag = get_address_tag(cp, address);

	// determine if address is in cache
	struct cache_blk_t block = NULL;
	for (int i = 0; i < cp->assoc; ++i) {
		if(cp->blocks[idx][i] == tag) {
			// hit
			block = cp->blocks[idx][i];
			break;
		}
	} 

	// if it's not, then pull address into cache
	if (block == NULL) {
		// determine if a block needs to be kicked out (and which one)
		struct cache_blk_t kick = NULL;
		unsigned long long min_ts = NULL;
		for (int i = 0; i < cp->assoc; ++i) {
			if (cp->block[idx][i].valid == 0) {
				kick = NULL;
				break;
			}
			if (min_ts == NULL || cp->block[idx][i].ts < min_ts) {
				min_ts = cp->block[idx][i].ts;
				kick = cp->block[idx][i];
			}
		}

		// if needed, write back to memory
		if (kick != NULL && kick.dirty == 1) {
			latency += mem_latency;
		}
		
		// if needed, kick out address with LRU
		if (kick != NULL) {
			kick.valid = 0;
		}

		// find empty block
		for (int i = 0; i < cp->assoc; ++i) {
			if(cp->blocks[idx][i].valid == 0) {
				block = cp->blocks[idx][i];
				break;
			}
		}

		// pull block into cache from mem
		latency += mem_latency;
		block.tag = tag;
		block.valid = 1;
		block.ts = now;
	}

	if (access_type == "w") {
		// mark dirty
		block->dirty = 1;
		// set timestamp
		block->ts = now;
	}

	return latency;
}

int get_cache_idx(struct cache_t *cp, unsigned long address) {
	return address%(cp->nsets);
}

int get_address_tag(struct cache_t *cp, unsigned long address) {
	return address/nsets;
}
