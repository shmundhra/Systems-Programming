# Assignment3 - Parallel Merge Sort using MPI

## How to run
	mpicc -lm <src_name> -o <obj_name>
	mpirun -np 8 ./<obj_name> <NUMBER of INTEGERS>

## Array Generation
We have used a Lehmer Random Generator to generate the array. The point to keep in mind is that Lehmer Random Generation is kind of a Markov Process and depends on the previous state. Hence we can not generate the numbers parallely since their is an inherent sequential nature to the generation. However we have ensured that no extra space is declared. Each node has an array with a size equal to the number of elements it will process. The previous node sends its last element to the next node to kickstart the array generation process in that node. **High degree of space optimisation has been achieved in this way**

## Sorting
We have created a subarray for each node and that shall be sorted in a sequential manner using normal *mergesort()* and *merge()*

## Merging
After the base condition we are basically left with merging _nproc_ sorted lists. There are many optimal algorithms usings heaps to solve this problem sequentially but alas they cannot be parallelized. Pipelined Mergesort has a classic subroutine to do this but we shall stick to a simpler Parallel Merging procedure. 
What we do is we merge (0 <- 1), (2 <- 3), (4 <- 5), (6 <- 7) in the first pass.
Then we merge (0 <- 2), (4 <- 6) in the second pass.
And then finally we merge (0 <- 4) in the third pass.
**This algorithm is Time optimal with an O(N) complexity.**
**However this algorithm is Work Suboptimal with an O(NlogP) complexity.**
