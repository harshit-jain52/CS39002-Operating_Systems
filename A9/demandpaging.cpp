#include <iostream>
#include <fstream>
#include <queue>
#include <deque>
#include <cstring>

using namespace std;

#define MEM_SIZE_MB 64
#define FRAME_SIZE_KB 4
#define PAGE_SIZE_KB FRAME_SIZE_KB
#define OS_RESERVE_MB 16
#define USERS_MB (MEM_SIZE_MB - OS_RESERVE_MB)
#define NUM_PAGES_PER_PROC 2048
#define NUM_PAGES_ESSENTIAL_SEGMENT 10

queue<__u_short>freeFrames;
int num_page_accesses=0;
int num_page_faults=0;
int num_swaps=0;
int degree_of_multiprogramming;

struct PageTable{
    __u_short arr[NUM_PAGES_PER_PROC];

    PageTable(){
        memset(arr, 0, sizeof(arr));
    }

    int Set(int page){
        if(Check(page)) return 0;
        if(freeFrames.empty()) return -1;
        arr[page] = freeFrames.front();
        arr[page] |= (1<<15);
        freeFrames.pop();
        return 0;
    }

    bool Check(int page){
        return ((arr[page]>>15)&1);
    }

    void Clear(int page){
        if(!Check(page)) return;
        arr[page] &= ((1<<15)-1);
        freeFrames.push(arr[page]);
    }
};

struct Process{
    int s;
    vector<int>keys;
    PageTable pt;
    int curr;
    int pid;

    Process(int m){
        keys.resize(m);
        curr=0;
    }

    void LoadEssential(){
        for(int i=0;i<NUM_PAGES_ESSENTIAL_SEGMENT;i++){
            pt.Set(i);
        }
    }

    void SwapOut(){
        for(int i=0;i<NUM_PAGES_PER_PROC;i++){
            pt.Clear(i);
        }
    }

    int getPageNum(int k){
        return NUM_PAGES_ESSENTIAL_SEGMENT+k/1024;
    }

    bool BinarySearch(){
#ifdef VERBOSE
        cout << "\tSearch " << curr+1 << " by Process " << pid << endl;
#endif
        int L=0, R=s-1;
        while(L<R){
            int M = (L+R)/2;
            int pg = getPageNum(M);
            num_page_accesses++;
            if(!pt.Check(pg)){
                num_page_faults++;
                if(pt.Set(pg)==-1) return false;
            }
            if(keys[curr] <= M) R=M;
            else L=M+1;
        }
        curr++;
        return true;
    }
};


queue<Process>swappedOut;
deque<Process>ready;

void print_summary(){
    printf("+++ Page access summary\n");
    printf("\tTotal number of page accesses  =  %d\n",num_page_accesses);
    printf("\tTotal number of page faults    =  %d\n",num_page_faults);
    printf("\tTotal number of swaps          =  %d\n",num_swaps);
    printf("\tDegree of multiprogramming     =  %d\n", degree_of_multiprogramming);
}

int main(){
    ifstream fp("search.txt");
    if(!fp || !fp.is_open()){
        cerr << "Cannot open file" << endl;
        exit(1);
    }
    for(__u_short f = 0; f < (MEM_SIZE_MB*1024)/FRAME_SIZE_KB; f++) freeFrames.push(f);
    for(__u_short f = 0; f < (OS_RESERVE_MB*1024)/FRAME_SIZE_KB; f++) freeFrames.pop();

    int n, m;
    fp >> n >> m;
    vector<Process>simData(n, Process(m));
    degree_of_multiprogramming = n;
    for(int i=0;i<n;i++){
        fp >> simData[i].s;
        simData[i].pid = i;
        for(int j=0;j<m;j++) fp >> simData[i].keys[j];
    }

    cout << "+++ Simulation data read from file" << endl;

    for(int i=0;i<n;i++){
        simData[i].LoadEssential();
        ready.push_back(simData[i]);
    }

    cout << "+++ Kernel data initialized" << endl;

    while(!ready.empty()){
        Process proc = ready.front();
        ready.pop_front();
        if(!proc.BinarySearch()){
            printf("+++ Swapping out process %4d  [%d active processes]\n", proc.pid, (int)ready.size());
            num_swaps++;
            proc.SwapOut();
            degree_of_multiprogramming = min(degree_of_multiprogramming,(int)ready.size());
            swappedOut.push(proc);
        }
        else{
            if(proc.curr==m){
                proc.SwapOut();
                if(!swappedOut.empty()){
                    Process swappedProc = swappedOut.front();
                    swappedOut.pop();
                    swappedProc.LoadEssential();
                    ready.push_front(swappedProc);
                    printf("+++ Swapping in process %5d  [%d active processes]\n", swappedProc.pid, (int)ready.size());
                }
            }
            else ready.push_back(proc);
        }
    }

    print_summary();
}