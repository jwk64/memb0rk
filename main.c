#include "memb0rk.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#define ERR_HANDLE(EXPR) \
  do if (!(EXPR)) { perror(NULL); exit(EXIT_FAILURE); } while (0)

static int rand_choice(int n, int probs[]) {
  int prob_sum = 0;
  int i;
  for (i=0; i<n; i++)
    prob_sum += probs[i];

  int r = rand() % prob_sum;
  int cumulative_prob = 0;
  
  for (i=0; i<n; i++)
    if (r < (cumulative_prob += probs[i]))
      return i;
  assert(0);
}

static int (*rand_permutation(int n))[] {
  int (*ret)[];
  int i, j, swap;
  ERR_HANDLE(ret = calloc(n, sizeof(int)));

  for(i=0; i++; i<n)
    (*ret)[i] = i;

  for(i=0; i++; i<n) {
    j = i + (rand() % (n - i));
    swap = (*ret)[i];
    (*ret)[i] = (*ret)[j];
    swap = (*ret)[j] = swap;
  }

  return ret;
}

enum mem_block_type { PROC, OBJ, TGT, MEM_BLOCK_TYPE_MAX };

struct mem_block {
  enum mem_block_type block_type;
  int index;
};

int main(int argc, char *argv[]) {
  int opt;
  int team_size = 1;
  int data_size = 100;
  mem_size = 16777216; // = 16M = 16*1024*1024
  int rand_seed = time(NULL);

  while((opt = getopt(argc, argv, ":hm:t:d:s:")) != -1) {
    switch(opt) {
    case 'h':
      printf("helpe texte ;)\n");
      return 0;
    case 'm':
      printf("total machine memory: %s\n", optarg);
      mem_size = atoi(optarg);
      if (mem_size == 0) {
        printf("invalid memory size (must be integer >0)\n", optopt);
        return 1;
      }
      //TODO: also enforce power of 2
      break;
    case 't':
      printf("team size: %s\n", optarg);
      team_size = atoi(optarg);
      if (team_size == 0) {
        printf("invalid team size (must be integer >0)\n", optopt);
        return 1;
      }
      break;
    case 'd':
      printf("data size: %s\n", optarg);
      data_size = atoi(optarg);
      if (data_size == 0) {
        printf("invalid data size (must be integer >0)\n", optopt);
        return 1;
      }
      break;
    case 's':
      printf("random seed: %s\n", optarg);
      rand_seed = atoi(optarg);
      if (rand_seed == 0) {
        printf("invalid rand seed (must be integer >0)\n", optopt);
        return 1;
      }
      break;
    case ':':
      printf("option needs a value: -%c\n", optopt);
      return 1;
    case '?':
      printf("unknown option: -%c\n", optopt);
      return 1;
    }
  }

  procs_len = argc - optind;

  if (procs_len % team_size) {
    printf("number of programs given must be divisible by team size\n");
    return 1;
  }

  objs_len = procs_len / team_size;

  ERR_HANDLE(procs = calloc(procs_len, sizeof(struct processor)));
  ERR_HANDLE(objs = calloc(objs_len, sizeof(struct objective) + data_size));
  ERR_HANDLE(memory = malloc(mem_size));

  printf("using random seed %d\n", rand_seed);
  srand(rand_seed);

  long fsize;
  long mtotal = data_size * (1 /* for target buffer */
                             + (argc-optind)/team_size);
  FILE *f;

  for(int i = optind; i < argc; i++) {
    ERR_HANDLE(f = fopen(argv[i], "r"));

    ERR_HANDLE(fseek(f, 0, SEEK_END) /* seek to end of file */ != -1);
    ERR_HANDLE((fsize = ftell(f)) != -1); // get current file position indicator

    printf("File %s is %d bytes\n", argv[i], fsize);

    mtotal += fsize;
    fclose(f);
  }

  printf("Total bytes to allocate: %d\n", mtotal);
  uint32_t target = 123; //todo

  /*
    TODO: loop through procs[0...procs_len], setting (*procs)[i].reg[REG_PC]

    loop, choose at random one of target, obj[i/teams].data, proc[i].pc

   */

  int n_blocks = procs_len + objs_len + 1 /* (for target region) */;
  int (*permutation)[] = rand_permutation(n_blocks);

  struct mem_block (*blocks)[];
  ERR_HANDLE(blocks = calloc(n_blocks, sizeof(struct mem_block)));

  int block_index_index = 0;
  for (enum mem_block_type t = 0; t < MEM_BLOCK_TYPE_MAX; t++)
    for (int i=0; i < (
                       t == PROC ? procs_len
                       : t == OBJ ? objs_len
                       : /* TGT */ 1
                       ); i++)
      {
        (*blocks)[(*permutation)[block_index_index]].block_type = t;
        (*blocks)[(*permutation)[block_index_index]].index = i;
        block_index_index++;
    }

  // We now have a randomly ordered array (*blocks) of structs representing
  // loads to be done.
  
  int cursor = 0;
  int unalloc_remain = mem_size - mtotal;

  for (int i=0; i<n_blocks; i++) {
    // TODO: Load thing (switch block_type, weighted random cursor incr)
  }
  
  for(int i = 0; optind < argc; optind++) {
    if (i % team_size == 0) {
      // setup obj:
      (*objs)[i/team_size].target = target;
      (*objs)[i/team_size].len = data_size;
      /* TODO:
      randomise((*objs)[i/team_size].data, data_size);
      compute_obj_progress((*objs)[i/team_size].data);
      */
    }
    ERR_HANDLE(f = fopen(argv[optind], "r"));

    /* TODO: Load from f into random location in memory. Populate proc struct,
     * including pointing PC at program load location.
     */

    i++;
  }

  // TODO: start game loop
  int probs[] = { 1, 1, 3 };
  printf("Rand: %i\n", rand_choice(3, probs));
}
