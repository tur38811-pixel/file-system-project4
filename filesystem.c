#include <string.h>
#include "disk.h"
#include <stdio.h>

#define FAT_SIZE 4096
#define FAT_FREE -2
#define MAX_FD 32
#define MAX_FILES 64

//strcut for root directory files to write to
typedef struct {
    char name[16];
    short first_block_idx; //where it starts in fat idx
    int size; //size of file
} dirfile;

typedef struct {
    int filled; //keeps track if slot is taken
    int root_idx;//keeps track of where the file is in the root directoru
    int current; // keeps trackof files current position in file
} fdesc;

fdesc fd_table[MAX_FD]; //creates teh file descripter with max 32

static short fat[FAT_SIZE]; //fat array from disk
dirfile root[MAX_FILES]; //creates struct for each root dor file

int make_fs(char *disk_name) {
    char buffer[BLOCK_SIZE];


    // makes and opens disk
    //returns -1 for fails    
    if (make_disk(disk_name) < 0) {
        return -1;
    }
    if (open_disk(disk_name)< 0) {
        return -1;
    }

    short fat_init[FAT_SIZE];
    for (int i = 0; i < FAT_SIZE; i++) {
        fat_init[i] = FAT_FREE;
    }

    //first half of FAT goes into block 1
    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = ((char*)fat_init)[i];
    }
    block_write(1, buffer);

    //second half copies into block 2
    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = ((char*)fat_init + BLOCK_SIZE)[i];
    }
    block_write(2, buffer);

    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }
    block_write(3, buffer);

    if (close_disk() < 0) {
        return -1;
    }

    return 0;
}

int mount_fs(char *disk_name) {
    char buffer[BLOCK_SIZE];

    for (int i = 0; i < MAX_FD; i++) {
        fd_table[i].filled = 0; //clears the fd table with every mount
    }

    if (open_disk(disk_name) < 0) {
        return -1;
    }

    //loads fat to to buffer
    block_read(1, buffer);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        ((char*)fat)[i] = buffer[i];
    }

    block_read(2, buffer); //reads second block and coppes into second half of fat
    for (int i = 0; i < BLOCK_SIZE; i++) {
        ((char*)fat + BLOCK_SIZE)[i] = buffer[i];
    }
    
    block_read(3, buffer);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        //copies byetes to root
        ((char*)root)[i] = buffer[i];
    }

    return 0;
}

int umount_fs(char *disk_name) {
    char buffer[BLOCK_SIZE];

    for (int i = 0; i < MAX_FD; i++) {
        fd_table[i].filled = 0; //clears the fd table with every umount
    }

    //loads biffer to first block
    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = ((char*)fat)[i];
    }
    block_write(1, buffer);

    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = ((char*)fat + BLOCK_SIZE)[i];
    }
    block_write(2, buffer); 

    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        //copies root back to third block
         buffer[i] = ((char*)root)[i];
    }
    block_write(3, buffer);

    if (close_disk() < 0) {
        return -1;
    }

    return 0;
}

int fs_open(char *name) {

    //iterates through root directoyu tp find file
    int root_idx = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(root[i].name[0], name) && root[i].name[0] != '\0') {
            root_idx = i;
            break;
        }
    }
    //If file not found
    if (root_idx == 0) {
        return -1;
    }

    // find an empry slot in fd table
    int fd = -1;
    for (int i = 0; i < MAX_FD; i++) {
        if (fd_table[i].filled == 0) {
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        return -1;
    }

    //fill in the fd table entry
    fd_table[fd].filled = 1;
    fd_table[fd]. root_idx  = root_idx;
    fd_table[fd].current = 0;

    return fd;
}

int main() {

    if (make_fs("mydisk") < 0) {
        printf("makefs fail\n");
        return -1;
    }
    
    if (mount_fs("mydisk") < 0) {
        printf("mountfs fail\n");
        return -1;
    }
    if (umount_fs("mydisk") < 0) {
        printf("umountfs fail\n");
        return -1;
    }
    printf("sucess");
    return 0;
}