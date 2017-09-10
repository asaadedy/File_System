#ifndef _FILE_SYSTEM_SHARED_H_
#define _FILE_SYSTEM_SHARED_H_
#include "ext2.h"

extern unsigned char * disk_pointer(char * file);
extern int find_inode(unsigned char *disk_ref, unsigned int inode, char * path);
extern void clean_path(char * path);
extern void print_name( char* name, unsigned char size);
extern int reserve_block(unsigned char *disk);
extern int reserve_inode(unsigned char *disk);
extern int remove_dir_entry(unsigned char *disk_ref, int dir_inode, char * entry_name);
extern void add_dir_entry(unsigned char *disk_ref, struct ext2_inode *dir, int inode, char *name, int lngth);
extern int dir_ent_size(int name_length);
extern void map_to_img(unsigned char * disk, char * file_name);
extern struct ext2_inode *inode_pointer(int inode, unsigned char *disk);
extern int rm_file(unsigned char * disk, char *file_path);
extern void free_block(unsigned char *disk, int block_num);
#endif // _FILE_SYSTEM_SHARED_H_
