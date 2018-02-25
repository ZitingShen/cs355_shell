#include<signal.h>
#include<sys/wait.h>

/* SIGCHLD_handler 
	Update job status with si_status.
	*/
void SIGCHLD_handler(int sig, siginfo_t *sip, void *notused); 

/* SIGINT_handler 
	rl_forced_update_display(); to print new prompt
	*/
void SIGINT_handler(int sig, siginfo_t *sip, void *notused);



void SIGCHLD_handler(int sig, siginfo_t *sip, void *notused){
	int exit_status = sip -> si_status;
	pid_t pid = sip -> si_pid;
	if (WIFEXITED(exit_status)){
		if (joblist.find_pid(pid) -> status == BG){
			joblist.find_pid(pid) -> status == DN;
		}
		else{
			//joblit.find_pid(pid) -> status == ;???
		}
	}
	else if (WIFSIGNALED(exit_status)){
		joblist.find_pid(pid) -> status == TN;
	}
	else if (WIFSTOPPED(exit_status)){
		joblist.find_pid(pid) -> status == ST;
		cout << "[" << joblist.pid2jid(pid) << "]" << "] (" << pid << ")\tStopped\t\tSignal " << WSTOPSIG(exit_status
			) << endl;
	}
}

void SIGINT_handler(int sig, siginfo *sip, void *notused){
	cout << "\n" << endl;
	//print prompt?
	rl_forced_update_display();
	//register again??
}

