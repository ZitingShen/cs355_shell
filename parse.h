#ifndef PARSE_H_
#define PARSE_H_

#include <iostream>
#include <vector>
#include <string>

using namespace std;

vector<string> separate_by_semicolon(char *cmdline);
vector<string> separate_by_vertical_bar(string *command);
vector<vector<string>> parse_segments(vector<string> *segments);

#endif