#include "parse.h"

using namespace std;

string trim(const string& str);

int separate_by_semicolon(char *cmdline, vector<string> *result) {
	string cmdline_string = cmdline;
	string delimiter = ";";

	size_t pos = 0;
	string token;
	
	while ((pos = cmdline_string.find(delimiter)) != string::npos) {
	    token = cmdline_string.substr(0, pos);
	    if (token != "") {
	    	string trimmed_token = trim(token);
	    	if (trimmed_token[trimmed_token.size()-1] == '&') {
	    		cerr << "syntax error near unexpected token `;'" << endl;
	    		return -1;
	    	}
	    	result->push_back(trimmed_token);
	    } else {
	    	cerr << "syntax error near unexpected token `;;'" << endl;
	    	return -1;
	    }
	    cmdline_string.erase(0, pos + delimiter.length());
	}
	result->push_back(cmdline_string);
	return 0;
}

int separate_by_vertical_bar(string *command, vector<string> *result) {
	string delimiter = "|";

	size_t pos = 0;
	string token;
	
	while ((pos = command->find(delimiter)) != string::npos) {
	    token = command->substr(0, pos);
	    if (token != "") {
	    	result->push_back(token);
		} else {
			cerr << "syntax error near unexpected token `||'" << endl;
	    	return -1;
		}
	    command->erase(0, pos + delimiter.length());
	}
	result->push_back(*command);
	return 0;
}

vector<vector<string>> parse_segments(vector<string> *segments) {
	vector<vector<string>> result;
	return result;
}

string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}