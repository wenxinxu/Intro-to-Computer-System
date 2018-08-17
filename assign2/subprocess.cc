/**
 * File: subprocess.cc
 * -------------------
 * Presents the implementation of the subprocess routine.
 */
#include <iostream>
#include "subprocess.h"
using namespace std;

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput, const set<int>& openfds) throw (SubprocessException) {

    int supply_fd[2];

    int supply_pip = pipe(supply_fd);
    if(supply_pip == -1) throw SubprocessException("ERR when Pipe supply");

    int ingest_fd[2];
    int ingest_pip = pipe(ingest_fd);
    if(ingest_pip == -1) throw SubprocessException("ERR when Pipe ingest");
    subprocess_t sub = {fork(),kNotInUse,kNotInUse};
    if(sub.pid == 0) {
        if(supplyChildInput) {
            int c = close(supply_fd[1]);// is supplied by parent,no need to write anything, close supply_write
            if(c == -1) throw SubprocessException("ERR when closing supply【1】");
            int d = dup2(supply_fd[0],STDIN_FILENO);
            if(d == -1) throw SubprocessException("supply child input has been stopped");
            close(supply_fd[0]);
        }
        else {
            close(supply_fd[1]);
            close(supply_fd[0]);


        }
        if(ingestChildOutput) {
            int c = close(ingest_fd[0]);// no need to read anything
            if(c == -1) throw SubprocessException("ERR when closing ingest【0】");
            int d = dup2(ingest_fd[1],STDOUT_FILENO);
            if(d == -1) throw SubprocessException("Ingest child output has been stopped");
            close(ingest_fd[1]);
        }
        else {
            close(ingest_fd[1]);
            close(ingest_fd[0]);
        }
        for(auto f :openfds) {
            int c = close(f);
            if(c == -1) throw SubprocessException("ERR when close openfds in child process");

        }
        int e = execvp(argv[0],argv);
        if(e == -1) throw SubprocessException("Encountered a problem when calling execvp");
       
    }
        //if now we are in parent process, receive content from sub
    int c1 = close(supply_fd[0]);
    int c2 = close(ingest_fd[1]);
   if (c1 == -1 || c2 == -1 ) {
       throw SubprocessException("Parent closing invalid fd!");
   }


   if (supplyChildInput) {
       sub.supplyfd = supply_fd[1];
   }
   if (ingestChildOutput) {
       sub.ingestfd = ingest_fd[0] == -1? STDOUT_FILENO: ingest_fd[0];
   }

   return sub;
   
}
