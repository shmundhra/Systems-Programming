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

void ErrorExit( string s ){
	perror(s.c_str()) ; 
	exit(EXIT_FAILURE) ; 
}

string Construct_PRS( int Pages )
{	
	struct timeval seed ; gettimeofday(&seed, NULL); srand(seed.tv_usec) ;	
	int PRS_Len = rand()%((10-2)*Pages + 1) + 2*Pages ;
	string Process_PRS = "" ; 
	int Page;
	for( int i = 0 ; i < PRS_Len; i++ )
	{
		struct timeval seed ; gettimeofday(&seed, NULL); srand(seed.tv_usec) ;	
		float prob = (float)rand()/RAND_MAX;		
		if( prob < 0.6 or (i == 0))
			Page = rand()%Pages;
		else
			Page = (Page + 1) % Pages ;  
		Process_PRS += str(Page)+" " ; 
	}
	return Process_PRS ; 
}

int main( int argc, char* argv[] )
{	
	pid_t Parent = getpid() ; 
	CYAN cout<<"MASTER Module with ID "<<Parent<<" Started Execution"<<endl; RESET1 ; 

	int k, m, f, s, X ;
	if( sscanf(argv[1], "%d", &k) != 1 )  perror("First Argument is 'Number of Processes' "		) ; 
	if( sscanf(argv[2], "%d", &m) != 1 ) perror("Second Argument is 'Max Pages for a Process' "	) ; 
	if( sscanf(argv[3], "%d", &f) != 1 )  perror("Third Argument is 'Number of Frames' "		) ; 
	if( sscanf(argv[4], "%d", &s) != 1 ) perror("Fourth Argument is 'Size of TLB' "				) ; 
	if( sscanf(argv[5], "%d", &X) != 1 )  perror("Fifth Argument is 'XTERM: No(2) Yes(0)' "		) ; 

	key_t key_MQ1 = 623; //ftok("Master.cpp",60); 
	key_t key_MQ2 = 341; //ftok("Master.cpp",61); 
	key_t key_MQ3 = 712; //ftok("Master.cpp",62);
	key_t key_SM1 = 412; //ftok("Master.cpp",63); 
	key_t key_SM2 = 123; //ftok("Master.cpp",64);
	
	int MQ1 = msgget(key_MQ1, IPC_CREAT|0666); 
	int MQ2 = msgget(key_MQ2, IPC_CREAT|0666); 
	int MQ3 = msgget(key_MQ3, IPC_CREAT|0666);
	int SM1 = shmget(key_SM1, sizeof(__ProcessInfo)*k, IPC_CREAT|0666 );
	int SM2 = shmget(key_SM2, sizeof(  __FrameInfo)*f, IPC_CREAT|0666 );
	__ProcessInfo *Process_Info = (__ProcessInfo *)shmat(SM1, NULL, 0 );
	  __FrameInfo *FreeFrame 		= (  __FrameInfo *)shmat(SM2, NULL, 0 );

	vector<string> Process_PRS(k) ;
	int TOTAL_Pages = 0;

	for( int i=0 ; i<k ; i++ )
	{	
		struct timeval seed ; gettimeofday(&seed, NULL); srand(seed.tv_usec) ;	
		int Pages = 1 + rand()%m ;			
		Process_PRS[i] = Construct_PRS(Pages) ; 
		Process_Info[i].Num_Pages = Pages;
		
		TOTAL_Pages += Pages;		
		
		for(int j = 0; j < Pages ; j ++ ){	
			(Process_Info[i].PageTable).Entry[j] = { -1, false } ; 
		}		
	}

	for ( int i=0 ; i<k ; i++ )
	{
		int Num_Frames = ( Process_Info[i].Num_Pages * f) / TOTAL_Pages ;
		Process_Info[i].Max_Frame = Num_Frames ;
		Process_Info[i].Count_Frame = 0;
	}

	for( int i=0 ; i<f ; i++ )
	{
		FreeFrame[i] = {-1, -1, 0};
	}

	for( int i=0 ; i<k ; i++ )
	{	
		CYAN 
		cout<<"Process"<<i<<" :"<<endl; 
		cout<<"Pages "<<Process_Info[i].Num_Pages<<endl;
		cout<<"PRS : "<<Process_PRS[i]<<endl ; 
		cout<<"Frames Allocated "<<Process_Info[i].Max_Frame<<endl;
		RESET1 ; 
	}

	int status ;
	map < int, int > Id2In ; 
	
	/* Creating SCHEDULER _________________________________________________________________________*/
	pid_t Sch = fork() ;
	if ( Sch < 0 ) ErrorExit("SCHEDULER Forking Error") ; 
	if ( Sch == 0 ) 
	{	
		vector <string> arg = { "xterm", "-e", "./Scheduler", str(key_MQ1), str(key_MQ2) } ;
		vector <char*> argv ;
		for(int j=X; j<arg.size(); j++) argv.push_back(&(arg[j].front())); argv.push_back(NULL);
		execvp( argv[0] , &argv.front() ) ;
		ErrorExit("SCHEDULER Execution");				
	}	
	
	/* Creating MMU_________________________________________________________________________________*/
	pid_t MMU = fork();
	if ( MMU < 0 ) ErrorExit("MMU Forking Error") ; 
	if ( MMU == 0 )
	{	
		vector <string> arg = {"xterm", "-e", "./MMU", str(key_SM1), str(key_SM2), str(key_MQ2), str(key_MQ3), str(k), str(m), str(f), str(s) } ;
		vector <char*> argv ;
		for(int j=X; j<arg.size(); j++) argv.push_back(&(arg[j].front())); argv.push_back(NULL);
		execvp( argv[0] , &argv.front() ) ;	
		ErrorExit("MMU Execution");
	}

	/* Creating Control Channel _____________________________________________________________________*/
	pid_t Control = fork() ;
	if ( Control < 0 ) ErrorExit("Control Channel Forking Error") ; 
	if ( Control == 0 )
	{
		/* Creating Processes _______________________________________________________________________*/
		for( int i = 0; i < k ; i++ )
		{	
			/* Process[i] */
			pid_t p = fork();
			if ( p < 0 ) ErrorExit("Process" + str(i) + " Forking Error") ; 
			if ( p == 0 )
			{	
				vector <string> arg = {"./Process", str(key_MQ1), str(key_MQ3), str(i), Process_PRS[i]}  ; 
				vector <char*> argv ; 
				for(int j=0; j<arg.size(); j++) argv.push_back( &(arg[j].front()) ); argv.push_back(NULL) ;
				execvp( argv[0] , &argv.front() ) ;	
				ErrorExit( ("P_Index" + str(i) + " Execution") );				
			}
			else
			{	
				Id2In[p] = i ; 
				Process_Info[i].PID = p ;
				usleep(250000);
			}
		}
		exit(EXIT_SUCCESS) ; 
	}
	else
	{	
		/* Wait on MQ1 to get Termination ACKs from SCHEDULER */
		int counter = 0 ; 
		while( counter < k )
		{	
			__Message Termination_ACK ; 
			if ( msgrcv( MQ1, &Termination_ACK, sizeof(__Message), 2, 0 ) < 0 ) ErrorExit("ACK Receive on MQ1") ; 
			int P_ID = atoi(Termination_ACK.Data);

			CYAN cout<<"Process"<<Id2In[P_ID]<<" with ID "<<P_ID<<" TERMINATED"; RESET1 ; 

			counter++ ; 
		}
		
	}	 	
	CYAN cout<<"Terminating SCHEDULER..."; RESET1; kill(Sch, SIGKILL) ;
	CYAN cout<<"Terminating MMU..."	  	 ; RESET1; kill(MMU, SIGKILL) ;	
	shmctl(SM1, IPC_RMID, NULL );
	shmctl(SM2, IPC_RMID, NULL );
	msgctl(MQ1, IPC_RMID, NULL );
	msgctl(MQ2, IPC_RMID, NULL );
	msgctl(MQ3, IPC_RMID, NULL );
	CYAN cout<<"MASTER Terminating." 	 ; RESET1; exit(EXIT_SUCCESS) ; 
}