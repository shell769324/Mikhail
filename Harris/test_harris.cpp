#include<iostream>
#include "SkipList.cpp"
#include "CycleTimer.h"
#include <fstream>
#include<string>
#include<thread>
#include<vector>
#include<algorithm>
#include"pthread.c"
//#define NUM_THREADS 1

using  std::cout;
using std::cin;
using std::endl;
using std::vector;
using std::string;


typedef struct {
string filename;
int thread_id;
SkipList* skiplist;
}threadArgs;

int kInitHeadHeight =10;

void checkAdd(){
    int size = 10;
    SkipList* skipList = new SkipList(10);
    Node * node1 = skipList->Search_SL(7);
    if(node1==NULL)
        cout<<"not found"<<endl;
    for (int i = 0; i < size; ++i) {
            skipList->Insert_SL(i);
            skipList->Insert_SL(i);
        }
    Node * node = skipList->Search_SL(7);
    skipList->Delete_SL(7);
}
void processTraceChunk(vector<int> &ops,vector<int> &values,SkipList *skipList){
        int n = ops.size();
        //cout<<" no of. elements"<<n<<endl;
        for(int i = 0;i<n;i++){
            
            int op= ops[i];
            int value =values[i];
            //cout<<"instr "<<i<<" "<<op<<" "<<value<<endl;
            if (op==0){
                auto node = skipList->Search_SL(value);
                /*if(DEBUG){
                    if(node==skipList.end()){
                        cout<<"not found"<<endl;
                    }
                    else{
                        cout<<"Found"<<endl;
                    }
                }*/
            }
            else if (op==1)
                skipList->Insert_SL(value);
                
            else 
                skipList->Delete_SL(value);
        }
    }
void* readFileAndProcess(void* argsptr){
        //Each thread should do the below
        std::ifstream fin;
        threadArgs args = *(threadArgs*) argsptr;
        string filename = args.filename;
        int thread_id = args.thread_id;
        SkipList* skipList = args.skiplist; 
        //cout<<"Doing thread "<<thread_id<<endl;
        //cout<<filename<<endl;
        fin.open(filename);
        
        string line;
        vector<int> ops;
        vector<int> values;
        int a, b;
        while(fin>>a>>b){
            //cout<<a<<b<<endl;
            ops.push_back(a);
            values.push_back(b);        
        }

        pthread_barrier_t mybarrier;
        pthread_barrier_wait(&mybarrier);
        double startTime = CycleTimer::currentSeconds();
            processTraceChunk(ops,values,skipList);
        double endTime = CycleTimer::currentSeconds();
        double* time_taken = (double*)malloc(sizeof(double));
        *time_taken = endTime-startTime;
        cout<<"Time taken by thread"<<thread_id<<" is "<<*time_taken<<endl;

        //cout<<"Completed Thread"<<thread_id<<endl;
        fin.close();
        //cout<<"All done"<<endl;
        return (void*)time_taken;
    }

void processTrace(string filename,int num_threads,int work){
        pthread_t thread[num_threads];
        threadArgs args[num_threads];
        //call function from each thread
        //double times[num_threads];
        void* status[num_threads];
        SkipList* skipList = new SkipList(kInitHeadHeight);

        for (int i=1; i<num_threads; i++){
            args[i].filename = "./files/"+filename+"_"+std::to_string(work)+"_"+std::to_string(i)+".txt";           
            args[i].thread_id = i;
            args[i].skiplist = skipList;
            //cout<<"Starting thread "<<i<<endl;
            pthread_create(&thread[i], NULL, readFileAndProcess,(void*)&args[i]);
        }
        //Parent thread setup Args and call
        args[0].filename = "./files/"+filename+"_"+std::to_string(work)+"_"+ std::to_string(0)+".txt";
        args[0].thread_id = 0;
        args[0].skiplist = skipList;
        status[0] = readFileAndProcess((void*)&args[0]);
        // wait for worker threads to complete
        for (int i=1; i<num_threads; i++)
            pthread_join(thread[i], &status[i]);
        double max_time = 0;
        cout<<"Threads joined"<<endl;
        for(int i =0;i<num_threads;i++)
            max_time = std::max(max_time,*(double*)status[i]);   
        cout<<"total time taken=  "<<max_time<<endl;
    }

int main(int argc, char *argv[]){
    checkAdd();
    int num_threads = atoi(argv[1]);
    int work = atoi(argv[2]);

    string filename = "random";

    string scriptname = "../create_tests.py "+ std::to_string(num_threads)+" "+ std::to_string(work);
    string command = "python ";
    command += scriptname;
    //cout<<command<<endl;
    system(command.c_str());
    

    //string filename(argv[1]);
    std::ifstream fin;
    fin.open("./files/"+filename+"_"+std::to_string(work)+"_0.txt");
    //cout<<"./files/"+filename+"_"+std::to_string(work)+"_0.txt"<<endl;
    int a,b;
    int count=0,limit=1;
    while(fin>>a>>b){
        cout<<a<<" "<<b<<endl;
        if(count++>=limit)
            break;
    }
    fin.close();
    double startTime = CycleTimer::currentSeconds();
    processTrace(filename,num_threads,work);
    double endTime = CycleTimer::currentSeconds();
    cout<<"Time taken with "<<num_threads<<" threads is "<<endTime-startTime<<endl;
    
}