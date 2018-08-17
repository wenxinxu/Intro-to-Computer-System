#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

#include <signal.h>
#include "subprocess.h"

using namespace std;

struct worker {
  worker() {}
  worker(char *argv[]) : sp(subprocess(argv, true, false)), available(false) {}
  subprocess_t sp;
  bool available;
};

static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN);
static vector<worker> workers(kNumCPUs);
static size_t numWorkersAvailable = 0;

static void markWorkersAsAvailable(int sig) {
  while (true) {
    pid_t pid = waitpid(-1, NULL, WNOHANG|WUNTRACED);//check if there is any zombie process that has not been reaped
    if (pid <= 0) break;
    for(size_t i = 0; i < kNumCPUs; i++) {
      if(workers[i].sp.pid==pid) {
        workers[i].available = true;
        numWorkersAvailable++;
        break;
      }
    }
  }
}

static  const char *kWorkerArguments[] = {"./factor.py", "--self-halting", NULL};
static void spawnAllWorkers() {
  cout << "There are this many CPUs: " << kNumCPUs << ", numbered 0 through " << kNumCPUs - 1 << "." << endl;
  for (size_t i = 0; i < kNumCPUs; i++) {
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(i, &my_set);
    sigset_t mask,oldmask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    // What if when constructing worker if receives SIGCHILD before assign to worker[i]??NOT GOOD
    sigprocmask (SIG_BLOCK, &mask, &oldmask);
    workers[i] = worker((char** )kWorkerArguments);

    sigprocmask (SIG_UNBLOCK, &mask, NULL);
    sched_setaffinity(workers[i].sp.pid, sizeof(cpu_set_t), &my_set);
    cout << "Worker " << workers[i].sp.pid << " is set to run on CPU " << i << "." << endl;
  }
}

static  size_t getAvailableWorker() {
  // Set up the mask of signals to temporarily block.
  sigset_t mask,oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  // Wait for a signal to arrive. 
  sigprocmask (SIG_BLOCK, &mask, &oldmask);
  while(!numWorkersAvailable) {
    sigsuspend(&oldmask);
  }
  //at least a worker available
  size_t num;
  for(size_t i = 0; i < kNumCPUs; i++) {
    if(workers[i].available) {
      num = i;
      break;
    }
  }
  sigprocmask (SIG_UNBLOCK, &mask, NULL);
  return num;
}

static void broadcastNumbersToWorkers() {
  while (true) {
    string line;
    getline(cin, line);         
    if (cin.fail()) break;
    size_t endpos;
    long long num = stoll(line, &endpos);
    if (endpos != line.size()) break;
    size_t i = getAvailableWorker();
    //tell worker i's process that we got an input
    dprintf(workers[i].sp.supplyfd,"%lld\n",num);
    //update worker's info
    workers[i].available = false;
    sigset_t mask,oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    // Prevent SIGCHILD signals increasing numWorkersAvailable
    sigprocmask (SIG_BLOCK, &mask, &oldmask);
    numWorkersAvailable--;
    sigprocmask (SIG_UNBLOCK, &mask, NULL);
    //Tell the workers process stop hanging
    kill(workers[i].sp.pid, SIGCONT); 
    
  }
}

static void waitForAllWorkers() {

  sigset_t mask,oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  // Wait for a signal to arrive. 
  sigprocmask (SIG_BLOCK, &mask, &oldmask);
  while(numWorkersAvailable < kNumCPUs) {
      sigsuspend(&oldmask);
  }
  //all worker collected
  sigprocmask (SIG_UNBLOCK, &mask, NULL);

}

static void closeAllWorkers() {
  signal(SIGCHLD, SIG_DFL);//send eof to every child
  for(size_t w = 0; w < kNumCPUs; w++) {
      // to be safe, close pipe's fd
      close(workers[w].sp.supplyfd);
      // in case some worker sleeping, tell them to continue working
      kill(workers[w].sp.pid, SIGCONT);
  }
  while(true) {
      //wait all process to finish
      pid_t pid = waitpid(-1,NULL,0);
      if(pid == -1) break;
  }
}
int main(int argc, char *argv[]) {
  signal(SIGCHLD, markWorkersAsAvailable);
  spawnAllWorkers();
  broadcastNumbersToWorkers();
  waitForAllWorkers();
  closeAllWorkers();
  return 0;
}

