#include "joblist.h"

using namespace std;

job_t::job_t(int jid, vector<pid_t> pids, job_status status, string cmdline) {
	this->jid = jid;
	this->pids = pids;
	this->status = status;
	this->cmdline = cmdline;
}

int joblist_t::add(pid_t pid, job_status status, string cmdline) {
	if(pid < 1) return -1;

	vector<pid_t> pids;
	pids.push_back(pid);
	jobs.emplace(jobs.end(), next_jid, pids, status, cmdline);
	next_jid++;
	return 0;
}

int joblist_t::remove_jid(int jid) {
	if(jid < 1) return -1;

	for (list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		if (it->jid == jid) {
			remove_helper(it);
			return 0;
		}
	}
	return -1;
}

// remove a job from the joblist
// if there is only one job left, reset next_jid to 1
list<job_t>::iterator joblist_t::remove_helper(list<job_t>::iterator it) {
	if(jobs.size() == 1) {
		next_jid = 1;
	}
	return jobs.erase(it);
}

int joblist_t::remove_pid(pid_t pid) {
	if (pid < 1) return -1;

	for(list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		if (it->pids[0] == pid) {
			remove_helper(it);
			return 0;
		}
	}
	return -1;
}

void joblist_t::remove_terminated_jobs() {
	pid_t pid;
	int status;
	for (list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		switch(it->status) {
			case DNBG:
				waitpid(it->pids[0], &status, 0);
				#if (DEBUG)
				cout << "[" << it->jid << "] (" << it->pid << ")\tDone\t" 
					<< it->cmdline << endl;
				#endif
				it = remove_helper(it);
				break;
			case DNFG:
				#if (DEBUG)
				cout << "[" << it->jid << "] (" << it->pid << ")\tDone\t" 
					<< it->cmdline << endl;
				#endif
				it = remove_helper(it);
				break;
			case TN:
				waitpid(it->pids[0], &status, 0);
				#if (DEBUG)
				cout << "[" << it->jid << "] (" << it->pid << ")\tTerminated\t" 
					<< it->cmdline << endl;
				#endif
				it = remove_helper(it);
			default:;
		}
	}
	while((pid = waitpid(-1, &status, WNOHANG)) > 1)
		remove_pid(pid);
}

job_t *joblist_t::find_jid(int jid) {
	if(jid < 1) return NULL;

	for(list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		if(it->jid == jid) {
			return &(*it);
		}
	}
	return NULL;
}

job_t *joblist_t::find_pid(pid_t pid) {
	if(pid < 1) return NULL;

	for(list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		if(find(it->pids.begin(), it->pids.end(), pid) != it->pids.end()) {
			return &(*it);
		}
	}
	return NULL;
}

pid_t joblist_t::jid2pid(int jid) {
	if(jid < 1) return -1;

	for(list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		if(it->jid == jid) {
			return it->pids[0];
		}
	}
	return -1;
}

int joblist_t::pid2jid(pid_t pid) {
	if(pid < 1) return -1;

	for(list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		if(find(it->pids.begin(), it->pids.end(), pid) != it->pids.end()) {
			return it->jid;
		}
	}
	return -1;
}

void joblist_t::listjobs() {
	for(list<job_t>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
		switch(it->status) {
			case BG:
			case FG:
				cout << '[' << it->jid << "]" << '\t' << "Running\t\t\t"
					 << it->cmdline << endl << flush;
				break;
			case ST:
				cout << '[' << it->jid << "]" << '\t' << "Stopped\t\t\t"
				     << it->cmdline << endl << flush;
				break;
			#if (DEBUG)
			case DNFG:
				cout << '[' << it->jid << "]" << '\t' << "Done (fg)\t\t\t"
					 << it->cmdline << endl << flush;
				break;
			case DNBG:
				cout << '[' << it->jid << "]" << '\t' << "Done (bg)\t\t\t"
					 << it->cmdline << endl << flush;
				break;
			case TN:
				cout << '[' << it->jid << "]" << '\t' << "Terminated\t\t\t"
					 << it->cmdline << endl << flush;
			#endif
			default:;
		}
	}
}
