#ifndef SFS_FREEBITMAP_H
#define SFS_FREEBITMAP_H


#include "sfs_base.h"


// TODO: Use an actual bitmap instead of using entire bytes?
struct freebitmap {
	// Defines the geometry of the disk
	struct super_block *super_block;
	// Number of blocks used for the free bitmap
	int num_blocks;
	char *data;
};

/*
 * Initializes a new free bitmap and flushes it to the disk.
 */
struct freebitmap sfs_freebitmap_new(struct super_block *sb);

/*
 * Loads an existing free bitmap from the disk.
 */
struct freebitmap sfs_freebitmap_from_disk(struct super_block *);

/*
 * Frees any dynamically-allocated memory and zeroes out the memory for the
 * entire free bitmap.
 */
void sfs_freebitmap_free(struct freebitmap *fbmp);

/*
 * Flushes the entire free bitmap to the disk.
 */
void sfs_freebitmap_flush(struct freebitmap *fbmp);

/*
 * Marks a block as available.
 *
 * The free bitmap is NOT flushed to the disk.
 */
void sfs_freebitmap_release_block(struct freebitmap *fbmp, disk_ptr block_num);

/*
 * Marks a block as used and returns a pointer to that block.
 *
 * The free bitmap is NOT flushed to the disk.
 */
disk_ptr sfs_freebitmap_reserve_block(struct freebitmap *fbmp);


#endif
