#include <termios.h>
#include <unistd.h>

/* Jobs states: FG (foreground), BG (background), ST (stopped), DN (done),
 *              TN (terminated)
 * Job state transitions and enabling actions:
 *    FG        -> ST       : ctrl-z
 *    ST        -> FG       : fg command
 *    ST        -> BG       : bg command
 *    BG        -> FG       : fg command
 *    BG        -> DN       : background process exit
 *    BG/FG/ST  -> TN       : ctrl-c/kill
 */
enum job_status {BG, FG, ST, DN, TN};

struct job_t {
	int jid; //job id
	pid_t pid;
	job_status status;
	string cmdline;
	struct termios ter;

	job(int jid, pid_t pid, job_status status, string *cmdline); /* constructor */
};

struct joblist_t {
	int next_jid;
	list<job> jobs;

	joblist();
	int add(pid_t pid, job_status status, string *cmdline);
	int remove_jid(int jid);
	int remove_pid(pid_t pid);
	void remove_terminated_jobs();
	list<job_t>::iterator remove_terminated_helper(list<job_t>::iterator it);
	bool fg_pid();
	struct job_t *find_jid(pid_t pid);
	struct job_t *find_pid(pid_t pid);
	pid_t jid2pid(int jid);
	int pid2jid(pid_t pid);
	void listjobs();
};