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
#define NFFMIN 1000
#define NFF ((int)FFLIST.size())

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

list<Frame>FFLIST;

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
};

struct Process{
    int s;
    vector<int>keys;
    PageTable pt;
    int curr;
    int pid;
    PageAccessData pad;
    
    Process(int m){
        keys.resize(m);
        curr=0;
    }

    void LoadEssential(){
        for(int i=0;i<NUM_PAGES_ESSENTIAL_SEGMENT;i++){
            pt.arr[i].frame_num = FFLIST.front().frame_num;
            FFLIST.pop_front();
            pt.SetValid(i);
        }
    }

    void Exit(){
        for(int i=0;i<NUM_PAGES_PER_PROC;i++){
            if(!pt.GetValid(i)) continue;
            pt.arr[i].frame_num = pt.getFrameNum(i);
            FFLIST.emplace_back(pt.arr[i].frame_num);
        }
    }

    int GetPage(int k){
        return NUM_PAGES_ESSENTIAL_SEGMENT+(k>>10);
    }

    int findReplacement(){
        int q = -1;
        for(int i=NUM_PAGES_ESSENTIAL_SEGMENT;i<NUM_PAGES_PER_PROC;i++){
            if(!pt.GetValid(i)) continue;
            if(q==-1) q=i;
            else if(pt.arr[i].history < pt.arr[q].history) q=i;
        }
        return q;
    }

    void pageFault(int pg){
        pad.page_faults++;
        if(NFF > NFFMIN){
            pt.arr[pg].frame_num = FFLIST.front().frame_num;
#ifdef VERBOSE
            printf("    Fault on Page %4d: Free frame %d found\n", pg, pt.arr[pg].frame_num);
#endif
            FFLIST.pop_front();
            pt.SetValid(pg);
        }
        else{
            pad.page_replacements++;
            int q = findReplacement();
            assert(q >= 0);
            pt.SetReference(q);
#ifdef VERBOSE
            printf("    Fault on Page %4d: To replace Page %d at Frame %d [history = %d]\n", pg, q, pt.getFrameNum(q), pt.arr[q].history);
#endif

            int f = -1;

            // Attempt 1
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
            
            // Attempt 2
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
            
            // Attempt 3
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
            
            // Attempt 4
            f = FFLIST.front().frame_num;
#ifdef VERBOSE
            printf("        Attempt 4: Free frame %d owned by Process %d chosen\n", f, FFLIST.front().last_pid);
#endif
            pad.replace_attempts[3]++;
            FFLIST.pop_front();

            replace:
            assert(f >= 0);
            pt.Update(q);
            pt.arr[q].frame_num = pt.getFrameNum(q);
            FFLIST.emplace_back(pt.arr[q].frame_num, q, pid);
            pt.arr[pg].history = 0xffff;

            pt.arr[pg].frame_num = f;
            pt.SetValid(pg);
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


queue<int>ready;

float percentage(int a, int b){
    return a*100.0/b;
}

int main(){
    ifstream fp("search.txt");
    if(!fp || !fp.is_open()){
        cerr << "Cannot open file" << endl;
        exit(1);
    }
    for(__u_short f = 0; f < (USERS_MB<<10)/FRAME_SIZE_KB; f++) FFLIST.emplace_back(f);

    int n, m;
    fp >> n >> m;
    vector<Process>simData(n, Process(m));
    for(int i=0;i<n;i++){
        fp >> simData[i].s;
        simData[i].pid = i;
        for(int j=0;j<m;j++) fp >> simData[i].keys[j];
    }

    for(int i=0;i<n;i++){
        simData[i].LoadEssential();
        ready.push(i);
    }

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
        printf("    %-3d       %-6d    %-6d(%.2f%%)    %-6d(%.2f%%)    %3d + %3d + %3d + %3d  (%.2f%% + %.2f%% + %.2f%% + %.2f%%)\n",
            simData[i].pid,
            simData[i].pad.page_accesses,
            simData[i].pad.page_faults,
            percentage(simData[i].pad.page_faults, simData[i].pad.page_accesses),
            simData[i].pad.page_replacements,
            percentage(simData[i].pad.page_replacements, simData[i].pad.page_accesses),
            simData[i].pad.replace_attempts[0],
            simData[i].pad.replace_attempts[1],
            simData[i].pad.replace_attempts[2],
            simData[i].pad.replace_attempts[3],
            percentage(simData[i].pad.replace_attempts[0], simData[i].pad.page_replacements),
            percentage(simData[i].pad.replace_attempts[1], simData[i].pad.page_replacements),
            percentage(simData[i].pad.replace_attempts[2], simData[i].pad.page_replacements),
            percentage(simData[i].pad.replace_attempts[3], simData[i].pad.page_replacements)
        );
        total.page_accesses += simData[i].pad.page_accesses;
        total.page_faults += simData[i].pad.page_faults;
        total.page_replacements += simData[i].pad.page_replacements;
        for(int j=0;j<4;j++){
            total.replace_attempts[j] += simData[i].pad.replace_attempts[j];
        }
    }

    printf("\n    Total     %-6d    %-6d(%.2f%%)    %-6d(%.2f%%)    %3d + %3d + %3d + %3d  (%.2f%% + %.2f%% + %.2f%% + %.2f%%)\n",
        total.page_accesses,
        total.page_faults,
        percentage(total.page_faults, total.page_accesses),
        total.page_replacements,
        percentage(total.page_replacements, total.page_accesses),
        total.replace_attempts[0],
        total.replace_attempts[1],
        total.replace_attempts[2],
        total.replace_attempts[3],
        percentage(total.replace_attempts[0], total.page_replacements),
        percentage(total.replace_attempts[1], total.page_replacements),
        percentage(total.replace_attempts[2], total.page_replacements),
        percentage(total.replace_attempts[3], total.page_replacements)
    );
}