#include <string.h>
#include "disk.h"

#define FAT_SIZE 4096
#define FAT_FREE -2

//strcut for root directory files to write to
typedef struct {
    char name[16];
    short first_block_idx; //where it starts in fat idx
    int size; //size of file
} dirfile;

short fat[FAT_SIZE]; //fat array from disk

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

int main() {
    return 0;
}