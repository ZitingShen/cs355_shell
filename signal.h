#include <csignal>
#include <sys/stat.h>

/* SIGCHLD_handler 
	Update job status with si_status.
	*/
void SIGCHLD_handler(int sig, siginfo_t *sip, void *notused); 

/* SIGINT_handler 
	rl_forced_update_display(); to print new prompt
	*/
void SIGINT_handler(int sig, siginfo_t *sip, void *notused);



