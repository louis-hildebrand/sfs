CC := gcc
CFLAGS := -ansi -pedantic -Wall -std=gnu99 `pkg-config fuse --cflags --libs`
LDFLAGS := `pkg-config fuse --cflags --libs`
SHELL := /bin/bash

.PHONY: all clean runtest test

SOURCES := sfs_api sfs_base sfs_directory sfs_freebitmap sfs_inode sfs_ofdt disk_emu
OBJECTS := $(addsuffix .o,$(SOURCES))


# FUSE
all: sfs_old sfs_new

sfs_old: fuse_wrap_old.o $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

sfs_new: fuse_wrap_new.o $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@


# Tests
runtest: test
	@./sfs_test0 &> test0.log; \
	echo "Test 0 status: $$?"; \
	./sfs_test1 &> test1.log; \
	echo "Test 1 status: $$?"; \
	./sfs_test2 &> test2.log; \
	echo "Test 2 status: $$?"; \
	./sfs_test3 &> test3.log; \
	echo "Test 3 status: $$?"

test: sfs_test0 sfs_test1 sfs_test2 sfs_test3

sfs_test0: sfs_test0.o $(OBJECTS)

sfs_test1: sfs_test1.o $(OBJECTS)

sfs_test2: sfs_test2.o $(OBJECTS)

sfs_test3: sfs_test3.o $(OBJECTS)


# Cleanup
clean:
	rm -f *.o *.log sfs_test[0-9] sfs_new sfs_old .sfs_store
	rm -rf ./filesystem/
