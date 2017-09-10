# File\_System

EXT2 file system implementation that has tools like ext2_ls, ext2_cp, ext2_mkdir, ext2_ln, and ext2_rm to modify ext2-format virtual disks with user’s needs. 

•	ext2_ls: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on the ext2 formatted disk. The program works like ls -1, printing each directory entry on a separate line. If the flag "-a" is specified (after the disk image argument), the program will also print the . and .. entries.

•	 takes three command line arguments. The first is the name of an ext2 formatted virtual disk. The second is the path to a file on your native operating system, and the third is an absolute path on your ext2 formatted disk. The program should work like cp, copying the file on your native file system onto the specified location on the disk. 

•	ext2_mkdir: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk. The second is an absolute path on your ext2 formatted disk. The program should work like mkdir, creating the final directory on the specified path on the disk.

•	ext2_ln: This program takes three command line arguments. The first is the name of an ext2 formatted virtual disk. The other two are absolute paths on your ext2 formatted disk. The program should work like ln, creating a link from the first specified file to the second specified path. Note that this version of ln only works with files. Additionally, this command may take a "-s" flag, after the disk image argument. When this flag is used, your program must create a symlink instead (other arguments remain the same). 

•	ext2_rm: This program takes two command line arguments. The first is the name of an ext2 formatted virtual disk, and the second is an absolute path to a file or link (not a directory) on that disk. The program should work like rm, removing the specified file from the disk. 


COMPILE: Run the Makefile to have all the tools set up as binary files 

