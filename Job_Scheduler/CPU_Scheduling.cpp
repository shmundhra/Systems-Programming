#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
using namespace std;

typedef pair<double, double> Process_Info;
typedef pair<double, int> SJF_Info;
typedef pair<double, int> HRN_Info;

#define debugI(ds_name)                   \
    fprintf(stderr, "%15s:: ", #ds_name); \
    for (auto val : ds_name)              \
        fprintf(stderr, "%8.0lf ", val);  \
    cerr << "\n";

#define debug(ds_name)                    \
    fprintf(stderr, "%15s:: ", #ds_name); \
    for (auto val : ds_name)              \
        fprintf(stderr, "%8.3lf ", val);  \
    cerr << "\n";

#define debug1(x)                   \
    fprintf(stderr, "%15s:: ", #x); \
    cerr << x << "\n";

#define DELTA 2
#define LAMBDA 0.2 // MEAN of Exponential Distribution = 1/0.2 = 5
#define Arrival first
#define Burst second

// Generates exponential ( LAMBDA = 0.2 ) Inter-Arrival Times and returns the
// Cumulated arrival times of all the processes in sorted order.
vector<double> generate_Arrival(int N)
{
    double uniform, expo; // Uniform and Exponential random variable generation
    vector<double> arrive(N, 0);
    srand(time(NULL));
    for (int i = 1; i < N; i++)
    {
        do
        {
            uniform = (double)((double)rand() / RAND_MAX); // uniform ~ U(0,1)
            expo = -(1.0 / LAMBDA) * log(uniform);

        } while (expo > 10);
        arrive[i] = arrive[i - 1] + expo;
    }
    return arrive;
}

// Generates Burst-Time for each Process as a Uniform Distribution ~ U(1,20)
vector<double> generate_CPU_Burst(int N)
{
    double uniform, burst_val;
    vector<double> burst(N);
    srand(time(NULL));
    for (int i = 0; i < N; i++)
    {
        uniform = (double)((double)rand() / RAND_MAX); // uniform ~ U(0,1)
        burst_val = uniform * 19 + 1;                  // burst_val is uniform between 1 and 20 ~ U(1,20)
        burst[i] = burst_val;
    }
    return burst;
}

double FCFS(vector<Process_Info> Job_Queue, int N)
{
    double Prev_EndTime = 0;             // Previous Finish Time
    double Curr_StartTime, Curr_EndTime; // Starting time of current process
    double Turnaround = 0;               // Turnaround time
    double Avg_Turnaround;
    pair<double, double> Curr_Job;
    vector<double> Schedule(N); // Schedule[i] of ith process

    for (int i = 0; i < N; i++)
    {
        Curr_Job = Job_Queue[i];
        Schedule[i] = Curr_StartTime = max(Prev_EndTime, Curr_Job.Arrival); //Schedule
        Curr_EndTime = Curr_StartTime + Curr_Job.Burst;
        Turnaround += (Curr_EndTime - Curr_Job.Arrival);

        Prev_EndTime = Curr_EndTime;
    }
    //debug(Schedule) ;
    Avg_Turnaround = Turnaround / N; //debug1(Avg_Turnaround) ;
    return Avg_Turnaround;
}

double NonPreEmptive_SJF(vector<Process_Info> Job_Queue, int N)
{
    double Curr_Time, Curr_EndTime, Curr_Job_Index;
    double Turnaround, Avg_Turnaround;
    SJF_Info Curr_Job;
    vector<double> Schedule(N);
    priority_queue<SJF_Info, vector<SJF_Info>, greater<SJF_Info>> Job_PQueue; //Priority Value is CPU burst

    Curr_Time = 0;  // Keep track of current time
    Turnaround = 0; // Total turnaround time

    for (int i = 0; !(Job_PQueue.empty() and i == N);)
    {
        /* New jobs which arrive <=curr_time are pushed into the queue */
        for (; i < N and Job_Queue[i].Arrival <= Curr_Time; i++)
        {
            Job_PQueue.push({Job_Queue[i].Burst, i});
        }
        if (Job_PQueue.empty())
        {
            Curr_Time = Job_Queue[i].Arrival;
            continue;
        }
        /* Schedule the job with lowest CPU burst and update Curr_Time*/
        Curr_Job = Job_PQueue.top();
        Job_PQueue.pop();
        Curr_Job_Index = Curr_Job.second;
        Schedule[Curr_Job_Index] = Curr_Time;
        Curr_EndTime = Curr_Time + Curr_Job.first;
        Turnaround += Curr_EndTime - Job_Queue[Curr_Job_Index].Arrival;

        Curr_Time = Curr_EndTime;
    }
    //debug(Schedule) ;
    Avg_Turnaround = Turnaround / N; //debug1(Avg_Turnaround) ;
    return Avg_Turnaround;
}

double PreEmptive_SJF(vector<Process_Info> Job_Queue, int N)
{
    int Curr_Job_Index;
    double Curr_Time, Curr_EndTime, Next_StartTime;
    double Turnaround, Avg_Turnaround;
    SJF_Info Curr_Job;
    vector<vector<double>> Schedule(N, vector<double>());
    priority_queue<SJF_Info, vector<SJF_Info>, greater<SJF_Info>> Job_PQueue;

    Curr_Time = 0;  // Keep track of current_time
    Turnaround = 0; // Total turnaround time

    for (int i = 0; !(Job_PQueue.empty() and i == N);)
    {
        /* Push New Jobs with arrival time <= Curr_time */
        for (; i < N and Job_Queue[i].Arrival <= Curr_Time; i++)
        {
            Job_PQueue.push({Job_Queue[i].Burst, i});
        }
        if (Job_PQueue.empty())
        {
            Curr_Time = Job_Queue[i].Arrival;
            continue;
        }
        Curr_Job = Job_PQueue.top();
        Job_PQueue.pop();
        Curr_Job_Index = Curr_Job.second;
        Schedule[Curr_Job_Index].push_back(Curr_Time);

        Curr_EndTime = Curr_Time + Curr_Job.first;
        /*Next start time is the arrival time of the next job*/
        Next_StartTime = i == N ? INT_MAX : Job_Queue[i].Arrival;

        if (Curr_EndTime <= Next_StartTime) // This means Curr_job runs to completion
        {
            Turnaround += Curr_EndTime - Job_Queue[Curr_Job_Index].Arrival;
            Curr_Time = Curr_EndTime;
        }
        else // Insert the New Job and repeat the same procedure
        {
            Job_PQueue.push({Curr_EndTime - Next_StartTime, Curr_Job_Index});
            Curr_Time = Next_StartTime;
        }
    }
    //for ( int i = 0 ; i < N ; i++ ){ fprintf(stderr , "%12s[%d]:: " , "Schedule" , i ) ; for ( auto val : Schedule[i] ) fprintf(stderr,"%6.3lf ",val); cerr<<"\n"; }

    Avg_Turnaround = Turnaround / N; //debug1(Avg_Turnaround) ;
    return Avg_Turnaround;
}

double RoundRobin(vector<Process_Info> Job_Queue, int N)
{
    int Curr_Job_Index;
    double Curr_Time, Curr_EndTime;
    double Turnaround, Avg_Turnaround;
    SJF_Info Curr_Job;
    vector<vector<double>> Schedule(N, vector<double>());
    queue<SJF_Info> Job_PQueue;

    Curr_Time = 0;  // Keep track of current_time
    Turnaround = 0; // Total turnaround time
    Curr_Job.second = -1;
    for( int i = 0 ; !(Job_PQueue.empty() and i == N) ; )
    {
        for( ; i < N and Job_Queue[i].Arrival <= Curr_Time; i++ )
        {
            Job_PQueue.push({Job_Queue[i].Burst, i});
        }
        if ( Curr_Job.second != -1 )
        {
            Job_PQueue.push(Curr_Job);
        }
        if ( Job_PQueue.empty() )
        {
            Curr_Time = Job_Queue[i].Arrival;
            continue;
        }
        Curr_Job = Job_PQueue.front();
        Job_PQueue.pop();
        Curr_Job_Index = Curr_Job.second;
        Schedule[Curr_Job_Index].push_back(Curr_Time);

        if ( DELTA < Curr_Job.first )           // When Time Quantum expires before Current Job
        {
            Curr_EndTime = Curr_Time + DELTA;
            Curr_Job.first -= DELTA;
        }
        else                                    // When the Job Finishes before the Time Quantum
        {
            Curr_EndTime = Curr_Time + Curr_Job.first;
            Curr_Job.second = -1;
            Turnaround += Curr_EndTime - Job_Queue[Curr_Job_Index].Arrival;
        }
        Curr_Time = Curr_EndTime;
    }
    //for ( int i = 0 ; i < N ; i++ ){ fprintf(stderr , "%12s[%d]:: " , "Schedule" , i ) ; for ( auto val : Schedule[i] ) fprintf(stderr,"%6.3lf ",val); cerr<<"\n"; }

    Avg_Turnaround = Turnaround / N; //debug1(Avg_Turnaround) ;
    return Avg_Turnaround;
}

double Highest_RespRatio(vector<Process_Info> Job_Queue, int N)
{
    double W, RR;
    double Curr_Time, Curr_EndTime, Curr_Job_Index;
    double Turnaround, Avg_Turnaround;
    Process_Info Curr_Job;
    vector<double> Schedule(N);
    set<HRN_Info, greater<HRN_Info>> Job_PQueue;
    vector<double> RespRatio(N, 0);

    Curr_Time = 0;
    Turnaround = 0;

    for (int i = 0; !(Job_PQueue.empty() and i == N);)
    {
        /* New Jobs added with correct priorities */
        for (; i < N and Job_Queue[i].Arrival <= Curr_Time; i++)
        {
            W = Curr_Time - Job_Queue[i].Arrival;
            RR = 1.0 + W / Job_Queue[i].Burst;
            RespRatio[i] = RR;
            Job_PQueue.insert({RR, -i});
        }
        if (Job_PQueue.empty())
        {
            Curr_Time = Job_Queue[i].Arrival;
            continue;
        }

        Curr_Job_Index = -(Job_PQueue.begin()->second);
        debug1(Curr_Job_Index);
        debug(RespRatio);

        Job_PQueue.erase(Job_PQueue.begin());
        Curr_Job = Job_Queue[Curr_Job_Index];

        Schedule[Curr_Job_Index] = Curr_Time;
        Curr_EndTime = Curr_Time + Curr_Job.Burst;
        RespRatio[Curr_Job_Index] = 0;
        Turnaround += Curr_EndTime - Curr_Job.Arrival;
        Curr_Time = Curr_EndTime;
        /*Update Priorities of existing jobs*/
        for (int j = 0; j < N; j++)
        {
            if (RespRatio[j])
            {
                Job_PQueue.erase({RespRatio[j], -j});
                W = Curr_Time - Job_Queue[j].Arrival;
                RR = 1.0 + W / Job_Queue[j].Burst;
                RespRatio[j] = RR;
                Job_PQueue.insert({RR, -j});
            }
        }
    }
    //debug(Schedule) ;

    Avg_Turnaround = Turnaround / N; //debug1(Avg_Turnaround) ;
    return Avg_Turnaround;
}

int main(int argc, char *argv[])
{
    int N, iter;
    char Process_Code;
    string File_Name, InputDirectory, ResultDirectory, GoBack;
    double FCFS_ATN, NPE_SJF_ATN, PE_SJF_ATN, RR_ATN, HRN_ATN;
    FILE *File_Ptr;
    vector<double> Process_Id;
    vector<double> Arrival_Time;
    vector<double> CPU_Burst;
    vector<Process_Info> Job_Queue; /*Job Queue implemented as vector*/

    if (!sscanf(argv[1], "%d", &N))
    {
        cerr << "Argument not an Integer\n";
        exit(EXIT_FAILURE);
    }
    if (!sscanf(argv[2], "%d", &iter))
    {
        cerr << "Argument not an Integer\n";
        exit(EXIT_FAILURE);
    }

    InputDirectory = "Input_Data";
    ResultDirectory = "Results";
    GoBack = "..";

    /* Generate Arrival_times and CPU Bursts */
    for (int i = 0; i < N; i++)
        Process_Id.push_back(i);
    Arrival_Time = generate_Arrival(N);
    CPU_Burst = generate_CPU_Burst(N);

    /*Save data in file */

    if (chdir(InputDirectory.c_str()) < 0)
    {
        mkdir(InputDirectory.c_str(), 0666);
        chdir(InputDirectory.c_str());
    }

    File_Name = to_string(N) + "_" + to_string(iter) + (string) ".txt";
    File_Ptr = freopen(File_Name.c_str(), "w", stdout);
    fprintf(stdout, "%15s %15s %15s\n", "Process_Id", "Arrival_Time", "CPU_Burst");
    for (int i = 0; i < N; i++)
    {
        fprintf(stdout, "%15.0lf %15.6lf %15.6lf\n", Process_Id[i], Arrival_Time[i], CPU_Burst[i]);
    }
    fclose(File_Ptr);
    chdir(GoBack.c_str());

    //debugI(Process_Id) ; debug(Arrival_Time) ; debug(CPU_Burst) ;

    /*Push Jobs in Queue */
    for (int i = 0; i < N; i++)
    {
        Job_Queue.push_back({Arrival_Time[i], CPU_Burst[i]});
    }

    FCFS_ATN = FCFS(Job_Queue, N);
    NPE_SJF_ATN = NonPreEmptive_SJF(Job_Queue, N);
    PE_SJF_ATN = PreEmptive_SJF(Job_Queue, N);
    RR_ATN = RoundRobin(Job_Queue, N);
    HRN_ATN = Highest_RespRatio(Job_Queue, N);

    cout << FCFS_ATN << " " << NPE_SJF_ATN << " " << PE_SJF_ATN << " " << RR_ATN << " " << HRN_ATN << " ";

    if (chdir(ResultDirectory.c_str()) < 0)
    {
        mkdir(ResultDirectory.c_str(), 0666);
        chdir(ResultDirectory.c_str());
    }
    File_Name = to_string(N) + "_" + to_string(iter) + (string) "_Results.txt";
    File_Ptr = freopen(File_Name.c_str(), "w", stdout);
    fprintf(stdout, "%15s:: %8.3lf\n", "FCFS_ATN", FCFS_ATN);
    fprintf(stdout, "%15s:: %8.3lf\n", "NPE_SJF_ATN", NPE_SJF_ATN);
    fprintf(stdout, "%15s:: %8.3lf\n", "PE_SJF_ATN", PE_SJF_ATN);
    fprintf(stdout, "%15s:: %8.3lf\n", "RR_ATN", RR_ATN);
    fprintf(stdout, "%15s:: %8.3lf\n", "HRN_ATN", HRN_ATN);
    fclose(File_Ptr);
    chdir(GoBack.c_str());
    exit(EXIT_SUCCESS);
}