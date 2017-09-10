#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include "ext2.h"
#include "file_system_shared.h"

/*A global pointer to the current location of the image disk*/
unsigned char * disk;

int main(int argc, char **argv){

	//an ext2 inode struct pointer
	struct ext2_inode *inode;
	unsigned int inode_num; 
	int offset = 0;
	int Blocks = 0;
	int signal = 0;
	struct ext2_dir_entry_2 * dir;

	char argument[3];
	argument[0] = '-';
	argument[1] = 'a';
	argument[2] = '\0';
	
    if((argc > 4) || (argc < 3)){   					    //argumetns has to be either 3 or 4 otherwise exit
        fprintf(stderr, "Usage: Wrong number of arguments was passed\n");
        exit(1);
	}else if(argc == 3){
		disk =disk_pointer(argv[1]);						//assign disk as apinter to the image disk file
		clean_path(argv[2]);
		inode_num = find_inode(disk,EXT2_ROOT_INO, argv[2]); 
	}else if(argc == 4){
		if( strcmp(argv[2], argument) == 0){
			disk = disk_pointer(argv[1]);       			//assign disk as a pointer to the image disk fie
		clean_path(argv[3]);
		inode_num = find_inode(disk, EXT2_ROOT_INO, argv[3]);
		}else{
			printf("%s is undifined\n", argv[2]);
			exit(1); 
		}
	}
	if(inode_num == -1){
		printf("No such file or directory\n");				
		return ENOENT; 
	}
	// point to the inode to see if the file is directory or not
	inode = (struct ext2_inode*) (disk + (5 * EXT2_BLOCK_SIZE)) + inode_num - 1;
	dir = (struct ext2_dir_entry_2 *) (disk + (Blocks * EXT2_BLOCK_SIZE));
	if(S_ISDIR(inode->i_mode)){
		if(argc == 4){
			printf(".\n");
			printf("..\n");
						
		}
		int * second_level = NULL;
		int i = 0;
		while((int)inode->i_block[i] != 0){
			if(i < 12){
	            Blocks = inode->i_block[i];
    	    }else if(i == 12){
        	    second_level = (int *) (disk + (inode->i_block[i] * EXT2_BLOCK_SIZE));
           		Blocks = second_level[0];
        	}else{
            	Blocks = second_level[(i - 12)];
        	}
			//Blocks = (int)inode->i_block[i];
			dir = (struct ext2_dir_entry_2 *) (disk + (Blocks * EXT2_BLOCK_SIZE));
			while(offset < 1024){
				signal = 0;
				if (dir->name_len > 1){
					if(!((dir->name[0] == '.') && (dir->name[1] == '\0'))){
						if (dir->name_len > 2){
							if(!((dir->name[0] == '.') && (dir->name[1] == '.') && (dir->name[2] == '\0'))){
								signal++;						
							}
						}
					}
				}
				if(!((dir->name_len == 1) && (dir->name[0] == '.'))){
					signal++;
				}
				if(!((dir->name_len == 2) && (dir->name[0] == '.') && (dir->name[1] == '.'))){
					signal++;
				}
				if(signal == 3){
					print_name(dir->name, dir->name_len);
				}else if((dir->name_len == 1) && (dir->name[0] != '.')){
					print_name(dir->name, dir->name_len);
				}  
				offset += dir->rec_len;
				dir = (struct ext2_dir_entry_2 *) (disk + offset + (Blocks * 1024));
			}
			i++;	
		}
	}else{
		print_name(dir->name, dir->name_len);
	}
	return 0;
}


