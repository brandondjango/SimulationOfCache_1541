#ifndef __SKELETON_H__
#define __SKELETON_H__

///////////////////////////////////////////////////////////////////////////////
//
// CS 1541 Introduction to Computer Architecture
// Use this skeleton code to create a cache instance and implement cache operations.
// Feel free to add new variables in the structure if needed.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>

struct cache_blk_t {
  unsigned long tag;
  char valid;
  char dirty;
  unsigned long long last_time;
  unsigned long long first_time;
};

enum cache_policy {
  LRU,
  FIFO
};

struct cache_t {
  int nsets;    // # sets
  int bsize;    // block size
  int assoc;    // associativity

  enum cache_policy policy;       // cache replacement policy
  struct cache_blk_t **blocks;    // cache blocks in the cache
};

struct cache_t * cache_create(int size, int blocksize, int assoc, enum cache_policy policy){
  int i;
  int nblocks = 1; // number of blocks in the cache
  int nsets = 1;   // number of sets (entries) in the cache

  nblocks = (size*1024) / blocksize;
  nsets = nblocks / assoc;

  struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

  C->nsets = nsets;
  C->bsize = blocksize;
  C->assoc = assoc;
  C->policy = policy;

  C->blocks = (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t));

  for(i = 0; i < nsets; i++) {
    C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
  }
  return C;
}

long calc_index(unsigned long address, int nsets, int blocksize){
//  printf("address %ld nsets %d blocksize %d", address, nsets, blocksize);
  return (address / blocksize) % nsets;
}

unsigned long calc_tag(unsigned long address, int nsets, int blocksize){
  return (address / blocksize) / nsets;
}

int detect_hit(struct cache_t *cp, unsigned long req_tag, long req_index,
               char access_type, unsigned long long now){
  int i;
//  printf("req_index %ld req_tag %lu\n", req_index, req_tag);
  for(i = 0; i < cp->assoc; i++){
    if(cp->blocks[req_index][i].tag == req_tag && cp->blocks[req_index][i].valid == 1){
      if(cp->policy == 0)
        cp->blocks[req_index][i].last_time = now;
      if(access_type == 1){
        cp->blocks[req_index][i].dirty = 1;
      }
      return 1;
    }
  }
  return 0;
}

int calc_LRU(struct cache_t *cp, long req_index){
  int LRU;
  unsigned long long temp_time;
  int i;

  temp_time = cp->blocks[req_index][0].last_time;
  LRU = 0;

  for(i = 0; i < cp->assoc; i++){
    if(cp->blocks[req_index][i].last_time < temp_time){
      temp_time = cp->blocks[req_index][i].last_time;
      LRU = i;
    }
  }
  return LRU;
}

int calc_FIFO(struct cache_t *cp, long req_index){
  int FIFO;
  unsigned long long temp_time;
  int i;

  temp_time = cp->blocks[req_index][0].first_time;
  FIFO = 0;

  for(i = 0; i < cp->assoc; i++){
    if(cp->blocks[req_index][i].first_time < temp_time){
      temp_time = cp->blocks[req_index][i].first_time;
      FIFO = i;
    }
  }
  return FIFO;
}

int detect_miss(struct cache_t *cp, unsigned long req_tag, long req_index,
                char access_type, unsigned long long now){
  int i;
  int replacement_index;
  int return_value;
  for(i = 0; i < cp->assoc; i++){
    if(cp->blocks[req_index][i].valid == 0){
      cp->blocks[req_index][i].tag = req_tag;
      cp->blocks[req_index][i].last_time = now;
      cp->blocks[req_index][i].first_time = now;
      cp->blocks[req_index][i].valid = 1;
      cp->blocks[req_index][i].dirty = 1;
      return 1;
    }
  }

  if(cp->policy == 0)
    replacement_index = calc_LRU(cp, req_index);
  else
    replacement_index = calc_FIFO(cp, req_index);

  if( cp->blocks[req_index][replacement_index].valid == 1){
    if(cp->blocks[req_index][replacement_index].dirty == 0){
      cp->blocks[req_index][replacement_index].tag = req_tag;
      cp->blocks[req_index][replacement_index].last_time = now;
      cp->blocks[req_index][replacement_index].first_time = now;
      cp->blocks[req_index][replacement_index].valid = 1;
      cp->blocks[req_index][replacement_index].dirty = 1;
      return 1;
    }
    else if(cp->blocks[req_index][replacement_index].dirty == 1){
      cp->blocks[req_index][replacement_index].tag = req_tag;
      cp->blocks[req_index][replacement_index].last_time = now;
      cp->blocks[req_index][replacement_index].first_time = now;
      cp->blocks[req_index][replacement_index].valid = 1;
      cp->blocks[req_index][replacement_index].dirty = 1;
      return 2;
    }
  }
  return 0;
}

// return 0 if a hit, 1 if a miss or 2 if a miss_with_write_back
int cache_access(struct cache_t *cp, unsigned long address,
                 char access_type, unsigned long long now){
  int i;
  int detected_miss;
  long requested_index;
  unsigned long requested_tag;
  struct cache_blk_t temp;


  requested_index = calc_index(address, cp->nsets, cp->bsize);
  requested_tag = calc_tag(address, cp->nsets, cp->bsize);
//  printf("req_index %ld req_tag %lu\n", requested_index, requested_tag);

  if(detect_hit(cp, requested_tag, requested_index, access_type, now) == 1){
    return 0;
  }

  detected_miss = detect_miss(cp, requested_tag, requested_index, access_type, now);

  if(detected_miss != 0){
    return detected_miss;
  }

  fprintf(stderr, "\nCRITICAL ERROR\n");
  return 0;
}

#endif
