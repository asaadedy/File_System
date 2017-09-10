#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include "ext2.h"
#include "file_system_shared.h"
#include "ext2_rm_bonus.h"

int main(int argc, char **argv){

	unsigned char * disk = NULL;	
	int inode_num = NULL;
	struct ext2_inode * inode = NULL;	
	char argument[3];
	argument[0] = '-';
	argument[1] = 'r';
	argument[2] = '\0';
	
	disk = disk_pointer(argv[1]);	
	if (argc == 3){
		clean_path(argv[2]);
		inode_num = find_inode(disk, EXT2_ROOT_INO, argv[2]);	//find the inode of the given path file
		if (inode_num == -1){
			printf("No such file or directory\n");
			return ENOENT;
		}
		inode = (struct ext2_inode*) (disk + (5 * EXT2_BLOCK_SIZE)) + inode_num - 1;
        	if(S_ISDIR(inode->i_mode)){
				printf("The file is a directory and cannot be removed, unless -r was passed as argument\n");
           		return EISDIR;
			}else{
				rm_file(disk, argv[2]);//remove non-directory file 
			}
	}else if (argc == 4){//check to see if a directory removal is requested
		if(strcmp(argv[2], argument) == 0){
			clean_path(argv[3]);
			inode_num = find_inode(disk, EXT2_ROOT_INO, argv[3]);
			inode = (struct ext2_inode*) (disk + (5*EXT2_BLOCK_SIZE)) + inode_num-1;
 			if (inode_num == -1){
        	    printf("No such file or directory\n");
    	        return ENOENT;
	        }
			if(S_ISDIR(inode->i_mode)){
				rm_dir(disk, inode, argv[3]);//remove directory
			}else{
				rm_file(disk, argv[3]);//remove non-drectory file
			}
		}else{
			printf("%s is undifined\n", argv[2]);
			exit(1); 
		}
	}else{
		fprintf(stderr, "Usage: Wrong number of arguments was passed\n");
        exit(1);
	}
	return 0;
}    
