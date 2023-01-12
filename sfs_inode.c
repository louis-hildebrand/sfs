#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk_emu.h"
#include "sfs_inode.h"


static int max(int a, int b) {
	return a >= b ? a : b;
}

static int min(int a, int b) {
	return a <= b ? a : b;
}

static struct inode *get_active_inode(struct inode_table *table, inode_idx i) {
	if (i < 0 || i >= table->size) {
		return NULL;
	}

	struct inode *inode = table->entries + i;
	if (!inode->active) {
		return NULL;
	}

	return inode;
}

// TODO: Implement function to flush just one inode?
// Would need to figure out which block that inode belongs to, whether it spans multiple blocks, etc.

/*
 * Flushes the entire inode table to the disk.
 */
static void flush_inode_table(struct inode_table *table) {
	write_contiguous_bytes_to_disk(
		1,
		table->size * sizeof(struct inode),
		table->entries,
		table->super_block->block_size
	);
}


/*
 * Returns a pointer to the nth data block in the given inode.
 */
static disk_ptr get_data_block_from_inode(struct super_block *sb, struct freebitmap *fbmp, struct inode *inode, int n, disk_ptr *indirect_block, int *indirect_block_fetched, int create) {
	char tmp_buffer[sb->block_size];
	const int disk_ptrs_per_block = sb->block_size / sizeof(disk_ptr);

	// Use direct pointer
	if (n < NUM_INODE_DIRECT_PTRS) {
		if (inode->direct_pointers[n] == DISK_NULL && create) {
			inode->direct_pointers[n] = sfs_freebitmap_reserve_block(fbmp);
			if (inode->direct_pointers[n] == DISK_NULL) {
				// No data blocks available
				return DISK_NULL;
			}
		}
		else if (inode->direct_pointers[n] == DISK_NULL && !create) {
			// Block before end of file is not allocated (sparse file?)
			return DISK_NULL;
		}

		return inode->direct_pointers[n];
	}
	// Use indirect pointer
	else {
		if (inode->indirect_pointer == DISK_NULL && create) {
			inode->indirect_pointer = sfs_freebitmap_reserve_block(fbmp);
			if (inode->indirect_pointer == DISK_NULL) {
				// No data blocks available
				return DISK_NULL;
			}
			*indirect_block_fetched = 1;
		}
		else if (inode->indirect_pointer == DISK_NULL && !create) {
			// Block before end of file is not allocated (sparse file?)
			return DISK_NULL;
		}
		// Load the indirect block from the disk if it hasn't been loaded yet
		else if (!(*indirect_block_fetched)) {
			read_blocks(inode->indirect_pointer, 1, tmp_buffer);
			memcpy(indirect_block, tmp_buffer, disk_ptrs_per_block * sizeof(disk_ptr));
			*indirect_block_fetched = 1;
		}

		int indirect_block_idx = n - NUM_INODE_DIRECT_PTRS;

		if (indirect_block_idx >= disk_ptrs_per_block) {
			// Reached max file size
			return DISK_NULL;
		}

		if (indirect_block[indirect_block_idx] == DISK_NULL && create) {
			indirect_block[indirect_block_idx] = sfs_freebitmap_reserve_block(fbmp);
			if (indirect_block[indirect_block_idx] == DISK_NULL) {
				// No data blocks available
				return DISK_NULL;
			}
		}
		else if (indirect_block[indirect_block_idx] == DISK_NULL && !create) {
			// Block before end of file is not allocated (sparse file?)
			return DISK_NULL;
		}

		return indirect_block[indirect_block_idx];
	}
}

struct inode_table sfs_inode_new_table(struct super_block *sb, struct freebitmap *fbmp) {
	struct inode_table table;

	table.super_block = sb;
	table.free_bitmap = fbmp;

	table.size = sb->num_inode_blocks * sb->block_size / sizeof(struct inode);
	table.entries = calloc_or_exit(table.size, sizeof(struct inode));

	flush_inode_table(&table);

	return table;
}

struct inode_table sfs_inode_table_from_disk(struct super_block *sb, struct freebitmap *fbmp) {
	struct inode_table table;

	table.super_block = sb;
	table.free_bitmap = fbmp;

	table.size = sb->num_inode_blocks * sb->block_size / sizeof(struct inode);
	table.entries = calloc_or_exit(table.size, sizeof(struct inode));
	read_contiguous_bytes_from_disk(1, table.size * sizeof(struct inode), table.entries, sb->block_size);

	return table;
}

void sfs_inode_free_table(struct inode_table *table) {
	if (table->entries != NULL) {
		free(table->entries);
	}
	memset(table, 0, sizeof *table);
}

inode_idx sfs_inode_reserve_inode(struct inode_table *table) {
	for (inode_idx i = 0; i < table->size; i++) {
		if (!table->entries[i].active) {
			memset(table->entries + i, 0, sizeof(struct inode));
			table->entries[i].active = 1;
			flush_inode_table(table);
			return i;
		}
	}
	return INODE_NULL;
}

void sfs_inode_delete_file(struct inode_table *table, inode_idx inode_idx) {
	struct inode *inode = get_active_inode(table, inode_idx);
	if (inode == NULL) {
		// Nothing to do
		return;
	}

	// Release the direct blocks
	for (int i = 0; i < NUM_INODE_DIRECT_PTRS; i++) {
		disk_ptr p = inode->direct_pointers[i];
		if (p != DISK_NULL) {
			sfs_freebitmap_release_block(table->free_bitmap, p);
		}
	}
	// Release the indirect block
	if (inode->indirect_pointer != DISK_NULL) {
		// TODO: Move this to its own helper function?
		const int disk_ptrs_per_block = table->super_block->block_size / sizeof(disk_ptr);
		disk_ptr indirect_block[disk_ptrs_per_block];
		read_contiguous_bytes_from_disk(inode->indirect_pointer, sizeof indirect_block, indirect_block, table->super_block->block_size);

		for (int i = 0; i < disk_ptrs_per_block; i++) {
			if (indirect_block[i] != DISK_NULL) {
				sfs_freebitmap_release_block(table->free_bitmap, indirect_block[i]);
			}
		}

		sfs_freebitmap_release_block(table->free_bitmap, inode->indirect_pointer);
	}

	sfs_freebitmap_flush(table->free_bitmap);

	memset(inode, 0, sizeof *inode);
	flush_inode_table(table);
}

void sfs_inode_force_reserve(struct inode_table *table, inode_idx idx) {
	if (table->entries[idx].active) {
		fprintf(stderr, "WARNING: Inode %d was already in use. Existing data will be deleted.\n", idx);
		sfs_inode_delete_file(table, idx);
	}
	else {
		table->entries[idx].active = 1;
		flush_inode_table(table);
	}
}

int sfs_inode_write(struct inode_table *table, inode_idx inode_idx, int start_byte, int num_bytes, const char *data) {
	const int block_size = table->super_block->block_size;

	struct inode *inode = get_active_inode(table, inode_idx);
	if (inode == NULL) {
		return -1;
	}

	disk_ptr block;
	int block_idx = start_byte / block_size;
	int position_in_block = start_byte % block_size;
	int num_bytes_written = 0;

	char tmp_buffer[block_size];
	const int disk_ptrs_per_block = block_size / sizeof(disk_ptr);
	disk_ptr indirect_block[disk_ptrs_per_block];
	memset(indirect_block, 0, disk_ptrs_per_block * sizeof(disk_ptr));
	int indirect_block_fetched = 0;
	int block_error = 0;
	while (num_bytes > 0) {
		block = get_data_block_from_inode(table->super_block, table->free_bitmap, inode, block_idx, indirect_block, &indirect_block_fetched, 1);
		if (block == DISK_NULL) {
			block_error = 1;
			break;
		}

		int bytes_this_block = min(num_bytes, block_size - position_in_block);

		read_blocks(block, 1, tmp_buffer);
		memcpy(tmp_buffer + position_in_block, data, bytes_this_block);
		write_blocks(block, 1, tmp_buffer);

		num_bytes_written += bytes_this_block;
		num_bytes -= bytes_this_block;
		data += bytes_this_block;
		block_idx++;
		position_in_block = 0;
	}

	if (num_bytes_written == 0 && block_error) {
		return -1;
	}

	if (indirect_block_fetched) {
		write_contiguous_bytes_to_disk(inode->indirect_pointer, disk_ptrs_per_block * sizeof(disk_ptr), indirect_block, block_size);
	}

	// after_final_byte_written is one more than the last byte written
	int after_final_byte_written = start_byte + num_bytes_written;
	inode->size = max(inode->size, after_final_byte_written);

	flush_inode_table(table);
	// TODO: should the free bitmap be flushed automatically?
	sfs_freebitmap_flush(table->free_bitmap);

	return num_bytes_written;
}

int sfs_inode_read(struct inode_table *table, inode_idx inode_idx, int start_byte, int num_bytes, char *data) {
	const int block_size = table->super_block->block_size;

	struct inode *inode = get_active_inode(table, inode_idx);
	if (inode == NULL) {
		return -1;
	}

	disk_ptr block;
	int block_idx = start_byte / block_size;
	int position_in_block = start_byte % block_size;
	int num_bytes_read = 0;

	num_bytes = min(num_bytes, inode->size - start_byte);
	num_bytes = max(num_bytes, 0);

	const int disk_ptrs_per_block = block_size / sizeof(disk_ptr);
	disk_ptr indirect_block[disk_ptrs_per_block];
	memset(indirect_block, 0, disk_ptrs_per_block * sizeof(disk_ptr));
	int indirect_block_fetched = 0;
	int block_error = 0;
	while (num_bytes > 0) {
		block = get_data_block_from_inode(table->super_block, NULL, inode, block_idx, indirect_block, &indirect_block_fetched, 0);
		if (block == DISK_NULL) {
			block_error = 1;
			break;
		}

		int bytes_this_block = min(num_bytes, block_size - position_in_block);

		// TODO: Move this to a helper function?
		char tmp_buffer[block_size];
		read_blocks(block, 1, tmp_buffer);
		memcpy(data, tmp_buffer + position_in_block, bytes_this_block);

		num_bytes_read += bytes_this_block;
		num_bytes -= bytes_this_block;
		data += bytes_this_block;
		block_idx++;
		position_in_block = 0;
	}

	if (num_bytes_read == 0 && block_error) {
		return -1;
	}

	return num_bytes_read;
}
