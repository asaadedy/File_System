#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include "ext2.h"
#include "file_system_shared.h"
/*A global pointer to the current location of the image disk*/
unsigned char *disk;

int main(int argc, char **argv){
    char buffer[EXT2_BLOCK_SIZE];
    char *name;
    struct ext2_inode *dest_inode, *new_inode;
	unsigned int dest_inode_num, new_inode_num;
    struct stat sb;
    FILE *source;
    long int source_blocks;
    int *second_level;
    int read = 0;
    size_t buff_size = 0;
    unsigned char *block;

    if(argc != 4){     // Only three arguments allowed
        fprintf(stderr, "Usage: ext2_cp [disk name] [file path] [copy destination path] \n");
        exit(-1);
    }else{
        disk = disk_pointer(argv[1]);   //assign disk as apinter to the image disk file
        clean_path(argv[3]);
        dest_inode_num = find_inode(disk, EXT2_ROOT_INO, argv[3]);
    }
    if(dest_inode_num == -1){
        return ENOENT;
    }

    if((source = fopen(argv[2], "r")) == NULL){
        return ENOENT;
    }
    if (stat(argv[2], &sb) == -1){
        return ENOENT;
    } 
    name = strrchr(argv[2], '/');
    if(name == NULL){
        name = argv[2];
    }
    double bl = (double)sb.st_size / EXT2_BLOCK_SIZE;
    source_blocks = (int) ceil(bl);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE));
    dest_inode = &inode_table[dest_inode_num - 1];

    if(S_ISREG(dest_inode->i_mode)){
        return ENOENT;
    }
	if(S_ISDIR(dest_inode->i_mode)){
        new_inode_num = reserve_inode(disk) + 1;
        new_inode = &inode_table[new_inode_num - 1];
        new_inode->i_mode = EXT2_S_IFREG;
        new_inode->i_links_count = 1;
        new_inode->i_dtime = 0;
        add_dir_entry(disk, dest_inode, new_inode_num, name, strlen(name));
    } else{
        return ENOENT;
    }

    for(int i = 0; i < source_blocks; i++) {
        // Copy into direct blocks
        if(i < 12){
            new_inode->i_block[i] = reserve_block(disk);
		    block = disk + (new_inode->i_block[i] * EXT2_BLOCK_SIZE);
        // Switch from direct blocks to the first indirect
        } else if(i == 12){
            new_inode->i_block[i] = reserve_block(disk); 
            second_level = (int *) (disk + (new_inode->i_block[i] * EXT2_BLOCK_SIZE));
            second_level[0] = reserve_block(disk);
            block = disk + (second_level[0] * EXT2_BLOCK_SIZE);
        } else{
            second_level[(i - 12)] = reserve_block(disk);
            block = disk + (second_level[(i - 12)] * EXT2_BLOCK_SIZE);
        }
        // Copy the content to the block
        buff_size = fread(buffer, 1, 1024, source);
        read += buff_size;
        memcpy(block, buffer, buff_size);
        new_inode->i_blocks += 2;
        if(read == sb.st_size){
            break;
        }
    }
    new_inode->i_size = sb.st_size;
	return 0;
}

