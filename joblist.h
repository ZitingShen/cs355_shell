#ifndef JOBLIST_H_
#define JOBLIST_H_

#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <sys/wait.h>

using namespace std;

/* Jobs states: FG (foreground), BG (background), ST (stopped), DN (done),
 *              TN (terminated)
 * Job state transitions and enabling actions:
 *    FG        -> ST       : ctrl-z
 *    ST        -> FG       : fg command
 *    ST        -> BG       : bg command
 *    BG        -> FG       : fg command
 *    FG        -> DNFG     : foreground process exit
 *    BG        -> DNBG     : background process exit
 *    BG/FG/ST  -> TN       : ctrl-c/kill
 */

enum job_status {BG, FG, ST, DNFG, DNBG, TN}; //foreground done???!!!

struct job_t {
	int jid; //job id
	vector<pid_t> pids;
	job_status status;
	string cmdline;
	struct termios ter;

	job_t(int jid, vector<pid_t> pids, job_status status, string cmdline); /* constructor */
};

struct joblist_t {
	int next_jid = 1;
	list<job_t> jobs;

	// return 0 on success, -1 on failure
	int add(pid_t pid, job_status status, string cmdline);
	int remove_jid(int jid);
	int remove_pid(pid_t pid);

	list<job_t>::iterator remove_helper(list<job_t>::iterator it);
	void remove_terminated_jobs();

	// return NULL on failure
	struct job_t *find_jid(int jid);
	struct job_t *find_pid(pid_t pid);

	// return -1 on failure
	pid_t jid2pid(int jid);
	int pid2jid(pid_t pid);

	void listjobs();
};

#endif