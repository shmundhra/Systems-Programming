#include "myfilesystem.h"

int my_errno = 0;
char **Block;

int myFileSystem(int FSS, int BS)
{
    /* Checking If Block Size is Ample */
    if (BS <= sizeof(_SuperBlock))
    {
        my_errno = 1;
        return -1;
    }

    if (BS <= sizeof(_FAT))
    {
        my_errno = 2;
        return -1;
    }

    if (BS <= sizeof(_Directory))
    {
        my_errno = 3;
        return -1;
    }

    int NumOfBlk = FSS / BS;
    if (NumOfBlk > MAX_Blk)
    {
        my_errno = 4;
        return -1;
    }

    /* Dynamically Allocate Memory to the Blocks */
    Block = new char *[NumOfBlk];
    for (int i = 0; i < NumOfBlk; i++)
    {
        Block[i] = new char[BS];
    }

    /* Initialise the Special Blocks */
    _SuperBlock SuperBlock = _SuperBlock(FSS, BS);
    _FAT FAT = _FAT();
    _Directory Directory;

    /* Store the Directory in a Block */
    int Dir_Blk_No = 2;
    SuperBlock.Ptr_Dir = Dir_Blk_No;
    SuperBlock.Valid[Dir_Blk_No] = 1;

    /* Store the Special Blocks in the Block System */
    memcpy(Block[0], &SuperBlock, sizeof(SuperBlock));
    memcpy(Block[1], &FAT, sizeof(FAT));
    memcpy(Block[Dir_Blk_No], &Directory, sizeof(Directory));

    return 0;
}

int myOpen(string FileName, string Mode)
{
    if (FileName.length() == 0) /* Filename not valid */
    {
        my_errno = 12;
        return -1;
    }

    if (Mode != "w" and Mode != "r") /* Check for valid mode */
    {
        my_errno = 12;
        return -1;
    }

    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _FAT *FAT = (_FAT *)Block[1];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    int Start = -1;
    int FD = -1;

    /* Checking if the file already exists */
    for (int i = 0; i < MAX_File; i++)
    {
        if (string(Directory->Entry[i].name) == FileName)
        {
            FD = i;
            Start = Directory->Entry[i].start_Blk;

            if (Mode == "w") /* File exists and needs to be overwritten */
            {
                /* Freeing up all the blocks which were allocated to this file previously */
                for (int index = Start; index != -1; index = FAT->Entry[index])
                {
                    FAT->Entry[index] = -1;
                    SuperBlock->Valid[index] = 0;
                }

                /* Creating a New Entry */
                SuperBlock->Valid[Start] = 1;
                strcpy(Directory->Entry[FD].name, FileName.c_str());
                Directory->Entry[FD].start_Blk = Start;
                Directory->Entry[FD].curr_Blk = Start;
                Directory->Entry[FD].curr_Off = 0;
                Directory->Entry[FD].end_Blk = Start;
                Directory->Entry[FD].eof = 0;
            }
            else if (Mode == "r") /* File exists and shall now be read from the start ~ (ios::beg) */
            {
                /* Updating the Existing Entry */
                Directory->Entry[FD].curr_Blk = Start;
                Directory->Entry[FD].curr_Off = 0;
            }

            return FD;
        }
    }

    if (FD == -1) /* File does not exist */
    {
        if (Mode == "r") /* No File exists to read */
        {
            my_errno = 10;
            return -1;
        }
        else if (Mode == "w") /* Create a new file and open for writing */
        {
            /* Find Starting Block using SuperBlock Valid Vector */
            for (int i = 0; i < MAX_Blk; i++)
            {
                if (SuperBlock->Valid[i] == 0) /* First Empty Block from the start */
                {
                    Start = i;
                    break;
                }
            }

            /* Find First Available File Descriptor in Directory */
            for (int i = 0; i < MAX_File; i++)
            {
                if (strlen(Directory->Entry[i].name) == 0) /* No Name implies index is available */
                {
                    FD = i;
                    break;
                }
            }

            /* Returning Error if No Start Block Available or No More Files Allowed */
            if (Start == -1)
            {
                my_errno = 5;
                return -1;
            }
            if (FD == -1)
            {
                my_errno = 7;
                return -1;
            }

            /* IF VALID Creating a New Entry */
            SuperBlock->Valid[Start] = 1;
            strcpy(Directory->Entry[FD].name, FileName.c_str());
            Directory->Entry[FD].start_Blk = Start;
            Directory->Entry[FD].curr_Blk = Start;
            Directory->Entry[FD].curr_Off = 0;
            Directory->Entry[FD].end_Blk = Start;
            Directory->Entry[FD].eof = 0;

            return FD;
        }
    }
}

int myClose(int fd)
{
    if (fd < 0) /* Check validity of file descriptor */
    {
        my_errno = 6;
        return -1;
    }
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    if (Directory->Entry[fd].curr_Blk == -1) /* Indicates file is already closed */
    {
        my_errno = 8;
        return -1;
    }
    /* Close the currently open file */
    Directory->Entry[fd].curr_Blk = -1;
    Directory->Entry[fd].curr_Off = -1;
    return 0;
}

int myRead(int fd, char *buffer, int length)
{
    if (fd < 0) /* Check validity of file descriptor */
    {
        my_errno = 6;
        return -1;
    }
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _FAT *FAT = (_FAT *)Block[1];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    if (Directory->Entry[fd].curr_Blk == -1) /* Indicates file is already closed */
    {
        my_errno = 8;
        return -1;
    }

    int BS = SuperBlock->Size_Block;
    int &curr = Directory->Entry[fd].curr_Blk;
    int &off = Directory->Entry[fd].curr_Off;
    int end = Directory->Entry[fd].end_Blk;
    int _EOF = Directory->Entry[fd].eof;

    int read_b = 0;
    while (curr != end and read_b < length)
    {
        buffer[read_b++] = Block[curr][off++];

        if (off == BS) /* Reached end of current block */
        {
            curr = FAT->Entry[curr]; /* Move to next block */
            off = 0;                 /* Set offset for next block back to 0 */
        }
    }
    while (curr == end and read_b < length and off < _EOF)
    {
        buffer[read_b++] = Block[curr][off++];
    }

    return read_b;
}

int myWrite(int fd, char *buffer, int length)
{
    if (fd < 0) /* Check validity of file descriptor */
    {
        my_errno = 6;
        return -1;
    }
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _FAT *FAT = (_FAT *)Block[1];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    if (Directory->Entry[fd].curr_Blk == -1) /* Indicates file is already closed */
    {
        my_errno = 8;
        return -1;
    }

    int BS = SuperBlock->Size_Block;
    int &curr = Directory->Entry[fd].curr_Blk;
    int &off = Directory->Entry[fd].curr_Off;
    int &end = Directory->Entry[fd].end_Blk;
    int &_EOF = Directory->Entry[fd].eof;

    int wrote = 0;
    while (curr != end and wrote < length)
    {
        Block[curr][off++] = buffer[wrote++];

        if (off == BS) /* Reached end of current block */
        {
            curr = FAT->Entry[curr]; /* Move to next block */
            off = 0;                 /* Set offset for next block back to 0 */
        }
    }
    while (curr == end and wrote < length and off < _EOF)
    {
        Block[curr][off++] = buffer[wrote++];
    }

    while (curr == end and wrote < length and off == _EOF) /* Just Reached _EOF */
    {
        if (off == BS) /* Need to Allocate New Last Block */
        {
            /* Find the New Block to Allocate Block */
            int New_Blk = -1;
            for (int i = 0; i < MAX_Blk; i++)
            {
                if (SuperBlock->Valid[i] == 0)
                {
                    New_Blk = i;
                    break;
                }
            }
            /* Cannot Find a New Block to Allocate */
            if (New_Blk < 0)
            {
                my_errno = 5;
                return -1;
            }

            /* Set up the New Block */
            SuperBlock->Valid[New_Blk] = 1;
            FAT->Entry[end] = New_Blk;
            end = New_Blk;
            curr = end;
            off = 0;
            _EOF = 0;
        }
        Block[curr][off++] = buffer[wrote++];
        _EOF++;
    }

    return wrote;
}

int myCopy_DtoL(string FileName1, string FileName2)
{
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    /* Checking for valid FileName */
    if (FileName1.length() == 0)
    {
        my_errno = 9;
        return -1;
    }

    /* Creating File */
    int fd1 = creat(FileName1.c_str(), 0666);
    if (fd1 < 0)
    {
        my_errno = 11;
        return -1;
    }

    /* Checking for valid FileName */
    if (FileName2.length() == 0)
    {
        my_errno = 9;
        return -1;
    }

    /* Finding file descriptor of given file */
    int fd2 = -1;
    for (int i = 0; i < MAX_File; i++)
    {
        if (string(Directory->Entry[i].name) == FileName2)
        {
            fd2 = i;
            break;
        }
    }
    if (fd2 == -1)
    {
        my_errno = 10;
        return -1;
    }

    _Entry entry = Directory->Entry[fd2]; /* Storing the current status of the File to be Copied, */
                                          /* since myOpen, myRead, myClose modify the status of File */
    fd2 = myOpen(FileName2, "r");

    if (fd2 < 0)
    {
        my_errno = 11;
        return -1;
    }
    char ch;
    int read_b = 0;
    while (read_b = myRead(fd2, &ch, sizeof(char)))
    {
        write(fd1, &ch, read_b);
    }
    myClose(fd2);

    Directory->Entry[fd2] = entry; /* Restoring the previous status of the File which was Copied */

    close(fd1);
    return 0;
}

int myCopy_LtoD(string FileName1, string FileName2)
{
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    /* Opening New File to write to */
    int fd1 = myOpen(FileName1, "w");
    if (fd1 < 0)
    {
        my_errno = 11;
        return -1;
    }

    if (FileName2.length() == 0)
    {
        my_errno = 9;
        return -1;
    }

    /* Opening existing file to copy from */
    int fd2 = open(FileName2.c_str(), O_RDONLY);
    if (fd2 < 0)
    {
        my_errno = 11;
        return -1;
    }

    char ch;
    int read_b = 0;
    while (read_b = read(fd2, &ch, sizeof(ch)))
    {
        myWrite(fd1, &ch, read_b);
    }

    close(fd2);
    myClose(fd1);
    return 0;
}

int myCat(string FileName)
{
    /* Checking Validity of FileName */
    if (FileName.length() == 0)
    {
        my_errno = 9;
    }

    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _Directory *Directory = (_Directory *)Block[SuperBlock->Ptr_Dir];

    /* Finding the required file descriptor of the given Filename to extract status */
    int fd = -1;
    for (int i = 0; i < MAX_File; i++)
    {
        if (string(Directory->Entry[i].name) == FileName)
        {
            fd = i;
            break;
        }
    }
    if (fd == -1)
    {
        my_errno = 10;
        return -1;
    }
    _Entry entry = Directory->Entry[fd]; /* Storing the current status of the File to be Displayed, */
                                         /* since myOpen, myRead, myClose modify the status of File */
    fd = myOpen(FileName, "r");
    if (fd < 0)
    {
        my_errno = 11;
        return -1;
    }
    char ch;
    while (myRead(fd, &ch, sizeof(char)))
    {
        cerr << ch;
    }
    myClose(fd);
    Directory->Entry[fd] = entry; /* Restoring the previous status of the File which was Displayed */

    return 0;
}

void myError(string msg)
{
    perr << msg << ErrMsg[my_errno] << endl;
    return;
}
