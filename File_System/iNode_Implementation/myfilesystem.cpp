#include "myfilesystem.h"

int my_errno = 0;
char** Block;

void check()
{
    for (int i = 3; i < 8; i++)
    {
        int *ptr = (int *)Block[i];
        cerr << *ptr << "\t";
    }
    cerr << endl;
}

int NewBlock()
{
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    int &Head = SuperBlock->Head;
    int &Tail = SuperBlock->Tail;
    int New_Block;

    if (Head == -1)
    {
        return -1;
    }
    if (Head == Tail)
    {
        New_Block = Head;
        Head = Tail = -1;
        return New_Block;
    }
    New_Block = Head;
    int *Next_Block = (int *)Block[New_Block]; /* Store the next block ka pointer */
    memcpy(&Head, Next_Block, sizeof(int));    /* in the start of Free Blocks */

    return New_Block;
}

pair<_Record *, bool> search_DP(const char *Name, int &Curr, int &End, int &_eof, int BS, Action flag)
{
    pair<_Record *, bool> Answer = {NULL, false};
    _Record *&Location = Answer.first;

    if (Curr == -1) /* If Curr is Not Allocated and we are here so we havent found element and wont find it */
    {
        if (flag == DONT_ALLOCATE)  /* If No Need to Allocate, just return Negative */
            return Answer;

        if ((Curr = NewBlock()) == -1)  /* Trying to Allocate but Failed to get a new block */
        {
            perr << "No Free Block Available" << endl;
            return Answer;
        }
        End = Curr;
        _eof = sizeof(_Record);
        Answer = {(_Record *)Block[Curr], false};
    }
    if (Location) return Answer;

    if (Curr != End) /* Searching in Non-End Blocks */
    {
        _Record *Record = (_Record *)Block[Curr];   /* Saving the Records in this block as an array */
        int Num_Rec = BS / sizeof(_Record);         /* Extracting Number of Records in this block */

        for (int i = 0; i < Num_Rec; i++)
        {
            if (!strcmp(Record[i].name, Name))      /* Found record */
            {
                Answer = {&Record[i], true};
                break;
            }
            if (flag == ALLOCATE and !strcmp(Record[i].name, ""))   /* Found space to allocate record */
            {
                if (!Location)                  /* Allocate if not already allocated */
                    Location = &Record[i];      /* Not breaking as we may find file afterwards */
            }
        }
        if (Location)
            return Answer;
    }

    if (Curr == End) /* Searching in the End Block */
    {
        _Record* Record = (_Record *) Block[ Curr ];    /* Saving the Records in this block as an array */
        int Num_Rec = _eof / sizeof(_Record);           /* Extracting Number of Records in this block */

        for( int i = 0; i < Num_Rec; i++ )
        {
            if (!strcmp(Record[i].name, Name))          /* Found record */
            {
                Answer = {&Record[i], true};
                break;
            }

            if (flag == ALLOCATE and !strcmp(Record[i].name, ""))   /* Found space to allocate record */
            {
                if (!Location)                   /* Allocate if not already allocated */
                    Location = &Record[i];       /* Not breaking as we may find file afterwards */
            }

        }
        if (Location)
            return Answer;

        if (flag == DONT_ALLOCATE)
            return Answer;                        /* If No Need to Allocate, just return Negative */

        if (_eof < BS)                            /* If there is space in the end block, allocate */
        {
            int loc = _eof;
            _eof += sizeof(_Record);
            Location = (_Record *)&Block[Curr][loc];
        }
    }

    return Answer;

}

pair<_Record *, bool> search_SIP(const char *Name, int &SIP, int Num_Ptrs, int &End, int &_eof, int BS, Action flag)
{
    pair<_Record *, bool> Answer = {NULL, false};
    _Record *&Location = Answer.first;

    if (SIP == -1)
    {
        if (flag == DONT_ALLOCATE)  // If No Need to Allocate, just return Negative
            return Answer;

        if ((SIP = NewBlock()) == -1)
        {
            perr << "No Free Block Available" << endl;
            exit(EXIT_FAILURE);
        }
        /* Initialising the new block */
        int *DP = (int *)Block[SIP];
        for (int i = 0; i < Num_Ptrs; i++)
            DP[i] = -1;
    }

    /* Checking the Direct Pointers */
        int *DP = (int *)Block[SIP];
        for (int dp = 0; dp < Num_Ptrs; dp++)
        {
            Answer = search_DP(Name, DP[dp], End, _eof, BS, flag);
            if (Location)
                return Answer;
        }

    return Answer;
}

pair<_Record *, bool> search_DIP(const char *Name, int &DIP, int Num_Ptrs, int &End, int &_eof, int BS, Action flag)
{
    pair<_Record *, bool> Answer = {NULL, false};
    _Record *&Location = Answer.first;

    if (DIP == -1)
    {
        if (flag)
            return Answer; // If No Need to Allocate, just return Negative

        if ((DIP = NewBlock()) == -1)
        {
            perr << "No Free Block Available" << endl;
            exit(EXIT_FAILURE);
        }
        int *SIP = (int *)Block[DIP];
        for (int sip = 0; sip < Num_Ptrs; sip++)
            SIP[sip] = -1;
    }

    /* Checking the Singly Indirect Pointers  */
        int *SIP = (int *)Block[DIP];
        for (int sip = 0; sip < Num_Ptrs; sip++)
        {
            Answer = search_SIP(Name, SIP[sip], Num_Ptrs, End, _eof, BS, flag);
            if (Location)
                return Answer;
        }

    return Answer;
}

/* 	Searches Directory of Inode for presence/ absence of "FileName"
    Returns Matching Record if FOUND , Returns Apt Address to Store if not FOUND */
pair<_Record *, bool> search_Dir(_inode *Inode, string FileName, Action flag)
{
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    int Num_Ptrs = SuperBlock->Num_Ptrs;
    int BS = SuperBlock->Size_Block;
    int &End = Inode->end_Blk;
    int &_eof = Inode->eof;
    pair<_Record *, bool> Ans = {NULL, false};
    _Record *&Location = Ans.first;

    /* Searching in the Direct Pointers  */
        for (int dp = 0; dp < 5; dp++)
        {
            Ans = search_DP(FileName.c_str(), Inode->DP[dp], Inode->end_Blk, Inode->eof, BS, flag);
            if (Location)
                return Ans;
        }

    /* Searching in the Singly Indirect Pointer */
        Ans = search_SIP(FileName.c_str(), Inode->SIP, Num_Ptrs, Inode->end_Blk, Inode->eof, BS, flag);
        if (Location)
            return Ans;

    /* Searching in the Doubly Indirect Pointer */
        Ans = search_DIP(FileName.c_str(), Inode->DIP, Num_Ptrs, Inode->end_Blk, Inode->eof, BS, flag);
        if (Location)
            return Ans;

    /* No Memory Left */
        debug1(static_cast<int>(flag));
        if (flag == ALLOCATE)
        {
            perr << "Memory Limit Exceeded" << endl;
        }
        return Ans;
}

int myMkdir( string FileName )
{
    if (FileName.length() == 0)
    {
        perr << "Invalid Name" << endl;
        return -1;
    }

    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode_CWD = I->Nodes[SuperBlock->CWD];

    /* Check if already present and get apt location accordingly */
    pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName, ALLOCATE);
    if (Address.first == NULL)
    {
        perr << "Memory Limit Exceeded" << endl;
        return -1;
    }
    if (Address.second == true)
    {
        perr << "Directory Already Exists" << endl;
        return -1;
    }

    /* Find Correct Index of Inode Table to insert new Inode */
    int Index = (SuperBlock->Num_Inodes);
    for (int i = 1; i < SuperBlock->Num_Inodes; i++)
    {
        if (I->Nodes[i].Loc == NULL)
        {
            Index = i;
            // break;
        }
    }

    /* Create Record for New Directory and Store in Parent ka DATA Block and update size */
        _Record New_Record = _Record(FileName, Index);
        *(Address.first) = New_Record;
        Inode_CWD.size += sizeof(_Record);

    /* Create new Inode for Directory */
        _inode Inode_New = _inode( Address.first, FileType::DIRECTORY );

        /* Make the '.' Entry */
            pair<_Record *, bool> Self = search_Dir(&Inode_New, ".", ALLOCATE);
            if (Self.second == true)
            {
                perr << "Some Error Occurred" << endl;
                return -1;
            }
            if (Self.first == NULL)
            {
                perr << "Memory Limit Exceeded" << endl;
                return -1;
            }
        /* Attach Record to Newly Made Directory and update */
            _Record Self_Record = _Record(".", Index);
            *(Self.first) = Self_Record;
            Inode_New.size += sizeof(_Record);

        /* Make the '..' Entry */
            pair<_Record *, bool> Parent = search_Dir(&Inode_New, "..", ALLOCATE);
            if (Parent.second == true)
            {
                perr << "Some Error Occurred" << endl;
                return -1;
            }
            if (Parent.first == NULL)
            {
                perr << "Memory Limit Exceeded" << endl;
                return -1;
            }
        /* Attach Record to Newly Made Directory and update */
            _Record Parent_Record = _Record("..", SuperBlock->CWD);
            *(Parent.first) = Parent_Record;
            Inode_New.size += sizeof(_Record);

        /* Attach the Inode to the Inode_Table */
            I->Nodes[Index] = Inode_New;
            if (Index == (SuperBlock->Num_Inodes))  /* If the iNode is inserted at the last */
                (SuperBlock->Num_Inodes)++;         /* Increment the number of iNodes */

            return 0;
}

int myChdir( string FileName )
{
    if (FileName.length() == 0)
    {
        perr << "Invalid Name" << endl;
        return -1;
    }

    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode_CWD = I->Nodes[SuperBlock->CWD];

    /* Check if present and get apt location accordingly */
        pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName, DONT_ALLOCATE);
        if (Address.second == false or Address.first == NULL)
        {
            perr << "Directory Does Not Exist" << endl;
            return -1;
        }

    SuperBlock->CWD = (Address.first)->i_no;
    return 0;
}

int myRmdir( string FileName )
{
    if (FileName.length() == 0)
    {
        perr << "Invalid Name" << endl;
        return -1;
    }

    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode_CWD = I->Nodes[SuperBlock->CWD];

    /* Check if present and get apt location accordingly */
        pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName, DONT_ALLOCATE);
        if (Address.second == false or Address.first == NULL)
        {
            perr << "Directory Does Not Exist" << endl;
            return -1;
        }

    /* Remove from Inode Table */
        I->Nodes[(Address.first)->i_no] = _inode();

    /* Remove Record from the Directory and update size */
        *(Address.first) = _Record("", -1);
        Inode_CWD.size -= sizeof(_Record);
        perr << "Deleted directory Successfully" << endl;
}

int myOpen(string FileName, string Mode)
{
    if (FileName.length() == 0)
    {
        perr << ErrMsg[my_errno = 9] << endl;
        return -1;
    }

    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode_CWD = I->Nodes[SuperBlock->CWD];

    if (Mode == "r")
    {
        perr << "Checking for '" << FileName << "' in CWD @Block" << SuperBlock->CWD << " in Mode : " << Mode << endl;

        /* Search and check in CWD if the file exists, if so get its record if NOT Return Error */
            pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName, DONT_ALLOCATE);
            if (Address.first == NULL or Address.second == false)
            {
                perr << "File Does Not Exist on the Current Working Directory" << endl;
                return -1;
            }

        perr<<"Record Found at Inode : " ; debug1(Address.first -> i_no);

        /* Extracting Inode of File and Marking it Open by setting Current Block and Offset */
            _inode &File_Inode = I->Nodes[(Address.first)->i_no];
            File_Inode.curr_Blk = File_Inode.DP[0];
            File_Inode.curr_Off = 0;

        return (Address.first->i_no);
    }

    if ( Mode == "w" )
    {
        perr << "Checking for '" << FileName << "' in CWD @Block" << SuperBlock->CWD << " in Mode : " << Mode << endl;

        /* Search and check in CWD if the file exists, if so get its record if NOT Create Record */
            pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName, Action::ALLOCATE);
            if (Address.first == NULL)
            {
                perr << "Memory Limit Exceeded" << endl;
                return -1;
            }
            if (Address.second == true)
            {
                /* Already Exists, get old Inode and convert into Brand New File O_TRUNC|O_WRONLY and return */
                _inode &File_Inode = I->Nodes[(Address.first)->i_no];
                File_Inode = _inode(Address.first, FileType::BINARY);
                perr << "Found Inode at Index " << Address.first->i_no << endl;

                return Address.first->i_no;
            }
            else if (Address.first) /* Need to create new Inode and Record, O_CREAT|O_WRONLY */
            {
                /* Find Correct Index of Inode Table to insert new Inode */
                    int Index = (SuperBlock->Num_Inodes);
                    for (int i = 1; i < SuperBlock->Num_Inodes; i++)
                    {
                        if (I->Nodes[i].Loc == NULL)
                        {
                            Index = i;
                        }
                    }
                    perr << "Found Inode at Index " << Index << endl;

                /* Create Record for New File and Store in CWD ka DATA Block and update size */
                    _Record New_Record = _Record(FileName, Index);
                    *(Address.first) = New_Record;
                    Inode_CWD.size += sizeof(_Record);

                /* Create new Inode for File and Attach to the Inode_Table*/
                    _inode Inode_New = _inode(Address.first, FileType::BINARY);
                    I->Nodes[Index] = Inode_New;
                    if (Index == (SuperBlock->Num_Inodes))
                        (SuperBlock->Num_Inodes)++;

                return Index;
            }
    }
}

int myClose(int fd)
{
    if (fd < 0)
    {
        perr << ErrMsg[my_errno = 6] << endl;
        return -1;
    }
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode = I->Nodes[fd];

    Inode.curr_Blk = -1;
    Inode.curr_Off = -1;

    return 0;
}

int myRead(int fd, char* buffer, int length)
{
    sleep(0.1);
    if ( fd < 0 ) {
        perr<<ErrMsg[my_errno = 6]<<endl;
        return -1;
    }
    _SuperBlock* SuperBlock = (_SuperBlock *) Block[0];
    _inodes* I              = (_inodes*) Block[1];
    _inode& Inode           =  I->Nodes[fd];

    int &curr_Blk           = Inode.curr_Blk;
    int &curr_Off           = Inode.curr_Off;
    int end_Blk             = Inode.end_Blk;
    int eof                 = Inode.eof;
    int Num_Ptrs            = SuperBlock->Num_Ptrs;
    int BS                  = SuperBlock->Size_Block;

    int read_bytes          = 0;
    int readflag            = 0;

    if ( curr_Blk == end_Blk and curr_Off == eof ){
        return 0;
    }

    int *DP = Inode.DP;
    if ( curr_Blk == -1 and DP[0] != -1  ){
        perr<<ErrMsg[my_errno = 8]<<endl;
        return -1;
    }

    /* Reading From the Direct Pointers */
        for(int i = 0; i < 5;i ++)
        {
            if(DP[i] == -1 or read_bytes == length) { // Data Blocks No Left or Read Everything
                return read_bytes;
            }
            if(DP[i] == curr_Blk) { 	//Reached current block, now can start reading
                readflag = 1;
            }
            if(!readflag) continue;  	//Not yet reached current block

            curr_Blk = DP[i];

            int maxread  = length - read_bytes;
            if (curr_Blk == end_Blk) maxread = min( maxread, eof - curr_Off );
            else                     maxread = min( maxread, BS  - curr_Off );

            for( int j=0; j<maxread; j++ )
            {
                buffer[read_bytes++] = Block[DP[i]][j+curr_Off];
            }
            curr_Off += maxread;

            if(read_bytes == length) return read_bytes;
            if(curr_Blk != end_Blk)  curr_Off = 0;
        }

    /* Traverse the Singly Indirect Pointer */
    int SIP = Inode.SIP;
    if(SIP == -1) return read_bytes;

    int* DP_1 = (int *)Block[SIP];
    /* Read from the Singly Indirect Direct Pointers */
        for(int i = 0; i < Num_Ptrs;i ++)
        {
            if(DP_1[i] == -1 or read_bytes == length) { // Data Blocks No Left or Read Everything
                return read_bytes;
            }
            if(DP_1[i] == curr_Blk) { 	//Reached current block, now can start reading
                readflag = 1;
            }
            if(!readflag) continue;  	//Not yet reached current block

            curr_Blk = DP_1[i];

            int maxread  = length - read_bytes;
            if (curr_Blk == end_Blk) maxread = min( maxread, eof - curr_Off );
            else 					 maxread = min( maxread, BS  - curr_Off );

            for( int j=0; j<maxread; j++ )
            {
                buffer[read_bytes++] = Block[DP_1[i]][j+curr_Off];
            }
            curr_Off += maxread;

            if(read_bytes == length) return read_bytes;
            if(curr_Blk != end_Blk)  curr_Off = 0;
        }

    /* Traverse the Doubly Indirect Pointer */
    int DIP = Inode.DIP;
    if(DIP == -1) return read_bytes;

    int *SIP_1 = (int *) Block[DIP];
    /* Read from the Singly Indirect Pointers */
        for(int k = 0; k < Num_Ptrs; k++)
        {
            if(SIP_1[k] == -1) return read_bytes;

            int* DP_2 = (int *)Block[SIP_1[k]];
            /* Read from the Doubly Singled Indirect Direct Pointers */
                for(int i = 0; i < Num_Ptrs;i ++)
                {
                    if(DP_2[i] == -1 or read_bytes == length) { // Data Blocks No Left or Read Everything
                        return read_bytes;
                    }
                    if(DP_2[i] == curr_Blk) { 	//Reached current block, now can start reading
                        readflag = 1;
                    }
                    if(!readflag) continue;  	//Not yet reached current block

                    curr_Blk = DP_2[i];

                    int maxread  = length - read_bytes;
                    if (curr_Blk == end_Blk) maxread = min( maxread, eof - curr_Off );
                    else 					 maxread = min( maxread, BS  - curr_Off );

                    for( int j=0; j<maxread; j++ )
                    {
                        buffer[read_bytes++] = Block[DP_2[i]][j+curr_Off];
                    }
                    curr_Off += maxread;

                    if(read_bytes == length) return read_bytes;
                    if(curr_Blk != end_Blk)  curr_Off = 0;
                }
        }

    return read_bytes;
}


int myWrite(int fd,char *buffer, int length)
{
    if ( fd < 0 ) {
        perr<<ErrMsg[my_errno = 6]<<endl;
        return -1;
    }
    _SuperBlock* SuperBlock = (_SuperBlock *) Block[0];
    _inodes* I              = (_inodes*) Block[1];
    _inode& Inode           =  I->Nodes[fd];

    int &curr_Blk           = Inode.curr_Blk;
    int &curr_Off           = Inode.curr_Off;
    int &size               = Inode.size;
    int &end_Blk            = Inode.end_Blk;
    int &eof                = Inode.eof;
    int Num_Ptrs            = SuperBlock->Num_Ptrs;
    int BS                  = SuperBlock -> Size_Block;

    int write_bytes         = 0;
    int write_flag          = 0;

    int *DP = Inode.DP;
    if ( curr_Blk == -1 and DP[0] != -1  ){
        perr<<ErrMsg[my_errno = 8]<<endl;
        return -1;
    }

    /* Writing to the Direct Pointers */
        for(int i = 0; i < 5;i ++)
        {
            if( write_bytes == length ){     // Wriitten Enough
                return write_bytes;
            }
            if(DP[i] == curr_Blk){           // Propogating to the Current Block
                write_flag = 1;
            }
            if(!write_flag) continue;

            if(DP[i] == -1)	DP[i] = NewBlock();
            curr_Blk = end_Blk = DP[i];

            int maxwrite = min( BS - curr_Off, length-write_bytes );

            for( int j=0; j < maxwrite; j++ )
            {
                Block[DP[i]][j + curr_Off] = buffer[write_bytes++];
                size++;
            }
            curr_Off += maxwrite;
            eof += maxwrite;

            if(write_bytes == length) return write_bytes;
            curr_Off = 0;
        }

    /* Traverse the Singly Indirect Pointer */
    int &SIP = Inode.SIP;
    if( SIP == -1) SIP = NewBlock();

    int* DP_1 = (int *)Block[SIP];
    /* Writing in the Singly Indirect Direct Pointers */
        for( int i=0; i < Num_Ptrs; i++ )
        {
            if( write_bytes == length ){	 	 // Wriitten Enough
                    return write_bytes;
                }
                if(DP[i] == curr_Blk){			 // Propogating to the Current Block
                    write_flag = 1;
                }
                if(!write_flag) continue;

                if(DP[i] == -1)	DP[i] = NewBlock();
                curr_Blk = end_Blk = DP[i];

                int maxwrite = min( BS - curr_Off, length-write_bytes );

                for( int j=0; j < maxwrite; j++ )
                {
                    Block[DP[i]][j + curr_Off] = buffer[write_bytes++];
                    size++;
                }
                curr_Off += maxwrite;
                eof += maxwrite;

                if(write_bytes == length) return write_bytes;
                curr_Off = 0;
        }

    /* Traverse the Doubly Indirect Pointer */
    int &DIP = Inode.DIP;
    if( DIP == -1) DIP = NewBlock();

    int *SIP_1 = (int *) Block[DIP];
    /* Write to the Singly Indirect Pointers */
        for(int k = 0; k < Num_Ptrs;k++)
        {
            if ( SIP_1[k] == -1 ) SIP_1[k] = NewBlock();

            int* DP_2 = (int *) Block[SIP_1[k]];
            /* Write to the Doubly Singled Indirect Direct Pointers */
                for( int i=0; i < Num_Ptrs; i++ )
                {
                    if( write_bytes == length ){	 	 // Wriitten Enough
                            return write_bytes;
                        }
                        if(DP_2[i] == curr_Blk){			 // Propogating to the Current Block
                            write_flag = 1;
                        }
                        if(!write_flag) continue;

                        if (DP_2[i] == -1)	DP_2[i] = NewBlock();
                        curr_Blk = end_Blk = DP_2[i];

                        int maxwrite = min( BS - curr_Off, length-write_bytes );

                        for( int j=0; j < maxwrite; j++ )
                        {
                            Block[DP_2[i]][j + curr_Off] = buffer[write_bytes++];
                            size++;
                        }
                        curr_Off += maxwrite;
                        eof += maxwrite;

                        if(write_bytes == length) return write_bytes;
                        curr_Off = 0;
                }
        }

    return write_bytes;
}

int myCopy_LtoD(string FileName1, string FileName2)
{
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];

    int fd1 = myOpen(FileName1, "w");
    if (fd1 < 0)
    {
        perr << ErrMsg[my_errno = 11] << endl;
        return -1;
    }

    if (FileName2.length() == 0)
    {
        perr << ErrMsg[my_errno = 9] << endl;
        return -1;
    }

    int fd2 = open(FileName2.c_str(), O_RDONLY);
    if (fd2 < 0)
    {
        perr << ErrMsg[my_errno = 11] << endl;
        return -1;
    }

    char *ch = new char;
    while (read(fd2, ch, sizeof(char) > 0))
    {
        myWrite(fd1, ch, sizeof(char));
    }

    close(fd2);
    myClose(fd1);
    return 0;
}

int myCopy_DtoL(string FileName1, string FileName2)
{
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode_CWD = I->Nodes[SuperBlock->CWD];

    if (FileName1.length() == 0)
    {
        perr << ErrMsg[my_errno = 9] << endl;
        return -1;
    }
    int fd1 = creat(FileName1.c_str(), 0666);
    if (fd1 < 0)
    {
        perr << ErrMsg[my_errno = 11] << endl;
        return -1;
    }

    if (FileName2.length() == 0)
    {
        perr << ErrMsg[my_errno = 9] << endl;
        return -1;
    }
    pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName2.c_str(), DONT_ALLOCATE);
    if (Address.first == NULL or Address.second == false)
    {
        perr << "File Does Not Exist on the Current Working Directory" << endl;
        return -1;
    }

    _inode File_Inode = I->Nodes[(Address.first)->i_no]; /* Storing Status of iNode of File */
    {                                                    /* since myOpen, myRead, myClose modify it */
        int fd2 = myOpen(FileName2.c_str(), "r");
        char *ch = new char;
        while (myRead(fd2, ch, sizeof(char) > 0))
        {
            write(fd1, ch, sizeof(char));
        }
        myClose(fd2);
    }
    I->Nodes[(Address.first)->i_no] = File_Inode; /* Restoring the status of the iNode */

    close(fd1);
    return 0;
}

int myCat(string FileName)
{
    if (FileName.length() == 0)
    {
        perr << ErrMsg[my_errno = 9] << endl;
        return -1;
    }
    _SuperBlock *SuperBlock = (_SuperBlock *)Block[0];
    _inodes *I = (_inodes *)Block[1];
    _inode &Inode_CWD = I->Nodes[SuperBlock->CWD];

    pair<_Record *, bool> Address = search_Dir(&Inode_CWD, FileName.c_str(), DONT_ALLOCATE);
    if (Address.first == NULL or Address.second == false)
    {
        perr << "File Does Not Exist on the Current Working Directory" << endl;
        return -1;
    }

    _inode File_Inode = I->Nodes[(Address.first)->i_no];
    {
        int fd = myOpen(FileName, "r");
        char *ch = new char;
        while (myRead(fd, ch, sizeof(char) > 0))
        {
            cerr << *ch;
        }
        myClose(fd);
    }
    I->Nodes[(Address.first)->i_no] = File_Inode;

    return 0;
}

int myFileSystem(int FSS, int BS)
{
    /* Checking If Block Size is Ample */
    if (BS <= sizeof(_SuperBlock))
    {
        my_errno = 1;
        return -1;
    }
    /* iNodes will be stored in Block[1] and Block[2] */
    if (2 * BS <= sizeof(_inodes))
    {
        my_errno = 2;
        return -1;
    }

    /* Dynamically Allocate Memory to the Blocks */
    int NumOfBlk = FSS / BS;
    Block = new char *[NumOfBlk];
    for (int i = 0; i < NumOfBlk; i++)
    {
        Block[i] = new char[BS];
    }

    /* Initialise the SuperBlock and Free Blocks */
    _SuperBlock SuperBlock = _SuperBlock(FSS, BS);
    for (int i = SuperBlock.Head; i < SuperBlock.Tail; i++)
    {
        memcpy(Block[i], new int(i + 1), sizeof(int));
    }
    memcpy(Block[SuperBlock.Tail], new int(-1), sizeof(int));

    /* Create the I_Node Table */
    _inodes I;

    /* Create Root Directory */
    _inode Inode_New = _inode(NULL, FileType::DIRECTORY);
    I.Nodes[0] = Inode_New;
    SuperBlock.CWD = 0;
    SuperBlock.Num_Inodes++;

    /* Store the Special Blocks in the Block System */
    memcpy(Block[0], &SuperBlock, sizeof(SuperBlock));
    memcpy(Block[1], &I, sizeof(_inodes));

    return 0;
}