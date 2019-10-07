#include<iostream>
#include<stdlib.h>
#include<vector>
#include<string>
#include<sstream>
#include<cstdio>
#include<unistd.h>
#include<string.h>
#include<cstdio>
#include<fstream>
#include<sys/types.h>
#include<sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "init.cpp"
using namespace std;
static  char  *pszCmdPrompt = "abcdefghijklmnopqrstuvwxyz";
static const char *Quit[] = { "exit" };
static const unsigned char ucQuitCmdNum = sizeof(Quit) / sizeof(Quit[0]);
static char *pszLineRead = NULL;
static char *pszStripLine = NULL;
static char *DeleteSpace(char *pszOrig);
static int IsUserQuitCmd(char *pszCmd);
char *ReadCmdLine(vector<string>cmd);
static char *CmdGenerator(const char *Text, int State);
static void InitialReadLine(void);
static char **Completion(const char *Text, int Start, int End);

const int _MAX_PATH=10000;
void ls_handle(vector<string> s);
void cd_handle(vector<string> s);
void pipe_base(vector<string> s);
void begin_cmd(vector<string> &cmd);
void pipe_twoprog(vector<string> split);
void pipe_threeprog(vector<string> split);
void pipe_fourprog(vector<string> split);
bool check_redirect(vector<string> s);
vector<string>cmd;
int pipe_1[2], pipe_2[2], pipe_3[2];
int main() {
    InitialReadLine();
	while (1) {
		string string1 = "";
		begin_cmd(cmd);
		
		for (int i = 0; i<cmd.size(); i++)
			string1 += cmd[i];
                 
		pszCmdPrompt = (char*)string1.c_str();
		char *pszCmdLine = ReadCmdLine(cmd);
		if (IsUserQuitCmd(pszCmdLine))
		{
			free(pszLineRead);
			break;
		}
		ExecCmd(pszCmdLine);
	}
}
void begin_cmd(vector<string> &cmd) {
	cmd.clear();
	string user = getenv("USER");
	user += "@";
	cmd.push_back(user);
	char path[_MAX_PATH] = {};
	getcwd(path, _MAX_PATH);
	string p;
	p.assign(path);
	cmd.push_back(p);
	cmd.push_back("> ");
}
bool check_redirect(vector<string> s) {
	vector<string> buf;
	for (int i = 0; i < s.size(); i++) {
		for (int j = 0; j < s[i].size(); j++) {
			if (s[i][j] == '>' || s[i][j] == '<')
				return true;
		}
	}
	return false;
}
void pipe_base(vector<string> s) {
	vector<string> split;
	for (int i = 0; i < s.size(); i++) {
		string buf;
		for (int j = 0; j < s[i].size(); j++) {
			if (s[i][j] != '|')
				buf += s[i][j];
			else if (s[i][j] == '|') {
				if (buf.size() != 0)
					split.push_back(buf);
				split.push_back("|");
				buf.clear();
			}
		}
		if (buf.size() != 0)
			split.push_back(buf);
	}
	int pipe_count = 0;
	for (int i = 0; i < split.size(); i++)
		if (split[i] == "|")
			pipe_count++;
	if (pipe_count == 1)
		pipe_twoprog(split);
	else if (pipe_count == 2)
		pipe_threeprog(split);
	else if (pipe_count == 3)
		pipe_fourprog(split);
	else
		return;
}
void pipe_twoprog(vector<string> split) {
	bool redirect = check_redirect(split);
	int redirect_position;
	if (redirect) {
		for (int i = 0; i < split.size(); i++)
			for (int j = 0; j < split[i].size(); j++) 
				if (split[i][j] == '>' || split[i][j] == '<') {
					redirect_position = i;
					break;
				}		
	}
	int pipe_position;
	for (int i = 0; i < split.size(); i++) 
		if (split[i] == "|") {
			pipe_position = i;
			break;
		}
	int pid;
	pipe(pipe_1);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_1[1], 1);
		close(pipe_1[0]);
		close(pipe_1[1]);
		vector<string> buf;
		char *argv[100];
		int i = 0;
		for (; i < pipe_position; i++){ 
			argv[i] = (char*)split[i].c_str();	
			buf.push_back(split[i]);
		}
		argv[i] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	pid = fork();
	if (pid == 0) {
		dup2(pipe_1[0], 0);
		close(pipe_1[0]);
		close(pipe_1[1]);
		char *argv[100];
		vector<string> buf;
		int i = pipe_position + 1, j = 0;
		if (redirect) {
			for (; i < redirect_position; i++, j++) {
				argv[j] = (char*)split[i].c_str();
				buf.push_back(split[i]);
			}
			argv[j] = NULL;
			string name;
			int name_pos = split.size() - 1;
			for (int k = 0; k < split[name_pos].size(); k++)
				if (split[name_pos][k] != '~' && split[name_pos][k] != '/')
					name += split[name_pos][k];
			FILE *fp;		
			fp = fopen((char*)name.c_str(), "w");
			dup2(fileno(fp), 1);
			fclose(fp);
			execvp((char*)buf[0].c_str(), argv);
		}
		else {
			for (; i < split.size(); i++, j++) {
				argv[j] = (char*)split[i].c_str();
				buf.push_back(split[i]);
			}
			argv[j] = NULL;
			execvp((char*)buf[0].c_str(), argv);
		}
		exit(1);
	}
	close(pipe_1[0]);
	close(pipe_1[1]);
	waitpid(pid, 0, NULL);
}
void pipe_threeprog(vector<string> split) {
	bool redirect = check_redirect(split);
	int redirect_position;
	if (redirect) {
		for (int i = 0; i < split.size(); i++)
			for (int j = 0; j < split[i].size(); j++)
				if (split[i][j] == '>' || split[i][j] == '<') {
					redirect_position = i;
					break;
				}
	}
	int pipe_position[2], count = 0;
	for (int i = 0; i < split.size(); i++)
		if (split[i] == "|") {
			pipe_position[count] = i;
			count++;
		}
	int pid;
	pipe(pipe_1);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_1[1], 1);
		close(pipe_1[0]);
		close(pipe_1[1]);
		vector<string> buf;
		char *argv[100];
		int i = 0;
		for (; i < pipe_position[0]; i++) {
			argv[i] = (char*)split[i].c_str();
			buf.push_back(split[i]);
		}
		argv[i] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	pipe(pipe_2);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_1[0], 0);
		dup2(pipe_2[1], 1);
		close(pipe_1[0]);
		close(pipe_1[1]);
		close(pipe_2[0]);
		close(pipe_2[1]);
		char *argv[100];
		vector<string> buf;
		int i = pipe_position[0] + 1, j = 0;
		for (; i < pipe_position[1]; i++, j++) {
			argv[j] = (char*)split[i].c_str();
			buf.push_back(split[i]);
		}
		argv[j] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	close(pipe_1[0]);
	close(pipe_1[1]);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_2[0], 0);
		close(pipe_2[0]);
		close(pipe_2[1]);
		char *argv[100];
		vector<string> buf;
		int i = pipe_position[1] + 1, j = 0;
		if (redirect) {
			for (; i < redirect_position; i++, j++) {
				argv[j] = (char*)split[i].c_str();
				buf.push_back(split[i]);
			}
			argv[j] = NULL;
			string name;
			int name_pos = split.size() - 1;
			for (int k = 0; k < split[name_pos].size(); k++)
				if (split[name_pos][k] != '~' && split[name_pos][k] != '/')
					name += split[name_pos][k];
			FILE *fp;
			fp = fopen((char*)name.c_str(), "w");
			dup2(fileno(fp), 1);
			fclose(fp);
			execvp((char*)buf[0].c_str(), argv);
		}
		else {
			for (; i < split.size(); i++, j++) {
				argv[j] = (char*)split[i].c_str();
				buf.push_back(split[i]);
			}
			argv[j] = NULL;
			execvp((char*)buf[0].c_str(), argv);
		}
		exit(1);
	}
	close(pipe_2[0]);
	close(pipe_2[1]);
	waitpid(pid,0,NULL);
}void pipe_fourprog(vector<string> split) {cout<<"four"<<endl;
	int pipe_position[3], count = 0;
	for (int i = 0; i < split.size(); i++)
		if (split[i] == "|") {
			pipe_position[count] = i;
			count++;
		}
	int pid;
	pipe(pipe_1);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_1[1], 1);
		close(pipe_1[0]);
		close(pipe_1[1]);
		vector<string> buf;
		char *argv[100];
		int i = 0;
		for (; i < pipe_position[0]; i++) {
			argv[i] = (char*)split[i].c_str();
			buf.push_back(split[i]);
		}
		argv[i] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	pipe(pipe_2);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_1[0], 0);
		dup2(pipe_2[1], 1);
		close(pipe_1[0]);
		close(pipe_1[1]);
		close(pipe_2[0]);
		close(pipe_2[1]);
		char *argv[100];
		vector<string> buf;
		int i = pipe_position[0] + 1, j = 0;
		for (; i < pipe_position[1]; i++, j++) {
			argv[j] = (char*)split[i].c_str();
			buf.push_back(split[i]);
		}
		argv[j] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	close(pipe_1[0]);
	close(pipe_1[1]);
	pipe(pipe_3);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_2[0], 0);
		dup2(pipe_3[1], 1);
		close(pipe_2[0]);
		close(pipe_2[1]);
		close(pipe_3[0]);
		close(pipe_3[1]);
		char *argv[100];
		vector<string> buf;
		int i = pipe_position[1] + 1, j = 0;
		for (; i < pipe_position[2]; i++, j++) {
			argv[j] = (char*)split[i].c_str();
			buf.push_back(split[i]);
		}
		argv[j] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	close(pipe_2[0]);
	close(pipe_2[1]);
	pid = fork();
	if (pid == 0) {
		dup2(pipe_3[0], 0);
		close(pipe_3[0]);
		close(pipe_3[1]);
		char *argv[100];
		vector<string> buf;
		int i = pipe_position[2] + 1, j = 0;
		for (; i < split.size(); i++, j++) {
			argv[j] = (char*)split[i].c_str();
			buf.push_back(split[i]);
		}
		argv[j] = NULL;
		execvp((char*)buf[0].c_str(), argv);
		exit(1);
	}
	close(pipe_3[0]);
	close(pipe_3[1]);
	waitpid(pid, 0, NULL);
}
void ls_handle(vector<string> s) {
	bool redirect = check_redirect(s);
	char path[_MAX_PATH] = {};
	getcwd(path, _MAX_PATH);

	if (s.size() == 1 || (redirect && s.size() == 3)) {// ls or ls > ~/xxxx.t
		pid_t pid;
		pid = fork();
		if (pid == 0) {
			if (redirect) {
				string ls = "ls>";
				string temp;
				for (int i = 2; i < s[2].size(); i++) {
					if (s[2][i] == '.')break;
					temp += s[2][i];
				}
				ls += temp;
				execlp("/bin/sh", "bin/sh", "-c", (char*)ls.c_str(), (char*)0);
			}
			else {
				char *argv[] = { "ls",path,NULL };
				execv("/bin/ls", argv);
			}
			exit(EXIT_SUCCESS);
		}
		else
			waitpid(pid, NULL, 0);
	}
	else if (s.size() == 2 || (redirect && s.size() == 4)) {// ls -al or ls -al > ~/xxxx.t 
		pid_t pid;
		pid = fork();
		if (pid == 0) {
			if (redirect) {
				string ls = "ls ";
				ls += s[1];
				ls += ">";
				string temp;
				for (int i = 2; i < s[3].size(); i++) {
					if (s[3][i] == '.')break;
					temp += s[3][i];
				}
				ls += temp;
				execlp("/bin/sh", "bin/sh", "-c", (char*)ls.c_str(), (char*)0);
			}
			else {
				char *command = new char[s[1].size()]();
				strcpy(command, s[1].c_str());
				char *argv[] = { "ls",command,path,NULL };
				execv("/bin/ls", argv);
			}
			exit(EXIT_SUCCESS);
		}
		else
			waitpid(pid, NULL, 0);
	}
	else if (s.size() == 3 || (redirect && s.size() == 5)) {
		pid_t pid;
		pid = fork();
		if (pid == 0) {
			if (redirect) {
				string ls = "ls ";
				ls += s[1];
				ls += " ";
				ls += s[2];
				ls += ">";
				string temp;
				for (int i = 2; i < s[4].size(); i++) {
					if (s[4][i] == '.')break;
					temp += s[4][i];
				}
				ls += temp;
				execlp("/bin/sh", "bin/sh", "-c", (char*)ls.c_str(), (char*)0);
			}
			else {
				char *command1 = new char[s[1].size()]();
				char *command2 = new char[s[2].size()]();
				strcpy(command1, s[1].c_str());
				strcpy(command2, s[2].c_str());
				char *argv[] = { "ls",command1,command2,path,NULL };
				execv("/bin/ls", argv);
			}
			exit(EXIT_SUCCESS);
		}
		else
			waitpid(pid, NULL, 0);
	}
	else return;
}
void cd_handle(vector<string> s) {
	char *ch = new char[s[1].size()]();
	strcpy(ch, s[1].c_str());
	char path[_MAX_PATH] = {};
	chdir(ch);
	getcwd(path, _MAX_PATH);
	string p;
	p.assign(path);
	cmd.resize(cmd.size() - 2);
	cmd.push_back(p);
	cmd.push_back("> ");
}
static int IsUserQuitCmd(char *pszCmd)
{
	unsigned char ucQuitCmdIdx = 0;
	for (; ucQuitCmdIdx<ucQuitCmdNum; ucQuitCmdIdx++)
	{
		if (!strcasecmp(pszCmd, Quit[ucQuitCmdIdx]))
			return 1;
	}
	return 0;
}

static char *DeleteSpace(char *pszOrig)
{
	if (NULL == pszOrig)
		return "NUL";
	char *pszStripHead = pszOrig;
	while (isspace(*pszStripHead))
		pszStripHead++;

	if ('\0' == *pszStripHead)
		return pszStripHead;

	char *pszStripTail = pszStripHead + strlen(pszStripHead) - 1;
	while (pszStripTail>pszStripHead && isspace(*pszStripTail))
		pszStripTail--;
	*(++pszStripTail) = '\0';

	return pszStripHead;
}
static char *CmdGenerator(const char *Text, int State)
{
	static int ListIdx = 0, TextLen = 0;
	if (!State)
	{
		ListIdx = 0;
		TextLen = strlen(Text);
	}
	const char *Name = NULL;
	while ((Name = GetCmdByIndex(ListIdx)))
	{
		ListIdx++;

		if (!strncmp(Name, Text, TextLen))
			return strdup(Name);
	}
	return NULL;
}
static char **Completion(const char *Text, int Start, int End)
{
	char **Matches = NULL;
	if (0 == Start)
		Matches = rl_completion_matches(Text, CmdGenerator);

	return Matches;
}

static void InitialReadLine(void)
{
	rl_attempted_completion_function = Completion;
}
char *ReadCmdLine(vector<string>cmd)
{
	vector <string> split;
	string buf, s;
	stringstream ss;
	char pathchar[_MAX_PATH] = {};
	getcwd(pathchar,_MAX_PATH);
	if (pszLineRead)
	{
		free(pszLineRead);
		pszLineRead = NULL;
	}

	pszLineRead = readline((const char*)pszCmdPrompt);
	pszStripLine = DeleteSpace(pszLineRead);
	s.assign(pszStripLine);
	ss << s;
	if (s.size() != 0)
        {
	while (ss >> buf)
		split.push_back(buf);
	bool pipe_in = false;
	for (int i = 0; i<split.size(); i++) {
		for (int j = 0; j<split[i].size(); j++) {
			if (split[i][j] == '|') {
				pipe_in = true;
				break;
			}
		}
	}
	if (pipe_in)
		pipe_base(split);
	else {
		if (split[0] == "ls")
			ls_handle(split);
		else if (split[0] == "cd")
			cd_handle(split);
		else if (split[0] == "exit") {
			cout << "ByeBye!" << endl;
			exit(1);
		}
	}

	if (pszStripLine && *pszStripLine)
		add_history(pszStripLine);
	}
	return pszStripLine;
}

