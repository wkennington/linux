#ifndef _BCACHE_COMPRESS_H
#define _BCACHE_COMPRESS_H

int bch2_bio_uncompress_inplace(struct bch_fs *, struct bio *,
			       unsigned, struct bch_extent_crc128);
int bch2_bio_uncompress(struct bch_fs *, struct bio *, struct bio *,
		       struct bvec_iter, struct bch_extent_crc128);
void bch2_bio_compress(struct bch_fs *, struct bio *, size_t *,
		      struct bio *, size_t *, unsigned *);

int bch2_check_set_has_compressed_data(struct bch_fs *, unsigned);
void bch2_fs_compress_exit(struct bch_fs *);
int bch2_fs_compress_init(struct bch_fs *);

#endif /* _BCACHE_COMPRESS_H */
