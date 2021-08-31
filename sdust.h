#ifndef SDUST_H
#define SDUST_H

#define _POSIX_SOURCE
#include <stdio.h>
#include <zlib.h>

#include "ketopt.h"
#include "kseq.h"
#include "kdq.h"
#include "kvec.h"

#define SDUST_VERSION "0.1-r5"
#define min(x,y) ((x)>(y)?(y):(x))

extern const char *__progname;

#ifdef __cplusplus
extern "C" {
#endif

#define SD_WLEN 3
#define SD_WTOT (1<<(SD_WLEN<<1))
#define SD_WMSK (SD_WTOT - 1)

unsigned char seq_nt4_table[256] = {
	0, 1, 2, 3,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 0, 4, 1,  4, 4, 4, 2,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  3, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4, 
	4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4,  4, 4, 4, 4
};

typedef struct {
	int start, finish;
	int r, l;
} perf_intv_t;

typedef kvec_t(perf_intv_t) perf_intv_v;
typedef kvec_t(uint64_t) uint64_v;

struct sdust_buf_s;
typedef struct sdust_buf_s sdust_buf_t;

// the simple interface
uint64_t *sdust(void *km, const uint8_t *seq, int l_seq, int T, int W, int *n);

// the following interface dramatically reduce heap allocations when sdust is frequently called.
sdust_buf_t *sdust_buf_init(void *km);
void sdust_buf_destroy(sdust_buf_t *buf);
const uint64_t *sdust_core(const uint8_t *seq, int l_seq, int T, int W, int *n, sdust_buf_t *buf);

KDQ_INIT(int)
KSEQ_INIT(gzFile, gzread)

struct sdust_buf_s {
	kdq_t(int) *w;
	perf_intv_v P; // the list of perfect intervals for the current window, sorted by descending start and then by ascending finish
	uint64_v res;  // the result
	void *km;      // memory pool
};

#ifdef __cplusplus
}
#endif

#endif
