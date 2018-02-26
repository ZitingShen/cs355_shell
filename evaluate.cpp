#include "evaluate.h"
#define shell_terminal STDIN_FILENO
#define ZERO 0
#define ONE 1

extern struct joblist_t joblist;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;

using namespace std;




void evaluate (string *command, vector<vector<string> > *parsed_segments, bool *cont){
	set<string> built_in_command = {"fg", "bg", "kill", "jobs"}
	int len = parsed_segments -> size();
	//bool bg = FALSE; //whether background
	enum job_status bg_fg = FG; //default as FG
	pid_t pid;
	pid_t chld_pid
	int status;
	vector<string> last_seg = parsed_segments -> end();
	if (len == 1){//no pipe
		if (built_in_command.count(last_seg.begin()) == ONE){ //if first argument is buildin comment
			//call builtin
			//need to do type checking of buildin
		}
		else{//not builtin){}
			if (last_seg.end() == "&"){ //check whether background or foreground
				bg_fg = FG;
			}
			no_pipe_exec(*command, last_seg, bg_fg)
		}
	}
	else{//pipe exist
		string inter_result;
	}
	return;
}


void no_pipe_exec (string *command, vector<string> command_segment, enum job_status bg_fg){
	pid_t pid;
	sigset_t signalSet;  
  	sigemptyset(&signalSet);
  	sigaddset(&signalSet, SIGCHLD);

  	/* Block SIGCHLD signal while fork(), setpgid and add joblist */  
  	sigprocmask(SIG_BLOCK, &signalSet, NULL);

  	/*fork*/
  	if ((pid = fork()) < ZERO ){
		cerr << "Failed to fork child process at process " << getpid() << endl;
	}

	if (pid == ZERO){ //in child process
		/*setpgid*/
		if ((setpgid(chld_pid, chld_pid)) < ZERO){
			ceer << "Failed to set a new group for child process " << pid << "!" << endl;
		}
		/*update joblist*/
		joblist.add(chld_pid, bg_fg, command);
		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
		if (execvp(last_seg.begin(), last_seg) < ZERO){
			cerr << "Child process of " << getppid() << "failed to execute in bg or the execution is interrupted!" << endl;
		}
	}
	else{ //parent process
		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
		//do nothing if bg, will clean up in the next loop.
		if (bg_fg == FG){ //waiting for fg child to complete, need to swap termio, also need to store termio
			//of child if child is stopeed
			waitpid(pid, &status, WUNTRACED);
			/*does this order matter? tcsetpgrp() first or tcgetattr() first?*/
			tcsetpgrp (shell_terminal, shell_pgid); //bring shell to fg
			if (WIFSTOPPED(status)){ //store child termio if stopped
				tcgetattr (shell_terminal, &joblist.find_pid(pid) -> ter); 
			}
			tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes); // restore shell termio
		}
	}
}

//resume only when job is ST, otherwise ignore
void bg(vector<int> *jid_list){
	//loop through every job in the list
	int cur_jid;
	pid_t cur_g_pid;
	for (vector<int>::iterator t = jid_list->begin(); t != jid_list->end(); ++t){
		cur_jid = *t;
		if (joblist.find_jid(cur_jid) -> status == ST){
			cur_g_pid = joblist.jid2pid(cur_jid);
			cur_g_pid = getpgid(cur_g_pid); //just to double check
			if (kill (- cur_g_pid, SIGCONT) < 0){ //let job continue
			cerr << "Job " << cur_jid << "failed to continue in background!" << endl;
			}
		}
	}
}

void fg(int jid){
	//check whether pid is valid?
	pid_t pid = joblist.jid2pid(jid)
	tcsetpgrp (shell_terminal, pid); //bring job to fg
	pid_t g_pid = getpgid(pid); //get group id
	if (joblist.find_pid(pid) -> status == ST || joblist.find_pid(pid) -> status == BG){
		tcgetattr (shell_terminal, &shell_tmodes); //store shell termio
		tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter); //reset termio if job stopped
		if (kill (- g_pid, SIGCONT) < 0){ //let job continue
			cerr << "Job " << joblist.pid2jid(pid) << "failed to continue when trying to be in foreground!" << endl;
		}
	}
	else{
		cerr << "Process " << pid << "can not be brought to foreground since it is neither ST nor BG!" << endl;
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
		if (kill (- g_pid, sig) < 0){
			cerr << "Job " << joblist.pid2jid(pid) << "failed to be killed!" << endl;
		}
		tcsetpgrp (shell_terminal, shell_pgid); //bring shell to fg
		tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes); //restore shell termio
	}
}

void jobs(){
	return;
}
