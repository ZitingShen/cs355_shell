#include "handle_signal.h"
#include "joblist.h" 

#define shell_terminal STDIN_FILENO

using namespace std;

extern struct joblist_t joblist;
extern struct termios shell_tmodes;

void sigchld_handler(int sig, siginfo_t *sip, void *notused){
	int exit_status = sip->si_status;
	pid_t pid = sip->si_pid;
	if (WIFEXITED(exit_status)){
		if (joblist.find_pid(pid) -> status == BG){
			joblist.find_pid(pid)->status = DNBG;
		}
		else{
			joblist.find_pid(pid) -> status = DNFG;
		}
	}
	else if (WIFSIGNALED(exit_status)){
		joblist.find_pid(pid)->status = TN;
	}
	else if (WIFSTOPPED(exit_status)){
		joblist.find_pid(pid)->status = ST;
		cout << "[" << joblist.pid2jid(pid) << "]" << "] (" << pid << ")\tStopped\t\tSignal " << WSTOPSIG(exit_status
			) << endl;
	}
	cout << pid << "\t" << exit_status << endl;
}

void sigint_handler(int sig){
	cout << endl;
	rl_forced_update_display();
}
