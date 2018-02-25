#include "joblist.h"

job_t::job_t(int jid, list<pid_t> pids, job_status status, string *cmdline) {

}

joblist_t::joblist_t() {

}

int joblist_t::add(pid_t pid, job_status status, string *cmdline) {
	return 0;
}

int joblist_t::remove_jid(int jid) {
	return 0;
}

int joblist_t::remove_pid(pid_t pid) {
	return 0;
}

void joblist_t::remove_terminated_jobs() {

}

list<job_t>::iterator joblist_t::remove_terminated_helper(list<job_t>::iterator it) {

}

bool joblist_t::fg_pid() {
	return true;
}

struct job_t *joblist_t::find_jid(pid_t pid) {

}

struct job_t *joblist_t::find_pid(pid_t pid) {

}

pid_t joblist_t::jid2pid(int jid) {

}

int joblist_t::pid2jid(pid_t pid) {

}

void joblist_t::listjobs() {
	
}
