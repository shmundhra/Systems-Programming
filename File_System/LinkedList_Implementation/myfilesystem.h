#ifndef _myfilesystem_h_
#define _myfilesystem_h_

#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

#define BLACK cout << "\033[1;30m";
#define RED cerr << "\033[1;31m";
#define GREEN cout << "\033[1;32m";
#define YELLOW cout << "\033[1;33m";
#define BLUE cout << "\033[1;34m";
#define MAGENTA cout << "\033[1;35m";
#define CYAN cout << "\033[1;36m";
#define WHITE cout << "\033[1;37m";
#define RESET1         \
    cout << "\033[0m"; \
    cout << endl;
#define RESET2         \
    cerr << "\033[0m"; \
    cerr << endl;
#define perr RED cerr
#define endl flush << "\033[1;37m" \
                   << "\n";
#define debug1(x) cerr << #x << " :: " << x << "\n";
#define debug2(x, y) cerr << #x << " :: " << x << "\t" << #y << " :: " << y << "\n";
#define debug3(x, y, z) cerr << #x << " :: " << x << "\t" << #y << " :: " << y << "\t" << #z << " :: " << z << "\n";
#define debug4(w, x, y, z) cerr << #w << " :: " << w << "\t" << #x << " :: " << x << "\t" << #y << " :: " << y << "\t" << #z << " :: " << z << "\n";

#define MAX_Blk 10
#define MAX_File 10
#define MAX_SIZE 25

/* Roughly sizeof(_SuperBlock) is around 25 */
struct _SuperBlock
{
    int Size_FileSystem;
    int Size_Block;
    bool Valid[MAX_Blk];
    int Ptr_Dir;
    _SuperBlock(int FSS, int BS)
    {
        Valid[0] = Valid[1] = 1;
        for (int i = 2; i < MAX_Blk; i++)
            Valid[i] = 0;
        Size_FileSystem = FSS;
        Size_Block = BS;
    }
};

/* Roughly sizeof(_FAT) is around 20 */
struct _FAT
{
    short int Entry[MAX_Blk];
    _FAT()
    {
        for (int i = 0; i < MAX_Blk; i++)
            Entry[i] = -1;
    }
};

/* Roughly sizeof(_Entry) is around 50 */
struct _Entry
{
    char name[MAX_SIZE];
    int start_Blk;
    int curr_Blk;
    int curr_Off;
    int end_Blk;
    int eof;
    _Entry()
    {
        memset(name, 0, MAX_SIZE);
        start_Blk = curr_Blk = curr_Off = end_Blk = eof = -1;
    }
};

/* Roughly sizeof(_Directory) is sizeof(Entry)*MAX_FILE = 500 */
struct _Directory
{
    _Entry Entry[MAX_File];
};

void myError(string ErrorMessage);
int myFileSystem(int FileSystemSize, int BlockSize);
int myOpen(string FileName, string Mode);
int myRead(int FileDescriptor, char *Buffer, int Length);
int myWrite(int FileDescriptor, char *Buffer, int Length);
int myCopy_DtoL(string LinuxFile, string DiskFile);
int myCopy_LtoD(string DiskFile, string LinuxFile);
int myCat(int FileDescriptor);
int myCat(string FileName);
int myClose(int FileDescriptor);

vector<string> ErrMsg = {
    " : Success",

    " : Block Size Too Low for SuperBlock",
    " : Block Size Too Low for FileAllocationTable",
    " : Block Size Too Low for Directory",
    " : Block Number Too High",
    " : Block Not Available",

    " : Invalid File Descriptor",
    " : File Descriptor Not Available",
    " : File Descriptor Not Open",

    " : Invalid File Name",
    " : File Not Present",
    " : Cannot Open File",

    " : Invalid Mode"};

#endif