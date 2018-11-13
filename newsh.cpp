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

const int setvarTokens = 3;
const int setpromptTokens = 2;
const int setdirTokens = 2;
const int showprocsTokens = 1;
const int doneTokens = 2;
const int validNumTokens = 1;

bool printTokens = false;
map<string, string> variables; // map of variables
int BUFFSIZE = 1024;
string prompt = "newsh$ "; // set initial prompt
string workingDirectory; // working directory
map<pid_t, string> processes; // map of processes
int argc;

//checks if variable valid based on starting letter, following numbers/letters
bool isValid(string var) {
    bool valid = false;

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
        //make sure first character is a letter
        if(!isalpha(var[0])){
            cout << "Name of variable invalid" << endl;
            valid = false;
        }
        else {
            //every following letter must be a letter, number or _
            for (int i=1; (unsigned)i<var.length(); i++) {
                if(isdigit(var[i]) || isalpha(var[i]) || var[i] == '_') {
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

int checkNumTokens(int tokenNum) {
    //if correct number of tokens
    if (tokenNum == argc)
        return 1;
    else {
        cout << "Expected " << tokenNum << " tokens, got " << argc << " tokens." << endl;
        return -1;
    }
}

void setVar(string varName, string varValue){
    //should be three tokens
    int correctNumTokens = checkNumTokens(3);

    if(varName == "PROMPT"){
        prompt = varValue + " ";
        return;
    }

    if(varName.compare("ShowTokens") == 0 && varValue.compare("1") == 0){
        printTokens = true;
    }
    if(correctNumTokens == 1){
        if(isValid(varName)){
            variables[varName] = varValue;
        }
    }
}

void setDir(string argv[]){
    //should be two tokens
    int correctNumTokens = checkNumTokens(2);

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

void showProcs() {
    if(processes.empty()) {
        cout << "Process list empty" << endl;
        return;
    }
    cout << "Background processes: " << endl;
    for (auto it : processes) {
        cout << "pid: " << it.first << " process: " << it.second << endl;
    }
}

void done(string argv[]) {
    if (argc > 1) {
        //should be two tokens
        int correctNumTokens = checkNumTokens(2);
        if (correctNumTokens == 1) {
            int exitNum = stoi(argv[1]);

            if (exitNum >= 0)
                exit(exitNum);
            else
                cout << "To quit, type 0" << endl;
        }
    }

    else exit(0);
    return;
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

    /*
    if (printTokens == true) {
        for (int j=0; j<argc; j++) {
            cout << "Token" << argv[j] << endl;
        }
    }
     */
}

//------------FUNCTION SETPROMPT------------------
//allows user to chance the prompt
//that starts with each new command line
void setPrompt(string argv[]) {
    int correctNumTokens = checkNumTokens(setpromptTokens);
    if (correctNumTokens == validNumTokens) {
        //set global variable prompt to argument
        prompt = argv[1];
    }
    return;
}

//------------TEMPORARY FUNCTION SHOW_VARS-------------**************************
void showVars() {
    for (auto i : variables)
        cout << i.first << " " << i.second << endl;
}

//--------------FUNCTION TOVAR--------------
void toVar(string argv[], string userInput) {
    pid_t pid = fork();
    FILE *fp;
    if (pid == 0) {
        // runs a child program
        const char*args[BUFFSIZE];
        for (int i = 0; i < argc-2; i++) {
            args[i] = const_cast<char *>(argv[i+2].c_str());
        }
        //puts stdout into /tmp/file.txt
        fp = freopen("/tmp/file.txt", "w+", stdout);
        execvp(args[0],(char* const*) args);
        fclose(fp);
    }
    else {
        waitpid(pid,NULL,0);
    }
    //puts the file into a string stout
    ifstream file;
    file.open("/tmp/file.txt");
    stringstream buffer1;
    buffer1 << file.rdbuf();
    string stout1 = buffer1.str();
    //map stdout to variable
    setVar(argv[1], stout1);

    return;
}

//--------------FUNCTION CHECK_PROCESSES-----------
//checks to see if fly processes have completed and
//if they have it removes them from the map
void checkProcesses() {
    for (auto iter : processes) {
        if (waitpid(iter.first, 0, WNOHANG) > 0) {
            cout << "Completed: " << iter.second << endl;
            processes.erase(iter.first);
        }
    }
}


//--------------------MAIN---------------------------
int main() {
    cout << "----------Welcome to our Shell----------" << endl;

    //initialize working directory
    variables["PATH"] = "/bin:/usr/bin";
    while(1) {
        checkProcesses();
        cout << prompt;
        string userInput;
        //gets user inputted command line
        getline(cin, userInput);
        //checks if the line is empty
        if (cin.rdstate()) { // error or no more input
            exit(0);
        }
        if (userInput.empty()) {
            continue;
        }
        string argv[BUFFSIZE];
        parse(userInput, argv);

        //set variable value
        if (argv[0].compare("set")==0)
            setVar(argv[1], argv[2]);
        else if (argv[0].compare("setprompt")==0)
            setPrompt(argv);
        //cd directoryName
        else if (argv[0].compare("cd")==0)
            setDir(argv);
        //bp
        else if (argv[0].compare("bp")==0)
            showProcs();
        //quit
        else if (argv[0].compare("quit")==0)
            done(argv);
        else if (argv[0].compare("cmd")==0)
            execute(argv);
        else if (argv[0].compare("tovar")==0)
            toVar(argv, userInput);
        else
            cout << "Command not recognized." << endl;
    }
    return 1;
}