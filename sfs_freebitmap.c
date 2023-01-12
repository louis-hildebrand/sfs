#include <string.h>

#include "disk_emu.h"
#include "sfs_freebitmap.h"


static int freebitmap_size(int total_num_blocks) {
	return total_num_blocks;
}

static int is_block_free(struct freebitmap *fbmp, disk_ptr block_num) {
	return fbmp->data[block_num];
}

static void mark_block(struct freebitmap *fbmp, disk_ptr block_num, int is_free) {
	fbmp->data[block_num] = is_free ? 1 : 0;
}


void sfs_freebitmap_release_block(struct freebitmap *fbmp, disk_ptr block_num) {
	mark_block(fbmp, block_num, 1);
}

void sfs_freebitmap_flush(struct freebitmap *fbmp) {
	write_contiguous_bytes_to_disk(
		fbmp->super_block->num_blocks - fbmp->num_blocks,
		freebitmap_size(fbmp->super_block->num_blocks),
		fbmp->data,
		fbmp->super_block->block_size
	);
}

struct freebitmap sfs_freebitmap_new(struct super_block *sb) {
	struct freebitmap fbmp;
	fbmp.super_block = sb;

	int num_bytes = sb->num_blocks;
	fbmp.num_blocks = ceil_div(num_bytes, sb->block_size);

	fbmp.data = calloc_or_exit(num_bytes, 1);
	// Mark all data blocks (i.e., blocks outside SB, inode table, and free bitmap) as free
	for (int i = 1 + sb->num_inode_blocks; i < sb->num_blocks - fbmp.num_blocks; i++) {
		mark_block(&fbmp, i, 1);
	}

	sfs_freebitmap_flush(&fbmp);

	return fbmp;
}

struct freebitmap sfs_freebitmap_from_disk(struct super_block *sb) {
	struct freebitmap fbmp;
	fbmp.super_block = sb;

	int num_bytes = sb->num_blocks;
	fbmp.num_blocks = ceil_div(num_bytes, sb->block_size);

	fbmp.data = calloc_or_exit(num_bytes, 1);
	read_contiguous_bytes_from_disk(sb->num_blocks - fbmp.num_blocks, num_bytes, fbmp.data, sb->block_size);

	return fbmp;
}

void sfs_freebitmap_free(struct freebitmap *fbmp) {
	if (fbmp->data != NULL) {
		free(fbmp->data);
	}
	memset(fbmp, 0, sizeof *fbmp);
}

disk_ptr sfs_freebitmap_reserve_block(struct freebitmap *fbmp) {
	disk_ptr first_data_block = 1 + fbmp->super_block->num_inode_blocks;
	disk_ptr first_fbmp_block = fbmp->super_block->num_blocks - fbmp->num_blocks;
	for (disk_ptr i = first_data_block; i < first_fbmp_block; i++) {
		if (is_block_free(fbmp, i)) {
			mark_block(fbmp, i, 0);
			return i;
		}
	}
	return DISK_NULL;
}
