#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "disk_emu.h"
#include "sfs_base.h"


void *calloc_or_exit(size_t nmemb, size_t size) {
	void *p = calloc(nmemb, size);

	if (p == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		exit(EXIT_FAILURE);
	}

	return p;
}

int ceil_div(int numerator, int denominator) {
	int result = numerator / denominator;
	if (numerator % denominator != 0) {
		result++;
	}
	return result;
}

void read_contiguous_bytes_from_disk(disk_ptr start_block, int num_bytes, void *data, const int block_size) {
	int num_blocks = ceil_div(num_bytes, block_size);

	char *buffer[num_blocks * block_size];

	read_blocks(start_block, num_blocks, buffer);

	memcpy(data, buffer, num_bytes);
}

void write_contiguous_bytes_to_disk(disk_ptr start_block, int num_bytes, void *data, const int block_size) {
	int num_blocks = ceil_div(num_bytes, block_size);

	char buffer[num_blocks * block_size];
	memset(buffer, 0, num_blocks * block_size);

	memcpy(&buffer, data, num_bytes);

	write_blocks(start_block, num_blocks, buffer);
}

struct super_block sfs_base_init_fresh_disk() {
	struct super_block sb;

	sb.block_size = BLOCK_SIZE;
	sb.num_blocks = NUM_BLOCKS;
	sb.num_inode_blocks = NUM_INODE_BLOCKS;
	sb.dir_inode_idx = 0;

	int success = init_fresh_disk(SFS_FILENAME, sb.block_size, sb.num_blocks);
	if (success < 0) {
		exit(EXIT_FAILURE);
	}
	write_contiguous_bytes_to_disk(0, sizeof sb, &sb, sb.block_size);

	return sb;
}

struct super_block sfs_base_init_old_disk() {
	struct super_block sb;

	int success = init_disk(SFS_FILENAME, BLOCK_SIZE, NUM_BLOCKS);
	if (success < 0) {
		exit(EXIT_FAILURE);
	}
	read_contiguous_bytes_from_disk(0, sizeof sb, &sb, BLOCK_SIZE);

	// Check whether file system parameters are as expected. If not, re-initialize the disk.
	// I think this should always work as long as BLOCK_SIZE is not less than sizeof(struct super_block)
	if (sb.block_size != BLOCK_SIZE || sb.num_blocks != NUM_BLOCKS) {
		close_disk();
		success = init_disk(SFS_FILENAME, sb.block_size, sb.num_blocks);
		if (success < 0) {
			exit(EXIT_FAILURE);
		}
		write_contiguous_bytes_to_disk(0, sizeof sb, &sb, sb.block_size);
	}

	return sb;
}

void sfs_base_super_block_free(struct super_block *sb) {
	memset(sb, 0, sizeof *sb);
}
