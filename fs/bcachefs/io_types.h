#ifndef _BCACHE_IO_TYPES_H
#define _BCACHE_IO_TYPES_H

#include "btree_types.h"
#include "buckets_types.h"
#include "keylist_types.h"
#include "super_types.h"

#include <linux/llist.h>
#include <linux/workqueue.h>

struct extent_pick_ptr {
	struct bch_extent_crc128	crc;
	struct bch_extent_ptr		ptr;
	struct bch_dev			*ca;
};

struct bch_read_bio {
	struct bch_fs		*c;

	/*
	 * Reads will often have to be split, and if the extent being read from
	 * was checksummed or compressed we'll also have to allocate bounce
	 * buffers and copy the data back into the original bio.
	 *
	 * If we didn't have to split, we have to save and restore the original
	 * bi_end_io - @split below indicates which:
	 */
	union {
	struct bch_read_bio	*parent;
	bio_end_io_t		*end_io;
	};

	/*
	 * Saved copy of bio->bi_iter, from submission time - allows us to
	 * resubmit on IO error, and also to copy data back to the original bio
	 * when we're bouncing:
	 */
	struct bvec_iter	bvec_iter;

	unsigned		submit_time_us;
	u8			flags;
	union {
	struct {
	u8			bounce:1,
				split:1,
				process_context:1,
				retry:2;
	};
	u8			_state;
	};

	struct extent_pick_ptr	pick;
	struct bversion		version;

	struct promote_op	*promote;

	/*
	 * If we have to retry the read (IO error, checksum failure, read stale
	 * data (raced with allocator), we retry the portion of the parent bio
	 * that failed (i.e. this bio's portion, bvec_iter).
	 *
	 * But we need to stash the inode somewhere:
	 */
	u64			inode;

	struct work_struct	work;

	struct bio		bio;
};

struct bch_write_bio {
	struct bch_fs		*c;
	struct bch_dev		*ca;
	union {
	struct bch_write_bio	*parent;
	struct closure		*cl;
	};

	u8			ptr_idx;
	u8			replicas_failed;
	u8			order;

	unsigned		split:1,
				bounce:1,
				put_bio:1,
				have_io_ref:1,
				used_mempool:1;

	unsigned		submit_time_us;
	void			*data;

	struct bio		bio;
};

struct bch_write_op {
	struct closure		cl;
	struct bch_fs		*c;
	struct workqueue_struct	*io_wq;

	unsigned		written; /* sectors */

	short			error;

	u16			flags;
	unsigned		csum_type:4;
	unsigned		compression_type:4;
	unsigned		nr_replicas:4;
	unsigned		alloc_reserve:4;
	unsigned		nonce:14;

	struct bpos		pos;
	struct bversion		version;

	/* For BCH_WRITE_DATA_COMPRESSED: */
	struct bch_extent_crc128 crc;
	unsigned		size;

	struct disk_reservation	res;

	struct write_point	*wp;

	union {
	u8			open_buckets[16];
	struct {
	struct bch_write_op	*next;
	unsigned long		expires;
	};
	};

	/*
	 * If caller wants to flush but hasn't passed us a journal_seq ptr, we
	 * still need to stash the journal_seq somewhere:
	 */
	union {
		u64			*journal_seq_p;
		u64			journal_seq;
	};

	int			(*index_update_fn)(struct bch_write_op *);

	struct bch_devs_mask	failed;

	struct keylist		insert_keys;
	u64			inline_keys[BKEY_EXTENT_U64s_MAX * 2];

	/* Must be last: */
	struct bch_write_bio	wbio;
};

#endif /* _BCACHE_IO_TYPES_H */
