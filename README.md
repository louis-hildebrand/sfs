# sfs (Louis Hildebrand)

## Background
This is a simple file system that can be run with FUSE on Ubuntu 18. It was developed for assignment 3 of ECSE 427: Operating Systems at McGill University in the Fall 2022 semester.

The disk emulator (`disk_emu.c`, `disk_emu.h`), FUSE wrapper (`fuse_wrap_old.c`, `fuse_wrap_new.c`), and tests (`sfs_test0.c`, `sfs_test1.c`, `sfs_test2.c`, `sfs_tst3.c`) were all provided by the course staff (Prof. Muthucumaru Maheswaran and the TAs). A Makefile was also provided, but it was substantially modified.

## Build System

### Building for FUSE
Run
```sh
make all
```

This will generate executables `sfs_new` and `sfs_old`.

Create a new directory (e.g., by running `mkdir filesystem`). Create a new "disk" by running
```sh
./sfs_new filesystem
```

If you wish to restart the file system, unmount the existing filesystem and then launch it again by running
```sh
fusermount -u filesystem
./sfs_old filesystem
```

### Building and Running Tests
Run
```sh
make test
```
to build all the tests. This will generate executables `sfs_test0`, `sfs_test1`, and so on. Alternatively, run `make sfs_test0` to build only test 0, run `make sfs_test1` to build only test 1, etc.

To run all the tests in succession, run
```sh
make runtest
```
All test output for `sfs_test<n>` will be redirected to `test<n>.log`. The command itself will simply print out the status code of each test. If a test passes, its status code will be 0.

The tests can also be run indivdually by running the corresponding executable.

### Cleaning All Artifacts
Run
```sh
make clean
```
