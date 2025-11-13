#include "memb0rk.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#define ERR_HANDLE(EXPR) \
  do if (!(EXPR)) { perror(NULL); exit(EXIT_FAILURE); } while (0)

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
  int rand_seed = time(NULL);

  // Parse options:
  while((opt = getopt(argc, argv, ":hm:t:d:s:")) != -1) {
    switch(opt) {
    case 'h':
      printf("helpe texte ;)\n");
      return 0;
    case 't':
      printf("team size: %s\n", optarg);
      team_size = atoi(optarg);
      if (team_size == 0) {
        printf("invalid team size (must be integer >0)\n", optopt);
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
  ERR_HANDLE(objs = calloc(objs_len, sizeof(struct objective)));
  ERR_HANDLE(targets = calloc(objs_len, TGT_SIZE));

  printf("using random seed %d\n", rand_seed);
  srand(rand_seed);

  long fsize;
  // The following represents the total amount of mem to be used by programs,
  // target/source obj data etc.
  long mtotal = TGT_SIZE * (1 /* for target buffer */
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

  uint32_t gaps_total = MEM_SIZE - mtotal;
  int n_blocks = procs_len + objs_len; // note this does not include target region

  // Generate randomised gaps between allocated blocks:
  uint32_t (*gaps)[];
  ERR_HANDLE(gaps = calloc(n_blocks+1 /* we need a leading and trailing gap,
                                         hence +1 */, sizeof(uint32_t)));
  for (int i=0; i<n_blocks; i++)
    (*gaps)[i] = rand32() % (gaps_total);

  qsort(gaps, n_blocks, sizeof(uint32_t), &comp_uint32);

  // replace with differences:
  uint32_t orig_last = (*gaps)[n_blocks-1];
  for (int i=n_blocks-1; i>0; i--)
    (*gaps)[i] -= (*gaps)[i-1];
  (*gaps)[0] += (gaps_total - orig_last);

  // Target is 0...TGT_SIZE, so we start cursor here:
  uint32_t cursor = TGT_SIZE;

  // Randomise order of blocks:
  int (*permutation)[] = rand_permutation(n_blocks);

  for (int i=0; i<n_blocks; i++) {
    int j = (*permutation)[i];

    // Select location
    cursor += (*gaps)[i];

    if (j < procs_len) {
      // alloc prog j
      cursor += 1 * WORD_LEN; // space for argument

      (*procs)[j].reg[REG_PC] = cursor;
      
      ERR_HANDLE(f = fopen(argv[optind + j], "r"));

      char c = getc(f);;
      while (!feof(f))
       {
         memory[cursor % MEM_SIZE] = c;
         c = getc(f);
         cursor++;
      }

      fclose(f);
    }
    else if ((j -= procs_len) < objs_len) {
      // alloc src j

      (*objs)[j].src = cursor;
      
      // generating random source data:
      for (int k=0; k<TGT_SIZE; k++) {
        memory[cursor % MEM_SIZE]
          = (*targets)[j][k]
          = rand();
        if ((*targets)[j][k] == '\0') // Wrong way to do this, but for now...
          (*objs)[j].progress++;
        cursor++;
      }
    }
    else {
      assert((j -= objs_len) == 0);
      // alloc tgt
    }
  }

  free(gaps);
  free(permutation);

  // Write in arguments for programs:
  for (int i = 0; i < procs_len; i++) {
    cursor = (*procs)[i].reg[REG_PC] - WORD_LEN;
    mem_write(cursor, (*objs)[i/team_size].src);
  }

  long long max_steps = 1000000; //TODO
  printf("Starting game\n");
  int result = game(max_steps);
  printf("Finished game\n");
  
  if (result == -1) {
    printf("No winners after %i steps\n", max_steps);
    printf("Mem beginning: %s\n", memory); // TODO: this, but as hex?
  }
  
  else for (int i = 0; i < team_size; i++)
    printf("Winner: %s\n", argv[optind + result * team_size + i]);
}
