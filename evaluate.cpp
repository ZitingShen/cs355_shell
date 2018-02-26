#include "evaluate.h"
#define shell_terminal STDIN_FILENO
#define ZERO 0
#define ONE 1

extern struct joblist_t joblist;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;

using namespace std;


void evaluate (string *command, vector<vector<string> > *parsed_segments, bool *cont){
	set<string> built_in_command = {"fg", "bg", "kill", "jobs"};
	int len = parsed_segments -> size();
	enum job_status bg_fg = FG; //default as FG
	vector<string> last_seg = parsed_segments -> back();
	if (len == ONE){//no pipe
		if (built_in_command.count(last_seg.front()) == ONE){ //if first argument is buildin comment
			//call builtin
			//need to do type checking of buildin
		}
		else{//not builtin){}
			if (last_seg.back().compare("&") == ZERO){ //check whether background or foreground
				bg_fg = BG;
			}
			no_pipe_exec(command, last_seg, bg_fg);
		}
	}
	else{//pipe exist
		string inter_result;
	}
	return;
}


void no_pipe_exec (string *command, vector<string> argv, enum job_status bg_fg){
	pid_t pid;
	sigset_t signalSet;  
  	sigemptyset(&signalSet);
  	sigaddset(&signalSet, SIGCHLD);

  	/*Store arguemtns in c strings.*/
  	char** argvc = new char*[argv.size()+1]; 
  	for(unsigned int i = 0; i < argv.size(); i++) {
    	char* temp = new char[argv[i].size()+1];
    	strcpy(temp, argv[i].c_str());
    	argvc[i] = temp;
  	}
  	argvc[argv.size()] = NULL;

  	/* Block SIGCHLD signal while fork(), setpgid and add joblist */  
  	sigprocmask(SIG_BLOCK, &signalSet, NULL);

  	/*fork*/
  	if ((pid = fork()) < ZERO ){
		cerr << "Failed to fork child process at process " << getpid() << endl;
	}

	if (pid == ZERO){ //in child process
		/*setpgid*/
		pid_t chld_pid = getpid();
		if ((setpgid(chld_pid, chld_pid)) < ZERO){
			cerr << "Failed to set a new group for child process " << chld_pid << "!" << endl;
		}
		/*update joblist*/
		joblist.add(chld_pid, bg_fg, command);
		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
		if (execvp(argvc[ZERO], argvc) < ZERO){
			cerr << "Child process of " << getppid() << "failed to execute or the execution is interrupted!" << endl;
		}
	}
	else{ //parent process
		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
		int status;
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
	for(unsigned int i = 0; i < argv.size(); i++) {
    delete[] argvc[i];
  	}
  	delete[] argvc;
}

/*
(1) requires % before job number
(2) can take any number of jids
(3) if invalid, will still check the next jid.
(4) -9 flag can only be at the second argument, otherwise -9 will be recognized as a job number
*/
void kill(vector<string> argv){

	pid_t pid =0;
	pid_t g_pid; 
	int signo = SIGTERM;

	if (argv[1].compare("-9") == 0){
		sig = SIGKILL;
	}

	

	if (kill (- g_pid, signo) < 0){
		cerr << "Job " << joblist.pid2jid(pid) << "failed to be killed!" << endl;
	}
}


/*
(1)resume only when job is ST, otherwise ignore
(2)can take a list of jids, w/ or w/o %
(3)the -9 flag can be any where 
*/
void bg(vector<string> argv){
	//loop through every job in the list
	string s_cur_jid;
	pid_t cur_pid;

	for (vector<string>::iterator t = next(argv.begin()); t != argv.end(); ++t){

		/*check whether there is %*/
		if (t -> c_str()[0] == '%') {
      		s_cur_jid = t ->c_str();
      		s_cur_jid = cur_jid.substr(1, string::npos);
      	} 
      	else {
      		s_cur_jid = t -> c_str();
      	}

		try {
			//check whether pid is valid?
        	cur_pid = joblist.jid2pid(stoi(s_cur_jid));
      	} catch (exception &e){
    		cerr << "No job with job number " << s_cur_jid << endl;
    		continue; //continue to the next iteration
    	}

    	/*only send sigcont when job is ST*/
		if (joblist.find_pid(cur_pid) -> status == ST){
			cur_pid = getpgid(cur_pid); //just to double check
			if (kill (- cur_pid, SIGCONT) < ZERO){ //let job continue
			cerr << "Job " << s_cur_jid << "failed to continue in background!" << endl;
			continue;
			}
			joblist.find_pid(cur_pid) -> status = BG;
		}
		else if (joblist.find_pid(cur_pid) -> status != BG){
			cerr << "Job " << s_cur_jid << "cannot continue in background, you can only bg a ST or BG job!" << endl;
		}
	}
}

/*
(1) take actions only when given a job with ST or BG status.
(2) argument can be any size, but only argv[1] will be take into fg, other will be ignored.
(3) job number can be without % */
void fg(vector<string> argv){
	pid_t pid;
	pid_t g_pid;
	/*check argv size*/
	if (argv.size() < 2){
		cerr << "fg command requires job id!" << endl;
		return;
	}

	/*convert string to int and then jid to pid*/
	try {
      if (argv[1][0] == '%') {
      	//check whether pid is valid?
        pid = joblist.jid2pid(stoi(argv[1].substr(1, string::npos)));
      } else {
        pid = joblist.jid2pid(stoi(argv[1]));
      }
    } catch (exception &e){
    	cerr << "Invalid job id" << argv[1] << endl;
      	return;
    }

	g_pid = getpgid(pid); //get group id, just to double check.

	tcsetpgrp (shell_terminal, pid); //bring job to fg

	if (joblist.find_pid(pid) -> status == ST || joblist.find_pid(pid) -> status == BG){
		tcgetattr (shell_terminal, &shell_tmodes); //store shell termio
		if (joblist.find_pid(pid) -> status == ST){ //reset termio if job stopped
			tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter); 
		}
		if (kill (- g_pid, SIGCONT) < ZERO){ //let job continue
			cerr << "Job " << joblist.pid2jid(pid) << "failed to continue when trying to be in foreground!" << endl;
		}
	}
	else{
		cerr << "Job " << argv[1] << "can not be brought to foreground since it is neither ST nor BG!" << endl;
	}
}


void jobs(){
	return;
}
