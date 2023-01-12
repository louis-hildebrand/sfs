#include <stdlib.h>
#include <string.h>

#include "sfs_ofdt.h"


// Reserves the first available OFDT entry and returns its index. Returns a negative number if no entries are available.
static int get_first_unused_file_descriptor(struct ofdt *ofdt) {
	for (int i = 0; i < ofdt->size; i++) {
		if (!ofdt->entries[i].active) {
			return i;
		}
	}
	return -1;
}

static void resize_ofdt(struct ofdt *ofdt) {
	int old_size = ofdt->size;
	int new_size = old_size * 2;
	ofdt->size = new_size;

	struct ofdt_entry *new_entries = calloc_or_exit(new_size, sizeof(struct ofdt_entry));
	memcpy(new_entries, ofdt->entries, old_size * sizeof(struct ofdt_entry));

	free(ofdt->entries);
	ofdt->entries = new_entries;
}


struct ofdt sfs_ofdt_new() {
	struct ofdt ofdt;

	ofdt.size = 10;
	ofdt.entries = calloc_or_exit(ofdt.size, sizeof(struct ofdt_entry));

	return ofdt;
}

void sfs_ofdt_free(struct ofdt *ofdt) {
	if (ofdt->entries != NULL) {
		free(ofdt->entries);
	}
	memset(ofdt, 0, sizeof *ofdt);
}

int sfs_ofdt_add_entry(struct ofdt *ofdt, inode_idx inode_idx, rw_pointer rw_pointer) {
	int fd = get_first_unused_file_descriptor(ofdt);
	if (fd < 0) {
		fd = ofdt->size;
		resize_ofdt(ofdt);
	}

	ofdt->entries[fd].active = 1;
	ofdt->entries[fd].inode_idx = inode_idx;
	ofdt->entries[fd].rw_pointer = rw_pointer;

	return fd;
}

struct ofdt_entry *sfs_ofdt_get_active_entry(struct ofdt *ofdt, int fd) {
	if (fd < 0 || fd >= ofdt->size) {
		return NULL;
	}

	struct ofdt_entry *ofdt_entry = ofdt->entries + fd;
	if (!ofdt_entry->active) {
		return NULL;
	}

	return ofdt_entry;
}

int sfs_ofdt_remove_entry(struct ofdt *ofdt, int fd) {
	struct ofdt_entry *ofdt_entry = sfs_ofdt_get_active_entry(ofdt, fd);
	if (ofdt_entry == NULL) {
		return 0;
	}

	// Clear out the entire OFDT entry
	memset(ofdt_entry, 0, sizeof *ofdt_entry);

	return 1;
}

int sfs_ofdt_find_by_inode(struct ofdt *ofdt, inode_idx inode_idx) {
	for (int i = 0; i < ofdt->size; i++) {
		if (ofdt->entries[i].active && ofdt->entries[i].inode_idx == inode_idx) {
			return i;
		}
	}
	return -1;
}
