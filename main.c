#include "memb0rk.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#define ERR_HANDLE(EXPR) \
  do if (!(EXPR)) { perror(NULL); exit(EXIT_FAILURE); } while (0)

#ifdef unused
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
#endif // unused

static uint32_t rand32() {
  uint32_t ret = 0;

  // rand() is only guarunteed to generate numbers up to 2^15.

  for (int i=0; i<4; i++)
    ret |= (rand() & 255) << (8*i);

  return ret;
}

int comp_uint32 (const void *ap, const void *bp) {
    uint32_t a = *((uint32_t *) ap);
    uint32_t b = *((uint32_t *) bp);
    if (a > b)
      return  1;
    if (a < b)
      return -1;
    return 0;
}

static int (*rand_permutation(int n))[] {
  int (*ret)[];
  int i, j, swap;
  ERR_HANDLE(ret = calloc(n, sizeof(int)));

  for(i=0; i<n; i++)
    (*ret)[i] = i;

  for(i=0; i<n; i++) {
    j = i + (rand() % (n - i));
    swap = (*ret)[i];
    (*ret)[i] = (*ret)[j];
    swap = (*ret)[j] = swap;
  }
  
  return ret;
}

int main(int argc, char *argv[]) {
  int opt;
  int team_size = 1;
  int data_size = 100;
  mem_size = 16777216; // = 16M = 16*1024*1024
  int rand_seed = time(NULL);

  // Parse options:
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
  // The following represents the total amount of mem to be used by programs,
  // target/source obj data etc.
  long mtotal = data_size * (1 /* for target buffer */
                             + (argc-optind)/team_size);
  FILE *f;

  // Determining program lengths:
  for(int i = optind; i < argc; i++) {
    ERR_HANDLE(f = fopen(argv[i], "r"));

    ERR_HANDLE(fseek(f, 0, SEEK_END) /* seek to end of file */ != -1);
    ERR_HANDLE((fsize = ftell(f)) != -1); // get current file position indicator

    printf("File %s is %d bytes\n", argv[i], fsize);

    mtotal += fsize + (3 * WORD_LEN /* for the 3 arguments that must be passed
                                       to the program for the memcpy */);
    fclose(f);
  }

  printf("Total bytes to allocate: %d\n", mtotal);

  uint32_t gaps_total = mem_size - mtotal;
  int n_blocks = procs_len + objs_len + 1 /* (for target region) */;

  // Generate randomised gaps between allocated blocks:
  uint32_t (*cuts)[];
  ERR_HANDLE(cuts = calloc(n_blocks-1, sizeof(uint32_t)));
  for (int i=0; i<n_blocks-1; i++)
    (*cuts)[i] = rand32() % (gaps_total);

  qsort(cuts, n_blocks-1, sizeof(uint32_t), &comp_uint32);

  // Randomise location of first block:
  uint32_t cursor = rand32() % mem_size;

  // Randomise order of blocks:
  int (*permutation)[] = rand_permutation(n_blocks);

  for (int i=0; i<n_blocks; i++) {
    int j = (*permutation)[i];

    // Select location
    if (i==1)
      cursor += (*cuts)[0];

    else if (i>1)
      // No need to %mem_size here, since that'll need to be done anyway on load
      // (just because first address is in range doesn't mean we won't
      // overflow):
      cursor += (*cuts)[i-1] - (*cuts)[i-2];

    if (j < procs_len) {
      // alloc prog j
      cursor += 3 * WORD_LEN; // space for arguments

      (*procs)[j].reg[REG_PC] = cursor;
      
      ERR_HANDLE(f = fopen(argv[optind + j], "r"));
      while (!feof(f)) {
        (*memory)[cursor % mem_size] = getc(f);
        cursor++;
      }

      fclose(f);
    }
    else if ((j -= procs_len) < objs_len) {
      // alloc src j

      (*objs)[i].src = cursor;
      
      // generating random source data:
      for (int k=0; k<data_size; k++) {
        (*memory)[cursor % mem_size]
          = (*objs)[i].data[k]
          = rand();
        cursor++;
      }
    }
    else {
      assert((j -= objs_len) == 0);
      // alloc tgt

      // set target pointer (and len while we're at it) in all objs
      for (int k = 0; k < objs_len; k++) {
        (*objs)[k].target = cursor;
        (*objs)[k].len = data_size;
      }
    }
  }

  free(cuts);
  free(permutation);

  // Write in arguments for programs:
  for (int i = 0; i < procs_len; i++) {
    cursor = (*procs)[i].reg[REG_PC] -WORD_LEN * 3;
    mem_write(cursor, (*objs)[i/team_size].target);
    cursor += WORD_LEN;
    mem_write(cursor, (*objs)[i/team_size].src);
    cursor += WORD_LEN;
    mem_write(cursor, (*objs)[i/team_size].len);
  }

  long long max_steps = 1000000; //TODO
  printf("Starting game\n");
  int result = game(max_steps);
  printf("Finished game\n");

  if (result == -1)
    printf("No winners after %i steps\n", max_steps);
  
  else for (int i = 0; i < team_size; i++)
    printf("Winner: %s\n", argv[optind + result * team_size + i]);
}
