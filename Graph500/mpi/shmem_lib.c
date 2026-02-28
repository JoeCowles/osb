/* Copyright (C) 2014  Oak Ridge National Laboratory.                      */
/*                                                                         */
/*  Authors: Ed D'Azevedo                                                  */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "shmem_lib.h"

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : (-(x)))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef ULONG_BOR
#define ULONG_BOR(x, y) (((unsigned long)(x)) | ((unsigned long)(y)))
#endif

#ifndef UINT_BOR
#define UINT_BOR(x, y) (((unsigned int)(x)) | ((unsigned int)(y)))
#endif

#ifndef ULONG_BAND
#define ULONG_BAND(x, y) (((unsigned long)(x)) & ((unsigned long)(y)))
#endif

#ifndef UINT_BAND
#define UINT_BAND(x, y) (((unsigned int)(x)) & ((unsigned int)(y)))
#endif

#ifndef FALSE
#define FALSE (0 == 1)
#endif

#ifndef TRUE
#define TRUE (1 == 1)
#endif

/*
 * -----------------------------------------
 * return elapse time, similar to MPI_Wtime()
 * -----------------------------------------
 */
double shmem_wtime() {
#ifdef USE_CLOCK
  /*
   *  The clock() function returns an approximation of processor time used by
   * the program. The  value  returned  is  the CPU time used so far as a
   * clock_t
   */
  clock_t uptime = clock();
  return (((double)uptime) / ((double)CLOCKS_PER_SEC));
#else
#ifdef USE_MPI_WTIME
  extern double MPI_Wtime(void);

  return (MPI_Wtime());

#else
#include <sys/time.h>

  /* Fall back to gettimeofday() if we have nothing else */
  double wtime = 0;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  wtime = tv.tv_sec;
  wtime += (double)tv.tv_usec / 1000000.0;
  return (wtime);
#endif

#endif
}

void shmem_long_min(long *gvar, long value, int pe) {
  long cval = 0;
  long lval = 0;
  int is_done = 0;

  assert((0 <= pe) && (pe < shmem_n_pes()));
  assert(shmem_pe_accessible(pe));
  assert(shmem_addr_accessible(gvar, pe));

  lval = shmem_long_atomic_fetch_add(gvar, (long)0, pe);
  if (value < lval) {
    do {
      cval = shmem_long_atomic_compare_swap(gvar, lval, MIN(lval, value), pe);
      is_done = (cval == lval) || (cval <= value);
      lval = cval;
    } while (!is_done);
  };
}

void shmem_long_max(long *gvar, long value, int pe) {
  long cval = 0;
  long lval = 0;
  int is_done = 0;

  assert((0 <= pe) && (pe < shmem_n_pes()));

  lval = shmem_long_atomic_fetch_add(gvar, (long)0, pe);
  if (value > lval) {
    do {
      cval = shmem_long_atomic_compare_swap(gvar, lval, MAX(lval, value), pe);
      is_done = (cval == lval) || (cval >= value);
      lval = cval;
    } while (!is_done);
  };
}

void shmem_ulong_bor(unsigned long *gvar, unsigned long value, int pe) {
  /*
   * perform Bitwise OR
   */
  long cval = 0;
  long lval = 0;
  unsigned long ulval = 0;
  unsigned long new_ulval = 0;
  long new_lval = 0;
  int is_done = 0;

  assert((0 <= pe) && (pe < shmem_n_pes()));
  assert(shmem_pe_accessible(pe));
  assert(shmem_addr_accessible(gvar, pe));

  lval = shmem_long_atomic_fetch_add((long *)gvar, (long)0, pe);

  do {

    memcpy(&ulval, &lval, sizeof(ulval));
    new_ulval = ulval | value;
    if (new_ulval == ulval) {
      /* bits cleared already */
      break;
    };

    memcpy(&new_lval, &new_ulval, sizeof(new_lval));

    cval = shmem_long_atomic_compare_swap((long *)gvar, lval, new_lval, pe);
    is_done = (cval == lval);
    lval = cval;

  } while (!is_done);
}

void shmem_int_min(int *gvar, int value, int pe) {
  int cval = 0;
  int lval = 0;
  int is_done = 0;

  assert((0 <= pe) && (pe < shmem_n_pes()));

  lval = shmem_int_atomic_fetch_add(gvar, (int)0, pe);
  if (value < lval) {
    do {
      is_done = (lval <= value);
      if (!is_done) {
        cval = shmem_int_atomic_compare_swap(gvar, lval, MIN(lval, value), pe);
        is_done = (cval == lval) || (cval <= value);
        lval = cval;
      }
    } while (!is_done);
  };
}

void shmem_int_max(int *gvar, int value, int pe) {
  int cval = 0;
  int lval = 0;
  int is_done = 0;

  assert((0 <= pe) && (pe < shmem_n_pes()));

  lval = shmem_int_atomic_fetch_add(gvar, (int)0, pe);
  if (value > lval) {
    do {
      is_done = (lval >= value);
      if (!is_done) {
        cval = shmem_int_atomic_compare_swap(gvar, lval, MAX(lval, value), pe);
        is_done = (cval == lval) || (cval >= value);
        lval = cval;
      }
    } while (!is_done);
  };
}

void shmem_uint_bor(int *gvar, unsigned int value, int pe) {
  /*
   * perform Bitwise OR
   */
  int cval = 0;
  unsigned int ulval = 0;
  int new_lval = 0;
  int lval = 0;
  int is_done = 0;

  assert((0 <= pe) && (pe < shmem_n_pes()));

  lval = shmem_int_atomic_fetch_add(gvar, (int)0, pe);
  do {
    ulval = UINT_BOR(lval, value);
    memcpy(&new_lval, &ulval, sizeof(lval));
    cval = shmem_int_atomic_compare_swap(gvar, lval, new_lval, pe);
    is_done = (cval == lval);
    lval = cval;

  } while (!is_done);
}

/*
 * ----------------------------------------
 * find the sum value across all processors
 * Uses SOS-1.5 team reduction API.
 * ----------------------------------------
 */
long shmem_long_sum_all(long lvalue_in) {
  static long gvalue = 0;
  static long lvalue = 0;

  lvalue = lvalue_in;
  gvalue = lvalue_in;

  shmem_barrier_all();
  shmem_long_sum_reduce(SHMEM_TEAM_WORLD, &gvalue, &lvalue, 1);
  shmem_barrier_all();

#ifdef USE_DEBUG
  fprintf(stderr, "shmem_long_sum_all: rank %d before %ld final %ld\n",
          shmem_my_pe(), lvalue_in, gvalue);
  fflush(stderr);
#endif

  return (gvalue);
}

/*
 * ----------------------------------------
 * find the max value across all processors
 * Uses SOS-1.5 team reduction API.
 * ----------------------------------------
 */
long shmem_long_max_all(long lvalue_in) {
  static long gvalue = 0;
  static long lvalue = 0;

  lvalue = lvalue_in;
  gvalue = lvalue_in;

  shmem_barrier_all();
  shmem_long_max_reduce(SHMEM_TEAM_WORLD, &gvalue, &lvalue, 1);
  shmem_barrier_all();

#ifdef USE_DEBUG2
  fprintf(stderr, "shmem_long_max_all: rank %d before %ld after %ld\n",
          shmem_my_pe(), lvalue_in, gvalue);
  fflush(stderr);
#endif

  assert(gvalue >= lvalue);
  return (gvalue);
}

/*
 * ----------------------------------------
 * find the max value across all processors
 * Uses SOS-1.5 team reduction API.
 * ----------------------------------------
 */
long double shmem_longdouble_max_all(long double lvalue_in) {
  static long double gvalue = 0;
  static long double lvalue = 0;

  lvalue = lvalue_in;
  gvalue = lvalue_in;

  shmem_barrier_all();
  shmem_longdouble_max_reduce(SHMEM_TEAM_WORLD, &gvalue, &lvalue, 1);
  shmem_barrier_all();

  assert(gvalue >= lvalue);
  return (gvalue);
}

/*
 * ----------------------------------------
 * find the min value across all processors
 * Uses SOS-1.5 team reduction API.
 * ----------------------------------------
 */
long shmem_long_min_all(long lvalue_in) {
  static long gvalue = 0;
  static long lvalue = 0;

  lvalue = lvalue_in;
  gvalue = lvalue_in;

  shmem_barrier_all();
  shmem_long_min_reduce(SHMEM_TEAM_WORLD, &gvalue, &lvalue, 1);
  shmem_barrier_all();

  assert(gvalue <= lvalue);
  return (gvalue);
}

/*
 * ----------------------------------------
 * find the bitwise or value across all processors
 * Uses SOS-1.5 team reduction API.
 * ----------------------------------------
 */
long shmem_long_or_all(long lvalue_in) {
  static long gvalue = 0;
  static long lvalue = 0;

  lvalue = lvalue_in;
  gvalue = lvalue_in;

  shmem_barrier_all();
  shmem_long_or_reduce(SHMEM_TEAM_WORLD, &gvalue, &lvalue, 1);
  shmem_barrier_all();

  return (gvalue);
}

/*
 * ----------------------------------------
 * find the bitwise and value across all processors
 * Uses SOS-1.5 team reduction API.
 * ----------------------------------------
 */
long shmem_long_and_all(long lvalue_in) {
  static long gvalue = 0;
  static long lvalue = 0;

  lvalue = lvalue_in;
  gvalue = lvalue_in;

  shmem_barrier_all();
  shmem_long_and_reduce(SHMEM_TEAM_WORLD, &gvalue, &lvalue, 1);
  shmem_barrier_all();

  return (gvalue);
}

int shmem_addr_accessible_all(void *ptr) {
  int pe = 0;
  int npes = shmem_n_pes();

  int is_accessible_all;

  is_accessible_all = TRUE;
  for (pe = 0; (pe < npes); pe++) {
    is_accessible_all = is_accessible_all && shmem_pe_accessible(pe) &&
                        shmem_addr_accessible(ptr, pe);
  };

  return (is_accessible_all);
}

/*
 * -------------------------------------------
 * perform bitwise OR reduction across all processors
 * thin interface to call shmem_int_or_to_all()
 * -------------------------------------------
 */

int shmem_int_or_all(int lvalue) {
  static int gvalue = 0;
  static int inval = 0;

  inval = lvalue;
  gvalue = lvalue;

  shmem_barrier_all();
  shmem_int_or_reduce(SHMEM_TEAM_WORLD, &gvalue, &inval, 1);
  shmem_barrier_all();

#ifdef USE_DEBUG2
  fprintf(stderr, "shmem_int_or_all: rank %d before %d after %d\n",
          shmem_my_pe(), lvalue, gvalue);
  fflush(stderr);
#endif

  return gvalue;
}

/*
 * ----------------------
 * perform the logical OR
 * across all processors
 * ----------------------
 */
int shmem_int_lor_all(int lvalue) {
  int gvalue = 0;

  shmem_barrier_all();
  gvalue = (shmem_int_or_all((lvalue != 0)) != 0);
  shmem_barrier_all();

#ifdef USE_DEBUG2
  fprintf(stderr, "shmem_int_lor_all: rank %d before %d after %d\n",
          shmem_my_pe(), lvalue, gvalue);
  fflush(stderr);
#endif

  return (gvalue);
}

/*
 * -------------------------------------------
 * perform bitwise AND reduction across all processors
 * thin interface to call shmem_int_and_to_all()
 * -------------------------------------------
 */

int shmem_int_and_all(int lvalue) {
  static int gvalue = 0;
  static int inval = 0;

  inval = lvalue;
  gvalue = lvalue;

  shmem_barrier_all();
  shmem_int_and_reduce(SHMEM_TEAM_WORLD, &gvalue, &inval, 1);
  shmem_barrier_all();

  return gvalue;
}

/*
 * -------------------------------------------
 * perform logical AND reduction across all processors
 * thin interface to call shmem_int_and_to_all()
 * -------------------------------------------
 */
int shmem_int_land_all(int lvalue) {
  int gvalue = ((shmem_int_and_all((lvalue != 0)) != 0));
#ifdef USE_DEBUG2
  fprintf(stderr, "shmem_int_land_all: rank %d before %d final %d\n",
          shmem_my_pe(), lvalue, gvalue);
  fflush(stderr);
#endif

  return (gvalue);
}

/*
 * ----------------------------------------------------------------------
 * each processor send data to all other processors (including self)
 * each processor receive data from all other processors (including self)
 * ----------------------------------------------------------------------
 */
void g500_shmem_int_alltoall(int *sendbuf, int *recvbuf) {
  int *source = 0;
  int *target = 0;
  unsigned int n_pes = (unsigned int)shmem_n_pes();
  unsigned int my_pe = (unsigned int)shmem_my_pe();
  unsigned int pe = 0;

  size_t nbytes = sizeof(int);
  nbytes *= n_pes;
  source = (int *)shmem_malloc(nbytes);
  assert(source != NULL);

  nbytes = sizeof(int);
  nbytes *= n_pes;
  target = (int *)shmem_malloc(nbytes);
  assert(target != NULL);

  for (pe = 0; pe < n_pes; pe++) {
    source[pe] = sendbuf[pe];
  };

  shmem_barrier_all();

  for (pe = 0; pe < n_pes; pe++) {
    size_t len = 1;
    shmem_int_put(&(target[my_pe]), &(source[pe]), len, (int)pe);
  };

  shmem_barrier_all();

  for (pe = 0; pe < n_pes; pe++) {
    recvbuf[pe] = target[pe];
  };

  shmem_barrier_all();

  shmem_free(source);
  shmem_free(target);
}

/*
 * ----------------------------------------------------------------------
 * each processor send data to all other processors (including self)
 * each processor receive data from all other processors (including self)
 * ----------------------------------------------------------------------
 */
void g500_shmem_long_alltoall(long *sendbuf, long *recvbuf) {
  long *source = 0;
  long *target = 0;
  unsigned int n_pes = (unsigned int)shmem_n_pes();
  unsigned int my_pe = (unsigned int)shmem_my_pe();
  unsigned int pe = 0;

  source = (long *)shmem_malloc(sizeof(int) * n_pes);
  assert(source != NULL);

  target = (long *)shmem_malloc(sizeof(int) * n_pes);
  assert(target != NULL);

  for (pe = 0; pe < n_pes; pe++) {
    source[pe] = sendbuf[pe];
  };

  shmem_barrier_all();

  for (pe = 0; pe < n_pes; pe++) {
    size_t len = 1;
    shmem_long_put(&(target[my_pe]), &(source[pe]), len, (int)pe);
  };

  shmem_barrier_all();

  for (pe = 0; pe < n_pes; pe++) {
    recvbuf[pe] = target[pe];
  };

  shmem_barrier_all();

  shmem_free(source);
  shmem_free(target);
}

/*
 * ----------------------------------------------------
 * perform equivalent of MPI_Alltoallv but all in bytes
 * ----------------------------------------------------
 */
void shmem_mem_alltoallv(void *sendbuf_in, int *sendcounts, int *sdispls,
                         void *recvbuf_in, int *recvcounts, int *rdispls,
                         size_t size_in_bytes) {

  char *sendbuf = (char *)sendbuf_in;
  char *recvbuf = (char *)recvbuf_in;
  char *target = NULL;

  unsigned int n_pes = (unsigned int)shmem_n_pes();
  unsigned int my_pe = (unsigned int)shmem_my_pe();

  unsigned long total_recvcounts = 0;
  unsigned long umax_recvcounts = 0;
  long max_recvcounts = 0;
  unsigned int pe = 0;

  const size_t max_memory = 1024 * 1024 * 128;

  assert(sendbuf != NULL);
  assert(recvbuf != NULL);
  assert(sdispls != NULL);
  assert(rdispls != NULL);
  assert(sendcounts != NULL);
  assert(recvcounts != NULL);

  /*
   * -----------------
   * compute local max
   * -----------------
   */
  umax_recvcounts = 0;
  for (pe = 0; pe < n_pes; pe++) {
    unsigned long len = (unsigned long)recvcounts[pe];
    umax_recvcounts = MAX(umax_recvcounts, len);
  };

  max_recvcounts = (long)umax_recvcounts;
  assert(max_recvcounts >= 0);

  max_recvcounts = shmem_long_max_all(max_recvcounts);
  umax_recvcounts = (unsigned long)max_recvcounts;

  total_recvcounts = n_pes * umax_recvcounts;

  long nb = MAX(1, ((max_memory / size_in_bytes) / n_pes));
  long ntimes = (max_recvcounts + (nb - 1)) / nb;
  long itime = 0;

#ifdef USE_DEBUG
  if (my_pe == 0) {
    printf("nb %ld umax_recvcounts %ld ntimes %ld\n", (long)nb,
           (long)umax_recvcounts, (long)ntimes);
  };
#endif

  target = (char *)shmem_malloc(
      MIN(nb * n_pes * size_in_bytes, total_recvcounts * size_in_bytes));
  if (target == NULL) {
    printf("pe %d: total_recvcounts %ld size_in_bytes %ld\n", shmem_my_pe(),
           total_recvcounts, size_in_bytes);
  };
  assert(target != NULL);

  /*
   * -------------------
   * perform remote copy
   * -------------------
   */
  for (itime = 1; itime <= ntimes; itime += 1) {

    shmem_barrier_all();

    for (pe = 0; pe < n_pes; pe++) {
      int ipe = (int)pe;

      size_t ioff = ((size_t)sdispls[ipe]) + (itime - 1) * nb;

      long jstart = 1 + (itime - 1) * nb;
      long jend = MIN(jstart + nb - 1, sendcounts[ipe]);
      long jsize = jend - jstart + 1;
      int has_work = (jsize > 0);

      size_t len = (size_t)jsize;
      size_t istart = my_pe * nb;

      len *= size_in_bytes;
      ioff *= size_in_bytes;
      istart *= size_in_bytes;

      if (has_work) {
        shmem_putmem(&(target[istart]), &(sendbuf[ioff]), len, (int)pe);
      };
    };

    shmem_barrier_all();

    /*
     * ---------------------------------------
     * copy from local shmem buffer to recvbuf
     * ---------------------------------------
     */
    for (pe = 0; pe < n_pes; pe++) {
      int ipe = (int)pe;
      long jstart = 1 + (itime - 1) * nb;
      long jend = MIN(jstart + nb - 1, recvcounts[ipe]);
      long jsize = jend - jstart + 1;
      int has_work = (jsize > 0);

      size_t len = (size_t)jsize;
      size_t ioff = ((size_t)rdispls[ipe]) + (itime - 1) * nb;
      size_t istart = pe * nb;

      len *= size_in_bytes;
      ioff *= size_in_bytes;
      istart *= size_in_bytes;
      if (has_work) {
        memcpy(&(recvbuf[ioff]), &(target[istart]), len);
      };
    };

  }; /* for (itime) */

  shmem_barrier_all();
  shmem_free(target);
}

unsigned long shmem_ulong_bor_all(unsigned long val_in) {
  /*
   * -------------------------------------------------
   * compute the bitwise or operation on unsigned long
   * -------------------------------------------------
   */
  unsigned int n_pes = (unsigned int)shmem_n_pes();
  unsigned int my_pe = (unsigned int)shmem_my_pe();

  static unsigned long source;
  static unsigned long target;
  static unsigned long result;

  target = 0;
  source = val_in;
  result = val_in;

  shmem_barrier_all();
  if (my_pe == 0) {
    /*
     * ---------------------------
     * perform computation on pe 0
     * ---------------------------
     */
    unsigned int pe;
    for (pe = 1; pe < n_pes; pe++) {
      shmem_getmem(&target, &source, sizeof(unsigned long), (int)pe);
      result |= target;
    };

    /*
     * ----------------
     * put results back
     * ----------------
     */
    for (pe = 1; pe < n_pes; pe++) {
      shmem_putmem(&result, &result, sizeof(unsigned long), (int)pe);
    };
  };
  shmem_barrier_all();
  return (result);
}

unsigned long shmem_ulong_max_all(unsigned long val_in) {
  /*
   * -------------------------------------------------
   * compute the MAX operation on unsigned long
   * -------------------------------------------------
   */
  const int use_longdouble = (sizeof(long double) > sizeof(unsigned long));
  unsigned long final_result = 0;

  if (use_longdouble) {
    final_result =
        ((unsigned long)shmem_longdouble_max_all((long double)val_in));
  } else {

    unsigned int n_pes = (unsigned int)shmem_n_pes();
    unsigned int my_pe = (unsigned int)shmem_my_pe();

    static unsigned long source;
    static unsigned long target;
    static unsigned long result;

    target = 0;
    source = val_in;
    result = val_in;

    shmem_barrier_all();
    if (my_pe == 0) {
      /*
       * ---------------------------
       * perform computation on pe 0
       * ---------------------------
       */
      unsigned int pe;
      for (pe = 1; pe < n_pes; pe++) {
        shmem_getmem(&target, &source, sizeof(unsigned long), (int)pe);
        result = MAX(result, target);
      };

      /*
       * ----------------
       * put results back
       * ----------------
       */
      for (pe = 1; pe < n_pes; pe++) {
        shmem_putmem(&result, &result, sizeof(unsigned long), (int)pe);
      };
    };
    shmem_barrier_all();
    final_result = (result);
  };
#ifdef USE_DEBUG
  printf("shmem_ulong_max_all: rank %d  initial %lu final_result %lu\n",
         shmem_my_pe(), val_in, final_result);
#endif

  return (final_result);
}
