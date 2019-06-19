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

void ErrorExit( string s ){
	perror(s.c_str()) ; 
	exit(EXIT_FAILURE) ; 
}

int main(int argc,char* argv[])
{	
	key_t key_MQ1  = atoi(argv[1]);
	key_t key_MQ3  = atoi(argv[2]);
	int P_Index    = atoi(argv[3]);
	int P_ID 	   = getpid();	
	string PRS(argv[4]) ; 

	int MQ1 = msgget(key_MQ1, IPC_CREAT|0666) ;
	int MQ3 = msgget(key_MQ3, IPC_CREAT|0666) ;
	
	BLUE cout<<"PROCESS"<<P_Index<<" Created with ID "<<P_ID; RESET1
		
	/* Process Puts Itself Into the Ready Queue */
		BLUE cout<<"Process"<<P_Index<<" with PID: "<<P_ID<<" Enqueueing into Ready Queue"; RESET1 
		__Message ReadyQueue_Msg = { 1 , "" } ; 
		sprintf(ReadyQueue_Msg.Data, "%d", P_ID);
		if ( msgsnd( MQ1, &ReadyQueue_Msg, sizeof(__Message), 0) < 0 ) ErrorExit("ReadyQueue_Msg Send on MQ1");
		
	/* Stop Itself Temporarily, SCHEDULER will wake it up with SIGCONT */
		kill( getpid() , SIGSTOP ) ; 

	/* Process Continues after Receiving a SIGCONT Signal from the SCHEDULER */
		BLUE cout<<"Process"<<P_Index<<" Scheduled by SCHEDULER " ; RESET1
	
	/* Extracting Pages by Parsing Reference String */
		int Page;		
		vector<int> Pages; 
		
		stringstream SS(PRS);
		while( SS>>Page ){
			Pages.push_back(Page);
		}
	
	for ( int i=0 ; i<Pages.size() ; i++ )
	{	
		Page = Pages[i] ; 		
		sleep(1);

		/* Process Sending Frame Request to MMU */
			BLUE cout<<"\nProcess"<<P_Index<<" Requesting Frame for Page "<<Page<<" from MMU"; RESET1 
			__Message PageReq_Msg = { 1 , "" } ; 
			sprintf(PageReq_Msg.Data,"%d %d", P_Index, Page );
			if ( msgsnd(MQ3, &PageReq_Msg, sizeof(__Message), 0) < 0 ) ErrorExit("PageReq_Msg Send on MQ3") ;
			
		/* Process Getting Frame Allocated by MMU */
			__Message FrameReturn_Msg; 
			if ( msgrcv(MQ3, &FrameReturn_Msg, sizeof(__Message), 2 , 0 ) < 0 ) ErrorExit("FrameReturn_Msg Receive on MQ3") ; 
			
			int Frame = atoi(FrameReturn_Msg.Data);
			GREEN cout <<"Received Frame "<<Frame << " from MMU" ; RESET1 ; 
		
		/*Checking Frame Validity */
			if(Frame == -2){  						// Invalid Frame, Exit as Failure
				exit(EXIT_FAILURE);
			}
			else if(Frame == -1){ 					// Page Fault, STOP itself while Page Fault is Handled
				i-- ; 
				kill(getpid(),SIGSTOP);
			}
			else{									// Valid Frame Returned

			}
	}
	
	/* Sending Termination Message to the MMU */
		BLUE cout<<"\nProcess"<<P_Index<<" Sending Termination Message to MMU" ; RESET2 ; 
		__Message Terminate_Msg = { 1 , "" } ; 							
		sprintf(Terminate_Msg.Data,"%d %d", P_Index, -9 );
		if ( msgsnd(MQ3, &Terminate_Msg, sizeof(Terminate_Msg), 0) < 0 ) ErrorExit("Terminate_Msg Send on MQ3") ;

	exit(EXIT_SUCCESS); 
}