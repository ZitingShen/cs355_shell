#include <csignal>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <list>
#include <readline/readline.h>
#include <readline/history.h>

/* SIGCHLD_handler 
	Update job status with si_status.
	*/
void sigchld_handler(int sig, siginfo_t *sip, void *notused); 

/* SIGINT_handler 
	rl_forced_update_display(); to print new prompt
	*/
void sigint_handler(int sig, siginfo_t *sip, void *notused);



