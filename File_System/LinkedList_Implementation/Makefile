CC = g++ -std=c++17 
.DEFAULT_GOAL := Menu
source = program1.cpp 


Menu	:	
	@echo "Welcome to MyFileSystem Makefile :: Default CC=$(CC) "
	@echo ""
	@echo "CHOOSE APPROPRIATE COMMAND : "
	@echo "Header  [CC=<CompilerVersion>] - Creates Header Executable \"myfilesystem.o\""
	@echo "Library [CC=<CompilerVersion>] - Creates Static Library \"libmfs.a\""
	@echo "User  [CC=<CompilerVersion>] [source=<SourceFile.cpp>] - Creates \"mfs_user.o\""
	@echo "clean   - Removes All files created in this Makefile and Clears Screen"
	@echo "remove  - Removes All files created in this Makefile"
	@echo ""

User 		: MFS_User 
	@echo ""
	@echo ">>\"./mfs_user.o\" TO START MFS_User "
	@echo ""

MFS_User 	:	libmfs.a
	$(CC) -o mfs_user.o $(source) -L. -lmfs 

Library :	libmfs.a
	@echo ""
	@echo ">>\"CC -o <mfs_user>.o <program1>.cpp -L. -lmfs \" TO COMPILE A PROGRAM"
	@echo ""

libmfs.a	:	myfilesystem.o
	ar rcs libmfs.a myfilesystem.o

Header	:	myfilesystem.o
	@echo ""
	@echo ">>\"ar rcs lib<mfs>.a myfilesystem.o\" TO CREATE STATIC LIBRARY"
	@echo ""

myfilesystem.o	:	myfilesystem.cpp	myfilesystem.h
	$(CC) myfilesystem.h 
	$(CC) -c -o myfilesystem.o myfilesystem.cpp

clean	:	
	rm -f mfs_user.o myfilesystem.o myfilesystem.h.gch libmfs.a 
	clear

remove	:	
	rm -f mfs_user.o myfilesystem.o myfilesystem.h.gch libmfs.a 
	