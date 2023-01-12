#ifndef SFS_OFDT_H
#define SFS_OFDT_H


#include "sfs_base.h"


struct ofdt_entry {
	// Whether this entry is active (i.e., represents an open file)
	int active;
	// Inode of the file
	inode_idx inode_idx;
	// Current location within the file
	rw_pointer rw_pointer;
};

struct ofdt {
	// Number of entries
	int size;
	struct ofdt_entry *entries;
};


/*
 * Initializes a new OFDT.
 */
struct ofdt sfs_ofdt_new();

/*
 * Frees any dynamically-allocated memory and zeroes out the memory for the
 * OFDT.
 */
void sfs_ofdt_free(struct ofdt *ofdt);

/*
 * Adds a new OFDT entry and returns its file descriptor.
 */
int sfs_ofdt_add_entry(struct ofdt *ofdt, inode_idx inode_idx, rw_pointer rw_pointer);

/*
 * Returns the OFDT entry with the given file descriptor if it is active, otherwise returns NULL.
 */
struct ofdt_entry *sfs_ofdt_get_active_entry(struct ofdt *ofdt, int fd);

/*
 * Removes the OFDT entry with the given file descriptor. Returns zero if no
 * such entry exists and returns a non-zero number on success.
 */
int sfs_ofdt_remove_entry(struct ofdt *ofdt, int fd);

/*
 * Returns the file descriptor for the given inode, or a negative number if
 * there is no such existing OFDT entry.
 */
int sfs_ofdt_find_by_inode(struct ofdt *ofdt, inode_idx inode_idx);


#endif
