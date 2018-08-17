/**
 * File: stsh.cc
 * -------------
 * Defines the entry point of the stsh executable.
 */

#include "stsh-parser/stsh-parse.h"
#include "stsh-parser/stsh-readline.h"
#include "stsh-parser/stsh-parse-exception.h"
#include "stsh-signal.h"
#include "stsh-job-list.h"
#include "stsh-job.h"
#include "stsh-process.h"
#include <cstring>
#include <iostream>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>  // for fork
#include <signal.h>  // for kill
#include <sys/wait.h>
#include <assert.h>
#include <fstream>
#include <string>
using namespace std;
static void userFg(const pipeline& pipeline);
static void userBg(const pipeline& pipeline);
static void userSlay(const pipeline& pipeline);
static void userHalt(const pipeline& pipeline);
static void userCont(const pipeline& pipeline);


static void childHandler(int sig);
static void intHandler(int sig);
static void tstpHandler(int sig);

static STSHJobList joblist; // the one piece of global data we need so signal handlers can access it

/**
 * Function: handleBuiltin
 * -----------------------
 * Examines the leading command of the provided pipeline to see if
 * it's a shell builtin, and if so, handles and executes it.  handleBuiltin
 * returns true if the command is a builtin, and false otherwise.
 */
static const string kSupportedBuiltins[] = {"quit", "exit", "fg", "bg", "slay", "halt", "cont", "jobs"};
static const size_t kNumSupportedBuiltins = sizeof(kSupportedBuiltins)/sizeof(kSupportedBuiltins[0]);
static bool handleBuiltin(const pipeline& pipeline) {
    const string& command = pipeline.commands[0].command;
    auto iter = find(kSupportedBuiltins, kSupportedBuiltins + kNumSupportedBuiltins, command);
    if (iter == kSupportedBuiltins + kNumSupportedBuiltins) return false;
    size_t index = iter - kSupportedBuiltins;

    switch (index) {
        case 0:
        case 1: exit(0);
        case 2: userFg(pipeline);
            break;
        case 3: userBg(pipeline);
            break;
        case 4: userSlay(pipeline);
            break;
        case 5: userHalt(pipeline);
            break;
        case 6: userCont(pipeline);
            break;
        case 7: cout << joblist;
            break;
        default: throw STSHException("Internal Error: Builtin command not supported."); // or not implemented yet
    }

    return true;
}

/**
 * Function: installSignalHandlers
 * -------------------------------
 * Installs user-defined signals handlers for four signals
 * (once you've implemented signal handlers for SIGCHLD,
 * SIGINT, and SIGTSTP, you'll add more installSignalHandler calls) and
 * ignores two others.
 */

static void installSignalHandlers() {
    installSignalHandler(SIGQUIT, [](int sig) { exit(0); });
    installSignalHandler(SIGTTIN, SIG_IGN);
    installSignalHandler(SIGTTOU, SIG_IGN);
    installSignalHandler(SIGCHLD, childHandler);
    installSignalHandler(SIGINT, intHandler);
    installSignalHandler(SIGTSTP, tstpHandler);
}

/**
 *  User defined SIGCHILD handler
 */
static void childHandler(int sig) {
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    while(true) {
        int status;
        pid_t pid = waitpid(-1,&status,WCONTINUED|WNOHANG|WUNTRACED);
        if(pid <= 0) break;
        if (!joblist.containsProcess(pid)) return;
        STSHProcessState st = kWaiting;
        //Update Joblist and process according to exit status
        if(WIFEXITED(status) || WIFSIGNALED(status)) {
            st = kTerminated;
        }
        if(WIFSTOPPED(status)) {
            st = kStopped;
        }
        if(WIFCONTINUED(status)) {
            st = kRunning;
        }
        STSHJob& job = joblist.getJobWithProcess(pid);
        assert(job.containsProcess(pid));
        STSHProcess& process = job.getProcess(pid);
        process.setState(st);
        joblist.synchronize(job);
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

}
/**
 * User defined SIGINT handler
 */
static void intHandler(int sig) {
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    if(!joblist.hasForegroundJob()) return;
    else {
        STSHJob &job = joblist.getForegroundJob();
        kill(-job.getGroupID(), SIGINT);
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/**
 * User defined SIGTSTP handler
 *
 */
static void tstpHandler(int sig) {
    sigset_t mask,oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    if(!joblist.hasForegroundJob()) return;
    else {
        STSHJob &job = joblist.getForegroundJob();
        kill(-job.getGroupID(), SIGTSTP);

    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}
/**
 * Function: transferControl
 * Detect if there is any foreground process
 * YES: transfer terminal control to this process
 * NO: Get control back to terminal
 * @para pgid is the parent process group id
 */
static void transferControl(pid_t pgid) {

    //Transferring foreground job if there is any
    if(joblist.hasForegroundJob()) {
        //tcsetpgrp to associate pgid with terminal standard in
        STSHJob forejob = joblist.getForegroundJob();
        if (tcsetpgrp(STDIN_FILENO, forejob.getGroupID()) < 0) {
            if (errno != ENOTTY) {
                throw STSHParseException("You are not authorized to transfer console control, or worse. ");
            }

        }
    }

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (joblist.hasForegroundJob()) {

        sigsuspend(&oldmask);
    }

    //All the foreground job went to background

    //parent process gets the terminal control back...
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
        if (errno != ENOTTY) {
            throw STSHParseException("You are not authorized to transfer console control, or worse. ");
        }
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);




}
/**
 * Function: createJob
 * -------------------
 * Creates a new job on behalf of the provided pipeline.
 */
static void createJob(const pipeline& pipeline) {

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    STSHJob& job = joblist.addJob(kForeground);
    size_t jobNum = job.getNum();

    pid_t pgid = 0;
    if(pipeline.background) {
        job.setState(kBackground);
    }
    //every command needs 2 fd as input,except for first and last
    int fds[pipeline.commands.size() - 1][2];
    for(size_t i = 0; i < pipeline.commands.size() - 1; i++) {
        pipe(fds[i]);//pipe every pair of fd
    }
    pid_t pid;
//  '>' and '<' can be recognize by pipeline and pop into input/output
    for (size_t i = 0; i < pipeline.commands.size(); i++) {
         pid = fork();
        if (pid == 0) {
            //child process
            if (i == 0) {
                pgid = getpid();
                if (!pipeline.input.empty()) {
                    int infd;
                    infd = open(pipeline.input.c_str(), O_RDONLY);
                    dup2(infd, STDIN_FILENO);
                    close(infd);
                }
                if (!pipeline.output.empty()) {
                    int outfd;
                    outfd = open(pipeline.output.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(outfd, STDOUT_FILENO);
                    close(outfd);

                }
                close(fds[0][0]);
                dup2(fds[0][1], STDOUT_FILENO);
                close(fds[0][1]);

                for (size_t j = 1; j < pipeline.commands.size() - 1; j++) {
                    close(fds[j][0]);
                    close(fds[j][1]);
                }

            }
            else if (i == pipeline.commands.size() - 1) {
                //last command
                setpgid(getpid(),pgid);
                if (!pipeline.output.empty()) {
                    int outfd;
                    outfd = open(pipeline.output.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(outfd, STDOUT_FILENO);
                    close(outfd);

                }
                close(fds[i - 1][1]);
                dup2(fds[i - 1][0], STDIN_FILENO);
                close(fds[i - 1][0]);
                for (size_t j = 0; j < pipeline.commands.size() - 1; j++) {
                    close(fds[j][0]);
                    close(fds[j][1]);
                }


            } else {
                //command between
                //fd between
                setpgid(getpid(),pgid);
                close(fds[i - 1][1]);
                dup2(fds[i - 1][0], STDIN_FILENO);
                close(fds[i - 1][0]);
                close(fds[i][0]);
                dup2(fds[i][1], STDOUT_FILENO);
                close(fds[i][1]);

                for (size_t j = 0; j < pipeline.commands.size() - 1; j++) {
                    if (j != i - 1 && j != i) {
                        close(fds[j][0]);
                        close(fds[j][1]);
                    }
                }
            }

            char *argv[kMaxArguments + 1] = {NULL};
            argv[0] = const_cast<char*> (pipeline.commands[i].command);
            for (size_t k = 0; k <= kMaxArguments && pipeline.commands[i].tokens[k] != NULL; k++) {
                argv[k + 1] = pipeline.commands[i].tokens[k];
            }
            execvp(argv[0], argv);
            string e = argv[0];
            throw STSHException(e + ": Command not found.");
            exit(0);
        } else {
            //parent process
            if(i == 0) {
                pgid = pid;
            }
            job.addProcess(STSHProcess(pid, pipeline.commands[i]));
            setpgid(pid,pgid);
        }
    }
    //parent process
    //close fds
    for (unsigned int i = 0; i < pipeline.commands.size() - 1; i++) {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    assert(joblist.containsJob(jobNum));
    vector<STSHProcess>& process = job.getProcesses();
    if(pipeline.background) {
        string s = "[" + to_string(jobNum) + "]";
        cout  << s << " ";
        for(auto p: process ) {
            cout << p.getID() <<" ";
        }
        cout<<endl;
    }
    sigprocmask(SIG_UNBLOCK, &mask, &oldmask);

    
    transferControl(getpgid(getpid()));


}

/**
 * Function:fg
 * prompts a stopped job to continue in the foreground ,
 * or brings a running background job into the foreground.
 * fg takes a single job number
 * @para: pipeline
 */

static void userFg(const pipeline& pipeline) {

    //misuse detection
    if(pipeline.commands[0].tokens[0] == NULL) {
        throw STSHParseException("Usage: fg <jobid>.");
    }
    string str = pipeline.commands[0].tokens[0];
    // argument detection
    int num = 0;
    try {
        num = stoi(str);
    }
    catch (exception& e) {
        throw STSHParseException("Usage: fg <jobid>.");
    }
    //job existence detection
    if(!joblist.containsJob(num)) {
        throw STSHException("fg "+to_string(num)+": No such job.");
    }
    //set job to foreground
    STSHJob& job = joblist.getJob(num);
    vector<STSHProcess> process = job.getProcesses();
    if(joblist.getJob(num).getState() == kBackground) {

        joblist.getJob(num).setState(kForeground);
    }
    for(auto p : process ) {
        kill(p.getID(),SIGCONT);

    }
    transferControl(getpgid(getpid()));

}

/**
 * Function: bg
 * which prompts a stopped job to continue in the background.
 * bg takes a single job number
 */
static void userBg(const pipeline& pipeline) {

    //misuse detection
    if(pipeline.commands[0].tokens[0] == NULL) {
        throw STSHParseException("Usage: bg <jobid>.");
    }
    string str = pipeline.commands[0].tokens[0];
    // argument detection
    size_t num = 0;
    try {
        num = stoi(str);
    }
    catch (exception& e) {
        throw STSHParseException("Usage: bg <jobid>.");
    }
    //job existence detection
    if(!joblist.containsJob(num)) {

        throw STSHException("bg "+to_string(num)+": No such job.");
    }

    //set job to background
    STSHJob& job = joblist.getJob(num);
    vector<STSHProcess>& process = job.getProcesses();

    if(joblist.getJob(num).getState() == kForeground) {
        joblist.getJob(num).setState(kBackground);
    }

    for(auto p : process ) {
        kill(p.getID(),SIGCONT);
    }


}
/**
 * Function: userSlay
 * which is used to terminate a single process
 * which may have many sibling processes as part of a larger pipeline
 *
 */
static void userSlay(const pipeline& pipeline) {
    //misuse detection
    if(pipeline.commands[0].tokens[0] == NULL) {
        throw STSHParseException("No enough argument! ");
    }
    string first = pipeline.commands[0].tokens[0];
    char * second = pipeline.commands[0].tokens[1];

    // argument detection
    if(second==NULL){
        //no second argument
        pid_t pid = 0;
        try {
            pid = stoi(first);
        }
        catch (exception& e) {
            throw STSHException(" Invalid argument! ");
        }
        if(!joblist.containsProcess(pid)) {
            throw STSHException("No such job.");
        }
        kill(pid,SIGKILL);
    }
    else {
        size_t jobNum = 0;
        try {
            jobNum= stoi(first);
        }
        catch(exception& e){
            throw STSHException(" Invalid argument! ");
        }
        //job existence detection
        if(!joblist.containsJob(jobNum))  throw STSHException(" There is no such job existing!");
        size_t processNum = stoi(second);
        STSHJob& job = joblist.getJob(jobNum);
        vector<STSHProcess> process = job.getProcesses();
        if(process.size() <= processNum ) {
            throw STSHException("No such job.");
        }
        //slay job
        kill(process[processNum - 1].getID(), SIGKILL);
    }

}
/**
 * Function:userHalt
 * should be halted (but not terminated)
 * if it isn’t already stopped.
 * If it’s already stopped, then don’t do anything and just return.
 */
static void userHalt(const pipeline& pipeline){

    if(pipeline.commands[0].tokens[0] == NULL) {
        throw STSHParseException("Usage: halt <jobid>.");
    }
    string first = pipeline.commands[0].tokens[0];
    char * second = pipeline.commands[0].tokens[1];
    if(second==NULL){
        //no second argument
        pid_t pid = 0;
        try {
            pid = stoi(first);
        }
        catch (exception& e) {
            throw STSHException(" Invalid argument! ");
        }

        if(!joblist.containsProcess(pid)) {
            throw STSHException("No such job.");
        }
        else {
            STSHJob& job = joblist.getJobWithProcess(pid);
            STSHProcess process = job.getProcess(pid);
            if(process.getState() == kStopped) return;
            else {
                kill(pid,SIGTSTP);
            }
        }

    }
    else {
        size_t jobNum = 0;
        try {
            jobNum= stoi(second);
        }
        catch(exception& e){
            throw STSHException(" Invalid argument! ");
        }
        if(!joblist.containsJob(jobNum))  throw STSHException("No such job.");
        size_t processNum = stoi(second);
        STSHJob& job = joblist.getJob(jobNum);
        vector<STSHProcess> process = job.getProcesses();
        if(process.size() <= processNum ) {
            throw STSHException("No such job.");

        }

        if (process[processNum - 1].getState() == kStopped) return;
        else {
            kill(process[processNum - 1].getID(), SIGTSTP);
        }
    }


}
/**
 * Fucntion: User-defined CONT, samilar to slay and halt
 * xcept that its one or two numeric arguments identify a single process
 * that should continue if it isn’t already running
 * @param pipeline
 */
static void userCont(const pipeline& pipeline){

    if(pipeline.commands[0].tokens[0] == NULL) {
        throw STSHParseException("Not enough argument! ");
    }
    string first = pipeline.commands[0].tokens[0];
    char * second = pipeline.commands[0].tokens[1];
    if(second==NULL){
        //no second argument
        pid_t pid = 0;
        try {
            pid = stoi(first);
        }
        catch (exception& e) {
            throw STSHException(" Invalid argument! ");
        }

        if(!joblist.containsProcess(pid)) {
            throw STSHException("No such job.");
        }
        else {
            STSHJob& job = joblist.getJobWithProcess(pid);
            STSHProcess process = job.getProcess(pid);
            if(process.getState() != kRunning) {
                kill(pid,SIGCONT);
                job.setState(kBackground);

            }
            else return;
        }

    }
    else {
        size_t jobNum = 0;
        try {
            jobNum= stoi(first);
        }
        catch(exception& e){
            throw STSHException(" Invalid argument! ");
        }
        if(!joblist.containsJob(jobNum))  throw STSHException("No such job.");
        size_t processNum = stoi(pipeline.commands[0].tokens[1]);
        STSHJob& job = joblist.getJob(jobNum);
        vector<STSHProcess> process = job.getProcesses();
        if(process.size() >= processNum ) {
            if (process[processNum - 1].getState() != kStopped) {
                kill(process[processNum - 1].getID(),SIGCONT);
                job.setState(kBackground);
            }
            else return;
        }
        else {

            throw STSHException(" There is no such job existing!");
        }
    }



}

/**
 * Function: main
 * --------------
 * Defines the entry point for a process running stsh.
 * The main function is little more than a read-eval-print
 * loop (i.e. a repl).
 */
int main(int argc, char *argv[]) {
    pid_t stshpid = getpid();
    installSignalHandlers();
    rlinit(argc, argv);
    while (true) {
        string line;
        if (!readline(line)) break;
        if (line.empty()) continue;
        try {
            pipeline p(line);
            bool builtin = handleBuiltin(p);
            if (!builtin) createJob(p);
        } catch (const STSHException& e) {
            cerr << e.what() << endl;
            if (getpid() != stshpid) exit(0); // if exception is thrown from child process, kill it
        }
    }

    return 0;
}

