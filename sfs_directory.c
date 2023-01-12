#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_directory.h"


static void resize_directory(struct directory *dir) {
	dir->capacity *= 2;
	struct directory_entry *new_entries = calloc_or_exit(dir->capacity, sizeof(struct directory_entry));
	memcpy(new_entries, dir->entries, dir->size * sizeof(struct directory_entry));
	free(dir->entries);
	dir->entries = new_entries;
}

static int index_of_dir_entry(struct directory *dir, const char filename[MAXFILENAME]) {
	for (int i = 0; i < dir->size; i++) {
		if (strcmp(dir->entries[i].filename, filename) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * Checks whether the given filename is at most MAXFILENAME characters,
 * including the null terminator.
 */
static int is_filename_too_long(const char *filename) {
	for (int i = 0; i < MAXFILENAME; i++) {
		if (filename[i] == '\0') {
			return 0;
		}
	}
	return 1;
}


struct directory sfs_directory_new(struct super_block *sb, struct inode_table *table) {
	struct directory dir;

	dir.super_block = sb;
	dir.inode_table = table;
	dir.capacity = 10;
	dir.size = 0;

	dir.entries = calloc(dir.capacity, sizeof(struct directory_entry));

	// The rest of the code assumes the directory uses inode 0
	sfs_inode_force_reserve(table, 0);

	return dir;
}

struct directory sfs_directory_from_disk(struct super_block *sb, struct inode_table *table) {
	struct directory dir;

	dir.super_block = sb;
	dir.inode_table = table;

	struct inode *dir_inode = table->entries + sb->dir_inode_idx;
	dir.size = dir_inode->size / sizeof(struct directory_entry);
	dir.capacity = dir.size * 2;

	dir.entries = calloc(dir.capacity, sizeof(struct directory_entry));
	int num_bytes_read = sfs_inode_read(table, sb->dir_inode_idx, 0, dir_inode->size, (char *) dir.entries);
	if (num_bytes_read < 0) {
		fprintf(stderr, "WARNING: failed to read directory inode (sfs_inode_read() returned %d).\n", num_bytes_read);
	}

	return dir;
}

void sfs_directory_free(struct directory *dir) {
	if (dir->entries != NULL) {
		free(dir->entries);
	}
	memset(dir, 0, sizeof *dir);
}

inode_idx sfs_directory_add_file(struct directory *dir, const char *filename) {
	if (is_filename_too_long(filename)) {
		return INODE_NULL;
	}

	inode_idx new_file_inode = sfs_inode_reserve_inode(dir->inode_table);
	if (new_file_inode == INODE_NULL) {
		return INODE_NULL;
	}

	if (dir->size >= dir->capacity) {
		resize_directory(dir);
	}

	strcpy(dir->entries[dir->size].filename, filename);
	dir->entries[dir->size].inode_idx = new_file_inode;

	int start_byte = dir->size * sizeof(struct directory_entry);
	sfs_inode_write(dir->inode_table, dir->super_block->dir_inode_idx, start_byte, sizeof(struct directory_entry), (char *) (dir->entries + dir->size));

	dir->size++;

	return new_file_inode;
}

int sfs_directory_remove_file(struct directory *dir, const char *filename) {
	// Fail early to prevent unterminated filenames from causing issues
	if (is_filename_too_long(filename)) {
		return 0;
	}

	int idx_to_remove = index_of_dir_entry(dir, filename);
	if (idx_to_remove < 0) {
		return 0;
	}

	// Delete the file itself
	struct directory_entry *dir_entry = dir->entries + idx_to_remove;
	sfs_inode_delete_file(dir->inode_table, dir_entry->inode_idx);

	// Update the directory in memory
	dir->size--;
	for (int i = idx_to_remove; i < dir->size; i++) {
		struct directory_entry *dest = dir->entries + i;
		struct directory_entry *src = dir->entries + i + 1;
		memcpy(dest, src, sizeof *dest);
	}
	memset(dir->entries + dir->size, 0, sizeof(struct directory_entry));

	// Flush the entire directory and its inode
	// Need to manually reset the size of the file so that existing data after the end of the directory is ignored
	struct inode *dir_inode = dir->inode_table->entries + dir->super_block->dir_inode_idx;
	dir_inode->size = dir->size * sizeof(struct directory_entry);
	sfs_inode_write(dir->inode_table, dir->super_block->dir_inode_idx, 0, dir_inode->size, (char *) dir->entries);

	return 1;
}

inode_idx sfs_directory_get_inode(struct directory *dir, const char *filename) {
	if (is_filename_too_long(filename)) {
		return INODE_NULL;
	}

	int i = index_of_dir_entry(dir, filename);
	return i < 0 ? INODE_NULL : dir->entries[i].inode_idx;
}
