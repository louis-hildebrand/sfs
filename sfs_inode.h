#ifndef SFS_INODE_H
#define SFS_INODE_H


#include "sfs_base.h"
#include "sfs_freebitmap.h"


#define NUM_INODE_DIRECT_PTRS 12


struct inode {
	// Whether the inode is being used (i.e., represents a file or directory)
	int active;
	// File size in bytes
	int size;
	disk_ptr direct_pointers[NUM_INODE_DIRECT_PTRS];
	disk_ptr indirect_pointer;
};

struct inode_table {
	// Defines the geometry of the inode table
	struct super_block *super_block;
	// Free bitmap to use when reserving data blocks
	struct freebitmap *free_bitmap;
	// Number of inodes
	int size;
	// inode data
	struct inode *entries;
};


/*
 * Initializes a new inode table and flushes it to the disk.
 *
 * The geometry of the table will be defined by the given super block and the
 * table will use the given free bitmap to reserve data blocks.
 */
struct inode_table sfs_inode_new_table(struct super_block *sb, struct freebitmap *fbmp);

/*
 * Reads an existing inode table from the disk.
 *
 * The geometry of the table will be defined by the given super block and the
 * table will use the given free bitmap to reserve data blocks.
 */
struct inode_table sfs_inode_table_from_disk(struct super_block *sb, struct freebitmap *fbmp);

/*
 * Frees any dynamically-allocated memory and zeroes out the memory for the
 * entire table.
 */
void sfs_inode_free_table(struct inode_table *table);

/*
 * Reserves an inode, flushes the inode table, and returns the index of the
 * reserved inode.
 */
inode_idx sfs_inode_reserve_inode(struct inode_table *table);

/*
 * Reserves the given inode. Any existing data will be deleted.
 */
void sfs_inode_force_reserve(struct inode_table *table, inode_idx inode_idx);

/*
 * Deletes the file defined by the given inode. All its data blocks will be
 * released (but not zeroed-out). The inode will be flushed.
 */
void sfs_inode_delete_file(struct inode_table *table, inode_idx inode_idx);

/*
 * Writes to the file defined by the given inode. Both the data blocks and the
 * inode itself will be updated and flushed.
 *
 * Returns the number of bytes written or a negative number on failure.
 */
int sfs_inode_write(struct inode_table *table, inode_idx inode_idx, int start_byte, int num_bytes, const char *data);

/* Reads from the file defined by the given inode.
 *
 * Returns the number of bytes read or a negative number on failure.
 */
int sfs_inode_read(struct inode_table *table, inode_idx inode_idx, int start_byte, int num_bytes, char *data);


#endif
