# SHM_SHELL: The Simple Command Line Interpreter

## Functionalities

### 1.) Runs External Commands
    Can Run external commands by invoking their executable files through the execvp() syscall 
	
	int execvp(const char *file, char *const argv[]) @ <unistd.h> 
	- The first argument is the name of the executable 
	- The second argument is an array of possible command line arguments 
	- Returns only when execution failure occurs , possible due to "executable not found" (ERRNO-2)

### 2.) Input-Output Redirection ( < / > )

	We take care of Input-Output Redirection by utilising the fact that the 
	- STANDARD INPUT is at File Descriptor - 0
	- STANDARD OUTPUT is at File Descriptor - 1

	What we do is we open the necessary file descriptor which will act as the source of input and duplicate it at the STDIN_FILENO i.e. 0 , and similarly we duplicate the source of output at the STDOUT_FILENO : 1 

	We use the dup2() function for this purpose
	int dup2(int oldfd, int newfd) @ <unistd.h> 
	-> This is a combination of : close(oldfd) , dup(newfd) ; 
	- The File Descriptor at 'oldfd' is closed , and the File Descriptor at 'newfd' is duplicated. 
	Since the recently closed File Descriptor is the minimum availabe one, duplicate happens at that number.

### 3.) Piped Processes Handling ( | )

	We take care of Piped Processes by creating pipes using pipe() function 
	int pipe(int pipefd[2])	@ <unistd.h> 
	- creates a pipe by initialising the PIPE File Descriptors at pipefd[0]-the READ end, and pipefd[1]-the WRITE end
	- returns -1 on error 

	We effectively use N-1 pipes for a N process piped command where the output of the i-th process is fed as input into to the (i+1)-th process.

	Each piped process is executed in a child process forked by the parent, where the Input/Output File Decriptors are changed accordingly and execvp() is called with the necessary arguments. 

	The Pipes are created by the parent using the pipe() just before the fork() is called to limit the scope of the pipe to the parent and that particular child. 

### 4.) Background Process Handling ( & )

	We take care of the Background process with the help of a flag Background_Process which is set to 1 if the Command Parser detects a syntactically correct presence of '&' operator. 
	We simply do not wait for the child processes to complete in order to exhibit this functionality. 

## The Command Parser

### 1.) Splitting the command wrt to pipes. 
	We first split the command with respect to pipes present in the input string ( if at all ) by creating a stringstream object initialised with it and splitting it wrt '|' using getline() 

### 2.) Handling Input-Output Redirection and Background Process Operators ( < / > , & )
	We create a temporary string with <, > and & inserted with trailing and succeeding spaces. 
	We then create a stringstream object initialised with this string and split it wrt to ' ' using getline() 

### 3.) File Handling, Background Execution and Command Line Arguments
	After the splitting is done, we run through the Vector of Strings- We add arguments to an array of character pointers, we open the string present after '<' as the input file and the string after '>' as the output file. 
	We set the flag Background_Process as true iff the string "&" is present at the absolute end of the last command in the piped mode. 

## Intricacies Handled
	
### 1.) Avoiding Simultaneous Redirection to File and Pipes
	In case of a clash for Input-Output redirection between pipes and files, we have given a preference to files.
	-> For Commands like './sample > out.txt | ./test' : The Output of './sample' is redirected to 'out.txt' and the pipe is closed, hence './test' just received the EOF and continues its execution. 
	-> For Commands like './sample | ./test < in.txt' : The Output of './sample' is redirected to the pipe but at the './test' process that pipe is closed and ignored and the input is redirected from 'in.txt'

### 2.) Background Process Handling
	In case the process is to be executed in the Background and it is printing output to the console, we redirect that output to a junk file 'Junk.txt' which is basically a dumpyard for this uneccesary output. 
	We check if for a process with '&' present, if the Output_FileDesc of the final command = STDOUT_FILENO,
	we then replace it with the Junk_FileDesc

### 3.) Error Handling 
	All Errors are printed in BOLD_RED colour on the terminal
	We flag an error if the
	1.) '&' occurs at any place other than the end of the command
	2.) If nothing is present between two pipes 
	3.) No Input File is mentioned after '<' or that Input File cannot be opened
	4.) No Output File is mentioned after '>' or that Output File cannot be opened
	5.) Pipe creation or Forking is unsuccesful
	6.) If execvp() returns due to some reason 
