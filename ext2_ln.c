#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include "ext2.h"
#include "file_system_shared.h"

/*A global pointer to the current location of the image disk*/
unsigned char * disk;

int main(int argc, char **argv){

	//an ext2 inode struct pointer
    int source_inode_num, link_inode_num, parent_inode_num, sym_flag, source_blocks;
    struct ext2_inode *link_inode, *parent_inode;
    char *link_arg, *link_path, *link_name, *parent_path, *source_path;
    unsigned char *block;
	char argument[] = "-s";

    if((argc > 5) || (argc < 4)){
        fprintf(stderr, "Usage: ext2_ln [-s] [ext2_disk] [abs_path] [abs_path]\n");
        exit(1);
	}else if(argc == 4){
		disk = disk_pointer(argv[1]);		//assign disk as a pointer to the image disk file
		clean_path(argv[2]);
        source_path = malloc(strlen(argv[2]) + 2);
        // Make sure that the path is absolute
        source_path[0] = '/';
        strcpy((source_path + 1), argv[2]);

        clean_path(argv[3]);
		source_inode_num = find_inode(disk, EXT2_ROOT_INO, argv[2]);
        sym_flag = 0;
        link_arg = argv[3];
    // Case for when the sym link is called
	}else if(argc == 5){
		if(strcmp(argv[2], argument) == 0){
			disk = disk_pointer(argv[1]);
		    clean_path(argv[3]);
            clean_path(argv[4]);
            source_path = malloc(strlen(argv[3]) + 2);
            source_path[0] = '/';
            strcpy((source_path + 1), argv[3]);

            link_arg = argv[4];
		    source_inode_num = find_inode(disk, EXT2_ROOT_INO, argv[3]);
            sym_flag = 1;
		}else{
			printf("%s is undifined\n", argv[2]);
			exit(1); 
		}
	}
	if(source_inode_num == -1){
		printf("No such file or directory\n");				
		return ENOENT;  
	}
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE));

    // If the path is relative
    if((strrchr(link_arg, '/')) == NULL){
       link_name = malloc(strlen(link_arg) + 1);
       link_path = malloc(strlen(link_arg) + 2);
       strcpy(link_name, link_arg);
       link_path[0] = '/';
       strcpy((link_path + 1), link_arg); 
    } else{
        link_name = malloc(strlen(strrchr(link_arg, '/')) + 1);
        link_path = malloc(strlen(link_arg) + 2);
        strcpy(link_name, (strrchr(link_arg, '/') + 1));
        link_path[0] = '/';
        strcpy((link_path + 1), link_arg);
    }
    
    clean_path(link_arg);
    link_inode_num = find_inode(disk, EXT2_ROOT_INO, link_arg);
    link_inode = &inode_table[link_inode_num - 1];
    // Check if the link already exists
    if(S_ISDIR(link_inode->i_mode)){
        return EISDIR;
    }
    if (S_ISLNK(link_inode->i_mode)){
        return EEXIST;
    }

    parent_path = malloc(strlen(link_path) + 1);
	strcpy(parent_path, link_path);
	*(strrchr(parent_path, '/') + 1) = '\0';
    clean_path(parent_path);

    parent_inode_num = find_inode(disk, EXT2_ROOT_INO, parent_path);
    parent_inode = &inode_table[parent_inode_num - 1];

    if(sym_flag){
        // set values for the new inode
        link_inode_num = reserve_inode(disk) + 1;
        link_inode = &inode_table[link_inode_num - 1];
        link_inode->i_mode = EXT2_S_IFLNK;
        link_inode->i_dtime = 0;
        link_inode->i_links_count = 1;
		add_dir_entry(disk, parent_inode, link_inode_num, link_name, (int) strlen(link_name));

        double bl = (double)strlen(source_path) / EXT2_BLOCK_SIZE;
        source_blocks = (int) ceil(bl);
        if(strlen(source_path) >= 59){
            // Copy the path into the link blocks
            for (int i = 0; i < source_blocks; i++) {
                link_inode->i_block[i] = reserve_block(disk);
                link_inode->i_blocks += 2;
                block = disk + (link_inode->i_block[i] * EXT2_BLOCK_SIZE);
                if (strlen(source_path) >= 1023){
                    memcpy(block, source_path, 1024);
                    source_path = &source_path[1024];    // Move pointer one after the last copied
                    link_inode->i_size += 1024;
                } else{
                    memcpy(block, source_path, (strlen(source_path)) + 1);
                    link_inode->i_size += (strlen(source_path) + 1);
                }
            }

            // Optimization for when the path is short
        } else{
            memcpy(link_inode->i_block, source_path, (strlen(source_path) + 1));
            link_inode->i_size += (strlen(source_path) + 1);
        }
    // Hard link case
    } else{
        clean_path(source_path);
        link_inode_num = find_inode(disk, EXT2_ROOT_INO, source_path);
        link_inode = inode_pointer(link_inode_num, disk);
    	add_dir_entry(disk, parent_inode, link_inode_num, link_name, (int) strlen(link_name));
    }
    return 0;

}
