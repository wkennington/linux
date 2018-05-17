#ifndef _BCACHEFS_SUPER_TYPES_H
#define _BCACHEFS_SUPER_TYPES_H

struct bch_sb_handle {
	struct bch_sb		*sb;
	struct block_device	*bdev;
	struct bio		*bio;
	unsigned		page_order;
	fmode_t			mode;
};

struct bch_devs_mask {
	unsigned long d[BITS_TO_LONGS(BCH_SB_MEMBERS_MAX)];
};

struct bch_devs_list {
	u8			nr;
	u8			devs[BCH_REPLICAS_MAX + 1];
};

struct bch_member_cpu {
	u64			nbuckets;	/* device size */
	u16			first_bucket;   /* index of first bucket used */
	u16			bucket_size;	/* sectors */
	u16			group;
	u8			state;
	u8			tier;
	u8			replacement;
	u8			discard;
	u8			data_allowed;
	u8			valid;
};

struct bch_replicas_cpu_entry {
	u8			data_type;
	u8			devs[BCH_SB_MEMBERS_MAX / 8];
};

struct bch_replicas_cpu {
	struct rcu_head		rcu;
	unsigned		nr;
	unsigned		entry_size;
	struct bch_replicas_cpu_entry entries[];
};

struct bch_disk_group_cpu {
	struct bch_devs_mask		devs;
	bool				deleted;
};

struct bch_disk_groups_cpu {
	struct rcu_head			rcu;
	unsigned			nr;
	struct bch_disk_group_cpu	entries[];
};

#endif /* _BCACHEFS_SUPER_TYPES_H */
