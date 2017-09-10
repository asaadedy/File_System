
#ifndef _EXT2_RM_BONUS_H_
#define _EXT2_RM_BONUS_H_

extern int rm_dir(unsigned char * disk, struct ext2_inode * inode, char *path);
extern void concatinate(char *path, char *name, int name_len); 
extern void seperate_strings(char *path, int name_len);

#endif // _EXT2_RM_BONUS_H_

