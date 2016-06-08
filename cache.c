#include <stdlib.h>
#include <stdio.h>
#include "trace_item.h"
#include "skeleton.h"

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;

// to keep statistics
unsigned int accesses = 0;
unsigned int read_accesses = 0;
unsigned int write_accesses = 0;
unsigned int hits = 0;
unsigned int misses = 0;
unsigned int misses_with_writeback = 0;

// Check if x is power of 2
char is_power_of_two(int x){
  // A number n is a power of two if the bitwise AND of n and n - 1 equals 0
  // First x in the below expression is for the case when x is 0
  return x && (!(x&(x-1)));
}

void trace_init(){
  trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE);

  if (!trace_buf) {
    fprintf(stdout, "** trace_buf not allocated\n");
    exit(-1);
  }

  trace_buf_ptr = 0;
  trace_buf_end = 0;
}

void trace_uninit(){
  free(trace_buf);
  fclose(trace_fd);
}

int trace_get_item(struct trace_item **item){
  int n_items;

  if (trace_buf_ptr == trace_buf_end) {
    // get new data
    n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
    if (!n_items) return 0;

    trace_buf_ptr = 0;
    trace_buf_end = n_items;
  }

  *item = &trace_buf[trace_buf_ptr];
  trace_buf_ptr++;

  return 1;
}

int main(int argc, char **argv){
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on, cache_size, block_size, cache_sets, replacement_policy;
  unsigned long long int cycle_count = 0;

  if (argc != 7) {
    fprintf(stdout, "\nUSAGE: cache <trace_file> <trace_view_on> <cache size (power of 2)> <block size (power of 2)> <cache associativity (power of 2)> <replacement policy 0 => LRU 1 => FIFO\n");
    exit(0);
  }

  trace_file_name    = argv[1];
  trace_view_on      = atoi(argv[2]);
  cache_size         = atoi(argv[3]);
  block_size         = atoi(argv[4]);
  cache_sets         = atoi(argv[5]);
  replacement_policy = atoi(argv[6]);

  // Input validations
  if (!is_power_of_two(cache_size)) {
    fprintf(stdout, "Cache size must be a power of 2");
    exit(1);
  }

  if (!is_power_of_two(block_size)){
    fprintf(stdout, "Block size must be a power of 2");
    exit(1);
  }

  if (!is_power_of_two(cache_sets)) {
    fprintf(stdout, "Associtivity must be a power of 2");
    exit(1);
  }

  if (trace_view_on != 0 && trace_view_on != 1){
    fprintf(stdout, "Trace view must be on (1) or off (0)");
    exit(1);
  }

  if (replacement_policy != 0 && replacement_policy != 1){
    fprintf(stdout, "Replacement policy must be LRU (0) or FIFO (1)");
    exit(1);
  }

  fprintf(stdout, "\n ** opening file %s ** \n\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();
  struct cache_t *cp = cache_create(cache_size, block_size, cache_sets, replacement_policy);

  while (1){
    size = trace_get_item(&tr_entry);
    int result = -1;

    if (!size) {       /* no more instructions to simulate */
      printf("+ number of accesses : %d \n", accesses);
      printf("+ number of reads : %d \n", read_accesses);
      printf("+ number of writes : %d \n", write_accesses);
      printf("+ number of hits : %d \n", hits);
      printf("+ number of misses : %d \n", misses);
      printf("+ number of misses with write back : %d \n", misses_with_writeback);
      break;
    }

    else{
      if (tr_entry->type == ti_LOAD){
        if (trace_view_on)
          printf("LOAD %x\n",tr_entry->Addr);
        accesses++;
        read_accesses++;
        result = cache_access(cp, tr_entry->Addr, 0 , cycle_count);

        switch (result){
          case 0:
            hits++;
            break;
          case 1:
            misses++;
            break;
          case 2:
            misses_with_writeback++;
            break;
          default:
            fprintf(stdout, "\n%s%s\n" , "** Return of cache_access invalid. ",
                            "Data integrity compromised. **\n");
            exit(1);
        }
      }

      if (tr_entry->type == ti_STORE){
        if (trace_view_on)
          printf("STORE %x\n",tr_entry->Addr);
        accesses++;
        write_accesses++;
        result = cache_access(cp, tr_entry->Addr, 1, cycle_count);

        switch (result){
          case 0:
            hits++;
            break;
          case 1:
            misses++;
            break;
          case 2:
            misses_with_writeback++;
            break;
          default:
            fprintf(stdout, "\n%s%s\n" , "** Return of cache_access invalid. ",
                            "Data integrity compromised. **\n");
            exit(1);
        }
      }
    }
    cycle_count++;
  }

  trace_uninit();

  exit(0);
}
