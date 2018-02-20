#include<signal.h>
#include<sys/wait.h>

/* SIGCHLD_handler will check the calling process pid from sip, depending on who the calling
process is.
	(1) if it is foreground, ignore it(do nothing)
	(2) if it is bg, double check with waitpid(), then WIFXXXX to find out the exit
	    status and update the job status in joblist accordingly.*/
void SIGCHLD_handler(int sig, siginfo_t *sip, void *notused); 

/* SIGINT_handler will check the calling process pid from sip, depending on who the calling
process is.
	(1) if there is a foregrounded process, it will exit the foreground process in the joblist, 
	    update the status of the job in the joblist.
	(2) if there is no foregrounded process, it will do nothing.
 	After (1) or (2), change the global flag, so thhat shell could continue to the next main loop.*/
void SIGINT_handler(int sig, siginfo_t *sip, void *notused);

/* SIGSTP_handler will check the calling process pid from sip, depending on who the calling
process is.
	(1) if there is a foregrounded process, it will find the foreground process in joblist, 
	    and suspend it by sending SIGSTOP, then update the status of the job in the joblist.
	(2) if there is no foregrounded process, it will do nothing.*/
void SIGSTP_handler(int sig, siginfo_t *sip, void *notused);


