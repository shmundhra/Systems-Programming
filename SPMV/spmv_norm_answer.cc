#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <unistd.h>

using namespace std;

double getWallTime()
{
    struct timeval time;
    if(gettimeofday(&time, NULL))
    return 0;
    double wallTime = (double)time.tv_sec + (double)time.tv_usec * 0.000001;
    return wallTime;
}

void setUpThreads(const int numThreads)
{
    omp_set_num_threads(numThreads);
}

int schedMode;
int numThreads, chunkSize, numSamples;
size_t numRows, numNnz;

/* Normalized Matrix-Vector Product */
void multiply(const vector<int> & rowPtr,
              const vector<int> & colInd,
              const vector<double> & nzv,
              const vector<double> & x,
                    vector<double> & y)
{   
    if (schedMode == 0){
        #pragma omp parallel for num_threads(numThreads) schedule(static, chunkSize)
        for(int i = 0; i < rowPtr.size()-1; i++)
            for(int j = rowPtr[i]; j < rowPtr[i+1]; j++)
            {   
                int col_j = colInd[j];
                /* A[i][j] is multiplied by x[j] and affects y[i] */
                y[i] += nzv[j] * x[col_j];
            }
    }

    else if (schedMode == 1){
        #pragma omp parallel for num_threads(numThreads) schedule(dynamic, chunkSize)
        for(int i = 0; i < rowPtr.size()-1; i++)
            for(int j = rowPtr[i]; j < rowPtr[i+1]; j++)
            {   
                int col_j = colInd[j];
                /* A[i][j] is multiplied by x[j] and affects y[i] */
                y[i] += nzv[j] * x[col_j];
            }
    }

    else if (schedMode == 2){
        #pragma omp parallel for num_threads(numThreads) schedule(guided, chunkSize)
        for(int i = 0; i < rowPtr.size()-1; i++)
            for(int j = rowPtr[i]; j < rowPtr[i+1]; j++)
            {   
                int col_j = colInd[j];
                /* A[i][j] is multiplied by x[j] and affects y[i] */
                y[i] += nzv[j] * x[col_j];
            }
    }    
    
    /* Normalization */
    double sum = 0.0;
    
    #pragma omp parallel for num_threads(numThreads) reduction(+:sum)
    for (int i = 0; i < y.size(); i++){
        sum += y[i];
    }

    if(sum == 0)
        return;

    #pragma omp parallel for num_threads(numThreads)
    for (int i = 0; i < y.size(); i++){
        y[i] /= sum;
    }
}

void multiply_serially(const vector<int> & rowPtr,
                       const vector<int> & colInd,
                       const vector<double> & nzv,
                       const vector<double> & x,
                             vector<double> & y)
{
    for(int i = 0; i < rowPtr.size()-1; i++)
        for(int j = rowPtr[i]; j < rowPtr[i+1]; j++)
        {   
            int col_j = colInd[j];
            /* A[i][j] is multiplied by x[j] and affects y[i] */
            y[i] += nzv[j] * x[col_j];
        }
    
    /* Normalization */
    double sum = 0.0;
    for (int i = 0; i < y.size(); i++){
        sum += y[i];
    }

    if(sum == 0)
        return;
    
    for (int i = 0; i < y.size(); i++){
        y[i] /= sum;
    }
}

void populateVectors(vector<double> & nnz,
                     vector<double> & x)
{
    /* Intialize random number generator */
    random_device rd;
    mt19937 e2(rd());
    uniform_real_distribution<> dist(-1.0, 1.0);
    
    /* Populate vector nnz with random numbers */
    nnz.resize(numNnz);
    for(size_t i = 0; i < numNnz; ++i)
    {
        nnz[i] = dist(e2);
    }

    /* Populate vector x with random numbers */
    x.resize(numRows);
    for(size_t i = 0; i < numRows; ++i)
    {
        x[i] = dist(e2);
    }
    
    return;
}

bool testCorrectness(const vector<int> & rowPtr,
                     const vector<int> & colInd,
                     const vector<double> & nnz,
                     const vector<double> & x)
{
    bool correct(true);
    vector<double> y1(x.size(), 0.0);
    vector<double> y2(x.size(), 0.0);
    
    /* Serial version */
    multiply_serially(rowPtr, colInd, nnz, x, y1);
    
    /* Parallel version */
    multiply(rowPtr, colInd, nnz, x, y2);
    
    /* Compare */
    const double reltol(1e-6), abstol(1e-9);
    for(size_t i = 0; i < y1.size(); ++i)
    {
        if(fabs(y2[i] - y1[i]) > abstol + reltol * fabs(y1[i]))
        {
            correct = false;
            break;
        }
    }
    return correct;
}

double testPerformance(const vector<int> & rowPtr,
                       const vector<int> & colInd,
                       const vector<double> & nnz,
                       const vector<double> & x)                       
{
    vector<double> y(x.size(), 0.0);

    /* Start timer */
    const double startTime = getWallTime();
    
    for(int i = 0; i < numSamples; ++i)
        multiply(rowPtr, colInd, nnz, x, y);
    
    /* End timer */
    const double stopTime = getWallTime();
    
    const double timeElapsed = stopTime - startTime;
    return timeElapsed/(double)numSamples;
}

void read(istream & is,
          vector<int> & rowPtr,
          vector<int> & colInd)
{
    int rowPtrSize(0), colIndSize(0);
    
    is.read((char *)&rowPtrSize, sizeof(int));
    rowPtr.resize(rowPtrSize);
    for(int i = 0; i < rowPtrSize; ++i)
    is.read((char *)&rowPtr[i], sizeof(int));

    is.read((char *)&colIndSize, sizeof(int));
    colInd.resize(colIndSize);
    for(int i = 0; i < colIndSize; ++i)
    is.read((char *)&colInd[i], sizeof(int));
    
    return;
}

/* Usage :
            Arg 1 :- Test Matrix location
            Arg 2 :- Chunk Size
            Arg 3 :- Number of threads
            Arg 4 :- Static, Dynamic, Guided = {0, 1, 2}
            Arg 5 :- Number of samples
            e.g. Run: ./spmv_norm matrices/mat1.bin 100 2 500 10000
*/
int main(const int argc, const char ** argv)
{
    /* Matrix A is stored in Row-Compressed Format          */
    vector<int> rowPtr;     /* Row Pointer array            */
    vector<int> colInd;     /* Column Index array           */
    vector<double> nnz;     /* Non Zeros                    */
    vector<double> x;       /* Vector to be multiplied      */

    chunkSize  = argc > 2 ? atoi(argv[2]) : 1000;
    numThreads = argc > 3 ? atoi(argv[3]) : 1;
    schedMode  = argc > 4 ? atoi(argv[4]) : 0;
    numSamples = argc > 5 ? atoi(argv[5]) : 1000;

    /* Read in the matrix A */
    ifstream ifs(argv[1], ios::in | ios::binary);
    read(ifs, rowPtr, colInd);
    ifs.close();

    /* Generate random numbers and populate nnz and x */
    numRows = size_t(rowPtr.size() - 1);
    numNnz = size_t(colInd.size());
    populateVectors(nnz, x);

    if(numThreads > 1)
    {
        setUpThreads(numThreads);
        /* Results should match between serial and multi-threaded implementation    */
        const bool correct = testCorrectness(rowPtr, colInd, nnz, x);
        if(!correct)
            cout << "Multi-threaded result does not match serial result" << endl;
    }
    
    const double perf = testPerformance(rowPtr, colInd, nnz, x);
    // cout << argv[1] << "\t" << rowPtr.size() << "\t" << colInd.size() << "\t" << numThreads << "\t";
    cout << perf << endl;
    return 1;
}
