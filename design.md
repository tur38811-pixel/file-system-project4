Logical and Physical Directory

![Logical and Physical Directory Diagram](labs/file-system-project4/physicallogicaldirs.png)

The physical directory is what's stored on mydisk. It holds the File Allocation Table In blocks 1 and 2 and the data stored there stays there even after the program stops running. It also hold the root stores the name, size and where the file is in the FAT. Whereas on the logical directory (the RAM), gets data from disk for the program to use quickly and then the changed data gets loaded back to the disk. 

Logical and Physical Directory Contents

The FAT is shared with all files in the file system and essentially, each file has a starting point in the table, and just follows the chain of where the next block is until it reaches –1 (EOF). 

![Diagram](labs/file-system-project4/dircontents.png)

The root directory holds the name and size of the files. The first block is how the program knows where to start from in the FAT. 


Directories Memory Allocation

Physical Directory: 
ALLOCATION: when fs_write() needs an empty block to write to, it will go through the file allocation table and look for –2 that will signal an empty block and will write –1 to it as a placeholder in case that’s all the writes. 

FREEING MEMORY: when fs_delete() is called it follows the chain of blocks back and marks each –2, whhich signals that they are free 


Logical Directory: 
ALLOCATION: When fs_create() is called it finds an empty slot in root and fills in name, size and its first block index. 

FREEING MEMORY: when fs_delete() is called the name is removed, the size is set to 0 and the first block is –2.  


File System Functions Explanations
make_fs(): creates disk using make_disk from disk.c and inititilaizes FAT array with -2 and then closes disk

mount_fs(): this opens the disk and reads the first two blocks into the FAT and block 3 into root.

umount_fs(): writes FAT back to to first blocks and root back to block 3, closes disk

fs_create(): looks for empty slot in root, and fills in information (name, size to 0 and first block to -2)

fs_open(): searches root for a name, finds free slot in fd and fills it with the root index

fs_delete(): checks if fd points to file, id not then it follows the FAT blocks and marks each block with -2

fs_read(): goes to current position using FAT blocks chain and reads block one a time and copies into the buffer until the total bytes is completed.

fs_write(): goes to current position and writes data block by block and then updates file size and current positon

fs_get_filesize(): returns root[root_idx].size, it uses fd to see what fils open and then gets the index of the size and returns it

fs_lseek(): chekcs if the given position is within the file size and then sets fd table block tocurrent to the given new position

fs_truncate(): goes to the FAT chain to the place you want to truncate to and marks tha tblock -1 and frees the rest by marking to -2.


Hexdumps
Before:
Boot: ![Boot](labs/file-system-project4/rootbefore.png)

Root: ![Root](labs/file-system-project4/rootdirbefore.png)

FAT: ![FAT](labs/file-system-project4/FATbefore.png)

After:
![FAT](labs/file-system-project4/after.png)
