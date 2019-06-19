k = 3
m = 10
f = 30
s = 5 

MENU:
	@echo "Welcome to Virtual Memory Simulation :: Default k=$(k) m=$(m) f=$(f) s=$(s)"
	@echo ""
	@echo "CHOOSE APPROPRIATE COMMAND : "
	@echo "EXEC [k=<NumProcess>] [m=<MaxPages>] [f=<TotalFrames] [s=<TLB_Size>] - Execute Simulation on Terminal" 	
	@echo "XTERM [k=<NumProcess>] [m=<MaxPages>] [f=<TotalFrames] [s=<TLB_Size>] - Execute Simulation on Terminal and XTERM	"
	@echo "IPCRM - Removes Previously used IPC Keys, if Valid "
	@echo "clean = Removes Created Files and Clears Screen "

EXEC:
	g++ Master.cpp -o Master
	g++ Scheduler.cpp -o Scheduler
	g++ MMU.cpp -o MMU
	g++ Process.cpp -o Process
	./Master $(k) $(m) $(f) $(s) 2

XTERM:
	g++ Master.cpp -o Master
	g++ Scheduler.cpp -o Scheduler
	g++ MMU.cpp -o MMU
	g++ Process.cpp -o Process
	./Master $(k) $(m) $(f) $(s) 0	

IPCRM:
	ipcrm -Q 623
	ipcrm -Q 341
	ipcrm -Q 712 
	ipcrm -M 412
	ipcrm -M 123	

clean:
	rm -f Master Scheduler MMU Process Result.txt
	clear 
