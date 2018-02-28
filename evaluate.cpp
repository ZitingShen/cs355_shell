#include "evaluate.h"
#define shell_terminal STDIN_FILENO


extern struct joblist_t joblist;
extern struct termios shell_tmodes;
extern pid_t shell_pid;

using namespace std;

bool evaluate (string *command, vector<vector<string> > *parsed_segments){
	set<string> built_in_commands = {"fg", "bg", "kill", "jobs", "history", "exit"};
	
	int len = parsed_segments->size();
	enum job_status bg_fg = FG; //default as FG
	vector<string> last_seg = parsed_segments->back();

	/* No pipe!!*/
	if (len == 1){
		string cmd; //get the command
		cmd = last_seg.front();
		if (built_in_commands.count(cmd) == 1){ //if first argument is buildin comment
			if(cmd.compare("kill") == 0) {
				kill(last_seg);
			} else if(cmd.compare("bg") == 0) {
				bg(last_seg);
			} else if(cmd.compare("fg") == 0) {
				fg(last_seg);
			} else if(cmd.compare("jobs") == 0) {
				jobs();
			}
			else if(cmd.compare("history") == 0) {
				history(last_seg);
			} else if(cmd.compare("exit") == 0) {
				return false;
			}
		}
		else{//not built_in
			if (last_seg.back().compare("&") == 0){ //check whether background or foreground
				bg_fg = BG;
				last_seg.pop_back();
			}
			no_pipe_exec(command, last_seg, bg_fg);
		}
	}
	/* Pipe exists!!*/
	else{ 
		string inter_result;
	}
	return true;
}


void no_pipe_exec (string *command, vector<string> argv, job_status bg_fg){
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
  	if ((pid = fork()) < 0 ){
		cerr << "Failed to fork child process at process " << getpid() << endl;
	}

	if (pid == 0){
		if (setpgid(0, 0) < 0){
			cerr << "Failed to set new group" << endl;
		}

		/*unmask SIGCHLD*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
		
		signal(SIGCHLD, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);

		if (bg_fg == FG){ 
  			tcsetpgrp(shell_terminal, getpid());
  		}

		signal(SIGTTOU, SIG_DFL);

		if (execvp(argvc[0], argvc) < 0){
			// TODO: print different error message depending on errno.
			cerr << "Child process of " << getppid() << " failed to execute or the execution is interrupted!" << endl;
		}

	}
	else{ //parent process
		/*update joblist*/
		joblist.add(pid, bg_fg, *command);

		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);

		if (setpgid(pid, pid) < 0){
			cerr<< "Failed to set new group"<<endl;
		}

		if (bg_fg == FG){ 
  			tcsetpgrp(shell_terminal, pid);
  		} 

		int status;

		//do nothing if bg, will clean up in the next loop.
		if (bg_fg == FG){ //waiting for fg child to complete, need to swap termio, also need to store termio
			//of child if child is stopeed
			waitpid(pid, &status, WUNTRACED);
			tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes); // restore shell termio
			tcsetpgrp(shell_terminal, shell_pid); //bring shell to fg

			if (WIFSTOPPED(status)){ //store child termio if stopped
				if (!joblist.find_pid(pid)){
					cerr << "No such process with process id " << pid << endl;
					//tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
					return;
				}
				if (tcgetattr(shell_terminal, &joblist.find_pid(pid) -> ter) < 0){
					cerr << "termio of stopped job not saved" << endl; 
				}
			}

			
		}
	}

	for(unsigned int i = 0; i < argv.size()+1; i++) {
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

	pid_t cur_pid;
	int signo = SIGTERM;
	unsigned int i = 1;
	if (argv[1].compare("-9") == 0){
		signo = SIGKILL;
		i++;
	}

	for (;i < argv.size(); i++){
		/* check if pid or jid */
		try {
			if (argv[i][0] == '%'){ //then this is jobid
				if (!joblist.find_jid(stoi(argv[i].substr(1)))){
					cerr << "Job " << argv[i].substr(1) << " does not exist!" << endl;
					continue;
				}
				cur_pid = joblist.jid2pid(stoi(argv[i].substr(1)));
			}
        	else{ //then is pid
        		cur_pid = stoi(argv[i]);
        		if (!joblist.find_pid(cur_pid)){
        			cerr << "Job with pid " << argv[i] << " does not exist" << endl;
        			continue;
        		}
        		
        	}
    	} catch (exception &e){
    		cerr << "No process with process number " << cur_pid << endl;
    		continue; //continue to the next iteration
   		}

   		
   		//send signo to pid
   		cur_pid = getpgid(cur_pid);//just to double check pgid
		if (kill(cur_pid, signo) > 0){
			cerr << "Job " << joblist.pid2jid(cur_pid) << "failed to be killed!" << endl;
		}
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

	for (unsigned int i = 1; i < argv.size(); i++){

		/*check whether there is %*/
		if ( argv[i][0] == '%') {
      		s_cur_jid = argv[i].substr(1, string::npos);
      	} 
      	else {
      		s_cur_jid = argv[i];
      	}

      	/*convert jid into pid*/
		try {
			//check whether pid is valid?
			if (!joblist.jid2pid(stoi(s_cur_jid))){
				cerr << "Job" << s_cur_jid << "does not exist!" << endl;
				return;
			}
        	cur_pid = joblist.jid2pid(stoi(s_cur_jid));
      	} catch (exception &e){
    		cerr << "No job with job number " << s_cur_jid << endl;
    		continue; //continue to the next iteration
    	}

    	if (!joblist.find_pid(cur_pid)){
			cerr << "No such job with process id: "<< cur_pid << endl;
			continue;
		}

    	/*only send sigcont when job is ST*/
		if (joblist.find_pid(cur_pid)->status == ST){
			cur_pid = getpgid(cur_pid); //just to double check
			if (kill (-cur_pid, SIGCONT) < 0){ //let job continue
				cerr << "Job " << s_cur_jid << "failed to continue in background!" << endl;
				continue;
			}
			joblist.find_pid(cur_pid)->status = BG;
		}
		else if (joblist.find_pid(cur_pid)->status != BG){
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
	//sigset_t masked_signals;
  	
	/*check argv size*/
	if (argv.size() < 2){
		cerr << "fg command requires job id!" << endl;
		return;
	}

	/*convert string to int and then jid to pid*/
	try {
      if (argv[1][0] == '%') {
      	//check whether pid is valid?
      	if (!joblist.jid2pid(stoi(argv[1].substr(1, string::npos)))){
      		cerr << "Job " << argv[1].substr(1, string::npos) << " does not exist!" << endl;
      		return;
      	}
        pid = joblist.jid2pid(stoi(argv[1].substr(1, string::npos)));
      } else {
      	if (!joblist.jid2pid(stoi(argv[1]))){
      		cerr << "Job " << argv[1] << " does not exist!" << endl;
      		return;
      	}
        pid = joblist.jid2pid(stoi(argv[1]));
      }
    } catch (exception &e){
    	cerr << "Invalid job id" << argv[1] << endl;
      	return;
    }

	pid = getpgid(pid); //get group id, just to double check.	

	
	if (!joblist.find_pid(pid)){
		cerr << "No such job "<< pid << endl;
		return;
	}

	/* if job is ST or BG */
	if (joblist.find_pid(pid) -> status == ST || joblist.find_pid(pid) -> status == BG){
		if (kill (-pid, SIGCONT) < 0){ //let job continue
			cerr << "Job " << joblist.pid2jid(pid) << " failed to continue when trying to be in foreground!" << endl;
			return;
		}

		joblist.find_pid(pid) -> status = BG;

		if (tcsetpgrp (shell_terminal, pid) != 0){ //bring job to fg
			cerr << "Job " << joblist.pid2jid(pid) << " failed to be brought to foreground, will continue in BG!" << endl;
			return;
		}
		//tcgetattr (shell_terminal, &shell_tmodes); //store shell termio
		if (joblist.find_pid(pid) -> status == ST){ //reset termio if job stopped
			if (tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter) != 0){
			cerr << "Job " << joblist.pid2jid(pid) << " failed to restore termio setting, will continue in BG!" << endl;
			tcsetpgrp(shell_terminal, shell_pid); //bring shell to fg
			return;
			}
		}

		joblist.find_pid(pid) -> status = FG;
		int status;

		waitpid(pid, &status, WUNTRACED);

		if (WIFSTOPPED(status)){ //store child termio if stopped
			tcgetattr (shell_terminal, &joblist.find_pid(pid) -> ter); 
		}
		if (tcsetpgrp (shell_terminal, shell_pid) != 0){ //bring shell to foreground
			cerr << "Failed to bring shell to the foreground" << endl;
			return;
		}
		if (tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes) != 0){ // restore shell termio
			cerr << "Failed restore shell termio setting!"<< joblist.pid2jid(pid) << endl;
		} 
	}

	/* if job is not ST or BG, cerr */
	else{
		cerr << "Job " << argv[1] << "can not be brought to foreground since it is neither ST nor BG!" << endl;
	}
}


void jobs(){
	joblist.listjobs();
}

void history(vector<string> argv) {
	int n = history_length;
	if (argv.size() > 2) {
		cout << "history: too many arguments" << endl;
		return;
	}
	if (argv.size() == 2) {
		try {
			n = stoi(argv[1]);
		} catch(exception &e) {
			cout << "Usage: history [n]" << endl;
			return;
		}
		if (n < 0) {
			cout << "Usage: history [n]" << endl;
			return;
		}
		if (n > history_length) n = history_length;
	}
	for (int i = history_base + history_length - n; 
		i < history_base + history_length; i++) {
		cout <<  i << " " << history_get(i)->line << endl;
	}
}
