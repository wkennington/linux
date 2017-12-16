#undef TRACE_SYSTEM
#define TRACE_SYSTEM bcachefs

#if !defined(_TRACE_BCACHE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_BCACHE_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(bpos,
	TP_PROTO(struct bpos p),
	TP_ARGS(p),

	TP_STRUCT__entry(
		__field(u64,	inode				)
		__field(u64,	offset				)
	),

	TP_fast_assign(
		__entry->inode	= p.inode;
		__entry->offset	= p.offset;
	),

	TP_printk("%llu:%llu", __entry->inode, __entry->offset)
);

DECLARE_EVENT_CLASS(bkey,
	TP_PROTO(const struct bkey *k),
	TP_ARGS(k),

	TP_STRUCT__entry(
		__field(u64,	inode				)
		__field(u64,	offset				)
		__field(u32,	size				)
	),

	TP_fast_assign(
		__entry->inode	= k->p.inode;
		__entry->offset	= k->p.offset;
		__entry->size	= k->size;
	),

	TP_printk("%llu:%llu len %u", __entry->inode,
		  __entry->offset, __entry->size)
);

DECLARE_EVENT_CLASS(bch_dev,
	TP_PROTO(struct bch_dev *ca),
	TP_ARGS(ca),

	TP_STRUCT__entry(
		__array(char,		uuid,	16	)
		__field(unsigned,	tier		)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, ca->uuid.b, 16);
		__entry->tier = ca->mi.tier;
	),

	TP_printk("%pU tier %u", __entry->uuid, __entry->tier)
);

DECLARE_EVENT_CLASS(bch_fs,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c),

	TP_STRUCT__entry(
		__array(char,		uuid,	16 )
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
	),

	TP_printk("%pU", __entry->uuid)
);

DECLARE_EVENT_CLASS(bio,
	TP_PROTO(struct bio *bio),
	TP_ARGS(bio),

	TP_STRUCT__entry(
		__field(dev_t,		dev			)
		__field(sector_t,	sector			)
		__field(unsigned int,	nr_sector		)
		__array(char,		rwbs,	6		)
	),

	TP_fast_assign(
		__entry->dev		= bio->bi_bdev ? bio->bi_bdev->bd_dev : 0;
		__entry->sector		= bio->bi_iter.bi_sector;
		__entry->nr_sector	= bio->bi_iter.bi_size >> 9;
		blk_fill_rwbs(__entry->rwbs, bio->bi_opf, bio->bi_iter.bi_size);
	),

	TP_printk("%d,%d  %s %llu + %u",
		  MAJOR(__entry->dev), MINOR(__entry->dev), __entry->rwbs,
		  (unsigned long long)__entry->sector, __entry->nr_sector)
);

DECLARE_EVENT_CLASS(page_alloc_fail,
	TP_PROTO(struct bch_fs *c, u64 size),
	TP_ARGS(c, size),

	TP_STRUCT__entry(
		__array(char,		uuid,	16	)
		__field(u64,		size		)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->size = size;
	),

	TP_printk("%pU size %llu", __entry->uuid, __entry->size)
);

/* io.c: */

DEFINE_EVENT(bio, read_split,
	TP_PROTO(struct bio *bio),
	TP_ARGS(bio)
);

DEFINE_EVENT(bio, read_bounce,
	TP_PROTO(struct bio *bio),
	TP_ARGS(bio)
);

DEFINE_EVENT(bio, read_retry,
	TP_PROTO(struct bio *bio),
	TP_ARGS(bio)
);

DEFINE_EVENT(bio, promote,
	TP_PROTO(struct bio *bio),
	TP_ARGS(bio)
);

TRACE_EVENT(write_throttle,
	TP_PROTO(struct bch_fs *c, u64 inode, struct bio *bio, u64 delay),
	TP_ARGS(c, inode, bio, delay),

	TP_STRUCT__entry(
		__array(char,		uuid,	16		)
		__field(u64,		inode			)
		__field(sector_t,	sector			)
		__field(unsigned int,	nr_sector		)
		__array(char,		rwbs,	6		)
		__field(u64,		delay			)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->inode		= inode;
		__entry->sector		= bio->bi_iter.bi_sector;
		__entry->nr_sector	= bio->bi_iter.bi_size >> 9;
		blk_fill_rwbs(__entry->rwbs, bio->bi_opf, bio->bi_iter.bi_size);
		__entry->delay		= delay;
	),

	TP_printk("%pU inode %llu  %s %llu + %u delay %llu",
		  __entry->uuid, __entry->inode,
		  __entry->rwbs, (unsigned long long)__entry->sector,
		  __entry->nr_sector, __entry->delay)
);

/* Journal */

DEFINE_EVENT(bch_fs, journal_full,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_fs, journal_entry_full,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bio, journal_write,
	TP_PROTO(struct bio *bio),
	TP_ARGS(bio)
);

/* bset.c: */

DEFINE_EVENT(bpos, bkey_pack_pos_fail,
	TP_PROTO(struct bpos p),
	TP_ARGS(p)
);

/* Btree */

DECLARE_EVENT_CLASS(btree_node,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b),

	TP_STRUCT__entry(
		__array(char,		uuid,		16	)
		__field(u64,		bucket			)
		__field(u8,		level			)
		__field(u8,		id			)
		__field(u32,		inode			)
		__field(u64,		offset			)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->bucket		= PTR_BUCKET_NR_TRACE(c, &b->key, 0);
		__entry->level		= b->level;
		__entry->id		= b->btree_id;
		__entry->inode		= b->key.k.p.inode;
		__entry->offset		= b->key.k.p.offset;
	),

	TP_printk("%pU bucket %llu(%u) id %u: %u:%llu",
		  __entry->uuid, __entry->bucket, __entry->level, __entry->id,
		  __entry->inode, __entry->offset)
);

DEFINE_EVENT(btree_node, btree_read,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b)
);

TRACE_EVENT(btree_write,
	TP_PROTO(struct btree *b, unsigned bytes, unsigned sectors),
	TP_ARGS(b, bytes, sectors),

	TP_STRUCT__entry(
		__field(enum bkey_type,	type)
		__field(unsigned,	bytes			)
		__field(unsigned,	sectors			)
	),

	TP_fast_assign(
		__entry->type	= btree_node_type(b);
		__entry->bytes	= bytes;
		__entry->sectors = sectors;
	),

	TP_printk("bkey type %u bytes %u sectors %u",
		  __entry->type , __entry->bytes, __entry->sectors)
);

DEFINE_EVENT(btree_node, btree_node_alloc,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b)
);

DEFINE_EVENT(btree_node, btree_node_free,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b)
);

TRACE_EVENT(btree_node_reap,
	TP_PROTO(struct bch_fs *c, struct btree *b, int ret),
	TP_ARGS(c, b, ret),

	TP_STRUCT__entry(
		__field(u64,			bucket		)
		__field(int,			ret		)
	),

	TP_fast_assign(
		__entry->bucket	= PTR_BUCKET_NR_TRACE(c, &b->key, 0);
		__entry->ret = ret;
	),

	TP_printk("bucket %llu ret %d", __entry->bucket, __entry->ret)
);

DECLARE_EVENT_CLASS(btree_node_cannibalize_lock,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c),

	TP_STRUCT__entry(
		__array(char,			uuid,	16	)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
	),

	TP_printk("%pU", __entry->uuid)
);

DEFINE_EVENT(btree_node_cannibalize_lock, btree_node_cannibalize_lock_fail,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(btree_node_cannibalize_lock, btree_node_cannibalize_lock,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(btree_node_cannibalize_lock, btree_node_cannibalize,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_fs, btree_node_cannibalize_unlock,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

TRACE_EVENT(btree_reserve_get_fail,
	TP_PROTO(struct bch_fs *c, size_t required, struct closure *cl),
	TP_ARGS(c, required, cl),

	TP_STRUCT__entry(
		__array(char,			uuid,	16	)
		__field(size_t,			required	)
		__field(struct closure *,	cl		)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->required = required;
		__entry->cl = cl;
	),

	TP_printk("%pU required %zu by %p", __entry->uuid,
		  __entry->required, __entry->cl)
);

TRACE_EVENT(btree_insert_key,
	TP_PROTO(struct bch_fs *c, struct btree *b, struct bkey_i *k),
	TP_ARGS(c, b, k),

	TP_STRUCT__entry(
		__field(u64,		b_bucket		)
		__field(u64,		b_offset		)
		__field(u64,		offset			)
		__field(u32,		b_inode			)
		__field(u32,		inode			)
		__field(u32,		size			)
		__field(u8,		level			)
		__field(u8,		id			)
	),

	TP_fast_assign(
		__entry->b_bucket	= PTR_BUCKET_NR_TRACE(c, &b->key, 0);
		__entry->level		= b->level;
		__entry->id		= b->btree_id;
		__entry->b_inode	= b->key.k.p.inode;
		__entry->b_offset	= b->key.k.p.offset;
		__entry->inode		= k->k.p.inode;
		__entry->offset		= k->k.p.offset;
		__entry->size		= k->k.size;
	),

	TP_printk("bucket %llu(%u) id %u: %u:%llu %u:%llu len %u",
		  __entry->b_bucket, __entry->level, __entry->id,
		  __entry->b_inode, __entry->b_offset,
		  __entry->inode, __entry->offset, __entry->size)
);

DECLARE_EVENT_CLASS(btree_split,
	TP_PROTO(struct bch_fs *c, struct btree *b, unsigned keys),
	TP_ARGS(c, b, keys),

	TP_STRUCT__entry(
		__field(u64,		bucket			)
		__field(u8,		level			)
		__field(u8,		id			)
		__field(u32,		inode			)
		__field(u64,		offset			)
		__field(u32,		keys			)
	),

	TP_fast_assign(
		__entry->bucket	= PTR_BUCKET_NR_TRACE(c, &b->key, 0);
		__entry->level	= b->level;
		__entry->id	= b->btree_id;
		__entry->inode	= b->key.k.p.inode;
		__entry->offset	= b->key.k.p.offset;
		__entry->keys	= keys;
	),

	TP_printk("bucket %llu(%u) id %u: %u:%llu keys %u",
		  __entry->bucket, __entry->level, __entry->id,
		  __entry->inode, __entry->offset, __entry->keys)
);

DEFINE_EVENT(btree_split, btree_node_split,
	TP_PROTO(struct bch_fs *c, struct btree *b, unsigned keys),
	TP_ARGS(c, b, keys)
);

DEFINE_EVENT(btree_split, btree_node_compact,
	TP_PROTO(struct bch_fs *c, struct btree *b, unsigned keys),
	TP_ARGS(c, b, keys)
);

DEFINE_EVENT(btree_node, btree_set_root,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b)
);

/* Garbage collection */

TRACE_EVENT(btree_gc_coalesce,
	TP_PROTO(struct bch_fs *c, struct btree *b, unsigned nodes),
	TP_ARGS(c, b, nodes),

	TP_STRUCT__entry(
		__field(u64,		bucket			)
		__field(u8,		level			)
		__field(u8,		id			)
		__field(u32,		inode			)
		__field(u64,		offset			)
		__field(unsigned,	nodes			)
	),

	TP_fast_assign(
		__entry->bucket		= PTR_BUCKET_NR_TRACE(c, &b->key, 0);
		__entry->level		= b->level;
		__entry->id		= b->btree_id;
		__entry->inode		= b->key.k.p.inode;
		__entry->offset		= b->key.k.p.offset;
		__entry->nodes		= nodes;
	),

	TP_printk("bucket %llu(%u) id %u: %u:%llu nodes %u",
		  __entry->bucket, __entry->level, __entry->id,
		  __entry->inode, __entry->offset, __entry->nodes)
);

TRACE_EVENT(btree_gc_coalesce_fail,
	TP_PROTO(struct bch_fs *c, int reason),
	TP_ARGS(c, reason),

	TP_STRUCT__entry(
		__field(u8,		reason			)
		__array(char,		uuid,	16		)
	),

	TP_fast_assign(
		__entry->reason		= reason;
		memcpy(__entry->uuid, c->disk_sb->user_uuid.b, 16);
	),

	TP_printk("%pU: %u", __entry->uuid, __entry->reason)
);

DEFINE_EVENT(btree_node, btree_gc_rewrite_node,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b)
);

DEFINE_EVENT(btree_node, btree_gc_rewrite_node_fail,
	TP_PROTO(struct bch_fs *c, struct btree *b),
	TP_ARGS(c, b)
);

DEFINE_EVENT(bch_fs, gc_start,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_fs, gc_end,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_fs, gc_coalesce_start,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_fs, gc_coalesce_end,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_dev, sectors_saturated,
	TP_PROTO(struct bch_dev *ca),
	TP_ARGS(ca)
);

DEFINE_EVENT(bch_fs, gc_sectors_saturated,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DEFINE_EVENT(bch_fs, gc_cannot_inc_gens,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

/* Allocator */

TRACE_EVENT(alloc_batch,
	TP_PROTO(struct bch_dev *ca, size_t free, size_t total),
	TP_ARGS(ca, free, total),

	TP_STRUCT__entry(
		__array(char,		uuid,	16	)
		__field(size_t,		free		)
		__field(size_t,		total		)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, ca->uuid.b, 16);
		__entry->free = free;
		__entry->total = total;
	),

	TP_printk("%pU free %zu total %zu",
		__entry->uuid, __entry->free, __entry->total)
);

DEFINE_EVENT(bch_dev, prio_write_start,
	TP_PROTO(struct bch_dev *ca),
	TP_ARGS(ca)
);

DEFINE_EVENT(bch_dev, prio_write_end,
	TP_PROTO(struct bch_dev *ca),
	TP_ARGS(ca)
);

TRACE_EVENT(invalidate,
	TP_PROTO(struct bch_dev *ca, size_t bucket, unsigned sectors),
	TP_ARGS(ca, bucket, sectors),

	TP_STRUCT__entry(
		__field(unsigned,	sectors			)
		__field(dev_t,		dev			)
		__field(__u64,		offset			)
	),

	TP_fast_assign(
		__entry->dev		= ca->disk_sb.bdev->bd_dev;
		__entry->offset		= bucket << ca->bucket_bits;
		__entry->sectors	= sectors;
	),

	TP_printk("invalidated %u sectors at %d,%d sector=%llu",
		  __entry->sectors, MAJOR(__entry->dev),
		  MINOR(__entry->dev), __entry->offset)
);

DEFINE_EVENT(bch_fs, rescale_prios,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

DECLARE_EVENT_CLASS(bucket_alloc,
	TP_PROTO(struct bch_dev *ca, enum alloc_reserve reserve),
	TP_ARGS(ca, reserve),

	TP_STRUCT__entry(
		__array(char,			uuid,	16)
		__field(enum alloc_reserve,	reserve	  )
	),

	TP_fast_assign(
		memcpy(__entry->uuid, ca->uuid.b, 16);
		__entry->reserve = reserve;
	),

	TP_printk("%pU reserve %d", __entry->uuid, __entry->reserve)
);

DEFINE_EVENT(bucket_alloc, bucket_alloc,
	TP_PROTO(struct bch_dev *ca, enum alloc_reserve reserve),
	TP_ARGS(ca, reserve)
);

DEFINE_EVENT(bucket_alloc, bucket_alloc_fail,
	TP_PROTO(struct bch_dev *ca, enum alloc_reserve reserve),
	TP_ARGS(ca, reserve)
);

TRACE_EVENT(freelist_empty_fail,
	TP_PROTO(struct bch_fs *c, enum alloc_reserve reserve,
		 struct closure *cl),
	TP_ARGS(c, reserve, cl),

	TP_STRUCT__entry(
		__array(char,			uuid,	16	)
		__field(enum alloc_reserve,	reserve		)
		__field(struct closure *,	cl		)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->reserve = reserve;
		__entry->cl = cl;
	),

	TP_printk("%pU reserve %d cl %p", __entry->uuid, __entry->reserve,
		  __entry->cl)
);

DECLARE_EVENT_CLASS(open_bucket_alloc,
	TP_PROTO(struct bch_fs *c, struct closure *cl),
	TP_ARGS(c, cl),

	TP_STRUCT__entry(
		__array(char,			uuid,	16	)
		__field(struct closure *,	cl		)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->cl = cl;
	),

	TP_printk("%pU cl %p",
		  __entry->uuid, __entry->cl)
);

DEFINE_EVENT(open_bucket_alloc, open_bucket_alloc,
	TP_PROTO(struct bch_fs *c, struct closure *cl),
	TP_ARGS(c, cl)
);

DEFINE_EVENT(open_bucket_alloc, open_bucket_alloc_fail,
	TP_PROTO(struct bch_fs *c, struct closure *cl),
	TP_ARGS(c, cl)
);

/* Moving IO */

DECLARE_EVENT_CLASS(moving_io,
	TP_PROTO(struct bkey *k),
	TP_ARGS(k),

	TP_STRUCT__entry(
		__field(__u32,		inode			)
		__field(__u64,		offset			)
		__field(__u32,		sectors			)
	),

	TP_fast_assign(
		__entry->inode		= k->p.inode;
		__entry->offset		= k->p.offset;
		__entry->sectors	= k->size;
	),

	TP_printk("%u:%llu sectors %u",
		  __entry->inode, __entry->offset, __entry->sectors)
);

DEFINE_EVENT(moving_io, move_read,
	TP_PROTO(struct bkey *k),
	TP_ARGS(k)
);

DEFINE_EVENT(moving_io, move_read_done,
	TP_PROTO(struct bkey *k),
	TP_ARGS(k)
);

DEFINE_EVENT(moving_io, move_write,
	TP_PROTO(struct bkey *k),
	TP_ARGS(k)
);

DEFINE_EVENT(moving_io, copy_collision,
	TP_PROTO(struct bkey *k),
	TP_ARGS(k)
);

/* Copy GC */

DEFINE_EVENT(page_alloc_fail, moving_gc_alloc_fail,
	TP_PROTO(struct bch_fs *c, u64 size),
	TP_ARGS(c, size)
);

DEFINE_EVENT(bch_dev, moving_gc_start,
	TP_PROTO(struct bch_dev *ca),
	TP_ARGS(ca)
);

TRACE_EVENT(moving_gc_end,
	TP_PROTO(struct bch_dev *ca, u64 sectors_moved, u64 keys_moved,
		u64 buckets_moved),
	TP_ARGS(ca, sectors_moved, keys_moved, buckets_moved),

	TP_STRUCT__entry(
		__array(char,		uuid,	16	)
		__field(u64,		sectors_moved	)
		__field(u64,		keys_moved	)
		__field(u64,		buckets_moved	)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, ca->uuid.b, 16);
		__entry->sectors_moved = sectors_moved;
		__entry->keys_moved = keys_moved;
		__entry->buckets_moved = buckets_moved;
	),

	TP_printk("%pU sectors_moved %llu keys_moved %llu buckets_moved %llu",
		__entry->uuid, __entry->sectors_moved, __entry->keys_moved,
		__entry->buckets_moved)
);

DEFINE_EVENT(bkey, gc_copy,
	TP_PROTO(const struct bkey *k),
	TP_ARGS(k)
);

/* Tiering */

DEFINE_EVENT(page_alloc_fail, tiering_alloc_fail,
	TP_PROTO(struct bch_fs *c, u64 size),
	TP_ARGS(c, size)
);

DEFINE_EVENT(bch_fs, tiering_start,
	TP_PROTO(struct bch_fs *c),
	TP_ARGS(c)
);

TRACE_EVENT(tiering_end,
	TP_PROTO(struct bch_fs *c, u64 sectors_moved,
		u64 keys_moved),
	TP_ARGS(c, sectors_moved, keys_moved),

	TP_STRUCT__entry(
		__array(char,		uuid,	16	)
		__field(u64,		sectors_moved	)
		__field(u64,		keys_moved	)
	),

	TP_fast_assign(
		memcpy(__entry->uuid, c->sb.user_uuid.b, 16);
		__entry->sectors_moved = sectors_moved;
		__entry->keys_moved = keys_moved;
	),

	TP_printk("%pU sectors_moved %llu keys_moved %llu",
		__entry->uuid, __entry->sectors_moved, __entry->keys_moved)
);

DEFINE_EVENT(bkey, tiering_copy,
	TP_PROTO(const struct bkey *k),
	TP_ARGS(k)
);

#endif /* _TRACE_BCACHE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
