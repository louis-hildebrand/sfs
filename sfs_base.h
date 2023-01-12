#ifndef SFS_BASE_H
#define SFS_BASE_H


#include <stdlib.h>


// sfs_test1 assumes 33 bytes (including the null terminator) is a valid filename
// sfs_test2 assumes 42 bytes (including the null terminator) is too long
#define MAXFILENAME 33
#define SFS_FILENAME ".sfs_store"
// This must be at least as large as sizeof(struct super_block)
static const int BLOCK_SIZE = 1024;
// Number of blocks in the entire disk for a new file system
static const int NUM_BLOCKS = 4096;
// Number of blocks containing inodes for a new file system
static const int NUM_INODE_BLOCKS = 64;


// Address of a block on disk
typedef int disk_ptr;
// Disk pointers should normally not be pointing to the super-block, so use that like NULL
#define DISK_NULL 0
// Index of an inode within the inode table
typedef int inode_idx;
#define INODE_NULL -1
// Position (in bytes) within a file
typedef int rw_pointer;


struct super_block {
	// Size of one block in bytes
	int block_size;
	// Total number of blocks on disk
	int num_blocks;
	// Number of blocks to be used for inodes
	int num_inode_blocks;
	// Index of the root directory inode
	inode_idx dir_inode_idx;
};


/*
 * Attempts to call calloc(nmemb, size). If calloc() fails, the program is terminated.
 */
void *calloc_or_exit(size_t nmemb, size_t size);

/*
 * Returns the ceiling of numerator / denominator.
 */
int ceil_div(int numerator, int denominator);

/*
 * Reads the requested number of bytes from the disk into the buffer. If more
 * than one block is needed, successive blocks are used.
 */
void read_contiguous_bytes_from_disk(disk_ptr start_block, int num_bytes, void *buffer, const int block_size);

/*
 * Writes the requested number of bytes from the buffer onto the disk. If more
 * than one block is needed, successive blocks are used.
 *
 * This function pads the given buffer with zeroes before writing to the disk.
 * It does not read the existing data. Therefore, existing data outside the
 * target location will be overwritten with zeroes.
 */
void write_contiguous_bytes_to_disk(disk_ptr start_block, int num_bytes, void *data, const int block_size);

/*
 * Initializes a fresh disk, creates a new super block with the default values,
* and flushes the super block to the disk.
 */
struct super_block sfs_base_init_fresh_disk();

/*
 * Reads the existing disk and returns its super block.
 */
struct super_block sfs_base_init_old_disk();

/*
 * Zeroes out the memory for the super block.
 */
void sfs_base_super_block_free(struct super_block *sb);


#endif
