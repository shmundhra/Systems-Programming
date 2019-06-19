#include <bits/stdc++.h>
using namespace std ; 
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BLACK cout<<"\033[1;30m";
#define RED cerr<<"\033[1;31m";
#define GREEN cout<<"\033[1;32m";
#define YELLOW cout<<"\033[1;33m";
#define BLUE cout<<"\033[1;34m";
#define MAGENTA cout<<"\033[1;35m";
#define CYAN cout<<"\033[1;36m" ;
#define WHITE cout<<"\033[1;37m" ;
#define RESET1 cout<<"\033[0m" ; cout.flush() ;  
#define RESET2 cerr<<"\033[0m" ; cerr<<endl ;  
#define debug( ds_name ) cerr<<"\n"<<#ds_name<<"::" ; for ( auto val : ds_name ) cerr<<val<<"_" ; cerr<<'\n' ; 
#define debug1(x) cerr<<#x<<" :: "<<x<<"\n";
#define debug2(x,y) cerr<<#x<<" :: "<<x<<"\t"<<#y<<" :: "<<y<<"\n";
#define debug3(x,y,z) cerr<<#x<<" :: "<<x<<"\t"<<#y<<" :: "<<y<<"\t"<<#z<<" :: "<<z<<"\n";

string trim ( string s ) // Trim left and right spaces
{
	int l , r ; 
	for ( l = 0 ; l < s.length() ; l++ ) if ( s[l]   != ' ' ) break ; 
	for ( r = s.length() ; r > 0 ; r-- ) if ( s[r-1] != ' ' ) break ; 
	return string( s.begin()+l , s.begin()+r ) ; 
}

int main ()
{	
	int error ; 
	int status ; 	
	int Junk_FileDesc , Background_Process ; //Background_Process checks if process needs to run in background ( '&' handling flag )
	int Process_In , Process_Out ; 
	int STDIN , STDOUT ; 
	string Input_FileName, Output_FileName ; 
	string command , tmp ; 
	vector <string> Command  ; 

	STDIN = dup(0) ; STDOUT = dup(1) ; 	// Creating Duplicated to STDIN , STDOUT in case need arises . 
	while(1)
	{	
		error = 0 ; 
		Command.clear() ; 	
		//Junk_FileDesc = open ( "/dev/null" , 0 ) ;
		Junk_FileDesc = open ( "Junk.txt" , O_CREAT | O_WRONLY | O_TRUNC  ) ; // Junk File created to redirect output from console in case of '&' 
						
		MAGENTA cout<<"SHM_SHELL >>> "; RESET1 
		CYAN getline(cin,command) ; RESET1 

		command = trim(command) ; 		
		if ( command.size() == 0 ) continue ; 
		if ( command == "exit" ) exit(EXIT_SUCCESS) ; 

		stringstream iss(command) ; 		 				  //Split command w.r.t pipes
		while ( getline( iss , tmp , '|' ) )
		{	
			Command.push_back( (tmp) ) ; 
		}		
		
		int PIPE[Command.size()][2] ; 						  //PIPES for communication between piped commands
		vector < int > Input_FileDesc(Command.size() , -1 ) ; 
		vector < int > Output_FileDesc(Command.size() , -1 ) ;
		vector < pid_t > process(Command.size()) ; 
		vector <char*> argv[Command.size()] ; 
		vector < string > arg[Command.size()] ;	

		*Input_FileDesc.begin() = 0 , *Output_FileDesc.rbegin() = 1 , Background_Process = 0 ;  

		for ( int i = 0 ; i < Command.size() ; i++ )
		{														
			command.clear() ; 
			/* Add spaces before and after special symbols if space not provided
			for ex:- ./a.out<input.txt is changed to ./a.out < input.txt */			
			for ( char c : Command[i] )
			{
				if ( c == '<' or c == '>' or c == '&' )
				{
					command.push_back(' ') ;
					command.push_back( c ) ;
					command.push_back(' ') ;
				}
				else
				{
					command.push_back( c ) ;
				}
			}
			
			stringstream css(command) ; /* Split command w.r.t spaces */
			while ( css >> tmp )
			{
				arg[i].push_back(tmp) ;
			}

			for ( int j = 0 ; j < arg[i].size() ; j++ )
			{
				if ( arg[i][j] == "&" ) //Background_Process
				{
					if ( ! ( i == (int)Command.size()-1 and j == (int)arg[i].size()-1 ) ) // We allow the '&' to be present only at the end of the last process in a piped command 
					{
						RED cerr<<"!!! SYNTAX ERROR : Misplaced \'&\'\n"; RESET2
						error = 1 ; 
						break ;
					}
					else
					{
						Background_Process = 1  ; 
						arg[i].pop_back() ;
					}
				}
			}
			if ( error == 1 ) break ;				 
	        
			if ( arg[i].size() == 0 )
			{
				RED cerr<<"!!! SYNTAX ERROR : No Process after \'|\'\n" ; RESET2 
				error = 1 ; 
				break ; 
			}

			for ( int j = 0 ; j < arg[i].size() ; j++ )
			{
				if ( arg[i][j] == "<" ) //Input from file
				{	
					if ( ++j == arg[i].size() )
					{
						RED cerr<<"!!! SYNTAX ERROR : No Input File after \'<\'\n" ; RESET2 
						error = 1 ; 
						break ; 
					}
					Input_FileName = arg[i][j] ;
					if ( (Input_FileDesc[i] = open ( Input_FileName.c_str() , O_RDONLY )) < 0 )
					{
						RED perror( ("%s " , Input_FileName.c_str()) ) ; RESET2
						error = 1 ;
						break ; 
					} 					
				}
				else if ( arg[i][j] == ">" ) //Output to file
				{	
					if ( ++j == arg[i].size() )
					{
						RED cerr<<"!!! SYNTAX ERROR : No Output File after \'>\'\n" ; RESET2 
						error = 1 ; 
						break ; 
					}
					Output_FileName = arg[i][j] ;
					if ((Output_FileDesc[i] = open ( Output_FileName.c_str() , O_CREAT | O_WRONLY | O_TRUNC )) < 0) 
					{	
						RED	perror(( "%s " , Output_FileName.c_str()) ) ; RESET2
						error = 1 ;
						break ;
					}
				}
				else 
				{
					argv[i].push_back( &arg[i][j].front() ) ;
				}
			}
			argv[i].push_back(NULL) ;	        			
	   	}
	    if ( error == 1 ) continue ; 

	   	if ( Background_Process == 1 )
	   	{
	   		if ( Input_FileDesc.front() == 0 ) *Input_FileDesc.begin() = Junk_FileDesc ; 
	   		if ( Output_FileDesc.back() == 1 ) *Output_FileDesc.rbegin() = Junk_FileDesc ; 
	   	}

		for ( int i = 0 ; i < Command.size() ; i++ )
		{	
			if ( pipe(PIPE[i]) < 0 ){
				RED perror("Pipe Creation unsuccessful ") ; RESET2
				break ; 
			}
			if( (process[i] = fork()) < 0 ){
				RED perror("Forking unsuccessful "); RESET2 
				break ; 
			}
			if( process[i] == 0 )
			{	
				if ( Input_FileDesc[i] != -1 )		 //if Input File specified
				{
					dup2 ( Input_FileDesc[i] , 0 ) ; //Redirect Input from Input File
					Process_In = Input_FileDesc[i] ; 
				}
				else
				{	//Input File not specified, so Input from Pipe 
					close( PIPE[i-1][1] ) ; 	//Close the writing end of the pipe
					dup2 ( PIPE[i-1][0] , 0 ) ; //Make the reading end of pipe as stdin
					Process_In = PIPE[i-1][0] ; 
				}

				if ( Output_FileDesc[i] != -1 )		//if Output File specified
				{	
					dup2 ( Output_FileDesc[i] , 1 ) ;  //Redirect Output to Output File 
					Process_Out = Output_FileDesc[i] ; 
				}	
				else
				{	
					//Output to Pipe to send to next command in Piped mode
					close( PIPE[i][0] ) ;		//Close the reading end of the pipe
					dup2( PIPE[i][1] , 1 ) ; 	//Make the writing end of pipe as stdout
					Process_Out = PIPE[i][1] ; 
				}			
					
				// RED debug( arg[i] ) ; debug3( Process_In , Process_Out , Background_Process ) ; RESET2 
				execvp( argv[i][0], &argv[i].front() );		//Execute the command		

				// Restore STDIN STDOUT in case execvp() returns to indicate failure
				dup2 ( STDIN , 0 ) , dup2 ( STDOUT , 1 ) ; 
				RED perror("!!! Execution Failure ") ; RESET2
				exit(EXIT_FAILURE) ; 
				
			}
			else
			{	
				usleep(100000) ; 						// Parent sleeps for 0.1 second simply for proper formatting of output on terminal 
				dup2( Junk_FileDesc , PIPE[i-1][0] ) ;  // Closes
				dup2( Junk_FileDesc , PIPE[ i ][1] ) ; 						
			}
			
		}

		if ( Background_Process == 0 )	
		{	
			for ( int i = 0 ; i < Command.size() ; i++ ) //If it is NOT a Background Process, then the parent has to wait for its child
			{
				waitpid(process[i] , &status , 0 ) ; 
			}
		}

		for ( int File_Desc = 5 ; File_Desc <= 5 + 2*Command.size() ; File_Desc++ ){
			close(File_Desc) ; // Closing the extra created File Descriptors
		}
	}

	return 0 ; 
}