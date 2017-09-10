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
#include <time.h>
#include "ext2.h"
#include "file_system_shared.h"


/*A global pointer to the current location of the image disk*/
unsigned char *disk;

int main(int argc, char **argv){
    struct ext2_inode *dest_inode;
	int  signal = 0;
	int dest_inode_num = 0;
	int parent_dir_inode = EXT2_ROOT_INO;   
 
	char current_dir [3] = {'.','\0','\0'};

	if(argc != 3){       // Only three arguments allowed
		fprintf(stderr, "Usage: ext2_mkdir [disk name] [dir path]\n");
        exit(-1);
    }else{
		disk = disk_pointer(argv[1]);	//assign disk as apinter to the image disk file
		clean_path(argv[2]);
		char hold_path[strlen(argv[2])];
		char hold_name[strlen(argv[2])];
		char * location = strrchr(argv[2], '/');
		if(location == NULL){
			strncpy(hold_name, argv[2], strlen(argv[2]));
			hold_name[strlen(argv[2])] = '\0';
			dest_inode_num = find_inode(disk, EXT2_ROOT_INO, argv[2]);
			if(dest_inode_num == -1){		// if == -1 means we need to create the dir 
				signal = 1;	
			}else{
				dest_inode = (struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE)) + dest_inode_num - 1;
			    if(S_ISDIR(dest_inode->i_mode)){
					printf("directory already exist\n");
					return EEXIST;
				}else{
					signal = 1;
				}	
			}
		}else{
			strncpy(hold_name, location + 1, strlen(location) - 1);
			strncpy(hold_path, argv[2], location - argv[2]);
			hold_name[strlen(location) -1 ] = '\0';
			hold_path[location - argv[2]] = '\0';
			dest_inode_num = find_inode(disk, EXT2_ROOT_INO, hold_path);
			parent_dir_inode = dest_inode_num;

			if(dest_inode_num == -1){		// if == -1 means we need to create the dir 
				printf("No such file or directory\n");
				return ENOENT;	
			}else{
				dest_inode_num = find_inode(disk, EXT2_ROOT_INO, argv[2]);
				if(dest_inode_num == -1){		// if == -1 means we need to create the dir 
					signal = 1;	
				}else{
					dest_inode = (struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE)) + dest_inode_num - 1;
			    	if(S_ISDIR(dest_inode->i_mode)){
						printf("directory already exist\n");
						return EEXIST;
					}else{
						signal = 1;
					}	
				}
			}	
		}
		dest_inode = (struct ext2_inode*) (disk + (5*EXT2_BLOCK_SIZE)) + parent_dir_inode - 1;
		
		if(signal == 1){	//create the directory if hold
			int available_inode = reserve_inode(disk);
			if(available_inode == -1){
				printf("No available inodes found");
				exit(1);     
			}
			struct ext2_inode * new_inode = (struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE)) + available_inode ;
 			new_inode->i_mode = EXT2_S_IFDIR;
			new_inode->i_size = dir_ent_size(1) + dir_ent_size(2); 
			new_inode->i_links_count += 1;	

		    // the created time of the directory
			dest_inode->i_ctime = (unsigned int) time(NULL);
			//the block location of the inode
			add_dir_entry(disk,new_inode, available_inode + 1, current_dir, 1);//for the current dir with inode available_inode
			current_dir[1] = '.';
			add_dir_entry(disk, new_inode, parent_dir_inode, current_dir, 2);//for parent dir  with inode parent_dir_inode
			add_dir_entry(disk, dest_inode , available_inode + 1, hold_name, strlen(hold_name));//for the current dir with inode available_inode
			
			double bl =(double)dest_inode->i_size / EXT2_BLOCK_SIZE;
    	    if((bl - (int) bl) > 0.0){
        		dest_inode->i_blocks = (int)(bl) + 1;
  			}else{
				dest_inode->i_blocks = (int)(bl);
   			}
		}
		struct ext2_group_desc * gd = (struct ext2_group_desc *) (disk +  (2 * EXT2_BLOCK_SIZE));
		gd->bg_used_dirs_count++;

		return 0;
	}
}
