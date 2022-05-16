CC=gcc
CFLAGS=-c -g -Wall -fno-stack-protector -nostdlib
ASM=yasm
ASMFLAGS=-f elf64 -DYASM -D__x86_64__ -DPIC

define cfile
	cp ~/ta/hw3/$(1).c .
	gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI $(1).c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o $(1) $(1).o start.o -L. -L.. -lmini
	rm $(1).o
endef

all:
	$(ASM) $(ASMFLAGS) libmini64.asm -o libmini64.o
	$(CC) $(CFLAGS) -fPIC libmini.c
	ld -shared -o libmini.so libmini64.o libmini.o

start:
	cp share/unix_prog/asm/start.o .

write1: start
	$(call cfile,$@)

alarm1: start
	$(call cfile,$@)

alarm2: start
	$(call cfile,$@)

alarm3: start
	$(call cfile,$@)

jmp1: start
	$(call cfile,$@)