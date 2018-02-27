#include "evaluate.h"
#define shell_terminal STDIN_FILENO


extern struct joblist_t joblist;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;

using namespace std;


void evaluate (string *command, vector<vector<string> > *parsed_segments){
	
	set<string> built_in_commands = {"fg", "bg", "kill", "jobs", "history", "exit"};
	
	int len = parsed_segments -> size();
	enum job_status bg_fg = FG; //default as FG
	vector<string> last_seg = parsed_segments -> back();

	/* No pipe!!*/
	if (len == 1){
		string cmd; //get the command
		cmd = last_seg.front();
		if (built_in_commands.count(cmd) == 1){ //if first argument is buildin comment
			if (cmd.compare("kill") == 0){
				kill(last_seg);
			}
			else if(cmd.compare("bg") == 0){
				bg(last_seg);
			}
			else if(cmd.compare("fg") == 0){
				fg(last_seg);
			}
			else if(cmd.compare("jobs") == 0){
				jobs();
			}
			else if(cmd.compare("exit") == 0){
				exit(EXIT_SUCCESS);
			}
		}
		else{//not built_in
			if (last_seg.back().compare("&") == 0){ //check whether background or foreground
				bg_fg = BG;
				last_seg.pop_back();
				cout << "running bg" << endl;
			}
			no_pipe_exec(command, last_seg, bg_fg);
		}
	}

	/* Pipe exists!!*/
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

  	for (int j = 0; j < argv.size(); j++){
  		cout << argv[j] << endl;
  	}

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

	if (pid == 0){ //in child process
		setpgid(0, 0);
		/*unmask signals*/
		sigprocmask(SIG_UNBLOCK, &signalSet, NULL);
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
		int status;
		//do nothing if bg, will clean up in the next loop.
		if (bg_fg == FG){ //waiting for fg child to complete, need to swap termio, also need to store termio
			//of child if child is stopeed
			waitpid(pid, &status, WUNTRACED);
			cout << "outout" <<endl;
			/*does this order matter? tcsetpgrp() first or tcgetattr() first?*/
			if (WIFSTOPPED(status)){ //store child termio if stopped
				tcsetpgrp (shell_terminal, shell_pgid); //bring shell to fg
				if (!joblist.find_pid(pid)){
					cerr << "No such process with process id " << pid << endl;
					tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes);
					return;
				}
				tcgetattr (shell_terminal, &joblist.find_pid(pid) -> ter); 
				tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes); // restore shell termio
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
	int i = 1;
	if (argv[1].compare("-9") == 0){
		cout << "flag found" << endl;
		signo = SIGKILL;
		i++;
	}

	for (;i < argv.size(); i++){
		/* check if pid or jid */
		try {
			if (argv[i][0] == '%'){ //then this is jobid
				cout << "trying kill by jid" << endl;
				if (!joblist.find_jid(stoi(argv[i].substr(1, string::npos)))){
					cerr << "Job " << argv[i].substr(1, string::npos) << " does not exist!" << endl;
					continue;
				}
				cur_pid = joblist.jid2pid(stoi(argv[i].substr(1, string::npos)));
			}
        	else{ //then is pid
        		cout << "trying kill by pid" << endl;
        		cur_pid = stoi(argv[i]);
        		if (!joblist.find_pid(cur_pid)){
        			cerr << "Job with pid " << argv[i] << "does not exist" << endl;
        			continue;
        		}
        		
        	}
    	} catch (exception &e){
    		cerr << "No process with process number " << cur_pid << endl;
    		continue; //continue to the next iteration
   		}

   		
   		//send signo to pid
   		cur_pid = getpgid(cur_pid);//just to double check gpid
		if (kill (- cur_pid, signo) < 0){
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
	int i = 1;

	for (; i < argv.size(); i++){

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
		if (joblist.find_pid(cur_pid) -> status == ST){
			cur_pid = getpgid(cur_pid); //just to double check
			if (kill (- cur_pid, SIGCONT) < 0){ //let job continue
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
		if (kill (- pid, SIGCONT) < 0){ //let job continue
			cerr << "Job " << joblist.pid2jid(pid) << " failed to continue when trying to be in foreground!" << endl;
		}
		tcsetpgrp (shell_terminal, pid); //bring job to fg
		tcsetattr (shell_terminal, &shell_tmodes);
		//tcgetattr (shell_terminal, &shell_tmodes); //store shell termio
		if (joblist.find_pid(pid) -> status == ST){ //reset termio if job stopped
			tcsetattr (shell_terminal, TCSADRAIN, &joblist.find_pid(pid) -> ter); 
		}
		joblist.find_pid(pid) -> status = FG;
		int status;
		waitpid(pid, &status, WUNTRACED);
		if (WIFSTOPPED(status)){ //store child termio if stopped
			tcgetattr (shell_terminal, &joblist.find_pid(pid) -> ter); 
		}
		tcsetattr (shell_terminal, TCSADRAIN, &shell_tmodes); // restore shell termio
	}

	/* if job is not ST or BG, cerr */
	else{
		cerr << "Job " << argv[1] << "can not be brought to foreground since it is neither ST nor BG!" << endl;
	}
}


void jobs(){
	joblist.listjobs();
}
