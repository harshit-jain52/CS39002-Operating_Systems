#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <pthread.h>
#include <vector>
#include <inttypes.h>
#include <unistd.h>
#include <algorithm>
#include <queue>
#include <cassert>
#include <list>
using namespace std;

#define MMAX 20
#define NMAX 100
#define MMIN 5
#define NMIN 10
#define msleep(x) usleep((x)*10000)

struct Request{
    int id;
    enum Type{
        ADDITIONAL,
        RELEASE,
        QUIT
    } type;
    vector<int> R;
    Request(int id, vector<int> R): id(id), R(R) {
        if(any_of(R.begin(), R.end(), [](int x){return x > 0;})) type = ADDITIONAL;
        else type = RELEASE;
    }
    Request(int id): id(id), type(QUIT) {}
    Request(): id(-1), R(0) {}
};

struct BinarySemaphore {
    pthread_mutex_t cmtx;
    pthread_cond_t cv;
    bool signaled;

    BinarySemaphore(): cmtx(PTHREAD_MUTEX_INITIALIZER), cv(PTHREAD_COND_INITIALIZER), signaled(false) {}

    void wait() {
        pthread_mutex_lock(&cmtx);
        while(!signaled) pthread_cond_wait(&cv, &cmtx);
        signaled = false;
        pthread_mutex_unlock(&cmtx);
    }

    void signal() {
        pthread_mutex_lock(&cmtx);
        signaled = true;
        pthread_cond_signal(&cv);
        pthread_mutex_unlock(&cmtx);
    }
};

bool operator<=(const vector<int>&a, const vector<int>&b){
    assert(a.size() == b.size() && "Vectors must have the same size");
    int m = a.size();
    for(int i=0; i<m; i++)
        if(a[i] > b[i]) return false;
    return true;
}

void operator+=(vector<int>& a, const vector<int>& b) {
    assert(a.size() == b.size() && "Vectors must have the same size");
    int m = a.size();
    for (int i=0; i<m; i++) 
        a[i] += b[i];
}

void operator-=(vector<int>& a, const vector<int>& b) {
    assert(a.size() == b.size() && "Vectors must have the same size");
    int m = a.size();
    for (int i=0; i<m; i++) 
        a[i] -= b[i];
}

vector<int> AVAILABLE;
vector<vector<int>> NEED, ALLOC;
#ifdef _DLAVOID
vector<bool> DEAD;
#endif
pthread_barrier_t BOS, REQB;
vector<pthread_barrier_t> ACK;
pthread_mutex_t rmtx, pmtx;
vector<BinarySemaphore> SEM;
Request GlobalRequest;

void logger(string msg){
    pthread_mutex_lock(&pmtx);
    cout << msg << endl;
    pthread_mutex_unlock(&pmtx);
}

void print_waiting_threads(list<Request> &Q){
    string waiting = "\t\tWaiting threads:";
    for(auto it = Q.begin(); it != Q.end(); ++it){
        waiting += (" " + to_string(it->id));
    }
    logger(waiting);
}

void print_left_threads(list<int> &users){
    string left = to_string(users.size()) + " threads left:";
    for(int id: users){
        left += (" " + to_string(id));
    }
    logger(left);
}

void print_available(int){
    string avail = "Available resources:";
    for(int r: AVAILABLE){
        avail += (" " + to_string(r));
    }
    logger(avail);
}

#ifdef _DLAVOID
bool isSafe(vector<int> &AVAILABLE, vector<vector<int>> &ALLOC, vector<vector<int>> &NEED){
    int n = ALLOC.size();
    vector<int> WORK = AVAILABLE;
    vector<bool> FINISH = DEAD;

    while(true){
        bool found = false;
        for(int i=0; i<n; i++){
            if(!FINISH[i] && NEED[i] <= WORK){
                found = true;
                FINISH[i] = true;
                WORK += ALLOC[i];
            }
        }
        if(!found) break;
    }

    return all_of(FINISH.begin(), FINISH.end(), [](bool x){return x;});
}
#endif

void* User(void* targ){
    int id = (intptr_t)targ;
    logger("\tThread " + to_string(id) + " born");

    pthread_barrier_wait(&BOS);
    ostringstream filename;
    filename << "input/thread" << setw(2) << setfill('0') << id << ".txt";
    ifstream fp(filename.str());
    if(!fp || !fp.is_open()){
        cerr << "User thread " << id << " : cannot open file" << endl;
        exit(1);
    }

    for(int i = 0; i < (int)NEED[id].size(); i++){
        fp >> NEED[id][i];
    }

    while(true){
        int delay; fp >> delay;
        msleep(delay);
        char type; fp >> type;
        if(type == 'Q'){
            pthread_mutex_lock(&rmtx);
            GlobalRequest = Request(id);
            // logger("\tThread " + to_string(id) + " sends resource request: type = RELEASE");
            pthread_barrier_wait(&REQB);
            pthread_barrier_wait(&ACK[id]);
            pthread_mutex_unlock(&rmtx);
            logger("\tThread " + to_string(id) + " going to quit");
            break;
        }

        vector<int> R(AVAILABLE.size());
        for(int i = 0; i < (int)R.size(); i++){
            fp >> R[i];
        }

        pthread_mutex_lock(&rmtx);
        GlobalRequest = Request(id, R);
        int reqType = GlobalRequest.type;
        logger("\tThread " + to_string(id) + " sends resource request: type = " + ((reqType==Request::ADDITIONAL)? "ADDITIONAL":"RELEASE"));
        pthread_barrier_wait(&REQB);
        pthread_barrier_wait(&ACK[id]);
        pthread_mutex_unlock(&rmtx);

        if(reqType == Request::ADDITIONAL){
            SEM[id].wait();
            logger("\tThread " + to_string(id) + " is granted its last resource request");
        }
        else{
            logger("\tThread " + to_string(id) + " is done with its resource release request");
        }
    }
    fp.close();
    return NULL;
}

void *Master(void* targ){
    ifstream fp("input/system.txt");
    if(!fp || !fp.is_open()){
        cerr << "Master thread: cannot open file" << endl;
        exit(1);
    }

    int m, n;
    fp >> m >> n;
    if(m < MMIN || m > MMAX || n < NMIN || n > NMAX){
        cerr << "Range of m: " << MMIN << "-" << MMAX << endl;
        cerr << "Range of n: " << NMIN << "-" << NMAX << endl;
        exit(1);
    }
    AVAILABLE.resize(m);
    NEED.resize(n, vector<int>(m));
    ALLOC.resize(n, vector<int>(m));
    ACK.resize(n);
    SEM.resize(n, BinarySemaphore());
#ifdef _DLAVOID
    DEAD.resize(n, false);
#endif
    
    for(int i = 0; i < m; i++){
        fp >> AVAILABLE[i];
    }
    fp.close();

    vector<pthread_t> user_th(n);
    list<int>users;

    pthread_barrier_init(&BOS, NULL, n+1);
    pthread_barrier_init(&REQB, NULL, 2);
    pthread_mutex_init(&rmtx, NULL);
    pthread_mutex_init(&pmtx, NULL);
    for(int i = 0; i < n; i++){
        pthread_barrier_init(&ACK[i], NULL, 2);
        pthread_create(&user_th[i], NULL, User, (void*)(intptr_t)i);
        users.push_back(i);
    }

    pthread_barrier_wait(&BOS);

    list<Request> Q;
    while(true){
        pthread_barrier_wait(&REQB);
        Request req = GlobalRequest;
        pthread_barrier_wait(&ACK[req.id]);

        if(req.type == Request::QUIT){
            for(int i=0; i<m; i++){
                AVAILABLE[i] += ALLOC[req.id][i];
                ALLOC[req.id][i] = 0;
#ifdef _DLAVOID
                NEED[req.id][i] = 0;
#endif
            }

            users.erase(find(users.begin(), users.end(), req.id));
            logger("Master thread releases resources of thread " + to_string(req.id));
#ifdef _DLAVOID
            DEAD[req.id] = true;
#endif
            print_waiting_threads(Q);
            print_left_threads(users);
            print_available(m);
            if(users.size() == 0) break;
        }
        else if(req.type == Request::RELEASE){
            AVAILABLE -= req.R;
            ALLOC[req.id] += req.R;
#ifdef _DLAVOID
            NEED[req.id] -= req.R;
#endif
        }
        else{
            for(int i=0; i<m; i++){
                if(req.R[i] < 0){
                    int r = -req.R[i];
                    AVAILABLE[i] += r;
                    ALLOC[req.id][i] -= r;
#ifdef _DLAVOID
                    NEED[req.id][i] += r;
#endif
                    req.R[i] = 0;
                }
            }
            Q.push_back(req);
            logger("Master thread stores resource request of thread " + to_string(req.id));
            print_waiting_threads(Q);
        }

        logger("Master thread tries to grant pending requests");
        for(auto it = Q.begin(); it != Q.end();){
            if(it->R <= AVAILABLE){
#ifdef _DLAVOID
                vector<int> TMP_AVAILABLE = AVAILABLE;
                vector<vector<int>> TMP_ALLOC = ALLOC;
                vector<vector<int>> TMP_NEED = NEED;
                TMP_AVAILABLE -= it->R;
                TMP_ALLOC[it->id] += it->R;
                TMP_NEED[it->id] -= it->R;
                if(!isSafe(TMP_AVAILABLE, TMP_ALLOC, TMP_NEED)){
                    logger("    +++ Unsafe to grant request of thread " + to_string(it->id));
                    ++it;
                    continue;
                }
#endif
                AVAILABLE -= (it->R);
                ALLOC[it->id] += (it->R);
#ifdef _DLAVOID
                NEED[it->id] -= (it->R);
#endif
                SEM[it->id].signal();
                logger("Master thread grants resource request for thread " + to_string(it->id));
                it = Q.erase(it);
            }
            else {
                logger("    +++ Insufficient resources to grant request of thread " + to_string(it->id));
                ++it;
            }
        }
        print_waiting_threads(Q);
    }

    for(int i=0; i<n;i++){
        pthread_join(user_th[i], NULL);
    }

    pthread_barrier_destroy(&BOS);
    pthread_barrier_destroy(&REQB);
    pthread_mutex_destroy(&rmtx);
    pthread_mutex_destroy(&pmtx);
    for(int i=0; i<n; i++){
        pthread_barrier_destroy(&ACK[i]);
        pthread_mutex_destroy(&SEM[i].cmtx);
        pthread_cond_destroy(&SEM[i].cv);
    }

    return NULL;
}

int main(){
    pthread_t master;
    pthread_create(&master, NULL, Master, NULL);
    pthread_join(master, NULL);
}