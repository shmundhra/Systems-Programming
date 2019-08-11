#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

#define BLACK cerr<<"\033[1;30m";
#define RED cerr<<"\033[1;31m";
#define GREEN cerr<<"\033[1;32m";
#define YELLOW cerr<<"\033[1;33m";
#define BLUE cerr<<"\033[1;34m";
#define MAGENTA cerr<<"\033[1;35m";
#define CYAN cerr<<"\033[1;36m";
#define WHITE cerr<<"\033[1;37m";
#define RESET1 cerr<<"\033[0m"; cerr<<endl;
#define RESET2 cerr<<"\033[0m"; cerr<<endl;
#define perr cerr << "\033[1;31m"
#define endl flush<<"\033[1;37m"<<"\n";
#define debug1(x) cerr<<#x<<" :: "<<x<<"\n";
#define debug2(x,y) cerr<<#x<<" :: "<<x<<"\t"<<#y<<" :: "<<y<<"\n";
#define debug3(x,y,z) cerr<<#x<<" :: "<<x<<"\t"<<#y<<" :: "<<y<<"\t"<<#z<<" :: "<<z<<"\n";
#define debug4(w,x,y,z) cerr<<#w<<" :: "<<w<<"\t"<<#x<<" :: "<<x<<"\t"<<#y<<" :: "<<y<<"\t"<<#z<<" :: "<<z<<"\n";

#define MAX_SIZE 14
#define MAX_File 10

enum FileType {
    BINARY,
    DIRECTORY,
};

enum Action {
    ALLOCATE,
    DONT_ALLOCATE,
};

struct _SuperBlock
{
    int Size_FileSystem; /* Stores the total size allocated to the FileSystem */
    int Size_Block;      /* Stores the size of eac block */
    int Head;            /* Block Number of Head of Free Block List */
    int Tail;            /* Block Number of Tail of Free Block List */
    int Num_Inodes;      /* Number of iNodes or Number of Files + Directories */
    int CWD;             /* Globally unique current working directory */
    int Num_Ptrs;        /* Number of pointers that can be stored in each block */
    _SuperBlock(int FSS, int BS) {
        Size_FileSystem = FSS;
        Size_Block 		= BS;
        Head 			= 3;
        Tail 			= FSS/BS - 1;
        Num_Inodes 		= 0;
        CWD 			= 0;
        Num_Ptrs		= BS/sizeof(int);
     }
};

struct _Record{
    char name[MAX_SIZE];                /* Name of File/Directory */
    short int i_no;                     /* Corresponding iNode for Record */
    _Record(string _name, short int num)
    {
        memset(name, 0, MAX_SIZE);
        strcpy(name, _name.c_str());
        i_no = num;
    }
}; 

struct _inode{
    _Record* Loc;
    int DP[5];      /* Holds the Pointers to the Data Blocks  */
    int SIP;        /* Holds Pointer to Direct Data Block Pointers */
    int DIP;        /* Holds Pointers to SIP Blocks */
    FileType type;  /* File/Directory type */
    int size;
    int curr_Blk;   /* Current Block where 'cursor' exists */
    int curr_Off;   /* Location in the current block where 'cursor' exists */
    int end_Blk;    /* Last Block where data exists */
    int eof;        /* Denotes offset of last block till where data exists */
    _inode() { Loc = NULL; }
    _inode( _Record* _Loc, FileType _type ){
        Loc = _Loc;
        type = _type;
        for( int i=0; i<5; i++) DP[i] = -1;
        SIP = DIP = -1;
        size = 0; curr_Blk = end_Blk = -1;
        curr_Off = eof = 0;
    }
};

struct _inodes{
    _inode Nodes[MAX_File];
};

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

    " : Invalid Mode"
};
