import os
import sys
import subprocess
import numpy as np
import matplotlib.pyplot as plt

Iter_ATN = []
N_Values = [10, 50, 100]
Algorithms = ["FCFS", "Non-Pre_Emptive SJF", "Pre_Emptive SJF", "Round Robin", "HRN" ];
Iterations = 10 
ObjectFile = "Scheduler"

if(os.path.exists(ObjectFile)):
	os.remove(ObjectFile)

Compile = subprocess.Popen([ 'g++' , sys.argv[1] , '-o' , ObjectFile ]) ; 
Compile.wait() ; 
 
for N in N_Values:
	Final_ATN = [ 0 for i in range( len(Algorithms) ) ]
	for i in range(Iterations):
		
		StdOut = subprocess.Popen( [ './' + ObjectFile , str(N), str(i) ], stdout = subprocess.PIPE, stderr=subprocess.PIPE ) ;
		Output = StdOut.communicate()[0].decode()
		ATN = list( map( float, Output.split() ) )

		for j in range( len(Algorithms) ) :
			Final_ATN[j] += ATN[j] ;

	for i in range( len(Algorithms) ) :
		Final_ATN[i] /= Iterations ;

	Iter_ATN.append(Final_ATN);
 
Algo_ATN = (np.array(Iter_ATN)).T;

X = np.array( [10, 50, 100] );

for i in range( len(Algorithms) ):
	plt.plot(X, Algo_ATN[i], '.-', label=Algorithms[i] ) ;

plt.xlabel('The number of processes, N');
plt.ylabel("Average TurnAround Time (in time units)");
plt.legend()
plt.savefig("ATN_Plot.pdf") ;
plt.show()
