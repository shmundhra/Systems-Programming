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

int main(int argc, char* argv[])
{	
	key_t key_MQ1 = atoi(argv[1]);
	key_t key_MQ2 = atoi(argv[2]);
	int MQ1 = msgget(key_MQ1, IPC_CREAT|0666) ; 
	int MQ2 = msgget(key_MQ2, IPC_CREAT|0666) ; 
	
	MAGENTA cout<<"SCHEDULER Created with ID "<<getpid() ; RESET1 ; 
	
	while(1)
	{	
		/* Scheduler Waiting for Process on Ready Queue */
			__Message ProcReady_Msg ;
			if ( msgrcv(MQ1, &ProcReady_Msg, sizeof(__Message), 1, 0) < 0 ) ErrorExit("ProcReady_Msg Receive on MQ1") ;
		
		/* Comes out of Busy-Waiting when it gets a Process */ 
			int Curr_PID = atoi(ProcReady_Msg.Data);
			MAGENTA cout<<"\n\nProcess " <<Curr_PID<<" has been Scheduled"; RESET2; 
		
		/* Signalling Process to Continue Execution from previously Self STOPPED State */
			kill(Curr_PID, SIGCONT); 		   						
		
		/* Scheduler Waiting for Process Status on Status Queue from MMU */
			__Message MMU_Status ;
			if ( msgrcv(MQ2, &MMU_Status, sizeof(__Message), 1, 0 ) < 0 ) ErrorExit("MMU_Status Receive on MQ2") ;  // get from mmu whether page fault/terminated
		
		/* Comes out of Busy-Waiting when it gets Process Status */ 
		
		RED cout<<"Process "<<Curr_PID<<" Status: ";
		
		if ( strcmp(MMU_Status.Data, "1" ) == 0 ) 				// If the Page-Fault is Handled
		{	
			RED cout<<"PAGE FAULT HANDLED"; RESET1 ; 

			/* Scheduler Enqueuing Process Back to Ready Queue */ 
				__Message ProcEnqueue_Msg = { 1 , "" } ; 
				sprintf(ProcEnqueue_Msg.Data ,"%d", Curr_PID );    
				if ( msgsnd(MQ1, &ProcEnqueue_Msg, sizeof(__Message), 0 ) < 0 ) ErrorExit("ProcEnqueue_Msg Send on MQ1") ; 
		}
		if ( strcmp(MMU_Status.Data, "2" ) == 0 ) 				// If the Process has Terminated
		{	
			RED cout<<"TERMINATED"; RESET1 ;

			/* Scheduler Sending Process Termination ACK to MASTER*/
				__Message Termination_ACK  = { 2, "" } ;
				sprintf(Termination_ACK.Data ,"%d", Curr_PID );  
				if ( msgsnd(MQ1, &Termination_ACK, sizeof(__Message), 0 ) < 0 ) ErrorExit("Termination_ACK Send on MQ1") ; 
		}

	}

}