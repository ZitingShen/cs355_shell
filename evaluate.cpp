
extern struct joblist_t joblist;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern int shell_terminal;

void evaluate (vector<string> *command, vector<vector<string>> *parsed_segments, struct joblist_t *joblist){

}

void bg(vector<int> *pid_list){

}

void fg(pid_t pid){
	//check whether pid is valid?
	tcsetpgrp (shell_terminal, pid);
	pid_t g_pid = getpgrp(pid);
	if (joblist.find_pid(pid) -> status == ST){
		tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter);
		if (kill (- g_pid, SIGCONT) < 0){
			cerr << "Job " << joblist.pid2jid(pid) << "failed to continue when trying to be in foreground!" << endl;
		}
	}
}

void kill(pid_t pid, bool flag_set){
	//check whether pid is valid?
	pid_t g_pid = getpgrp(pid);
	int sig;
	if (flag_set){
		sig = SIGTERM;
	}
	else{
		sig = SIGKILL;
	}
	if (joblist.find_pid(pid) -> status == FG){
		tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid -> ter);
		if (kill (- g_pid, sig) < 0){
			cerr << "Job " << joblist.pid2jid(pid) << "failed to be killed!" << endl;
		}
		tcsetpgrp (shell_terminal, shell_pgid); //bring shell to fg
		tcgetattr (shell_terminal, & joblist.find_pid(pid) -> ter); //store child termio
		tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes); //restore shell termio
	}
}

void jobs(){

}