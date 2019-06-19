#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
using namespace std;
#define str(X) to_string(X) 
#define BLACK cout<<"\033[1;30m";
#define RED cerr<<"\033[1;31m";
#define GREEN cout<<"\033[1;32m";
#define YELLOW cout<<"\033[1;33m";
#define BLUE cout<<"\033[1;34m";
#define MAGENTA cout<<"\033[1;35m";
#define CYAN cout<<"\033[1;36m" ;
#define WHITE cout<<"\033[1;37m" ;
#define RESET1 cout<<"\033[0m" ; cout.flush(); cout<<endl;
#define RESET2 cerr<<"\033[0m" ; cerr.flush(); cerr<<endl;

struct __Message{
	long Type;
	char Data[100];
};
//Structure for Messages

struct __Entry{
	int Frame;
	bool isValid;
};
struct __PageTable{
	__Entry Entry[100];
};
struct __ProcessInfo{
	pid_t PID;
	int Num_Pages;
	int Count_Frame;
	int Max_Frame;
	__PageTable PageTable; 
};

struct __FrameInfo{
	int P_Index ;
	int PageMap ;
	int Timestamp ; 
};

struct Node{ 
	int val; 
	Node *next; 
	Node *prev; 
	Node(int x): val(x), next(NULL), prev(NULL) {}; 
};

struct __List{
	Node* front;
	Node* rear ;
	int MAX_SIZE;
	int size ; 
	void init(int num){
		 front = rear = NULL ; 
		 MAX_SIZE = num , size = 0 ;
	}
	int clear(){
		for( Node* it = rear; it; ){
			Node* tmp = it ; 
			it = it->next ; 
			delete tmp ; 
		}		 
	} 
	bool isEmpty(){
		return(front==NULL);
	}
	bool isFull(){
		return(size==MAX_SIZE);
	}
	int first(){
		return front->val ; 
	}
	Node* deque(){
		Node* tmp = front; 
		if ( front == rear ){
			front = rear = NULL ; 
		}
		else{
			front = front->prev;
			front->next = NULL;	
		}		
		tmp->prev = NULL;
		return tmp ;
	}
	Node* enqueue(int val){
		Node* tmp = new Node(val);
		if( isEmpty() ){
			front = rear = tmp;
		}
		else{
			rear->prev = tmp; 
			tmp->next = rear;
			rear = tmp ; 
		}
		return tmp ; 
	}
};

struct __TLB{
	map <int, int> P2F ; 
	map <int, Node*> P2N ; 
	__List List ; 
	void init(int size){
		P2F.clear();
		P2N.clear();		
		List.init(size);
	}
	void clear(){
		P2F.clear();
		P2N.clear();
		List.clear();
	}
	int getFrame( int page ){
		if ( P2F.find(page)==P2F.end() ){
			return -1; 
		}
		Node *tmp1, *tmp2, *tmp3 ; 
		tmp1 = P2N[page]; 
		tmp2 = tmp1->prev;
		tmp3 = tmp1->next;

		if(tmp2) tmp2->next = tmp3 ; 
		else List.rear = tmp3 ; 

		if(tmp3) tmp3->prev = tmp2 ; 
		else List.front = tmp2; 

		delete tmp1 ; 
		P2N[page] = List.enqueue(page) ; 
		return P2F[page] ; 
	}
	int setFrame( int page, int frame ){
		if ( P2F.find(page)!=P2F.end() ){
			P2F[page] = frame; 
			getFrame(page); 
		}
		else{
			if( List.isFull() ){
				List.size -- ; 
				int Victim = List.first() ;
				List.deque();
				P2F.erase(Victim);
				P2N.erase(Victim);
			}
			P2F[page] = frame ; 
			P2N[page] = List.enqueue(page) ;
			List.size++ ;
		}
	} 
	
};

void ErrorExit( string s ){
	perror(s.c_str()) ; 
	exit(EXIT_FAILURE) ; 
}

int main( int argc, char* argv[] )
{	
	WHITE cout<<"MMU Created "<<endl ; RESET1 ; 

	key_t key_SM1 = atoi(argv[1]);
	key_t key_SM2 = atoi(argv[2]);
	key_t key_MQ2 = atoi(argv[3]);
	key_t key_MQ3 = atoi(argv[4]);  
	int k = atoi(argv[5]);
	int m = atoi(argv[6]); 
	int f = atoi(argv[7]);
	int s = atoi(argv[8]);

	int MQ2 = msgget(key_MQ2, IPC_CREAT|0666); 
	int MQ3 = msgget(key_MQ3, IPC_CREAT|0666);
	int SM1 = shmget(key_SM1, sizeof(__ProcessInfo)*k, IPC_CREAT|0666 ) ; 
	int SM2 = shmget(key_SM2, sizeof(  __FrameInfo)*f, IPC_CREAT|0666 ) ; 
	__ProcessInfo *ProcessInfo = (__ProcessInfo *)shmat(SM1, NULL, 0 );
	  __FrameInfo *FreeFrame	    =   (__FrameInfo *)shmat(SM2, NULL, 0 ); 
	
	__TLB TLB ;
	int TimeCount = 0 ; 	
    int LP_Index = -1 ; 
    FILE* result = fopen("Result.txt", "w") ; 
    fprintf(result, "%10s%15s%10s\t\t%3s%19s%2s\t\t%s\n", "", "GLOBAL ORDERING", "", "", "PAGE FAULT SEQUENCE", "", "RESIDENT FRAME" );
    fprintf(result, "%s  %s  %s\t\t%s  %s\t\t%4s%s%4s", "Timestamp", "ProcessIndex", "PageNumber", "ProcessIndex", "PageNumber", "", "Number", "");

	while(1)
	{ 	
		/*MMU waits on MQ3 for a Page Request from some Process */
			__Message PageReq_Msg; 
			if ( msgrcv(MQ3, &PageReq_Msg, sizeof(PageReq_Msg), 1, 0 ) < 0 ) ErrorExit("PageReq_Msg Receive on MQ3") ;
		/*MMU Comes out of Bust Waiting on Receiving Page Request */
		
		int P_Index, Page ; 
		if ( sscanf(PageReq_Msg.Data, "%d %d", &P_Index, &Page) != 2 ) ErrorExit("Invalid Page Request Format") ; 

		WHITE cout<<"Request from Process"<<P_Index<<" for Page "<<Page; RESET1 ;  

		fprintf(result, "\n%9d  %12d  %10d\t\t", ++TimeCount, P_Index, Page ) ; fflush(result);

		if ( Page == -9 )													/* Process Termination */
		{	
			/*Clear Page Table */
				__PageTable& Proc_PageTable = ProcessInfo[P_Index].PageTable;
				for ( int i = 0 ; i < ProcessInfo[P_Index].Num_Pages ; i++ )
				{
					Proc_PageTable.Entry[i] = {-1, false } ; 
				}

			/*Static Release of Frames */ 
				for ( int i=0; i<f ; i++ )
				{
					if ( FreeFrame[i].P_Index == P_Index ){
						FreeFrame[i] = {-1, -1 , 0 } ;
					}
				}


			/*Type2 Message (TERMINATED) to Scheduler */
				WHITE cout<<"Process"<<P_Index<<" has TERMINATED due to Completion" ; RESET1 ;
				__Message MMU_Status = { 1 , "2" } ;
				if ( msgsnd(MQ2, &MMU_Status, sizeof(__Message), 0 ) < 0 ) ErrorExit("MMU_Status Send on MQ2") ;

			continue ; 
		}

		if ( !( 0 <= Page and Page < ProcessInfo[P_Index].Num_Pages ) )		/* Invalid Page Request */
		{	
			/* Send Frame -2 to Process P_Index  */
				__Message FrameReturn_Msg = { 2 , "-2" } ; 
				if ( msgsnd(MQ3, &FrameReturn_Msg, sizeof(__Message), 0 ) < 0 )  ErrorExit("FrameReturn_Msg Send on MQ3") ; 

			/* Type2 Message to Scheduler */
				WHITE cout<<"Process"<<P_Index<<" has TERMINATED due to Invalid Page Request" ; RESET1 ;
				__Message MMU_Status = { 1 , "2" } ;
				if ( msgsnd(MQ2, &MMU_Status, sizeof(__Message), 0 ) < 0 ) ErrorExit("MMU_Status Send on MQ2") ;

			continue ; 
		}

		/* Check for Context Switch :: Clear High Speed TLB Cache if Switch */
			if ( LP_Index != P_Index ){				
				TLB.clear() ;
				TLB.init(s) ; 
				LP_Index = P_Index ;
			}		 

		/* Level 1 : Check TLB  */	
		int Frame_Alloc = TLB.getFrame(Page); 	
		if ( Frame_Alloc >= 0 ){
					
					/* TLB-HIT has Taken Place :: Update TLB */ 
						WHITE cout << "TLB-Hit "; RESET1 ; 
						TLB.setFrame(Page, Frame_Alloc );
						fprintf(result, "%12s  %10s\t\t", "", "" ) ; fflush(result);


					/* Update FreeFrameList */
						FreeFrame[Frame_Alloc].Timestamp = TimeCount ;

					/* Send Frame to Process P_Index */					
						__Message FrameReturn_Msg = { 2 , "" };
						sprintf(FrameReturn_Msg.Data, "%d", Frame_Alloc ) ; 
						if ( msgsnd(MQ3, &FrameReturn_Msg, sizeof(__Message), 0 ) < 0 )  ErrorExit("FrameReturn_Msg Send on MQ3") ; 

		}
		else		/* TLB-Miss :: Level 2 Check Process Page Table accessed by P_Index  */
		{
			__PageTable& Proc_PageTable = ProcessInfo[P_Index].PageTable; 			
			Frame_Alloc = Proc_PageTable.Entry[Page].Frame ;

			if ( Proc_PageTable.Entry[Page].isValid ){ 	
					
					/* Page Table Hit has Taken Place  :: Update Page Table */
						WHITE cout <<"Page Table Hit "; RESET1 ; 
						fprintf(result, "%12s  %10s\t\t", "", "" ) ; fflush(result);

					/* Update FreeFrameList */
						FreeFrame[Frame_Alloc].Timestamp = TimeCount ;

					/* Update TLB */
						TLB.setFrame(Page,Frame_Alloc);

					/* Send Frame to Process P_Index */					
						__Message FrameReturn_Msg = { 2 , ""};
						sprintf(FrameReturn_Msg.Data, "%d", Frame_Alloc ) ; 
						if ( msgsnd(MQ3, &FrameReturn_Msg, sizeof(__Message), 0 ) < 0 )  ErrorExit("FrameReturn_Msg Send on MQ3") ; 

			}	
			else	/* Page-Fault Encountered  :: Level 3 Check Free-Frame List  */
			{		
					RED cout <<"Page Fault Encountered"; RESET1 ; 
					fprintf(result, "%12d  %10d\t\t", P_Index, Page ) ; fflush(result);
					
					/*  Send -1 to Process to STOP it while Page Fault is Handled */					
						__Message FrameReturn_Msg = { 2 , "-1"};
						if ( msgsnd(MQ3, &FrameReturn_Msg, sizeof(__Message), 0 ) < 0 )  ErrorExit("FrameReturn_Msg Send on MQ3") ; 
						sleep(1);

					/* Check FreeFrame List */
						int& Count = ProcessInfo[P_Index].Count_Frame ;
						int& Max = ProcessInfo[P_Index].Max_Frame ; 
						int VictimTime = INT_MAX ; 

						for ( int ff=0 ; ff<f ; ff++ )
						{	
							if ( FreeFrame[ff].P_Index == -1 ){
								if ( Count < Max ){
									WHITE cout<<"Free Frame "<<ff<<" Found"; RESET1 ;
									Frame_Alloc = ff ; 
									Count++ ; 
									break ;
								} 
							}
							else if ( FreeFrame[ff].P_Index == P_Index ){
								if (  FreeFrame[ff].Timestamp < VictimTime ){
									VictimTime = FreeFrame[ff].Timestamp;
									Frame_Alloc = ff ; 
								}
							}
						}

					/* We have Empty/LRU Frame saved in FrameAlloc */ 

					/* Update PageTable */
						int prev_Page = FreeFrame[Frame_Alloc].PageMap ; 
						if ( prev_Page >= 0 ){	
							WHITE cout<<"Replacing Page "<<prev_Page<<" at Frame "<<Frame_Alloc; RESET1;
							Proc_PageTable.Entry[prev_Page].isValid = false ; 
						}
						Proc_PageTable.Entry[Page] = {Frame_Alloc, true} ;
					
					/* Update FreeFrameList */
						FreeFrame[Frame_Alloc] = {P_Index, Page, TimeCount };

					/* Send Type1 to Scheduler to Indicated PAGE FAULT HANDLED */ 
						RED cout<<"Page Fault Handled for Process"<<P_Index; RESET1 ; 
						__Message MMU_Status = { 1 , "1" } ;
						if ( msgsnd(MQ2, &MMU_Status, sizeof(__Message), 0 ) < 0 ) ErrorExit("MMU_Status Send on MQ2") ;
 
			}


		}
		fprintf(result, "%6s%2d%6s", "", Frame_Alloc ,"" ) ; fflush(result);

	}

	

}