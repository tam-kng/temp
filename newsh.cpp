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

bool printTokens;
int argc;
map<string, string> variables;
string workingDirectory;
map<pid_t, string> processes;
string prompt;

//check if variable valid, begin with letter and continue with letters and numbers
bool isValid(string variable) {
    bool valid = false;

    //if variable has only 1 character
    if(variable.length() == 1){
        if (isalpha(variable[0]))
            valid = true;
        else {
            cout << "Variable name invalid" << endl;
            valid = false;
        }
    }
    else if(variable.length() > 1){
        if (isalpha(variable[0])) {
            //check for only letters and numbers after first character
            for (int i = 1; (unsigned)i < variable.length(); i++) {
                if (isalpha(variable[i]) || isdigit(variable[i]))
                    valid = true;
                else {
                    cout << "Variable name invalid" << endl;
                    valid = false;
                }
            }
        }
        else {
            cout << "Variable name invalid" << endl;
            valid = false;
        }
    }

    return valid;
}

//sets variable
void setVariable(string varName, string varValue){
    if(varName == "PROMPT"){
        prompt = varValue;
    }
    
    if(argc != 3){
        cout << "3 tokens needed" << endl;
        return;
    }

    if(varName.compare("1") == 0 && varName.compare("printTokens") == 0){
        printTokens = true;
    }

    if(isValid(varName)){
        variables[varName] = varValue;
    }
}

void setDirectory(string argv[]){
    if(argc != 2){
        cout << "2 tokens needed" << endl;
        return;
    }

    //check if directory is valid
    if (chdir(argv[1].c_str()) < 0) {
        cout << argv[1] << " is an invalid directory" << endl;
    }
        //if directory is valid; set it
    else {
        char buf[1024];
        workingDirectory = getcwd(buf, 1024);
    }
}

void listProcesses(){
    if(!processes.empty()){
        cout << "Background processes:" << endl;
        for (auto it : processes) {
            cout << "pid: " << it.first << " process: " << it.second << endl;
        }
    }
    else{
        cout << "No background processes." << endl;
    }

    return;
}

void createProcess(string argv[]){
    //create fork
    pid_t pid = fork();

    if(pid != 0){
        if (argv[0].compare("run") == 0)
            waitpid(pid, NULL, 0);
        //records pid if command is fly
        else if (argv[0].compare("fly") == 0) {
            processes[pid] = argv[1];
        }
    }
    else{
        const char* args[1024];
        for(int i = 0; i < argc-1; i++){
            args[i] = const_cast<char *>(argv[i+1].c_str());
        }
        //runs child program
        execvp(args[0],(char* const*) args);
    }

    return;
}

//handle system processes, checks for completion
void cleanProcesses() {
    for (auto it : processes) {
        if (waitpid(it.first, 0, WNOHANG) > 0) {
            cout << "Completed process: " << it.second << endl;
            processes.erase(it.first);
        }
    }
}

//searches variable map
string searchMap(string key) {
    string value = key;
    string temp;

    if (value[value.length() - 1] == '"')
        key = key.substr(0, key.length() - 1);
    for(auto it : variables){
        temp = it.first;
        string key1 = key.substr(1);
        if (temp.compare(key1)==0)
            value = it.second;
    }

    return value;
}

//--------------FUNCTION TOVAR--------------
void toVar(string argv[], string userInput) {
    pid_t pid = fork();
    FILE *fp;
    if (pid == 0) {
        // runs a child program
        const char*args[1024];
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
    setVariable(argv[1], stout1);

    return;
}

//assigns input to array of values
void parse(string input, string argv[]){
    //resets argc
    argc = 0;
    string token;

    //ignores comments denoted by %
    input = input.substr(0, input.find_first_of("%"));

    while(token != input){
        token = input.substr(0, input.find_first_of(" "));
        input = input.substr(input.find_first_of(" ") + 1);
        argv[argc] = token;

        //if space is not at the end of input
        if(token.compare("") != 0){
            argc++;
        }
    }

    for(int i=0; i<argc; i++){
        if (argv[i][0] == '^') {
            string temp = searchMap(argv[i]);
            argv[i] = temp;
        }
        //checks for quotes
        if (argv[i][0] == '"') {
            string temp = argv[i].substr(1);
            argv[i] = temp;
        }
        if (argv[i][argv[i].length()-1] == '"') {
            string temp = argv[i].substr(0,argv[i].length()-1);
            argv[i] = temp;
        }
    }

    if(printTokens == true){
        for (int j = 0; j < argc; j++) {
            cout << "TOKEN = " << argv[j] << endl;
        }
    }
}

//----------------FUNCTION DONE----------------
void done(string argv[]) {
    if(argc > 1){
        if(argc != 2){
            cout << "2 tokens needed" << endl;
            return;
        }

        int exitNum = stoi(argv[1]);
        //exit at the status of the number entered
        if(exitNum >= 0){
            exit(exitNum);
        }
        else {
            cout << "Parameter to done must be a non-negative integer." << endl;
        }
    }

    // else exit at status 0
    else exit(0);
    return;
}

int main() {
    cout << "newsh shell" << endl;

    //initialize working directory
    variables["PATH"] = "/bin:/usr/bin";
    while(1) {
        cleanProcesses();
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
        string argv[1024];
        parse(userInput, argv);
        //calls correct function based on command
        if (argv[0].compare("setvar")==0)
            setVariable(argv[1], argv[2]);
        /*
        else if (argv[0].compare("setprompt")==0)
            setPrompt(argv);
            */
        else if (argv[0].compare("setdir")==0)
            setDirectory(argv);
        else if (argv[0].compare("showprocs")==0)
            listProcesses();
        /*
        else if (argv[0].compare("showvars")==0)
            showVars();
            */
        else if (argv[0].compare("run")==0)
            createProcess(argv);
        else if (argv[0].compare("fly")==0)
            createProcess(argv);
        else if (argv[0].compare("tovar")==0)
            toVar(argv, userInput);
        else if (argv[0].compare("done")==0)
            done(argv);
        else
            cout << "Command not recognized." << endl;
    }
    return 1;
}