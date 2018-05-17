/*
 * random utiility code, for bcache but in theory not specific to bcache
 *
 * Copyright 2010, 2011 Kent Overstreet <kent.overstreet@gmail.com>
 * Copyright 2012 Google, Inc.
 */

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/log2.h>
#include <linux/math64.h>
#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/sched/clock.h>

#include "util.h"

#define simple_strtoint(c, end, base)	simple_strtol(c, end, base)
#define simple_strtouint(c, end, base)	simple_strtoul(c, end, base)

#define STRTO_H(name, type)					\
int bch2_ ## name ## _h(const char *cp, type *res)		\
{								\
	int u = 0;						\
	char *e;						\
	type i = simple_ ## name(cp, &e, 10);			\
								\
	switch (tolower(*e)) {					\
	default:						\
		return -EINVAL;					\
	case 'y':						\
	case 'z':						\
		u++;						\
	case 'e':						\
		u++;						\
	case 'p':						\
		u++;						\
	case 't':						\
		u++;						\
	case 'g':						\
		u++;						\
	case 'm':						\
		u++;						\
	case 'k':						\
		u++;						\
		if (e++ == cp)					\
			return -EINVAL;				\
	case '\n':						\
	case '\0':						\
		if (*e == '\n')					\
			e++;					\
	}							\
								\
	if (*e)							\
		return -EINVAL;					\
								\
	while (u--) {						\
		if ((type) ~0 > 0 &&				\
		    (type) ~0 / 1024 <= i)			\
			return -EINVAL;				\
		if ((i > 0 && ANYSINT_MAX(type) / 1024 < i) ||	\
		    (i < 0 && -ANYSINT_MAX(type) / 1024 > i))	\
			return -EINVAL;				\
		i *= 1024;					\
	}							\
								\
	*res = i;						\
	return 0;						\
}								\

STRTO_H(strtoint, int)
STRTO_H(strtouint, unsigned int)
STRTO_H(strtoll, long long)
STRTO_H(strtoull, unsigned long long)

ssize_t bch2_hprint(char *buf, s64 v)
{
	static const char units[] = "?kMGTPEZY";
	char dec[4] = "";
	int u, t = 0;

	for (u = 0; v >= 1024 || v <= -1024; u++) {
		t = v & ~(~0U << 10);
		v >>= 10;
	}

	if (!u)
		return sprintf(buf, "%lli", v);

	/*
	 * 103 is magic: t is in the range [-1023, 1023] and we want
	 * to turn it into [-9, 9]
	 */
	if (v < 100 && v > -100)
		scnprintf(dec, sizeof(dec), ".%i", t / 103);

	return sprintf(buf, "%lli%s%c", v, dec, units[u]);
}

ssize_t bch2_scnprint_string_list(char *buf, size_t size,
				  const char * const list[],
				  size_t selected)
{
	char *out = buf;
	size_t i;

	if (size)
		*out = '\0';

	for (i = 0; list[i]; i++)
		out += scnprintf(out, buf + size - out,
				 i == selected ? "[%s] " : "%s ", list[i]);

	if (out != buf)
		*--out = '\0';

	return out - buf;
}

ssize_t bch2_read_string_list(const char *buf, const char * const list[])
{
	size_t i, len;

	buf = skip_spaces(buf);

	len = strlen(buf);
	while (len && isspace(buf[len - 1]))
		--len;

	for (i = 0; list[i]; i++)
		if (strlen(list[i]) == len &&
		    !memcmp(buf, list[i], len))
			break;

	return list[i] ? i : -EINVAL;
}

ssize_t bch2_scnprint_flag_list(char *buf, size_t size,
				const char * const list[], u64 flags)
{
	char *out = buf, *end = buf + size;
	unsigned bit, nr = 0;

	while (list[nr])
		nr++;

	if (size)
		*out = '\0';

	while (flags && (bit = __ffs(flags)) < nr) {
		out += scnprintf(out, end - out, "%s,", list[bit]);
		flags ^= 1 << bit;
	}

	if (out != buf)
		*--out = '\0';

	return out - buf;
}

u64 bch2_read_flag_list(char *opt, const char * const list[])
{
	u64 ret = 0;
	char *p, *s, *d = kstrndup(opt, PAGE_SIZE - 1, GFP_KERNEL);

	if (!d)
		return -ENOMEM;

	s = strim(d);

	while ((p = strsep(&s, ","))) {
		int flag = bch2_read_string_list(p, list);
		if (flag < 0) {
			ret = -1;
			break;
		}

		ret |= 1 << flag;
	}

	kfree(d);

	return ret;
}

bool bch2_is_zero(const void *_p, size_t n)
{
	const char *p = _p;
	size_t i;

	for (i = 0; i < n; i++)
		if (p[i])
			return false;
	return true;
}

void bch2_time_stats_clear(struct time_stats *stats)
{
	spin_lock(&stats->lock);

	stats->count = 0;
	stats->last_duration = 0;
	stats->max_duration = 0;
	stats->average_duration = 0;
	stats->average_frequency = 0;
	stats->last = 0;

	spin_unlock(&stats->lock);
}

void __bch2_time_stats_update(struct time_stats *stats, u64 start_time)
{
	u64 now, duration, last;

	stats->count++;

	now		= local_clock();
	duration	= time_after64(now, start_time)
		? now - start_time : 0;
	last		= time_after64(now, stats->last)
		? now - stats->last : 0;

	stats->last_duration = duration;
	stats->max_duration = max(stats->max_duration, duration);

	if (stats->last) {
		stats->average_duration = ewma_add(stats->average_duration,
						   duration << 8, 3);

		if (stats->average_frequency)
			stats->average_frequency =
				ewma_add(stats->average_frequency,
					 last << 8, 3);
		else
			stats->average_frequency  = last << 8;
	} else {
		stats->average_duration = duration << 8;
	}

	stats->last = now ?: 1;
}

void bch2_time_stats_update(struct time_stats *stats, u64 start_time)
{
	spin_lock(&stats->lock);
	__bch2_time_stats_update(stats, start_time);
	spin_unlock(&stats->lock);
}

/**
 * bch2_ratelimit_delay() - return how long to delay until the next time to do
 * some work
 *
 * @d - the struct bch_ratelimit to update
 *
 * Returns the amount of time to delay by, in jiffies
 */
u64 bch2_ratelimit_delay(struct bch_ratelimit *d)
{
	u64 now = local_clock();

	return time_after64(d->next, now)
		? nsecs_to_jiffies(d->next - now)
		: 0;
}

/**
 * bch2_ratelimit_increment() - increment @d by the amount of work done
 *
 * @d - the struct bch_ratelimit to update
 * @done - the amount of work done, in arbitrary units
 */
void bch2_ratelimit_increment(struct bch_ratelimit *d, u64 done)
{
	u64 now = local_clock();

	d->next += div_u64(done * NSEC_PER_SEC, d->rate);

	if (time_before64(now + NSEC_PER_SEC, d->next))
		d->next = now + NSEC_PER_SEC;

	if (time_after64(now - NSEC_PER_SEC * 2, d->next))
		d->next = now - NSEC_PER_SEC * 2;
}

int bch2_ratelimit_wait_freezable_stoppable(struct bch_ratelimit *d)
{
	while (1) {
		u64 delay = bch2_ratelimit_delay(d);

		if (delay)
			set_current_state(TASK_INTERRUPTIBLE);

		if (kthread_should_stop())
			return 1;

		if (!delay)
			return 0;

		schedule_timeout(delay);
		try_to_freeze();
	}
}

/*
 * Updates pd_controller. Attempts to scale inputed values to units per second.
 * @target: desired value
 * @actual: current value
 *
 * @sign: 1 or -1; 1 if increasing the rate makes actual go up, -1 if increasing
 * it makes actual go down.
 */
void bch2_pd_controller_update(struct bch_pd_controller *pd,
			      s64 target, s64 actual, int sign)
{
	s64 proportional, derivative, change;

	unsigned long seconds_since_update = (jiffies - pd->last_update) / HZ;

	if (seconds_since_update == 0)
		return;

	pd->last_update = jiffies;

	proportional = actual - target;
	proportional *= seconds_since_update;
	proportional = div_s64(proportional, pd->p_term_inverse);

	derivative = actual - pd->last_actual;
	derivative = div_s64(derivative, seconds_since_update);
	derivative = ewma_add(pd->smoothed_derivative, derivative,
			      (pd->d_term / seconds_since_update) ?: 1);
	derivative = derivative * pd->d_term;
	derivative = div_s64(derivative, pd->p_term_inverse);

	change = proportional + derivative;

	/* Don't increase rate if not keeping up */
	if (change > 0 &&
	    pd->backpressure &&
	    time_after64(local_clock(),
			 pd->rate.next + NSEC_PER_MSEC))
		change = 0;

	change *= (sign * -1);

	pd->rate.rate = clamp_t(s64, (s64) pd->rate.rate + change,
				1, UINT_MAX);

	pd->last_actual		= actual;
	pd->last_derivative	= derivative;
	pd->last_proportional	= proportional;
	pd->last_change		= change;
	pd->last_target		= target;
}

void bch2_pd_controller_init(struct bch_pd_controller *pd)
{
	pd->rate.rate		= 1024;
	pd->last_update		= jiffies;
	pd->p_term_inverse	= 6000;
	pd->d_term		= 30;
	pd->d_smooth		= pd->d_term;
	pd->backpressure	= 1;
}

size_t bch2_pd_controller_print_debug(struct bch_pd_controller *pd, char *buf)
{
	/* 2^64 - 1 is 20 digits, plus null byte */
	char rate[21];
	char actual[21];
	char target[21];
	char proportional[21];
	char derivative[21];
	char change[21];
	s64 next_io;

	bch2_hprint(rate,	pd->rate.rate);
	bch2_hprint(actual,	pd->last_actual);
	bch2_hprint(target,	pd->last_target);
	bch2_hprint(proportional, pd->last_proportional);
	bch2_hprint(derivative,	pd->last_derivative);
	bch2_hprint(change,	pd->last_change);

	next_io = div64_s64(pd->rate.next - local_clock(), NSEC_PER_MSEC);

	return sprintf(buf,
		       "rate:\t\t%s/sec\n"
		       "target:\t\t%s\n"
		       "actual:\t\t%s\n"
		       "proportional:\t%s\n"
		       "derivative:\t%s\n"
		       "change:\t\t%s/sec\n"
		       "next io:\t%llims\n",
		       rate, target, actual, proportional,
		       derivative, change, next_io);
}

void bch2_bio_map(struct bio *bio, void *base)
{
	size_t size = bio->bi_iter.bi_size;
	struct bio_vec *bv = bio->bi_io_vec;

	BUG_ON(!bio->bi_iter.bi_size);
	BUG_ON(bio->bi_vcnt);

	bv->bv_offset = base ? offset_in_page(base) : 0;
	goto start;

	for (; size; bio->bi_vcnt++, bv++) {
		bv->bv_offset	= 0;
start:		bv->bv_len	= min_t(size_t, PAGE_SIZE - bv->bv_offset,
					size);
		BUG_ON(bio->bi_vcnt >= bio->bi_max_vecs);
		if (base) {
			bv->bv_page = is_vmalloc_addr(base)
				? vmalloc_to_page(base)
				: virt_to_page(base);

			base += bv->bv_len;
		}

		size -= bv->bv_len;
	}
}

int bch2_bio_alloc_pages(struct bio *bio, gfp_t gfp_mask)
{
	int i;
	struct bio_vec *bv;

	bio_for_each_segment_all(bv, bio, i) {
		bv->bv_page = alloc_page(gfp_mask);
		if (!bv->bv_page) {
			while (--bv >= bio->bi_io_vec)
				__free_page(bv->bv_page);
			return -ENOMEM;
		}
	}

	return 0;
}

size_t bch2_rand_range(size_t max)
{
	size_t rand;

	do {
		get_random_bytes(&rand, sizeof(rand));
		rand &= roundup_pow_of_two(max) - 1;
	} while (rand >= max);

	return rand;
}

void memcpy_to_bio(struct bio *dst, struct bvec_iter dst_iter, void *src)
{
	struct bio_vec bv;
	struct bvec_iter iter;

	__bio_for_each_segment(bv, dst, iter, dst_iter) {
		void *dstp = kmap_atomic(bv.bv_page);
		memcpy(dstp + bv.bv_offset, src, bv.bv_len);
		kunmap_atomic(dstp);

		src += bv.bv_len;
	}
}

void memcpy_from_bio(void *dst, struct bio *src, struct bvec_iter src_iter)
{
	struct bio_vec bv;
	struct bvec_iter iter;

	__bio_for_each_segment(bv, src, iter, src_iter) {
		void *srcp = kmap_atomic(bv.bv_page);
		memcpy(dst, srcp + bv.bv_offset, bv.bv_len);
		kunmap_atomic(srcp);

		dst += bv.bv_len;
	}
}

size_t bch_scnmemcpy(char *buf, size_t size, const char *src, size_t len)
{
	size_t n;

	if (!size)
		return 0;

	n = min(size - 1, len);
	memcpy(buf, src, n);
	buf[n] = '\0';

	return n;
}

#include "eytzinger.h"

static int alignment_ok(const void *base, size_t align)
{
	return IS_ENABLED(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) ||
		((unsigned long)base & (align - 1)) == 0;
}

static void u32_swap(void *a, void *b, size_t size)
{
	u32 t = *(u32 *)a;
	*(u32 *)a = *(u32 *)b;
	*(u32 *)b = t;
}

static void u64_swap(void *a, void *b, size_t size)
{
	u64 t = *(u64 *)a;
	*(u64 *)a = *(u64 *)b;
	*(u64 *)b = t;
}

static void generic_swap(void *a, void *b, size_t size)
{
	char t;

	do {
		t = *(char *)a;
		*(char *)a++ = *(char *)b;
		*(char *)b++ = t;
	} while (--size > 0);
}

static inline int do_cmp(void *base, size_t n, size_t size,
			 int (*cmp_func)(const void *, const void *, size_t),
			 size_t l, size_t r)
{
	return cmp_func(base + inorder_to_eytzinger0(l, n) * size,
			base + inorder_to_eytzinger0(r, n) * size,
			size);
}

static inline void do_swap(void *base, size_t n, size_t size,
			   void (*swap_func)(void *, void *, size_t),
			   size_t l, size_t r)
{
	swap_func(base + inorder_to_eytzinger0(l, n) * size,
		  base + inorder_to_eytzinger0(r, n) * size,
		  size);
}

void eytzinger0_sort(void *base, size_t n, size_t size,
		     int (*cmp_func)(const void *, const void *, size_t),
		     void (*swap_func)(void *, void *, size_t))
{
	int i, c, r;

	if (!swap_func) {
		if (size == 4 && alignment_ok(base, 4))
			swap_func = u32_swap;
		else if (size == 8 && alignment_ok(base, 8))
			swap_func = u64_swap;
		else
			swap_func = generic_swap;
	}

	/* heapify */
	for (i = n / 2 - 1; i >= 0; --i) {
		for (r = i; r * 2 + 1 < n; r = c) {
			c = r * 2 + 1;

			if (c + 1 < n &&
			    do_cmp(base, n, size, cmp_func, c, c + 1) < 0)
				c++;

			if (do_cmp(base, n, size, cmp_func, r, c) >= 0)
				break;

			do_swap(base, n, size, swap_func, r, c);
		}
	}

	/* sort */
	for (i = n - 1; i > 0; --i) {
		do_swap(base, n, size, swap_func, 0, i);

		for (r = 0; r * 2 + 1 < i; r = c) {
			c = r * 2 + 1;

			if (c + 1 < i &&
			    do_cmp(base, n, size, cmp_func, c, c + 1) < 0)
				c++;

			if (do_cmp(base, n, size, cmp_func, r, c) >= 0)
				break;

			do_swap(base, n, size, swap_func, r, c);
		}
	}
}

void sort_cmp_size(void *base, size_t num, size_t size,
	  int (*cmp_func)(const void *, const void *, size_t),
	  void (*swap_func)(void *, void *, size_t size))
{
	/* pre-scale counters for performance */
	int i = (num/2 - 1) * size, n = num * size, c, r;

	if (!swap_func) {
		if (size == 4 && alignment_ok(base, 4))
			swap_func = u32_swap;
		else if (size == 8 && alignment_ok(base, 8))
			swap_func = u64_swap;
		else
			swap_func = generic_swap;
	}

	/* heapify */
	for ( ; i >= 0; i -= size) {
		for (r = i; r * 2 + size < n; r  = c) {
			c = r * 2 + size;
			if (c < n - size &&
			    cmp_func(base + c, base + c + size, size) < 0)
				c += size;
			if (cmp_func(base + r, base + c, size) >= 0)
				break;
			swap_func(base + r, base + c, size);
		}
	}

	/* sort */
	for (i = n - size; i > 0; i -= size) {
		swap_func(base, base + i, size);
		for (r = 0; r * 2 + size < i; r = c) {
			c = r * 2 + size;
			if (c < i - size &&
			    cmp_func(base + c, base + c + size, size) < 0)
				c += size;
			if (cmp_func(base + r, base + c, size) >= 0)
				break;
			swap_func(base + r, base + c, size);
		}
	}
}

void mempool_free_vp(void *element, void *pool_data)
{
	size_t size = (size_t) pool_data;

	vpfree(element, size);
}

void *mempool_alloc_vp(gfp_t gfp_mask, void *pool_data)
{
	size_t size = (size_t) pool_data;

	return vpmalloc(size, gfp_mask);
}
