#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfs_api.h"

#define MAX_FNAME_LENGTH 20 

void red () {
  printf("\033[1;31m");
}

void green () {
  printf("\033[1;32m");
}

void reset () {
  printf("\033[0m");
}

 
char *rand_name() 
{
  char fname[MAX_FNAME_LENGTH];
  int i;

  for (i = 0; i < MAX_FNAME_LENGTH; i++) {
    if (i != 16) {
      fname[i] = 'A' + (rand() % 26);
    }
    else {
      fname[i] = '.';
    }
  }
  fname[i] = '\0';
  return (strdup(fname));
}

int main() {
    mksfs(1);       /* Initialize the file system. */
    int f = sfs_fopen("test.txt");
    char my_data[] = "The quick brown fox jumps over the lazy dog";
    char out_data[1024];
    sfs_fwrite(f, my_data, sizeof(my_data)+1);
    sfs_fseek(f, 0);
    sfs_fread(f, out_data, sizeof(out_data)+1);
    if (strcmp(out_data,"The quick brown fox jumps over the lazy dog") != 0) {
        red();
        printf("ERROR: sfs_fseek and sfs_fread failed. Expected 'The quick brown fox jumps over the lazy dog' but got '%s'\n",out_data);
    }
    reset();

    // closing the file 
    if (sfs_fclose(f) != 0) {
        red();
        printf("ERROR: closing file\n");
    } else {
        green();
        printf("Closed file successfully\n");
    }

    int size = sfs_getfilesize("test.txt");
    if (size != 45) {
        red();
        printf("ERROR: sfs_getfilesize returned a wrong value. Expected 45 but got %d\n",size);
    } else {
        green();
        printf("sfs_getfilesize test passed.\n");
    }

    // reset outdata 
    memset(out_data, 0, sizeof out_data);

    // open the file again and write data one more time
    f = sfs_fopen("test.txt");
    sfs_fwrite(f, my_data, sizeof(my_data)+1);
    sfs_fseek(f, 16);
    sfs_fread(f, out_data, sizeof(out_data)+1);
    if (strcmp(out_data,"fox jumps over the lazy dog") != 0) {
        red();
        printf("ERROR: sfs_fseek and sfs_fread failed. Expected 'fox jumps over the lazy dog' but got '%s'\n",out_data);
    }
    reset();

    // reset outdata 
    memset(out_data, 0, sizeof out_data);

    // Do an fseek on the last character
    sfs_fseek(f, 43);
    sfs_fread(f, out_data, sizeof(out_data)+1);
    if (strcmp(out_data, "") != 0) {
        red();
        printf("ERROR: sfs_fseek and sfs_fread failed. Exptected '' but got '%s'\n",out_data);
    }
    reset();

    // Update size once again
    size = sfs_getfilesize("test.txt");

    // closing the file 
    if (sfs_fclose(f) != 0) {
        red();
        printf("ERROR: closing file\n");
    } else {
        green();
        printf("Closed file successfully\n");
    }
    
    if (size != 90) {
        red();
        printf("ERROR: sfs_getfilesize returned a wrong value. Expected 90 but got %d\n", size);
    } else {
        green();
        printf("sfs_getfilesize test passed\n");
    }

    // Try to read from a closed file
     if (sfs_fread(f, out_data, sizeof(out_data)+1) > 0) {
         red();
        printf("ERROR: read from a closed file testcase failed\n");
    } else {
        green();
        printf("Read from a closed file test passed\n");
    }

    // remove file 
    sfs_remove("test.txt");

    reset();

    // Create three more files
    char *names[2];
    int ncreate = 0;
    int fds[3];
    for (int i = 0; i < 3; i++) {
        names[i] = rand_name();
        fds[i] = sfs_fopen(names[i]);
        if (fds[i] < 0) {
            break;
        }
        sfs_fclose(fds[i]);
        ncreate++;
    }

    printf("Created %d files in the root directory\n", ncreate);

    int nopen = 0;
    for (int i = 0; i < ncreate; i++) {
        fds[i] = sfs_fopen(names[i]);
        if (fds[i] < 0) {
        break;
        }
        nopen++;
    }
    printf("Simultaneously opened %d files\n", nopen);

    int tmp;
    static char test_str[] = "The quick brown fox jumps over the lazy dog.\n";

    for (int i = 0; i < nopen; i++) {
        tmp = sfs_fwrite(fds[i], test_str, strlen(test_str));
        if (tmp != strlen(test_str)) {
            red();
            printf("ERROR: Tried to write %d, returned %d\n", (int)strlen(test_str), tmp);
        }
        if (sfs_fclose(fds[i]) != 0) {
            red();
            printf("ERROR: Closing file of handle %d failed\n", fds[i]);
        }
    }

    reset();
    printf("Directory listing\n");
    char *filename = (char *)malloc(MAXFILENAME);
    int max = 0;
    while (sfs_getnextfilename(filename)) {
        if (strcmp(filename, names[max]) != 0) {
            red();
            printf("ERROR misnamed file %d: %s %s\n", max, filename, names[max]);
        }
        max++;
    }
    green();
    printf("Succesfully passed the sfs_getnextfilename tests\n");

    reset();
}