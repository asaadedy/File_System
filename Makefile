COMPILE= -Wall -std=gnu99


all: ext2_ls ext2_cp ext2_mkdir ext2_ln ext2_rm

ext2_ls :  ext2_ls.o file_system_shared.o
	gcc -Wall -g -o ext2_ls ext2_ls.o file_system_shared.o

ext2_cp :  ext2_cp.o file_system_shared.o
	gcc -Wall -g -o ext2_cp ext2_cp.o -lm file_system_shared.o

ext2_mkdir : ext2_mkdir.o file_system_shared.o
	gcc -Wall -g -o ext2_mkdir ext2_mkdir.o file_system_shared.o

ext2_ln : ext2_ln.o file_system_shared.o
	gcc -Wall -g -o ext2_ln ext2_ln.o -lm file_system_shared.o

ext2_rm :ext2_rm.o file_system_shared.o ext2_rm_bonus.o 
	gcc -Wall -g -o ext2_rm ext2_rm.o file_system_shared.o ext2_rm_bonus.o

%.o : %.c ext2.h file_system_shared.h ext2_rm_bonus.h
	gcc ${COMPILE} -g -c $<


clean:
	rm -f *.o ext2_ls ext2_cp ext2_mkdir ext2_ln ext2_rm

