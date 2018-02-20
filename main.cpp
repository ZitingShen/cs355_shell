#include "joblist.h"
#include "parse.h"
#include <iostream>

using namespace std;

struct joblist_t joblist;

int main(int argc, char **argv) {
  bool cont = true;
  struct sigaction sa_sigchld, sa_sigint;
  string cmdline;

  // register signal handler for SIGCHLD and SIGINT using sigaction

  // mask SIGSTOP for the main process

  while(cont) {
    joblist.delete_terminated_jobs();
    cout << "Thou shell not crash >"

    if(cin.eof()) {
      cont = false;
      continue;
    }

    getline(cin, cmdline);
    
    if (cmdline == "exit") {
      cont = false;
      continue;
    }

    // (history command to expand the command line)
    
    // (add cmdline to history)

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
