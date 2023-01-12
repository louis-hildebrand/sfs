#ifndef SFS_DIRECTORY_H
#define SFS_DIRECTORY_H


#include "sfs_base.h"
#include "sfs_inode.h"


struct directory_entry {
	char filename[MAXFILENAME];
	inode_idx inode_idx;
};

struct directory {
	// Defines the geometry of the disk
	struct super_block *super_block;
	// Inode table to use when updating the directory, creating files, etc.
	struct inode_table *inode_table;
	// Number of entries (files) in the directory
	int size;
	// Number of entries that could fit in the directory before resizing is
	// required
	int capacity;
	struct directory_entry *entries;
};

/* 
 * Initializes a new directory with the given capacity and reserves an inode
 * for it.
 */
struct directory sfs_directory_new(struct super_block *sb, struct inode_table *table);

/*
 * Reads an existing directory from the disk.
 */
struct directory sfs_directory_from_disk(struct super_block *sb, struct inode_table *table);

/*
 * Frees any dynamically-allocated memory and zeroes out the memory for the
 * entire table.
 */
void sfs_directory_free(struct directory *dir);

/*
 * Reserves an inode for a new empty file and adds an entry in the given directory.
 *
 * Returns the inode index on success, otherwise returns INODE_NULL.
 */
inode_idx sfs_directory_add_file(struct directory *dir, const char *filename);

/*
 * Deletes the given file and removes its entry from the directory.
 *
 * Returns zero on failure and a nonzero number on success.
 */
int sfs_directory_remove_file(struct directory *dir, const char *filename);

/*
 * Looks up the inode index for the given filename. Returns INODE_NULL if there
 * is no such file.
 */
inode_idx sfs_directory_get_inode(struct directory *dir, const char *filename);


#endif
