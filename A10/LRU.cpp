#include <iostream>
#include <fstream>
#include <queue>
#include <list>
#include <cstring>
#include <cassert>

using namespace std;

#define MEM_SIZE_MB 64
#define FRAME_SIZE_KB 4
#define PAGE_SIZE_KB FRAME_SIZE_KB
#define OS_RESERVE_MB 16
#define USERS_MB (MEM_SIZE_MB - OS_RESERVE_MB)
#define NUM_PAGES_PER_PROC 2048
#define NUM_PAGES_ESSENTIAL_SEGMENT 10

#define NFFMIN 1000 // Minimum no. of free frames
#define NFF ((int)FFLIST.size()) // No. of Free Frames

float percentage(int a, int b){
    return a*100.0/b;
}

struct Frame{
    __u_short frame_num;
    int page_num;
    int last_pid;
    
    Frame(int f){
        frame_num = f;
        page_num = -1;
        last_pid = -1;
    }

    Frame(int f, int pg, int pid) : frame_num(f), page_num(pg), last_pid(pid) {}
};

struct Page{
    __u_short frame_num;
    __u_short history;

    Page(){
        frame_num = 0;
        history = 0xffff;
    }
};

list<Frame>FFLIST; // Free Frame List

struct PageTable{
    vector<Page> arr;

    PageTable(){
        arr.resize(NUM_PAGES_PER_PROC);
    }

    void SetValid(int page){
        arr[page].frame_num |= (1<<15);
    }

    void SetReference(int page){
        arr[page].frame_num |= (1<<14);
    }

    bool GetValid(int page){
        return ((arr[page].frame_num>>15)&1);
    }

    bool GetReference(int page){
        return ((arr[page].frame_num>>14)&1);
    }

    void ClearReference(int page){
        int valid = GetValid(page);
        arr[page].frame_num &= ((1<<14)-1);
        arr[page].frame_num |= (valid<<15);
    }

    void ClearValid(int page){
        arr[page].frame_num &= ((1<<15)-1);
    }

    void Update(int page){
        if(!GetValid(page)) return;
        int ref = GetReference(page);
        arr[page].history = (arr[page].history>>1) | (ref << 15);
        ClearReference(page);
    }
    
    int getFrameNum(int page){
        return arr[page].frame_num & ((1<<14)-1);
    }
};

struct PageAccessData{
    int page_accesses;
    int page_faults;
    int page_replacements;
    int replace_attempts[4];

    PageAccessData(){
        page_accesses = 0;
        page_faults = 0;
        page_replacements = 0;
        memset(replace_attempts, 0, sizeof(replace_attempts));
    }

    void operator+=(const PageAccessData& other){
        page_accesses += other.page_accesses;
        page_faults += other.page_faults;
        page_replacements += other.page_replacements;
        for(int i=0;i<4;i++){
            replace_attempts[i] += other.replace_attempts[i];
        }
    }

    void printSummary(int pid){
        if(pid < 0) printf("\n    Total     ");
        else printf("    %-3d       ", pid);
        printf("%-6d    %-6d(%.2f%%)    %-6d(%.2f%%)    %3d + %3d + %3d + %3d  (%.2f%% + %.2f%% + %.2f%% + %.2f%%)\n",
            page_accesses,
            page_faults,
            percentage(page_faults, page_accesses),
            page_replacements,
            percentage(page_replacements, page_accesses),
            replace_attempts[0],
            replace_attempts[1],
            replace_attempts[2],
            replace_attempts[3],
            percentage(replace_attempts[0], page_replacements),
            percentage(replace_attempts[1], page_replacements),
            percentage(replace_attempts[2], page_replacements),
            percentage(replace_attempts[3], page_replacements)
        );
    }
};

struct Process{
    int s;
    vector<int>keys;
    PageTable pt;
    int curr;
    int pid;
    PageAccessData pad;
    
    // Constructor
    Process(int m){
        keys.resize(m);
        curr=0;
    }

    // Load Essential Pages
    void LoadEssential(){
        for(int i=0;i<NUM_PAGES_ESSENTIAL_SEGMENT;i++){
            pt.arr[i].frame_num = FFLIST.front().frame_num;
            FFLIST.pop_front();
            pt.SetValid(i);
        }
    }

    // Process Exit
    void Exit(){
        for(int i=0;i<NUM_PAGES_PER_PROC;i++){
            if(!pt.GetValid(i)) continue;
            pt.arr[i].frame_num = pt.getFrameNum(i);
            FFLIST.emplace_back(pt.arr[i].frame_num);
        }
    }

    // Get Page Number
    int GetPage(int k){
        return NUM_PAGES_ESSENTIAL_SEGMENT+(k>>10);
    }

    // Find Replacement (Victim) Page
    int findReplacement(){
        int q = -1;
        for(int i=NUM_PAGES_ESSENTIAL_SEGMENT;i<NUM_PAGES_PER_PROC;i++){
            if(!pt.GetValid(i)) continue;
            if(q==-1) q=i;
            else if(pt.arr[i].history < pt.arr[q].history) q=i;
        }
        return q;
    }

    // Page Fault Handler
    void pageFault(int pg){
        pad.page_faults++;
        if(NFF > NFFMIN){
            // Free Frame available, no replacement needed
            pt.arr[pg].frame_num = FFLIST.front().frame_num;
#ifdef VERBOSE
            printf("    Fault on Page %4d: Free frame %d found\n", pg, pt.arr[pg].frame_num);
#endif
            FFLIST.pop_front();
        }
        else{
            // Page Replacement needed
            pad.page_replacements++;
            int q = findReplacement();
            assert(q >= 0);
#ifdef VERBOSE
            printf("    Fault on Page %4d: To replace Page %d at Frame %d [history = %d]\n", pg, q, pt.getFrameNum(q), pt.arr[q].history);
#endif

            int f = -1;

            // Attempt 1 - Check if there's a free frame with the same last owner and page number
            for(auto it = FFLIST.begin(); it != FFLIST.end(); it++){
                if(it->last_pid == pid && it->page_num == pg){
                    f = it->frame_num;
#ifdef VERBOSE
                    printf("        Attempt 1: Page found in free frame %d\n", f);
#endif
                    pad.replace_attempts[0]++;
                    FFLIST.erase(it);
                    break;
                }
            }
            if(f!=-1) goto replace;
            
            // Attempt 2 - Check if there's a free frame with no owner
            for(auto it = FFLIST.rbegin(); it != FFLIST.rend(); it++){
                if(it->last_pid == -1){
                    f = it->frame_num;
#ifdef VERBOSE
                    printf("        Attempt 2: Free frame %d owned by no process found\n", f);
#endif
                    pad.replace_attempts[1]++;
                    FFLIST.erase(prev(it.base()));
                    break;
                }
            }
            if(f!=-1) goto replace;
            
            // Attempt 3 - Check if there's a free frame with the same last owner
            for(auto it = FFLIST.rbegin(); it != FFLIST.rend(); it++){
                if(it->last_pid == pid){
                    f = it->frame_num;
#ifdef VERBOSE
                    printf("        Attempt 3: Own page %d found in free frame %d\n", it->page_num, f);
#endif
                    pad.replace_attempts[2]++;
                    FFLIST.erase(prev(it.base()));
                    break;
                }
            }
            if(f!=-1) goto replace;
            
            // Attempt 4 - Pick a random (here, first) free frame
            f = FFLIST.front().frame_num;
#ifdef VERBOSE
            printf("        Attempt 4: Free frame %d owned by Process %d chosen\n", f, FFLIST.front().last_pid);
#endif
            pad.replace_attempts[3]++;
            FFLIST.pop_front();

            replace:
            assert(f >= 0);

            // Free the frame storing victim page
            pt.arr[q].frame_num = pt.getFrameNum(q);
            FFLIST.emplace_back(pt.arr[q].frame_num, q, pid);

            pt.arr[pg].frame_num = f;
        }
    }

    void BinarySearch(){
#ifdef VERBOSE
        printf("+++ Process %d: Search %d\n", pid, curr+1);
#endif
        int L=0, R=s-1;
        while(L<R){
            int M = (L+R)/2;
            int pg = GetPage(M);
            if(!pt.GetValid(pg)){
                pageFault(pg);
                pt.arr[pg].history = 0xffff;
                pt.SetValid(pg);
            }
            pad.page_accesses++;
            pt.SetReference(pg);
            if(keys[curr] <= M) R=M;
            else L=M+1;
        }
        curr++;
        for(int i=NUM_PAGES_ESSENTIAL_SEGMENT;i<NUM_PAGES_PER_PROC;i++){
            pt.Update(i);
        }
    }
};


queue<int>ready; // Ready Queue


int main(){
    ifstream fp("search.txt");
    if(!fp || !fp.is_open()){
        cerr << "Cannot open file" << endl;
        exit(1);
    }

    // Initialize Free Frame List
    for(__u_short f = 0; f < (USERS_MB<<10)/FRAME_SIZE_KB; f++) FFLIST.emplace_back(f);

    // Read Simulation Data
    int n, m;
    fp >> n >> m;
    vector<Process>simData(n, Process(m));
    for(int i=0;i<n;i++){
        fp >> simData[i].s;
        simData[i].pid = i;
        for(int j=0;j<m;j++) fp >> simData[i].keys[j];
    }

    // Initialize Kernel Data
    for(int i=0;i<n;i++){
        simData[i].LoadEssential();
        ready.push(i);
    }

    // Simulation Loop
    while(!ready.empty()){
        int proc = ready.front();
        ready.pop();
        simData[proc].BinarySearch();
        if(simData[proc].curr >= m){
            simData[proc].Exit();
            continue;
        }
        ready.push(proc);
    }

    PageAccessData total;

#ifdef VERBOSE
    printf("\n");
#endif

    printf("+++ Page access summary\n");
    printf("    PID     Accesses        Faults         Replacements                        Attempts\n");
    for(int i=0;i<n;i++){
        simData[i].pad.printSummary(simData[i].pid);
        total += simData[i].pad;
    }

    total.printSummary(-1);
}