# Sparse Matrix - Vector Multiplication - OpenMP

## Scripts
	
	create_ChunkWise.py   - Creates DataFrame for each (Chunksize, Matrix) over all the schedules with sampling sys.argv[1]
	create_ThreadWise.py  - Creates DataFrame for each (NumThreads, Matrix) over all the schedules with sampling sys.argv[1]
	
	chunkTOthread.py      - Converts output folder of (1) into an output folder of (2)
	threadTOchunk.py      - Converts output folder of (2) into an output folder of (1) **
	
	plot_TimeVsChunk      - Creates Execution Time vs ChunkSize keeping NumThreads Fixed for each Matrix
	plot_TimeVsThreads    - Creates Execution Time vs NumThreads keeping ChunkSize Fixed for each Matrix

	plot_MinExecVsChunk   - Creates Minimum Execution Time over all NumThreads vs Chunksize for each Matrix
	plot_MinExecVsThreads - Creates Minimum Execution Time over all Chunksize vs NumThreads for each Matrix

	plot_MaxExecVsChunk   - Creates Maximum Execution Time over all NumThreads vs Chunksize for each Matrix
	plot_MaxExecVsThreads - Creates Maximum Execution Time over all Chunksize vs NumThreads for each Matrix

	plot_SpeedupVsChunk   - Creates Speedup Factor over all NumThreads vs Chunksize for each Matrix
	plot_SpeedupVsThreads - Creates Speedup Factor over all Chunksize vs NumThreads for each Matrix

	compile_MatrixWise    - Compiles the above generated plots MatrixWise **


## Dependencies
	
	create_*      		- matrices/mat[1-4].bin 

	chunkTothread 		- sys.argv[1]/Chunk_Matrix[1-4]/<ChunkSize>.tsv
	threadTOchunk 		- sys.argv[1]/Threads_Matrix[1-4]/<NumThreads>.tsv

	plot_*VsChunk 	 	- sys.argv[1]/Chunk_Matrix[1-4]/<ChunkSize>.tsv

	plot_*VsThreads 	- sys.argv[1]/Threads_Matrix[1-4]/<NumThreads>.tsv	

	compile_MatrixWise	- sys.argv[1]/Chunk_Matrix[1-4]/<ChunkSize>.tsv
				- sys.argv[1]/Threads_Matrix[1-4]/<NumThreads>.tsv	

## Output

	create_ChunkWise    	- Sampling<sys.argv[1]>/Chunk_Matrix[1-4]/<ChunkSize>.tsv
	create_ThreadWise	- Sampling<sys.argv[1]>/Threads_Matrix[1-4]/<NumThreads>.tsv
	
	chunkTOthread 		- Sampling<sys.argv[1]>/Threads_Matrix[1-4]/<NumThreads>.tsv
	threadTOchunk 		- Sampling<sys.argv[1]>/Chunk_Matrix[1-4]/<ChunkSize>.tsv

	plot_TimeVsChunk	- sys.argv[1]/TimeVsChunk/<NumThreads>.jpg
	plot_TimeVsThreads	- sys.argv[1]/TimeVsThreads/<ChunkSize>.jpg

	plot_MinExecVsChunk   	- sys.argv[1]/MinExecVsChunk/Matrix[1-4].jpg
	plot_MinExecVsThreads 	- sys.argv[1]/MinExecVsThreads/Matrix[1-4].jpg

	plot_MaxExecVsChunk   	- sys.argv[1]/MaxExecVsChunk/Matrix[1-4].jpg
	plot_MaxExecVsThreads 	- sys.argv[1]/MaxExecVsThreads/Matrix[1-4].jpg

	plot_SpeedupVsChunk   	- sys.argv[1]/SpeedupVsChunk/Matrix[1-4].jpg
	plot_SpeedupVsThreads 	- sys.argv[1]/SpeedupVsThreads/Matrix[1-4].jpg

	compile_MatrixWise	- sys.argv[1]/Matrix[1-4].jpg

## Runnable
	
	spmv_norm_answer.cc		- g++ -std=c++11 -O3 spmv_norm_answer.cc -fopenmp -o spmv_norm 
		    sys.argv[1] 	- Test Matrix location
		    sys.argv[2]		- Chunk Size : [10, 20, 50, 100 .... ]
		    sys.argv[3]		- Number of threads : [1-24]
		    sys.argv[4]		- Schedule Type : Static, Dynamic, Guided = {0, 1, 2}
		    sys.argv[5]		- Number of samples
