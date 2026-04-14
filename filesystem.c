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
        if (strcmp(root[i].name, name) == 0 && root[i].name[0] != '\0') {
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
    fd_table[fd].root_idx  = root_idx;
    fd_table[fd].current = 0;

    return fd;
}

int fs_close(int fd) {
    if (fd < 0 || fd >= MAX_FD || fd_table[fd].filled == 0) {
        return -1;
    }
    fd_table[fd].filled = 0; //marks the slot as empty
    return 0;
}

int fs_create(char *name) {
    //check if the file already exists
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(root[i].name, name) == 0 && root[i].name[0] != '\0') {
            return -1;
        }
    }

    //find an empty spot in root directory
    int root_idx = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (root[i].name[0] == '\0') { //checks if the name is empy
            root_idx = i; // root isx saves empty spot
            break;
        }
        
    }
    if (root_idx == -1) {
        return -1; //if root idx stays -1 no empty in root
    }

    //create the file in the root directory
    strncpy(root[root_idx].name, name, 16); //copes name to new file
    root[root_idx].first_block_idx = FAT_FREE; //no blocs allocated yet
    root[root_idx].size = 0; ///file starts as empty

    return 0;
}

int fs_delete(char *name) {
    //find the file in the root directory
    int root_idx = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(root[i].name, name) == 0 && root[i].name[0] != '\0') {
            root_idx = i;
            break;
        }
    }
    if (root_idx == -1) {
        return -1; //file not found
    }

    for (int i = 0; i < MAX_FD; i++) {
        if (fd_table[i].filled == 1 && fd_table[i].root_idx == root_idx) { // if the slot is filled and if root isx is the current idx
            return -1; //file is alrady open
        
        }
    }


    //free the blocks allocated to the file in fat
    int block_idx = root[root_idx].first_block_idx;

    while (block_idx != -1) {
        int next_block_idx = fat[block_idx]; //get next block index from FAT
        fat[block_idx] = FAT_FREE; //mark  block as free
        block_idx = next_block_idx; 
    }

    //remove the file from the root directory
    for (int i = 0; i < 16; i++) {
        root[root_idx].name[i] = '\0'; //clears the name
    }

    root[root_idx].first_block_idx = FAT_FREE; //reset first block index
    root[root_idx].size = 0; //reset size


    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte) {
    //check if fd is valid
    if (fildes < 0 || fildes >= MAX_FD || fd_table[fildes].filled == 0) {
        return -1;
    }

    int root_idx = fd_table[fildes].root_idx; //root index from fd table
    int file_size = root[root_idx].size; //get file size from rot directory

    if (nbyte > file_size - fd_table[fildes].current) { //adjust count if it exceeds file size
        nbyte = file_size - fd_table[fildes].current;
    }

    int bytes_read = 0;
    int block_idx = root[root_idx].first_block_idx; //get first block index from root directory

    //navigate to the current position in the file
    int current_pos = fd_table[fildes].current;
    while (current_pos >= BLOCK_SIZE && block_idx != FAT_FREE) {
        block_idx = fat[block_idx]; //move to next block
        current_pos -= BLOCK_SIZE; 
    }

    while (bytes_read < nbyte && block_idx != FAT_FREE) {
        char block_buffer[BLOCK_SIZE];
        block_read(block_idx + 4096, block_buffer); //read the current block into buffer

        int bytes_to_copy = BLOCK_SIZE - current_pos; //calculate how many bytes to copy from the currnt block
        if (bytes_to_copy > nbyte - bytes_read) {
            bytes_to_copy = nbyte - bytes_read; //adjust bytes to copy if it exceeds remaining count
        }


        for (int i = 0; i < bytes_to_copy; i++) {
            ((char*)buf)[bytes_read + i] = block_buffer[current_pos + i]; //copy bytes userbuffer
        }

        bytes_read += bytes_to_copy; //update bytes reas
        current_pos = 0; 
        block_idx = fat[block_idx]; //move to next block
    }

    fd_table[fildes].current += bytes_read; //update current position in fd table

    return bytes_read;
}

int fs_write(int fildes, void *buf, size_t nbyte) {
    //checkss if fd is valid
    if (fildes < 0 || fildes >= MAX_FD || fd_table[fildes].filled == 0) {
        return -1;
    }
    int bytes_written = 0;

    int root_idx = fd_table[fildes].root_idx; //get root idx from fd table

    int block_idx = root[root_idx].first_block_idx; //get first block index from root directory

    //urrent pos in the file
    int current_pos = fd_table[fildes].current;
    int prev = -1;

    while (current_pos >= BLOCK_SIZE) {

        if (block_idx == FAT_FREE) { //if we need to allocate a new block
            block_idx = -1;
            //allocat new block and get its index
            for (int i = 0; i < FAT_SIZE; i++) {
                if (fat[i] == FAT_FREE) {
                    block_idx = i;
                    break;
                }
            }

            if (block_idx == -1) {
                break; //no more blocks available
            }


            if (root[root_idx].first_block_idx == FAT_FREE) {
                root[root_idx].first_block_idx = block_idx; //set first block index in root directory if it's the first block
            } else {

                fat[prev] = block_idx; //mark the new block as end of file in FAT
            }
        }
        block_idx = fat[block_idx]; //move to next block
        current_pos -= BLOCK_SIZE; 
    }


    while (bytes_written < nbyte) {
        if (block_idx == FAT_FREE) { //if we need to allocate a new block
            block_idx = -1;
            //allocate new block and get its index
            for (int i = 0; i < FAT_SIZE; i++) {
                if (fat[i] == FAT_FREE) {
                    block_idx = i;
                    break;
                }
            }

            if (block_idx == -1) {
                break; //no more blocks available
            }

            if (root[root_idx].first_block_idx == FAT_FREE) {
                root[root_idx].first_block_idx = block_idx; //set first block index in root directory if it's the first block
            } else {
                fat[block_idx] = FAT_FREE; //mark the new block as end of file in FAT
            }
        }

        char block_buffer[BLOCK_SIZE];

        block_read(block_idx + 4096, block_buffer); //reads to buffera
        int bytes_to_copy = BLOCK_SIZE - current_pos; //calculate how many bytes to copy to the current block
        if (bytes_to_copy > nbyte - bytes_written) {

            bytes_to_copy = nbyte - bytes_written; //adjust bytes to copy if it exceeds remaining count
        }
        for (int i = 0; i < bytes_to_copy; i++) {
            block_buffer[current_pos + i] = ((char*)buf)[bytes_written + i]; //copy bytes from user buffer to block buffer
        }

        block_write(block_idx + 4096, block_buffer); //write the current block back to disk
        bytes_written += bytes_to_copy; //update bytes written
        current_pos = 0;
        prev = block_idx;

        block_idx = fat[block_idx]; //move to next block   
    }
    fd_table[fildes].current += bytes_written; //update current position in fd table
    if (fd_table[fildes].current > root[root_idx].size) {
        root[root_idx].size = fd_table[fildes].current; //update file size in root directory if we wrote past the previous end of file
    }
    return bytes_written;
}



int main() {

    return 0;
}



