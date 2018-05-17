#ifndef _BCACHEFS_SYSFS_H_
#define _BCACHEFS_SYSFS_H_

#include <linux/sysfs.h>

#ifndef NO_BCACHEFS_SYSFS

struct attribute;
struct sysfs_ops;

extern struct attribute *bch2_fs_files[];
extern struct attribute *bch2_fs_internal_files[];
extern struct attribute *bch2_fs_opts_dir_files[];
extern struct attribute *bch2_fs_time_stats_files[];
extern struct attribute *bch2_dev_files[];

extern struct sysfs_ops bch2_fs_sysfs_ops;
extern struct sysfs_ops bch2_fs_internal_sysfs_ops;
extern struct sysfs_ops bch2_fs_opts_dir_sysfs_ops;
extern struct sysfs_ops bch2_fs_time_stats_sysfs_ops;
extern struct sysfs_ops bch2_dev_sysfs_ops;

#else

static struct attribute *bch2_fs_files[] = {};
static struct attribute *bch2_fs_internal_files[] = {};
static struct attribute *bch2_fs_opts_dir_files[] = {};
static struct attribute *bch2_fs_time_stats_files[] = {};
static struct attribute *bch2_dev_files[] = {};

static const struct sysfs_ops bch2_fs_sysfs_ops;
static const struct sysfs_ops bch2_fs_internal_sysfs_ops;
static const struct sysfs_ops bch2_fs_opts_dir_sysfs_ops;
static const struct sysfs_ops bch2_fs_time_stats_sysfs_ops;
static const struct sysfs_ops bch2_dev_sysfs_ops;

#endif /* NO_BCACHEFS_SYSFS */

#endif  /* _BCACHEFS_SYSFS_H_ */
