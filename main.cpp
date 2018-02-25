#include "joblist.h"
#include "parse.h"
#include "signal.h"
#include <iostream>

using namespace std;

struct joblist_t joblist;

int main(int argc, char **argv) {
  //register signals
  sigprocmask(SIGSTOP); // mask 
  sigaction(SIGCHLD, SIGCHLD_handler, NULL);
  sigaction(SIGINT, SIGINT_handler, NULL);

  bool cont = true;
  struct sigaction sa_sigchld, sa_sigint;
  char *cmdline;

  // register signal handler for SIGCHLD and SIGINT using sigaction

  // mask SIGSTOP for the main process

  // configure readline to auto-complete paths when the tab key is hit
  rl_bind_key('\t', rl_complete);

  using_history();

  while(cont) {
    joblist.delete_terminated_jobs();
    cout << "Thou shell not crash >"

    if(cin.eof()) {
      cont = false;
      continue;
    }
    
    cmdline = readline("shell> ");
    
    if (strcmp(cmdline, "exit") == 0) {
      cont = false;
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

      // check semicolons, separate cmdline by semicolons
      // no consecutive semicolons are allowed
      // semicolon cannot be directly preceded by ampersend
      vector<string> commands = separate_by_semicolon(&cmdline);

      for(string command: commands) {
        vector<string> segments = separate_by_vertical_bar(&command);

        // when parsing segments, separate <, >, >> from strings 
        // before and after
        vector<vector<string>> parsed_segments = parse_segments(&segments);
        
        // hand processed segments to evaluate
        evaluate(&command, &parsed_segments, &joblist);
      }
    }
  }
}
