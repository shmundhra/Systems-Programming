#Simulation of Virtual Memory using Demand Paging

##Overview
In this folder, we attempt to simulate the demand paging paradigm to allocate page frames to the page demands of different processes. We use **Proportional Frame Allocation** to partition the memory and the **Locally - Least Recently Used** Page Replacement Algorithm, along with the use of a Transition Lookahead Buffer (TLB) to handle the page demands of a process.  

**Proportional Frame Allocation**  
This involves assigning processes main-memory page-frames in a ratio which is proportional to the ratio of the number of pages required by each process. This ensures that each process can occupy only that number of frames which it technically needs as compared to other processes. 

**Local Least Recently Used**    
This is a Page Replacement Algorithm which comes into action when we hit a Page Fault, i.e. when the _Demanded Page_ is not present in the Process' Page Table and when there are no _Free Frames_ available for allocation. In such a situation an _Interrupt Occurs_ and the _Interrupt Service Routine_ is invoked. The ISR then performs a page replacement. Here since the Algorithm is **Local**, it is expected that their is some partition in the memory ( which is true) for each process. The ISR then looks up the Locally Available Frames and takes away the Frame allocated to the _Victim Page_ as given by the **LRU Algorithm**. It then assigns the freshly vacated frame to the demanded page and then makes the necessary updates to the **Free-Frame List** and the **Process Page Table**. 

The implementation is broken up into four different modules which communicate to each other using XSI-IPC Message Queues, and access some Shared Memory for the different data structures required; keys of which are shared through Command Line Arguments.

##Master Module 
The Master Module is like the main module of the simulation, which is the starting and the ending point of the whole program.
It is responsible for the creation of the other three modules: **Scheduler**, **MMU**, and the **Processes**. Before that, it creates and dynamically allocates memory to the Shared Memory and Message Queues using unique Keys.

###Shared Memory segment 1 (SM1)   
It contains the Page Tables for each process.  
There are **'k' Page Tables**, one for each process.  
**Process#i** page table consists of **m#i fields**, each field corresponds to a page of the process.   
**Page#j** field consists of (frame number, validity bit) which stores which frame it was last assigned to, and whether that frame still holds it or not. 

###Shared Memory segment 2 (SM2)  
It contains the Free-Frame list of the Main Memory.  

###Shared Memory segment 3 (SM3)  
It contains the details of each process.   
There is an array of **( pid\_t, p\_index )** sorted by the *pid_t* to allow a quick lookup mapping *Process_Id* to *Process_Index*.     
There is another array indexed by the *Process_Indices* which has one field corresponding to details of each process - **( pid\_t, No. of Pages, Allocated Frames )**

###Message Queue 1 (MQ1)  
is shared between the **Master** and the **Scheduler Module**, and acts as the _Ready-Queue_, with the **Master** creating processes at regular intervals and sends them for scheduling through the _Ready-Queue_ to the **Scheduler**.
>	**Type-1 Message** :: New Process added to *Ready-Queue* by **Master** to be scheduled by **Scheduler**.  
>	**Type-2 Message** :: Process Termination ACK to **Master** by **Scheduler**.

###Message Queue 2 (MQ2)  
is shared between the **Scheduler** and the **MMU Module**, and acts as the _Status-Queue_, with the **MMU** processing the page demands from the Processes and sending the status through the _Status-Queue_ to the **Scheduler**.  
>	**Type-1 Message** :: Process Terminated  
>	**Type-2 Message** :: Page-Fault Handled

###Message Queue 3 (MQ3)  
is shared between the **MMU** and the **Processes**, and acts as the _Request-Queue_. At an instant only one process can request for a page, and its process_id can be exracted by the **MMU** from the *msqid\_ds.msg\_lspid*, and other information can be taken from **MQ3**.

A **Process** parses the page numbers from the Reference String and sends the request through the _Request-Queue_ to the **MMU**.  
>	**Non-Negative Number**  :: Index of Page Requested  
>	**Negative Number(-9)**  :: Page Reference String Finished, Process Terminated    

The **MMU** then finds the apt frame to be allocated after processing the _TLB_, _Process Page Table_ and the _ISR_ if required, and returns the frame number.   
>	**Non-Negative Number**  :: Index of the Page Frame Allocated  
>	**Negative Number(-1)**  :: Page-Fault Encountered  
>	**Negative Number(-2)**	 :: Invalid Page Requested    

###Control Flow
0.  Counter-intuitively, we need to allocate No. of Pages to each Process beforehand as this distribution shall 	help us in making the Shared Memory and during Local Frame Allocation
1.	The **Master Module** first creates these instances of the **two XSI-IPC forms** and then goes on producing 	the  **Scheduler** and the  **MMU** by *forking a child and calling exec()*.  
2.	It then *forks a child*, and this child then starts creating *k-processes* by generating *Page Reference 		Strings*( interspaced by 250ms ).  
	After creation of a process, the child sends the *Process_Index* to the **Scheduler** for Scheduling as a *Type-1 Message*. After *k-processes* have been created, the child terminates.  
2.	The *parent process* continually waits for *Type-2 process termination ACKs* from the **Scheduler**. After 		receiving *k-terminations*, it kills the **Scheduler** and the  **MMU** and then terminates itself. 

##Scheduler
This Module takes care of the Process Scheduling part of the simulation.  
After being created by the **Master Module**, it goes on to *busy-waiting* on the *Ready Queue* waiting for a process to arrive.  

After a *Process_Index* arrives as a Type-1 Message : 

1.  It Dequeues the First Process from the *Ready-Queue*  
2.  Sends it a Signal to *Wake it up* and *start execution*  
3.  *Busy-Waits* on **MQ2**, the *Status-Queue* to receive an update from the **MMU**   
	4. If it receives a *Type-1 Message* :: *Page-Fault* was encountered and handled - Process is added back to the *Ready-Queue*  
	4. If it receives a *Type-2 Message* :: The Process has *Terminated* - sends a *Type-2 Message* to the **Master**   
5.  Goes back to *busy-waiting* on the *Ready-Queue*   

It is Finally Killed by the **Master Module** during its *Busy-Wait*

##MMU - Memory Management Unit
This Module takes care of the Page Requests from the various processes scheduled by the **Scheduler**, after they are created by the **Master Module**.   
After the **Master Module** creates it, it goes on to *busy-waiting* on the *Request-Queue* waiting for a page request to arrive.   

After a *Page Request* arrives :  

1.  The **MMU** first has to decode which process sends the reqeuest. Since at a time only one process can send 	in a page request, the **MMU** looks up the *msqid\_ds.msg\_lspid* to get the *Process_Id* of the process 		which sent in the *Page Request*.  
	It then looks up the **MQ3** Map to get the *Process_Index* and then it has all the required details about the process.
	2. If the *Page Request* is for (-9), it marks Process Termination. It releases allocated frames, updates *Free-Frame list*and sends a *Type-2 Message* to the **Scheduler**, 
	2. If the *Page Request* is invalid, it sends a Frame (-2) to the sending process. It sends a *Type-2 Message* to the **Scheduler**
2.  If the *Page Request* is valid, it looks up the *Transition Lookahead Buffer* and checks if that particular 	page has been assigned a frame in the recent history.  
   	***IF*** the requested page is present in the *TLB*, the corresponding frame is returned ***ELSE***
3. 	It then looks up the *Process' Page Table* to see if the *Requested Page* has been a frame at all.  
   	***IF*** the requested page is present in the *Page Table*, the corresponding frame is returned ***ELSE***
4. 	*Page-Fault* has occured. This means that we need to *Choose a Frame* to assign the *Requested Page*. We 		choose the apt frame by doing a *Local LRU* Frame Replacement. We take care of *Empty Frames* by treating 		them as the *Leastest Recently Used* frames. So first the empty frames get allocated, then the allocated 		might get replaced.  
	In either case, we make a necessary update to the *Process' Page Table*

It is Finally Killed by the **Master Module** during its *Busy-Wait*

##Processes
These are the module which simulate the execution of a process after being scheduled by the **Scheduler**.   
Each process has a reference string and generation of page numbers by parsing the string and getting back frames can be assumed to be equivalent to the execution of a process in Main Memory.  
After being created by the **Master Module**, it goes into wait awaiting a *signal from the Scheduler* which wakes it up and it starts executing.  

1.  It parses the Reference String to generate *Page Numbers* and sends them to the **MMU** through *MQ3*. 
2.  It then goes on to *busy-waiting* on the *Request Queue* waiting for an allocated frame to arrive.
	3. If *Returned Frame* is (-2) : Marks Invalid Page Request, it **Terminates itself as a Failure**    
	3. If *Returned Frame* is *Non-Negative* : Marks Valid Page Request, it **goes back to Parsing Step**   
	3. If *Returned Frame* is (-1) : Marks occurence of a Page-Fault, it **goes back to wait()** for scheduling. 
4.	At Termination of Reference String : *Page Request* for (-9) is sent and it **goes back to wait()** for 		termination.

It is Finally Killed by the **Master Module** during its *Wait*
