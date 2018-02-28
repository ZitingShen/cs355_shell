#include "evaluate.h"
#define shell_terminal STDIN_FILENO

extern struct joblist_t joblist;
extern struct termios shell_tmodes;
extern pid_t shell_pid;

using namespace std;

void handle_error(string exec);

bool evaluate (string *command, vector<vector<string> > *parsed_segments){
	set<string> built_in_commands = {"fg", "bg", "kill", "jobs", "history", "exit"};
	
	bool cont = true;
	int len = parsed_segments->size();
	enum job_status bg_fg = FG; //default as FG
	vector<string> last_seg = parsed_segments->back();

	/* No pipe!!*/
	if (len == 1){
		string cmd; //get the command
		cmd = last_seg.front();
		if (built_in_commands.count(cmd) == 1){ //if first argument is buildin comment
			if(cmd.compare("kill") == 0) {
				return kill(last_seg);
			} 
			else if(cmd.compare("bg") == 0) {
				return bg(last_seg);
			} 
			else if(cmd.compare("fg") == 0) {
				return fg(last_seg);
			} 
			else if(cmd.compare("jobs") == 0) {
				return jobs();
			}
			else if(cmd.compare("history") == 0) {
				return history(last_seg);
			} else if(cmd.compare("exit") == 0) {
				return false;
			}
		}
		else{//not built_in
			if (last_seg.back().compare("&") == 0){ //check whether background or foreground
				bg_fg = BG;
				last_seg.pop_back();
			}
			cont = no_pipe_exec(command, last_seg, bg_fg);
		}
	}
	/* Pipe exists!!*/
	else{ 
		string inter_result;
	}
	return cont;
}


bool no_pipe_exec (string *command, vector<string> argv, job_status bg_fg){
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
			handle_error(argv[0]);
			for(unsigned int i = 0; i < argv.size()+1; i++) {
    			delete[] argvc[i];
  			}
  			delete[] argvc;
  			return false;
		}

	}
	else{ //parent process
		/*update joblist*/
		joblist.add(pid, bg_fg, *command, argv[0]);

		if (setpgid(pid, pid) < 0){
			cerr << pid << ": failed to set new group"<<endl;
		}
		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);

		//do nothing if bg, will clean up in the next loop.
		if (bg_fg == FG){ //waiting for fg child to complete, need to swap termio, also need to store termio
			//of child if child is stopeed
			tcsetpgrp(shell_terminal, pid);

			int status;
			waitpid(pid, &status, WUNTRACED);

			tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes); // restore shell termio
			tcsetpgrp(shell_terminal, shell_pid); //bring shell to fg

			if (WIFSTOPPED(status)){ //store child termio if stopped
				if (joblist.find_pid(pid)){
					if (tcgetattr(shell_terminal, &joblist.find_pid(pid)->ter) < 0){
						cerr << "termios of stopped job not saved" << endl; 
					}
				} else {
					cerr << pid << " not found in the joblist" << endl;
				}
				
			}
		}
	}

	for(unsigned int i = 0; i < argv.size()+1; i++) {
    	delete[] argvc[i];
  	}
  	delete[] argvc;
  	return true;
}

/*
(1) requires % before job number
(2) can take any number of jids
(3) if invalid, will still check the next jid.
(4) -9 flag can only be at the second argument, otherwise -9 will be recognized as a job number
*/
bool kill(vector<string> argv){
	pid_t cur_pid;
	int signo = SIGTERM;
	unsigned int i = 1;
	if (argv.size() < 2){
		cerr << "kill: usage: kill [-sigspec] pid | jobspec ..." << endl;
		return true;
	}

	if (argv[1][0] == '-'){
		try{
			signo = stoi(argv[i].substr(1));
			i++;
		} catch (exception &e) {
		}
	}

	for (;i < argv.size(); i++){
		/* check if pid or jid */
		try {
			if (argv[i][0] == '%'){ //then this is jobid
				if (!joblist.find_jid(stoi(argv[i].substr(1)))){
					cerr << "kill: " << argv[i] << " arguments must be process or job IDs" << endl;
					continue;
				}
				cur_pid = joblist.jid2pid(stoi(argv[i].substr(1)));
			}
        	else{ //then is pid
        		cur_pid = stoi(argv[i]);
        		if (!joblist.find_pid(cur_pid)){
        			cerr << "kill: "  << argv[i] << " arguments must be process or job IDs" << endl;
        			continue;
        		}
        		
        	}
    	} catch (exception &e){
    		cerr << "kill: "  << argv[i] << " arguments must be process or job IDs" << endl;
    		continue; //continue to the next iteration
   		}

   		
   		//send signo to pid
   		cur_pid = getpgid(cur_pid);//just to double check pgid
		if (kill(-cur_pid, signo) > 0){
			cerr << ": job " << "kill: "  << argv[i] << "failed to be killed" << endl;
		}
	}
	return true;
}


/*
(1)resume only when job is ST, otherwise ignore
(2)can take a list of jids, w/ or w/o %
(3)the -9 flag can be any where 
*/
bool bg(vector<string> argv){
	//loop through every job in the list
	string s_cur_jid;
	int cur_jid;
	if (argv.size() < 2){
		cerr << "bg: current: no such job" << endl;
		return true;
	}

	for (unsigned int i = 1; i < argv.size(); i++){
		if ( argv[i][0] == '%') {
      		s_cur_jid = argv[i].substr(1);
      	} 
      	else {
      		s_cur_jid = argv[i];
      	}

		try {
			cur_jid = stoi(s_cur_jid);
      	} catch (exception &e){
    		cerr << "bg: current: " << argv[i] << " no such job"<< endl;
    		continue;
    	}

    	job_t *target_job = joblist.find_jid(cur_jid);
    	if (!target_job){
			cerr << "bg: current: "<< argv[i] << " no such job" << endl;
			continue;
		}

    	/*only send sigcont when job is ST*/
		if (target_job->status == ST){
			pid_t cur_pid = target_job->pids[0];
			if (kill(-cur_pid, SIGCONT) < 0){
				cerr << "bg: current: " << argv[i] << " failed to continue in background!" << endl;
				continue;
			}
			target_job->status = BG;
		}
		else if (target_job->status != BG){
			string err_mes = " ";
			if (target_job->status != TN){
				err_mes = "terminated!";
			}
			else if (target_job->status != DNBG || target_job->status != DNFG){
				err_mes = "been done!";
			}
			cerr << "bg: current: " << argv[i] << " job has " << err_mes << endl;
		}
	}
	return true;
}

/*
(1) take actions only when given a job with ST or BG status.
(2) argument can be any size, but only argv[1] will be take into fg, other will be ignored.
(3) job number can be without % */
bool fg(vector<string> argv){
	int jid;

	/*check argv size*/
	if (argv.size() < 2){
		cerr << "fg: current: no such job" << endl;
		return true;
	}

	/*convert argument to jid*/
	try {
      if (argv[1][0] == '%') {
      	jid = stoi(argv[1].substr(1));
      } else {
      	jid = stoi(argv[1]);
      }
    } catch (exception &e){
    	cerr << "fg: current: " << argv[1] << " no such job" << endl;
      	return true;
    }
	
	job_t *target_job = joblist.find_jid(jid);
	if (!target_job){
		cerr << "fg: current: " << argv[1] << " no such job" << endl;
		return true;
	}

	/* if job is ST or BG */
	if (target_job->status == ST || target_job->status == BG){
		pid_t pid = target_job->pids[0];
		if (kill (-pid, SIGCONT) < 0){
			cerr << "fg: current: " << argv[1] << " failed to continue when trying to be in foreground!" << endl;
			return true;
		}

		target_job->status = BG;

		if (tcsetpgrp(shell_terminal, pid) != 0){ //bring job to fg
			cerr << "fg: current: " << argv[1] << " failed to be brought to foreground, will continue in BG!" << endl;
			return true;
		}

		if (target_job->status == ST){ //reset termio if job stopped
			if (tcsetattr(shell_terminal, TCSADRAIN, &target_job->ter) != 0){
			cerr << "fg: current: " << argv[1] << " failed to restore termio setting of current job, will continue in BG!" << endl;
			tcsetpgrp(shell_terminal, shell_pid); //bring shell to fg
			return true;
			}
		}

		target_job->status = FG;
		int status;

		waitpid(pid, &status, WUNTRACED);

		if (WIFSTOPPED(status)){ //store child termio if stopped
			tcgetattr (shell_terminal, &target_job-> ter); 
		}
		if (tcsetpgrp (shell_terminal, shell_pid) != 0){ //bring shell to foreground
			cerr << "fg: current: " << argv[1] <<" failed to bring shell to the foreground" << endl;
			return false;
		}
		if (tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes) != 0){ // restore shell termio
			cerr << "fg: current: " << argv[1] <<" failed to restore shell termio setting!" << endl;
			return false;
		} 
	}

	/* if job is not ST or BG, cerr */
	else{
		string err_mes = " ";
		if (target_job->status != TN){
			err_mes = "terminated!";
		}
		else if (target_job->status != DNBG || target_job->status != DNFG){
			err_mes = "been done!";
		}
		cerr << "fg: current: " << argv[1] << " job has " << err_mes << "." << endl;
	}
	return true;
}


bool jobs(){
	joblist.listjobs();
	return true;
}

bool history(vector<string> argv) {
	int n = history_length;
	if (argv.size() > 2) {
		cout << "history: too many arguments" << endl;
		return true;
	}
	if (argv.size() == 2) {
		try {
			n = stoi(argv[1]);
		} catch(exception &e) {
			cout << "Usage: history [n]" << endl;
			return true;
		}
		if (n < 0) {
			cout << "Usage: history [n]" << endl;
			return true;
		}
		if (n > history_length) n = history_length;
	}
	for (int i = history_base + history_length - n; 
		i < history_base + history_length; i++) {
		cout <<  i << " " << history_get(i)->line << endl;
	}
	return true;
}

void handle_error(string exec) {
	switch(errno) {
		case E2BIG:
			cerr << exec << ": argument list too long" << endl;
			break;
		case EACCES:
			cerr << exec << ": permission denied" << endl;
			break;
		case EAGAIN:
			cerr << exec << ": resource temporarily unavailable" << endl;
			break;
		case EFAULT:
			cerr << exec << ": bad address" << endl;
			break;
		case EINTR:
			cerr << exec << ": interrupted function call" << endl;
			break;
		case ELOOP:
			cerr << exec << ": too many levels of symbolic links" << endl;
			break;
		case ENAMETOOLONG:
			cerr << exec << ": filename too long" << endl;
			break;
		case ENOENT:
			cerr << exec << ": command not found" << endl;
			break;
		case ENOLINK:
			cerr << exec << ": link has been severed" << endl;
			break;
		case ENOTDIR:
			cerr << exec << ": not a directory" << endl;
			break;
		case ENOEXEC:
			cerr << exec << ": exec format error" << endl;
			break;
		case ENOMEM:
			cerr << exec << ": not enough space/cannot allocate memory" << endl;
			break;
		case ETXTBSY:
			cerr << exec << ": text file busy" << endl;
			break;
		default:
			cerr << exec << ": error" << endl;
			break;
	}
}