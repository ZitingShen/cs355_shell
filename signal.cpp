#include "signal.h"
#include "joblist.h" 

using namespace std;

extern struct joblist_t joblist;

void SIGCHLD_handler(int sig, siginfo_t *sip, void *notused){
	int exit_status = sip -> si_status;
	pid_t pid = sip -> si_pid;
	if (WIFEXITED(exit_status)){
		//if (joblist.find_pid(pid) -> status == BG){
		joblist.find_pid(pid) -> status = DN;
		//}
		//else{
			//joblit.find_pid(pid) -> status == ;???
		//}
	}
	else if (WIFSIGNALED(exit_status)){
		joblist.find_pid(pid) -> status = TN;
	}
	else if (WIFSTOPPED(exit_status)){
		joblist.find_pid(pid) -> status = ST;
		cout << "[" << joblist.pid2jid(pid) << "]" << "] (" << pid << ")\tStopped\t\tSignal " << WSTOPSIG(exit_status
			) << endl;
	}
}

void SIGINT_handler(int sig, siginfo_t *sip, void *notused){
	cout << "\n" << endl;
	//print prompt?
	rl_forced_update_display();
	//register again??
}