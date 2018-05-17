#ifndef _BCACHEFS_FS_H
#define _BCACHEFS_FS_H

#include "opts.h"
#include "str_hash.h"

#include <linux/seqlock.h>
#include <linux/stat.h>

struct bch_inode_info {
	struct inode		v;

	struct mutex		ei_update_lock;
	u64			ei_journal_seq;
	unsigned long		ei_last_dirtied;

	struct bch_hash_info	ei_str_hash;

	/* copy of inode in btree: */
	struct bch_inode_unpacked ei_inode;
};

#define to_bch_ei(_inode)					\
	container_of_or_null(_inode, struct bch_inode_info, v)

static inline struct bch_inode_info *file_bch_inode(struct file *file)
{
	return to_bch_ei(file_inode(file));
}

static inline u8 mode_to_type(umode_t mode)
{
	return (mode >> 12) & 15;
}

static inline unsigned nlink_bias(umode_t mode)
{
	return S_ISDIR(mode) ? 2 : 1;
}

struct bch_inode_unpacked;

#ifndef NO_BCACHEFS_FS

/* returns 0 if we want to do the update, or error is passed up */
typedef int (*inode_set_fn)(struct bch_inode_info *,
			    struct bch_inode_unpacked *, void *);

int __must_check __bch2_write_inode(struct bch_fs *, struct bch_inode_info *,
				    inode_set_fn, void *);
int __must_check bch2_write_inode(struct bch_fs *,
				  struct bch_inode_info *);

void bch2_vfs_exit(void);
int bch2_vfs_init(void);

#else

static inline void bch2_vfs_exit(void) {}
static inline int bch2_vfs_init(void) { return 0; }

#endif /* NO_BCACHEFS_FS */

#endif /* _BCACHEFS_FS_H */
