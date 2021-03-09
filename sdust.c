#include "sdust.h"

sdust_buf_t *sdust_buf_init(void *km)
{
	sdust_buf_t *buf;
	buf = (sdust_buf_t*)kcalloc(km, 1, sizeof(sdust_buf_t));
	buf->km = km;
	buf->w = kdq_init(int, buf->km);
	kdq_resize(int, buf->w, 8);
	return buf;
}

void sdust_buf_destroy(sdust_buf_t *buf)
{
	if (buf == 0) return;
	kdq_destroy(int, buf->w);
	kfree(buf->km, buf->P.a); kfree(buf->km, buf->res.a); kfree(buf->km, buf);
}

static inline void shift_window(int t, kdq_t(int) *w, int T, int W, int *L, int *rw, int *rv, int *cw, int *cv)
{
	int s;
	if ((int)kdq_size(w) >= W - SD_WLEN + 1) { // TODO: is this right for SD_WLEN!=3?
		s = *kdq_shift(int, w);
		*rw -= --cw[s];
		if (*L > (int)kdq_size(w))
			--*L, *rv -= --cv[s];
	}
	kdq_push(int, w, t);
	++*L;
	*rw += cw[t]++;
	*rv += cv[t]++;
	if (cv[t] * 10 > T<<1) {
		do {
			s = kdq_at(w, kdq_size(w) - *L);
			*rv -= --cv[s];
			--*L;
		} while (s != t);
	}
}

static inline void save_masked_regions(void *km, uint64_v *res, perf_intv_v *P, int start)
{
	int i, saved = 0;
	perf_intv_t *p;
	if (P->n == 0 || P->a[P->n - 1].start >= start) return;
	p = &P->a[P->n - 1];
	if (res->n) {
		int s = res->a[res->n - 1]>>32, f = (uint32_t)res->a[res->n - 1];
		if (p->start <= f) // if overlapping with or adjacent to the previous interval
			saved = 1, res->a[res->n - 1] = (uint64_t)s<<32 | (f > p->finish? f : p->finish);
	}
	if (!saved) kv_push(uint64_t, km, *res, (uint64_t)p->start<<32|p->finish);
	for (i = P->n - 1; i >= 0 && P->a[i].start < start; --i); // remove perfect intervals that have falled out of the window
	P->n = i + 1;
}

static void find_perfect(void *km, perf_intv_v *P, const kdq_t(int) *w, int T, int start, int L, int rv, const int *cv)
{
	int c[SD_WTOT], r = rv, i, max_r = 0, max_l = 0;
	memcpy(c, cv, SD_WTOT * sizeof(int));
	for (i = (long)kdq_size(w) - L - 1; i >= 0; --i) {
		int j, t = kdq_at(w, i), new_r, new_l;
		r += c[t]++;
		new_r = r, new_l = kdq_size(w) - i - 1;
		if (new_r * 10 > T * new_l) {
			for (j = 0; j < (int)P->n && P->a[j].start >= i + start; ++j) { // find insertion position
				perf_intv_t *p = &P->a[j];
				if (max_r == 0 || p->r * max_l > max_r * p->l)
					max_r = p->r, max_l = p->l;
			}
			if (max_r == 0 || new_r * max_l >= max_r * new_l) { // then insert
				max_r = new_r, max_l = new_l;
				if (P->n == P->m) kv_resize(perf_intv_t, km, *P, P->n + 1);
				memmove(&P->a[j+1], &P->a[j], (P->n - j) * sizeof(perf_intv_t)); // make room
				++P->n;
				P->a[j].start = i + start, P->a[j].finish = kdq_size(w) + (SD_WLEN - 1) + start;
				P->a[j].r = new_r, P->a[j].l = new_l;
			}
		}
	}
}

const uint64_t *sdust_core(const uint8_t *seq, int l_seq, int T, int W, int *n, sdust_buf_t *buf)
{
	int rv = 0, rw = 0, L = 0, cv[SD_WTOT], cw[SD_WTOT];
	int i, start, l; // _start_: start of the current window; _l_: length of a contiguous A/C/G/T (sub)sequence
	unsigned t; // current word

	buf->P.n = buf->res.n = 0;
	buf->w->front = buf->w->count = 0;
	memset(cv, 0, SD_WTOT * sizeof(int));
	memset(cw, 0, SD_WTOT * sizeof(int));
	if (l_seq < 0) l_seq = strlen((const char*)seq);
	for (i = l = t = 0; i <= l_seq; ++i) {
		int b = i < l_seq? seq_nt4_table[seq[i]] : 4;
		if (b < 4) { // an A/C/G/T base
			++l, t = (t<<2 | b) & SD_WMSK;
			if (l >= SD_WLEN) { // we have seen a word
				start = (l - W > 0? l - W : 0) + (i + 1 - l); // set the start of the current window
				save_masked_regions(buf->km, &buf->res, &buf->P, start); // save intervals falling out of the current window?
				shift_window(t, buf->w, T, W, &L, &rw, &rv, cw, cv);
				if (rw * 10 > L * T)
					find_perfect(buf->km, &buf->P, buf->w, T, start, L, rv, cv);
			}
		} else { // N or the end of sequence; N effectively breaks input into pieces of independent sequences
			start = (l - W + 1 > 0? l - W + 1 : 0) + (i + 1 - l);
			while (buf->P.n) save_masked_regions(buf->km, &buf->res, &buf->P, start++); // clear up unsaved perfect intervals
			l = t = 0;
		}
	}
	*n = buf->res.n;
	return buf->res.a;
}

uint64_t *sdust(void *km, const uint8_t *seq, int l_seq, int T, int W, int *n)
{
	uint64_t *ret;
	sdust_buf_t *buf;
	buf = sdust_buf_init(km);
	ret = (uint64_t*)sdust_core(seq, l_seq, T, W, n, buf);
	buf->res.a = 0;
	sdust_buf_destroy(buf);
	return ret;
}

void usage(const char *a, const int W, const int T)
{
	fprintf(stderr, "\nUsage: \033[1;31m%s\033[0;0m \033[2m[options]\033[0m <in.fa>\n\n", a);
	fprintf(stderr, "  -w [INT]  Dust window length [%d]\n", W);
	fprintf(stderr, "  -t [INT]  Dust level (score threshold for subwindows) [%d]\n", T);
	fprintf(stderr, "  -m [CHAR] Mask dusted sequences (-d) with character X or N [X]\n");
	fprintf(stderr, "  -c [INT]  Column wrapping number (no wrapping)\n");
	fprintf(stderr, "  -d        Output sequences instead of dust bed intervals)\n\n");
	fprintf(stderr, "  -h        Display this help\n");
	fprintf(stderr, "  -v        Show program version\n\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	gzFile fp;
	kseq_t *ks;
	int W = 64, T = 20, c, d = 0, m = 0, wrap = 0;
	ketopt_t o = KETOPT_INIT;

	while ((c = ketopt(&o, argc, argv, 1, "w:t:m:c:dvh", 0)) >= 0) {
		if (c == 'h') usage(basename(argv[0]), W, T);
		else if (c == 'v') {puts(SDUST_VERSION); return 0;}
		else if (c == 'd') d = 1;
		else if (c == 'm') m = *o.arg;
		else if (c == 'w') W = atoi(o.arg);
		else if (c == 't') T = atoi(o.arg);
		else if (c == 'c') wrap = atoi(o.arg);
	}
	if (o.ind == argc)
	{
		if (!isatty(fileno(stdin)))
			fp = gzdopen(fileno(stdin), "r");
		else
			usage(basename(argv[0]), W, T);
	}
	else
		fp = strcmp(argv[o.ind], "-")? gzopen(argv[o.ind], "r") : gzdopen(fileno(stdin), "r");
	// check masker
	if (m && !strchr("XNxn", (char)m))
	{
		fprintf(stderr, "Error: invalid masker character [%c]\n", (char)m);
		exit(1);
	}
	ks = kseq_init(fp);
	while (kseq_read(ks) >= 0) {
		uint64_t *r;
		int i, j, n = 0;
		r = sdust(0, (uint8_t*)ks->seq.s, -1, T, W, &n);
		if (d)
		{
			putchar('>');
			fputs(ks->name.s, stdout);
			if (ks->comment.l)
			{
				putchar(' ');
				puts(ks->comment.s);
			}
			putchar('\n');
			if (n)
			{
				char *s = ks->seq.s;
				for (i = 0; i < n; ++i)
				{
					int a = (int)(r[i]>>32), b = (int)r[i] - 1;
					for (j = a; j <= b; ++j) s[j] = m ? (char)m : tolower(s[j]);
				}
			}
			if (wrap)
			{
				for (i = 0; i < ks->seq.l; ++i)
				{
					putchar(ks->seq.s[i]);
					if ((i + 1) % wrap == 0) putchar('\n');
				}
				if (ks->seq.l % wrap) putchar('\n');
			}
			else
				puts(ks->seq.s);
		}
		else
			for (i = 0; i < n; ++i)
				printf("%s\t%d\t%d\n", ks->name.s, (int)(r[i]>>32), min(ks->seq.l, (int)r[i]));
		free(r);
	}
	kseq_destroy(ks);
	gzclose(fp);
	return 0;
}
