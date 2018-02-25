#include "evaluate.h"
#define shell_terminal STDIN_FILENO

extern struct joblist_t joblist;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;


void evaluate (string *command, vector<vector<string> > *parsed_segments, bool *cont){
	return;
}

//resume only when job is ST, otherwise ignore
void bg(vector<int> *pid_list){
	//loop through every job in the list
	pid_t cur_pid;
	pid_t cur_g_pid;
	for (vector<int>::iterator t = pid_list->begin(); t != pid_list->end(); ++t){
		cur_pid = *t;
		if (joblist.find_pid(cur_pid) -> status == ST){
			cur_g_pid = getpgid(cur_pid);
			if (kill (- cur_g_pid, SIGCONT) < 0){ //let job continue
			cerr << "Job " << joblist.pid2jid(cur_pid) << "failed to continue when in background!" << endl;
			}
		}
	}
}

void fg(pid_t pid){
	//check whether pid is valid?
	tcsetpgrp (shell_terminal, pid); //bring job to fg
	pid_t g_pid = getpgid(pid); //get group id
	if (joblist.find_pid(pid) -> status == ST){
		tcgetattr (shell_terminal, &shell_tmodes); //store shell termio
		tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter); //reset termio if job stopped
		if (kill (- g_pid, SIGCONT) < 0){ //let job continue
			cerr << "Job " << joblist.pid2jid(pid) << "failed to continue when trying to be in foreground!" << endl;
		}
	}
}

void kill(pid_t pid, bool flag_set){
	//check whether pid is valid?
	pid_t g_pid; 
	int sig;
	if (flag_set){ //check whether flag is set to determine which signal to send
		sig = SIGTERM;
	}
	else{
		sig = SIGKILL;
	}
	if (joblist.find_pid(pid) -> status == FG){ //if kill fg need to bring shell to fg
		g_pid = getpgid(pid);
		tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter);
		if (kill (- g_pid, sig) < 0){
			cerr << "Job " << joblist.pid2jid(pid) << "failed to be killed!" << endl;
		}
		tcsetpgrp (shell_terminal, shell_pgid); //bring shell to fg
		tcgetattr (shell_terminal, & joblist.find_pid(pid) -> ter); //store child termio
		tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes); //restore shell termio
	}
}

void jobs(){
	return;
}
