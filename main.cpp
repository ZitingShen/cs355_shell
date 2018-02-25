#include <iostream>

#include "evaluate.h"
#include "handle_signal.h"
#include "joblist.h"
#include "parse.h"

using namespace std;

struct joblist_t joblist;

int main(int argc, char **argv) {
  bool cont = true;
  struct sigaction sa_sigchld, sa_sigint;
  char *cmdline;
  pid_t shell_pgid;
  struct termios shell_tmodes;

  // register signal handler for SIGCHLD and SIGINT using sigaction
  sa_sigchld.sa_sigaction = &sigchld_handler;
  sigemptyset(&sa_sigchld.sa_mask);
  sa_sigchld.sa_flags = SA_SIGINFO;
  if (sigaction(SIGCHLD, &sa_sigchld, 0) == -1) {
    cerr << "Failed to register SIGCHLD" << endl;
    exit(1);
  }

  sa_sigint.sa_sigaction = &sigchld_handler;
  sigemptyset(&sa_sigint.sa_mask);
  sa_sigint.sa_flags = SA_SIGINFO;
  if (sigaction(SIGINT, &sa_sigint, 0) == -1) {
    cerr << "Failed to register SIGINT" << endl;
    exit(1);
  }  

  // mask SIGSTOP and other signals for the main process
  sigset_t masked_signals;
  sigemptyset(&masked_signals);
  sigaddset(&masked_signals, SIGSTOP);
  sigaddset(&masked_signals, SIGTERM);
  sigaddset(&masked_signals, SIGTTIN);
  sigaddset(&masked_signals, SIGTTOU);
  sigaddset(&masked_signals, SIGQUIT);
  sigprocmask(SIG_BLOCK, &masked_signals ,NULL);

  /* Set previous directory to current directory */
  string prevDir = getenv("PWD");

  // configure readline to auto-complete paths when the tab key is hit
  rl_bind_key('\t', rl_complete);

  using_history();

  while(cont) {
    joblist.remove_terminated_jobs();
    
    cmdline = readline("Thou shell not crash> ");
    
    if (cmdline == NULL) { /* End of file (ctrl-d) */
      cout << endl;
      cont = 0;
      continue;
    }

    // check for history expansion
    char *output;
    int expansion_result = history_expand(cmdline, &output);

    // If history expansion exists, overwrite cmdline.
    if (expansion_result > 0) {
      strcpy(cmdline, output);
    }
    free(output);

    // If history expansion doesn't exist, print error message.
    if (expansion_result < 0) {
      cerr << cmdline << ": event not found" << endl;
      continue;
    } else if (strcmp(cmdline, "") != 0) {
      add_history(cmdline);

      // semicolon cannot be directly preceded by ampersend
      vector<string> commands;
      if (separate_by_semicolon(cmdline, &commands) < 0) {
        continue;
      }

      for(string command: commands) {
        vector<string> segments;
        if (separate_by_vertical_bar(&command, &segments) < 0) {
          continue;
        }

        // when parsing segments, separate <, >, >> from strings 
        // before and after
        vector<vector<string>> parsed_segments = parse_segments(&segments);
        
        // hand processed segments to evaluate
        evaluate(&command, &parsed_segments, &cont);
      }
    }
  }
}
