#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include "ext2.h"
#include "file_system_shared.h"

/*
Opens the file and the assigns the pointer disk_ref on the image_disk_file 
PARAMS: file, the image disk file name
PARAMS: disk_ref, an unsigned char pointer to the disk
*/
unsigned char * disk_pointer(char * file){
	unsigned char * disk_ref = NULL; 
	int fd = open(file, O_RDWR);		//open the image disk file 
	disk_ref = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk_ref == MAP_FAILED){
		perror("mmap");
		exit(1);
	}
	if(close(fd) == -1){
		perror("fclose");
		exit(1);
	}
	return disk_ref;	 
}


/*
PRECONDITION: '/' should be removed from the beginning and the end of path if exist. However, if path has only '/' then there is no need to remove anything 

Finds the inode of the file specified by path starting from a given inode
PARAMS: disk_ref, an unsigned char pointer to the disk
PARAMS: inode_num, the inode of the file the search is supposed to start
PARAMS: path, the absloute path of the desired file from the given inode 
PARAMS: file_name, the name of the file we are looking for 
RETURN: the inode of the file specified by the path and -1 if the path does not exist
*/
int find_inode(unsigned char *disk_ref, unsigned int inode_num, char * path){
	int Blocks;
    int offset = 0;
	int * second_level = NULL;
	char file_name [EXT2_NAME_LEN];
	struct ext2_inode * inode = (struct ext2_inode*) (disk_ref + (5 * EXT2_BLOCK_SIZE)) + inode_num-1;
	struct ext2_dir_entry_2 * dir_file = NULL;
	char * location = strchr(path, '/');

	if((strlen(path) == 1) && (path[0] == '/')){
		return EXT2_ROOT_INO; 
	}

	if(location != NULL) {
		strncpy(file_name, path, location - path);
		file_name[location - path] = '\0'; 
	}else{
		strncpy(file_name, path, strlen(path));
		file_name[strlen(path)] = '\0';
	} 
	if(S_ISDIR(inode->i_mode)) {
		int i = 0;
		while(((int)inode->i_block[i] != 0) & (i<15)){
			if (i < 12){ 
				Blocks = (int)inode->i_block[i];
			} else if(i == 12){
				second_level = (int *) (disk_ref + (inode->i_block[i] * EXT2_BLOCK_SIZE));
				Blocks = second_level[0];  
			} else{
				Blocks = second_level[i - 12];
			} 
			dir_file = (struct ext2_dir_entry_2 *) (disk_ref + (Blocks * EXT2_BLOCK_SIZE));
			while (offset < EXT2_BLOCK_SIZE) {
				//printf("name : %s\n",dir_file->name);
				if(strlen(file_name) <= dir_file->name_len){
					if(strncmp(dir_file->name, file_name, dir_file->name_len) == 0) {
						inode = (struct ext2_inode*) (disk_ref + (5*EXT2_BLOCK_SIZE)) + (int)dir_file->inode - 1;
						if(S_ISDIR(inode->i_mode)){
							if(location == NULL){
								return dir_file->inode;
							}else{
								path = location + 1;
								return find_inode(disk_ref, dir_file->inode , path);
							}
						}else{
							if(location == NULL){
								return dir_file->inode;
							}else{
								return -1;
							}
						}
					} 
				}
				offset += dir_file->rec_len;
				dir_file = (struct ext2_dir_entry_2 *) (disk_ref + offset + (Blocks * 1024));
			}
			i++;
		}
	}
		return -1;
}

/* remove uneccesary '/' from the path*/
void clean_path(char * path){
	char path_copy [EXT2_NAME_LEN];
	int path_index = 0; 
	int signal = 0;
	strncpy(path_copy, path, strlen(path)+1);
	for(int i = 0; i < strlen(path); i++) {
		if((path_copy[i] == '/') && (signal == 0)) {
			path[path_index] = path_copy[i];
			path_index ++;
			signal = 1;
		}
		else if(path_copy[i] != '/'){
			path[path_index] = path_copy[i];
			path_index ++;
			signal = 0;
		}
	}
	path[path_index] = '\0';
 
	if(strlen(path) > 1) { 
		if(path[0] == '/'){
			for(int i = 0; i < strlen(path); i++) {
				path[i] = path[i+1];
			}
		}
	}
	if(strlen(path) > 1) {
        if(path[strlen(path)-1] == '/'){
			path[strlen(path)-1] = '\0';
		}
	}

}

/*print string of character starting from the char * name and moving forward by bytes until a null terminator is incountered or size characters are printed
PARAM: name, A pointer to a string that contain name of a file 
PARAM: size, the lenght of the name */
void print_name( char* name, unsigned char size){
	int count = 0;
	while((count < (int)size) && (name[count]) != '\0'){
		printf("%c", name[count]);	
		count++;
	}
	printf("\n");
}


/*return an index of an available bit for the block bitmap, and it reserve that block by changing the bit from 0 to 1
PARAM: image disk pointer
RETURN: availabe block index, and -1 when there is no availabele block
*/
int reserve_block(unsigned char *disk ){
	char * loc = (char *)(disk + (3 * EXT2_BLOCK_SIZE));
    struct ext2_super_block *sp = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc * gd = (struct ext2_group_desc *) (disk +  (2 * EXT2_BLOCK_SIZE));
    //sp->s_blocks_count++;
    sp->s_free_blocks_count--;
    gd->bg_free_blocks_count--;
	unsigned int shift = 0x1;
	for (int i =0; i<16; i++){
        loc = (char *)(disk + (3 * EXT2_BLOCK_SIZE) + i);
        if (!((*loc) & 0x01)){ 
			*loc |= shift;
			return (i * 8) + 7; 
        }if (!((*loc >> 1) & 0x1)){
			*loc |= (shift << 1);
			return (i * 8) + 6;
        }if (!((*loc >> 2) & 0x1)){
			*loc |= (shift << 2);
			return (i * 8) + 5;
        }if (!((*loc >> 3) & 0x1)){
			*loc |= (shift << 3);
			return (i * 8) + 4;
        }if (!((*loc >> 4) & 0x1)){
			*loc |= (shift << 4);
			return (i * 8) + 3;
        }if (!((*loc >> 5) & 0x1)){
			*loc |= (shift << 5);
			return (i * 8) + 2;
        }if (!((*loc >> 6) & 0x1)){
			*loc |= (shift << 6);
			return (i * 8) + 1;
        }if (!((*loc >> 7) & 0x1)){
			*loc |= (shift << 7);
			return (i * 8) + 0;
        }
	}
	return -1;
}
//TO IHOR: if you wanna use the return value as the inode add 1 to it since the retrun is the index
/*return an index of an available bit for the inode bitmap, and it reserve that inode by changing the bit from 0 to 1
PARAM: image disk pointer
RETURN: availabe inode index , and -1 when there is no availabe inode 
*/
int reserve_inode(unsigned char *disk){
	unsigned int shift = 0x1;
	int available = -1;
    struct ext2_super_block *sp = (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc * gd = (struct ext2_group_desc *) (disk +  (2 * EXT2_BLOCK_SIZE));
    //sp->s_inodes_count++;
    sp->s_free_inodes_count--;
    gd->bg_free_inodes_count--;
    char * loc = (char *)(disk + (4 * EXT2_BLOCK_SIZE));
	for (int i =0; i<4; i++){
        loc = (char *)(disk + (4 * EXT2_BLOCK_SIZE) + i);
        if (!((*loc) & 0x01)){ 
			*loc |= shift;
			available = (i * 8) + 7; 
        }else if (!((*loc >> 1) & 0x1)){
			*loc |= (shift << 1);
			available =  (i * 8) + 6;
        }else if (!((*loc >> 2) & 0x1)){
			*loc |= (shift << 2);
			available =  (i * 8) + 5;
        }else if (!((*loc >> 3) & 0x1)){
			*loc |= (shift << 3);
			available =  (i * 8) + 4;
        }else if (!((*loc >> 4) & 0x1)){
			*loc |= (shift << 4);
			available = (i * 8) + 3;
        }else if (!((*loc >> 5) & 0x1)){
			*loc |= (shift << 5);
			available = (i * 8) + 2;
        }else if (!((*loc >> 6) & 0x1)){
			*loc |= (shift << 6);
			available = (i * 8) + 1;
        }else if (!((*loc >> 7) & 0x1)){
			*loc |= (shift << 7);
			available = (i * 8) + 0;
        }     
    }

	struct ext2_inode * inode = (struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE)) + (int)available;
	inode->i_blocks = 0;
	for (int i = 0; i< 15; i++){ 
	inode->i_block[i] = 0;
	}
	inode->i_size = 0;
	return available;
}



/*
remove a directory entry form a directory entry block. 
PARAM: disk_ref, a pointer to a maped disk on a memory
PARAM: dir_inode, the inode of the directory that the entry will be deleted from
PARAM: entry_name, the name of the entry intended to be delete
RETURN: return 0 if successfull, else return -1
*/
int remove_dir_entry(unsigned char *disk_ref, int dir_inode, char * entry_name){
	struct ext2_inode * inode = (struct ext2_inode*) (disk_ref + (5*EXT2_BLOCK_SIZE)) + dir_inode - 1;
	int offset = 0;
	int Blocks = 0;
	int * second_level = NULL;
	struct ext2_dir_entry_2 * dir_file = NULL;
	struct ext2_dir_entry_2 * dir_file_previouse = NULL;	
	struct ext2_dir_entry_2 * hold_dir = NULL;


	if(S_ISDIR(inode->i_mode)) {
		int i = 0;
		while((int)inode->i_block[i] != 0){
			if (i < 12){ 
				Blocks = (int)inode->i_block[i];
			} else if(i == 12){
				second_level = (int *) (disk_ref + (inode->i_block[i] * EXT2_BLOCK_SIZE));
				Blocks = second_level[0];  
			} else{
				Blocks = second_level[i - 12];
			}
			Blocks = (int)inode->i_block[i];
			dir_file = (struct ext2_dir_entry_2 *) (disk_ref + (Blocks * EXT2_BLOCK_SIZE));
			hold_dir = dir_file;
			while (offset < EXT2_BLOCK_SIZE) {
				if(strncmp(dir_file->name, entry_name, dir_file->name_len) == 0) {
					if(dir_file == hold_dir){
						if(dir_file->rec_len == 1024){
							inode->i_block[i] = 0;
							return 0;
						}
						else{
							hold_dir = (struct ext2_dir_entry_2 *) (disk_ref + dir_file->rec_len + (Blocks * EXT2_BLOCK_SIZE));
							int size = sizeof(unsigned int) + sizeof(unsigned short) + (2 * sizeof(unsigned char)) + dir_file->name_len;
							hold_dir->rec_len += dir_file->rec_len;
							strncpy((char *)dir_file, (char *) hold_dir, size); 
							return 0;
						}
					}
					else{
						dir_file_previouse->rec_len += dir_file->rec_len;
						return 0;
					}
				} 
				offset += dir_file->rec_len;
				dir_file_previouse = dir_file;
				dir_file = (struct ext2_dir_entry_2 *) (disk_ref + offset + (Blocks * EXT2_BLOCK_SIZE));
					
			}
			i++;
		}
		printf("the directory entry does not exist in the given inode");
		return -1;
	}else{
		printf("the inode dir_inode is not a directory inode");
		return -1;
	}
}
/*
Add a given file to the directory entry list.
PARAM: disk_ref, a pointer to a maped disk on a memory.
PARAM: dir, direcotry inode.
PARAM: inode, inode number of the new directory entry.
PARAM: name, name of the new directory entry.
PARAM: lngth, length of the name.
*/
void add_dir_entry(unsigned char *disk_ref, struct ext2_inode *dir, int inode, char *name, int lngth) {
    int len = dir_ent_size(lngth);
    struct ext2_dir_entry_2 *dir_ent;
    struct ext2_dir_entry_2 *curr_dir;
    struct ext2_inode *entry = (struct ext2_inode*) (disk_ref + (5*EXT2_BLOCK_SIZE)) + (int)inode - 1;
    int offset = 0, curr_size = 0, new_block = 0;
    unsigned char *block;

    for(int i = 0; i < 12; i++) {
        if(dir->i_block[i] == 0){
            dir->i_size += EXT2_BLOCK_SIZE;
            dir->i_block[i] = reserve_block(disk_ref);
            new_block = 1;
        }
        block = disk_ref + (dir->i_block[i] * EXT2_BLOCK_SIZE);
        offset = 0;
        while(offset < EXT2_BLOCK_SIZE) {
            if (!new_block){
                curr_dir = (struct ext2_dir_entry_2 *) (block + offset);
                curr_size = dir_ent_size(curr_dir->name_len);
                char *allign = (((char*)curr_dir) + curr_size);
                // Set the allignment
                int allignment = 4 - (curr_size % 4);
                allign += allignment;
                // If reached the end of the directory
                if(((offset + curr_dir->rec_len) == EXT2_BLOCK_SIZE) && ((curr_dir->rec_len) > (len + allignment))){
            	    dir_ent = (struct ext2_dir_entry_2 *) (((char*)curr_dir) + curr_size + allignment);
                    dir_ent->inode = inode;
                    dir_ent->name_len = lngth;
                    switch(entry->i_mode) {
        		        case EXT2_S_IFLNK :
                            dir_ent->file_type = EXT2_FT_SYMLINK;
                            break;
                        case EXT2_S_IFREG :
                            dir_ent->file_type = EXT2_FT_REG_FILE; 
                            break;
                        case EXT2_S_IFDIR :
                            dir_ent->file_type = EXT2_FT_DIR;
                            break; 
                        default :
                            dir_ent->file_type = EXT2_FT_UNKNOWN;
                            break;
                    }
                    strncpy(dir_ent->name, name, lngth);
                    dir_ent->rec_len = EXT2_BLOCK_SIZE - (offset + curr_size + allignment);
                    curr_dir->rec_len = curr_size + allignment;
                    return;
                }
                offset += curr_dir->rec_len;
            // else new block is used
            } else{
                dir_ent = (struct ext2_dir_entry_2 *)block;
                dir_ent->rec_len = EXT2_BLOCK_SIZE;
                dir_ent->inode = inode;
                dir_ent->name_len = lngth;
                switch(entry->i_mode) {
        		    case EXT2_S_IFLNK :
                        dir_ent->file_type = EXT2_FT_SYMLINK;
                        break;
                    case EXT2_S_IFREG :
                        dir_ent->file_type = EXT2_FT_REG_FILE; 
                        break;
                    case EXT2_S_IFDIR :
                        dir_ent->file_type = EXT2_FT_DIR;
                        break; 
                    default :
                        dir_ent->file_type = EXT2_FT_UNKNOWN;
                        break;
                }
                strncpy(dir_ent->name, name, lngth);
                return;
            } 
        }
    }
    // If reached this point, something went wrong.

}

int dir_ent_size(int name_length) {
    return sizeof(unsigned int) + sizeof(unsigned short) + (sizeof(unsigned char)*2) +
        (sizeof(char) * name_length);
}

void map_to_img(unsigned char * disk, char * file_name) {
	
	int fd = open(file_name, O_RDWR);		//open the image disk file 
	for (int i = 0; i < (EXT2_BLOCK_SIZE * 128); i++){
		write(fd, disk, 1);
		disk++;
	} 
	if(close(fd) == -1){
		perror("fclose");
		exit(1);
	}
}
struct ext2_inode *inode_pointer(int inode, unsigned char *disk) {
	return &((struct ext2_inode *) (disk + (5 * EXT2_BLOCK_SIZE)))[inode - 1];
}

/*Remove the given file with the name file_name from the disk image file with the name disk_img_name file
PARAM: disk_img_name, the name of the Image disk
PARAM: file_name, the name of the file intended to be deleted
*/

int rm_file(unsigned char *disk, char *file_path){
	struct ext2_inode *dest_inode;
	int dest_inode_num = 0;
 	int * second_level;

	clean_path(file_path);
	dest_inode_num = find_inode(disk, EXT2_ROOT_INO, file_path);
	dest_inode = (struct ext2_inode*) (disk + (5*EXT2_BLOCK_SIZE)) + dest_inode_num-1;

	if (dest_inode->i_links_count == 1){
		unsigned int shift = 0x1;
		char * loc = (char *)(disk + (4 * EXT2_BLOCK_SIZE));
		//free the inode bitmap of the removed file	
    	loc = (char *)(disk + (4 * EXT2_BLOCK_SIZE) + ((dest_inode_num-1)/8));
		int count_shift = 0;
		if(((dest_inode_num)%8) == 0){
			count_shift = 8;
		}
		else{
			count_shift = (dest_inode_num)%8; 
		}
		*loc &= ~(shift << (8 - count_shift));
	}

	//free the block bitmaps of the removed file
	double bl =(double)dest_inode->i_size / EXT2_BLOCK_SIZE;
	int source_blocks = 0;
	if((bl - (int) bl) > 0.0){ 
		source_blocks = (int)(bl) + 1;
	} else {
		source_blocks = (int)(bl);
	}
	for(int i = 0; i < source_blocks; i++) {
       	if(i < 12) {
           	free_block(disk, dest_inode->i_block[i]);
		} else if(i == 12) {
       	   	second_level = (int *) (disk + (dest_inode->i_block[i] * EXT2_BLOCK_SIZE));
	        free_block(disk, second_level[0]);
		} else {
       	    free_block(disk, second_level[(i - 12)]);
        }
		dest_inode->i_block[i] = 0;
	}
	//no more blocks reserved
	dest_inode->i_blocks = 0;	

	// get the file's directory inode number
	char hold_path[strlen(file_path)];
    char hold_name[strlen(file_path)];
    char * location = strrchr(file_path, '/');
	int dir_inode_num = NULL;
    if( location == NULL){
    	strncpy(hold_name, file_path, strlen(file_path));
        hold_name[strlen(file_path)] = '\0';
		dir_inode_num = EXT2_ROOT_INO;
 	} else {
		strncpy(hold_name, location + 1, strlen(location) - 1);
        strncpy(hold_path, file_path, location - file_path);
        hold_name[strlen(location) -1 ] = '\0';
        hold_path[location - file_path] = '\0';
        dir_inode_num = find_inode(disk, EXT2_ROOT_INO, hold_path);
	}
	dest_inode->i_size = 0;//TODO: important to tell ihor to make the size of . 1 and ..2 in add_entry 
	dest_inode->i_links_count -= 1;	
	dest_inode->i_dtime = (unsigned int) time(NULL);
	remove_dir_entry(disk, dir_inode_num, hold_name);//for the current dir with inode available_inode
	
	
   return 0;
}
/*
set the block bitmap with the block number block_num to 0, since the block will be available to be used
PARAM: disk, unsigned char *, a pointer to the mapped image disk
PARAM: block_num the position of the block which is equivellant to (block index + 1) 
*/
void free_block(unsigned char *disk, int block_num){
	unsigned int shift = 0x1;
	char *loc = (char *)(disk + (3 * EXT2_BLOCK_SIZE) + (block_num/8));
	int count_shift = 0;
	if(((block_num + 1)%8) == 0){
		count_shift = 8;
	} else{
		count_shift = (block_num + 1)%8; 
	}	
	*loc &= ~(shift << (8 - count_shift));
}
