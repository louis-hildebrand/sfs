#include <stdlib.h>
#include <string.h>

#include "disk_emu.h"
#include "sfs_api.h"
#include "sfs_base.h"
#include "sfs_directory.h"
#include "sfs_freebitmap.h"
#include "sfs_inode.h"
#include "sfs_ofdt.h"


static struct super_block super_block;
static struct inode_table inode_table;
static struct freebitmap free_bitmap;
static struct directory directory;
static struct ofdt ofdt;
// Index of the current file in sfs_getnextfilename()
static int current_file_idx;
// Whether the exit handler has already been registered
static int exit_func_registered = 0;


static void reset_in_memory_system() {
	sfs_inode_free_table(&inode_table);

	sfs_freebitmap_free(&free_bitmap);

	sfs_directory_free(&directory);

	sfs_ofdt_free(&ofdt);

	sfs_base_super_block_free(&super_block);

	close_disk();
}


void mksfs(int fresh) {
	if (!exit_func_registered) {
		atexit(reset_in_memory_system);
		exit_func_registered = 1;
	}

	reset_in_memory_system();

	if (fresh) {
		super_block = sfs_base_init_fresh_disk();
		inode_table = sfs_inode_new_table(&super_block, &free_bitmap);
		free_bitmap = sfs_freebitmap_new(&super_block);
		directory = sfs_directory_new(&super_block, &inode_table);
	}
	else {
		super_block = sfs_base_init_old_disk();
		inode_table = sfs_inode_table_from_disk(&super_block, &free_bitmap);
		free_bitmap = sfs_freebitmap_from_disk(&super_block);
		directory = sfs_directory_from_disk(&super_block, &inode_table);
	}

	ofdt = sfs_ofdt_new();
}

int sfs_getnextfilename(char *filename) {
	if (current_file_idx >= directory.size) {
		current_file_idx = 0;
		return 0;
	}

	strcpy(filename, directory.entries[current_file_idx].filename);

	current_file_idx++;

	return 1;
}

int sfs_getfilesize(const char *filename) {
	inode_idx inode_idx = sfs_directory_get_inode(&directory, filename);
	if (inode_idx == INODE_NULL) {
		return -1;
	}

	return inode_table.entries[inode_idx].size;
}

int sfs_fopen(const char* filename) {
	inode_idx inode_idx = sfs_directory_get_inode(&directory, filename);

	rw_pointer rw_pointer;
	if (inode_idx == INODE_NULL) {
		inode_idx = sfs_directory_add_file(&directory, filename);
		if (inode_idx == INODE_NULL) {
			return -1;
		}

		rw_pointer = 0;
	}
	else {
		rw_pointer = inode_table.entries[inode_idx].size;
	}

	// Reuse the existing file descriptor if there is one
	int existing_fd = sfs_ofdt_find_by_inode(&ofdt, inode_idx);
	if (existing_fd >= 0) {
		return existing_fd;
	}

	int fd = sfs_ofdt_add_entry(&ofdt, inode_idx, rw_pointer);
	return fd;
}

int sfs_fclose(int fd) {
	int success = sfs_ofdt_remove_entry(&ofdt, fd);

	return success ? 0 : -1;
}

int sfs_fwrite(int fd, const char *buffer, int length) {
	struct ofdt_entry *ofdt_entry = sfs_ofdt_get_active_entry(&ofdt, fd);
	if (ofdt_entry == NULL) {
		return -1;
	}

	int num_bytes_written = sfs_inode_write(&inode_table, ofdt_entry->inode_idx, ofdt_entry->rw_pointer, length, buffer);

	if (num_bytes_written > 0) {
		ofdt_entry->rw_pointer += num_bytes_written;
	}

	return num_bytes_written;
}

int sfs_fread(int fd, char *buffer, int length) {
	struct ofdt_entry *ofdt_entry = sfs_ofdt_get_active_entry(&ofdt, fd);
	if (ofdt_entry == NULL) {
		return -1;
	}

	int num_bytes_read = sfs_inode_read(&inode_table, ofdt_entry->inode_idx, ofdt_entry->rw_pointer, length, buffer);

	if (num_bytes_read > 0) {
		ofdt_entry->rw_pointer += num_bytes_read;
	}

	return num_bytes_read;
}

int sfs_fseek(int fd, int location) {
	struct ofdt_entry *ofdt_entry = sfs_ofdt_get_active_entry(&ofdt, fd);
	if (ofdt_entry == NULL) {
		return -1;
	}

	// Don't allow seeking past the end of the file and obviously don't allow seeking to negative location
	struct inode *inode = inode_table.entries + ofdt_entry->inode_idx;
	if (location < 0 || location > inode->size) {
		return -1;
	}

	ofdt_entry->rw_pointer = location;

	return 0;
}

int sfs_remove(const char *filename) {
	// File not found
	inode_idx inode_idx = sfs_directory_get_inode(&directory, filename);
	if (inode_idx == INODE_NULL) {
		return -1;
	}

	// Don't remove open files
	if (sfs_ofdt_find_by_inode(&ofdt, inode_idx) >= 0) {
		return -1;
	}

	int success = sfs_directory_remove_file(&directory, filename);

	// TODO: Should things get flushed automatically here? Check every occurrence of sfs_freebitmap_flush()
	sfs_freebitmap_flush(&free_bitmap);

	return success ? 0 : -1;
}
