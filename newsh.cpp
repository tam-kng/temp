#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <streambuf>
#include <fstream>
#include <ios>

#include <string>
#include <map>
using namespace std;

//variable map
map<string, string> variables;
//process map
map<pid_t, string> processes;
//initial map
string prompt = "newsh$ ";
string workingDirectory;
int argc;

//BUILD-IN COMMAND HELPERS
//checks if variable valid based on starting letter, following numbers/letters
bool isValid(string var) {
    bool valid = false;

    //if variable is one cahracter
    if(var.length() == 1){
        if(!isalpha(var[0])) {
            cout << "Name of variable invalid" << endl;
            valid = false;
        }
        else{
            valid = true;
        }
    }
    else{
        //ensures first character is a latter
        if(!isalpha(var[0])){
            cout << "Name of variable invalid" << endl;
            valid = false;
        }
        else {
            //every following letter must be a letter or number
            for (int i=1; (unsigned)i<var.length(); i++) {
                if(isdigit(var[i]) || isalpha(var[i])) {
                    valid = true;
                }
                else{
                    cout << "Name of variable invalid" << endl;
                    valid = false;
                }
            }
        }
    }

    return valid;
}

int checkTokenNum(int tokenNum) {
    //if correct number of tokens
    if (tokenNum == argc)
        return 1;
    else {
        cout << "Invalid number tokens found" << endl;
        return -1;
    }
}

//BUILD IN COMMANDS
//set variable value
void setVariable(string varName, string varValue){
    //should be three tokens
    int correctNumTokens = checkTokenNum(3);

    if(varName == "PROMPT"){
        prompt = varValue + " ";
        return;
    }

    if(correctNumTokens == 1){
        if(isValid(varName)){
            variables[varName] = varValue;
        }
    }
}

//cd directoryName
void setDirectory(string argv[]){
    //should be two tokens
    int correctNumTokens = checkTokenNum(2);

    if(correctNumTokens == 1){
        //check if directory is valid
        if(chdir(argv[1].c_str()) < 0){
            cout << argv[1] << " is an invalid directory" << endl;
        }
        else{
            char buf[1024];
            workingDirectory = getcwd(buf, 1024);
        }
    }
}

//bp
void listProcesses() {
    //returns empty if no processes running
    if(processes.empty()) {
        cout << "Process list empty" << endl;
        return;
    }
    //iterates through process list
    cout << "Background processes: " << endl;
    for (auto it : processes) {
        cout << "pid: " << it.first << " process: " << it.second << endl;
    }
}

//PROGRAM CONTROL COMMANDS
//cmd param*
void execute(string argv[]) {
    pid_t pid = fork();
    if(pid == 0){
        const char*args[1024];
        for (int i = 0; i < argc-1; i++) {
            args[i] = const_cast<char *>(argv[i+1].c_str());
        }

        //executes child fork
        execvp(args[0],(char* const*) args);
    }
    else{
        if(argv[0].compare("run") == 0){
            waitpid(pid, NULL, 0);
        }
        else if(argv[0].compare("fly") == 0){
            processes[pid] = argv[1];
        }
    }

    return;
}

//PROGRAM HELPER FUNCTIONS

void quit(string argv[]) {
    if (argc > 1) {
        //should be two tokens
        int correctNumTokens = checkTokenNum(2);
        if (correctNumTokens == 1) {
            int exitNum = stoi(argv[1]);

            if (exitNum >= 0)
                exit(exitNum);
            else
                cout << "To quit, type quit" << endl;
        }
    }

    else exit(0);
    return;
}

//searches variables map
string searchMap(string key) {
    string temp;
    string foundValue = key;

    if(foundValue[foundValue.length() - 1] == '"'){
        key = key.substr(0, key.length() - 1);
    }

    for(auto it : variables){
        temp = it.first;
        string findKey1 = key.substr(1);
        if(temp.compare(findKey1) == 0) {
            foundValue = it.second;
        }
    }
    return foundValue;
}

void parse(string userInput, string argv[]) {
    argc = 0;
    string token;

    //remove end comments denoted by %
    userInput = userInput.substr(0, userInput.find_first_of("%"));

    //extract words from strings and place in tokens
    while(token != userInput){
        token = userInput.substr(0,userInput.find_first_of(" "));
        userInput = userInput.substr(userInput.find_first_of(" ") + 1);
        argv[argc] = token;
        argc++;

        //decrements argc based on space at end of value
        if (token.compare("")==0)
            argc--;
    }

    for (int i=0; i<argc; i++) {
        if (argv[i][0] == '^') {
            string temp = searchMap(argv[i]);
            argv[i] = temp;
        }
        if (argv[i][0] == '"') {
            string temp = argv[i].substr(1);
            argv[i] = temp;
        }
        if (argv[i][argv[i].length()-1] == '"') {
            string temp = argv[i].substr(0,argv[i].length()-1);
            argv[i] = temp;
        }
    }
}

void checkProcesses() {
    for (auto it : processes) {
        if (waitpid(it.first, 0, WNOHANG) > 0) {
            cout << "Completed: " << it.second << endl;
            processes.erase(it.first);
        }
    }
}

int main() {
    cout << "CS 270: Project 3 New Shell" << endl;

    //sets working directory
    variables["PATH"] = "/bin:/usr/bin";
    while(1) {
        checkProcesses();
        cout << prompt;
        string input;

        getline(cin, input);
        //tests if line empty
        if(cin.rdstate()){
            exit(0);
        }
        if(input.empty()){
            continue;
        }

        string argv[1024];
        parse(input, argv);

        //set variable value
        if (argv[0].compare("set") == 0)
            setVariable(argv[1], argv[2]);
        //cd directoryName
        else if (argv[0].compare("cd") == 0)
            setDirectory(argv);
        //bp
        else if (argv[0].compare("bp") == 0)
            listProcesses();
        //quit
        else if (argv[0].compare("quit") == 0)
            quit(argv);
        //cmd param*
        else if (argv[0].compare("cmd") == 0)
            execute(argv);
        else
            cout << "Invalid command" << endl;
    }
}