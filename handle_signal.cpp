#include "handle_signal.h"
#include "joblist.h" 

#define shell_terminal STDIN_FILENO

using namespace std;

extern struct joblist_t joblist;
extern struct termios shell_tmodes;

void sigchld_handler(int sig, siginfo_t *sip, void *notused){
	//int exit_status = sip->si_status;
	int exit_code = sip -> si_code;
	pid_t pid = sip->si_pid;
	if (exit_code == CLD_EXITED){
		if (joblist.find_pid(pid) -> status == BG){
			//cout << "DNBG" << endl;
			joblist.find_pid(pid)->status = DNBG;
		}
		else{
			//cout << "DNFG" << endl;
			joblist.find_pid(pid) -> status = DNFG;
		}
	}
	else if (exit_code == CLD_KILLED){
		//cout << "TN" << endl;
		joblist.find_pid(pid)->status = TN;
	}
	else if (exit_code == CLD_STOPPED){
		//cout << "ST" << endl;
		joblist.find_pid(pid)->status = ST;
		//cout << "[" << joblist.pid2jid(pid) << "]" << "] (" << pid << ")\tStopped\t\tSignal " << WSTOPSIG(exit_status
			//) << endl;
		cout << "[" << joblist.pid2jid(pid) << "] (" << pid << ")\tStopped\t\tSignal " << WSTOPSIG(exit_code) << endl;
	}
	//cout << pid << "\t" << exit_code << endl;
}

void sigint_handler(int sig){
	cout << endl;
	rl_forced_update_display();
}
