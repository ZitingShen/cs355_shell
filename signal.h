#include<signal.h>
#include<sys/wait.h>

/* SIGCHLD_handler will check the calling process pid from sip, depending on who the calling
process is.
	(1) if it is foreground, ignore it(do nothing)
	(2) if it is bg, double check with waitpid(), then WIFXXXX to find out the exit
	    status and update the job status in joblist accordingly.*/
void SIGCHLD_handler(int sig, siginfo_t *sip, void *notused); 

/* SIGCHLD_handler will check the calling process pid from sip, depending on who the calling
process is.
	(1)if sig received in the main process, do nothing;
 	(2)if not in the main process, will exit the foreground process in the joblist, update joblist;
 	After (1) or (2), change the global flag, so thhat shell could continue to the next main loop.*/
void SIGINT_handler(int sig, siginfo_t *sip, void *notused);

/* SIGCHLD_handler will check the calling process pid from sip, depending on who the calling
process is.
	(1)if sig received in the main process, do nothing;
	(2)if not in the main process, will find the foreground process in joblist, and suspend
	   it by sending SIGSTOP. Then update the joblist.
	After (1) or (2), change the global flag, so thhat shell could continue to the next main loop*/
void SIGSTP_handler(int sig, siginfo_t *sip, void *notused);


