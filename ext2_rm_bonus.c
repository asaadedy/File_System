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

/* PRECONDITION: a directory inode should be passed as an argument
removes a directory with it's content files
PARAM: disk, a pointer to the mapped image disk
PARAM: inode, a struct pointer of type ext2_inode, which represent a directory inode
PARAM: path, the abslout path to the given directory
RETURNL: Returns 0 on successfull removal
*/
int rm_dir(unsigned char * disk, struct ext2_inode * inode, char *path){ 
	
	int Blocks;
    int offset = 0;
	int signal = 0;
    int * second_level = NULL;
	struct ext2_dir_entry_2 * dir_file = NULL;
	struct ext2_inode * hold_inode = NULL;
	char file_path[260];
	
	strncpy(file_path, path, strlen(path));
	file_path[strlen(path)] = '\0';

	// remove each file in the directory
	int i = 0;
    while(((int)inode->i_block[i] != 0) & (i<15)){
    	if(i < 12){
        	Blocks = (int)inode->i_block[i];
        }else if(i == 12){
        	second_level = (int *) (disk + (inode->i_block[i] * EXT2_BLOCK_SIZE));
            Blocks = second_level[0];
        }else{
        	Blocks = second_level[i - 12];
        }
		while(offset < EXT2_BLOCK_SIZE){
			signal = 0;
			dir_file = (struct ext2_dir_entry_2 *) (disk + offset + (Blocks * EXT2_BLOCK_SIZE));
			hold_inode = (struct ext2_inode*) (disk + (5*EXT2_BLOCK_SIZE)) + (int)dir_file->inode - 1;
			concatinate(file_path, dir_file->name, dir_file->name_len);
			if(S_ISDIR(hold_inode->i_mode)){//if file is directory
				if(dir_file->name_len > 1){
                    if(((dir_file->name[0] == '.') && (dir_file->name[1] == '\0'))){
                		signal = 1;
					}
				}
				if(dir_file->name_len > 2){
                	if(((dir_file->name[0] == '.') && (dir_file->name[1] == '.') && (dir_file->name[2] == '\0'))){
                    	signal = 1;
		        	}
				}	    
				if(((dir_file->name_len == 1) && (dir_file->name[0] == '.'))){
					signal = 1;
                }
                if(((dir_file->name_len == 2) && (dir_file->name[0] == '.') && (dir_file->name[1] == '.'))){
					signal = 1;
				}
				if(signal == 0){	
					 rm_dir(disk, hold_inode, file_path);
				}
			}else{
               	rm_file(disk, file_path);
            }
		
			seperate_strings(file_path, dir_file->name_len);//remove the name at the end of char *file_path
			offset += dir_file->rec_len;
		}
		i++;
	}	
	rm_file(disk, file_path);
	return 0;
}

/*create the updated path in the new directory. Adds name to the end of the path*/
void concatinate(char *path, char *name, int name_len){
	int len = strlen(path);
	if(path[len -1 ] != '/'){
		path[len] = '/';
		path[len+1] = '\0';
		len += 1;
	}
	strncat(path, name, name_len);
	path[len + name_len  ] = '\0';
}


/*separate the name from end of the path*/
void seperate_strings(char *path, int name_len){
	int len = strlen(path);
	if(path[len -1 ] != '/'){
		path[len - name_len] = '\0';
	}else{
		path[len - name_len +1] = '\0';
	}
}

