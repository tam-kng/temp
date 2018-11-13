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
string prompt = "msh > "; // set initial prompt
string wd; // working directory
map<pid_t, string> ProcsMap; // map of processes
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

//-------------FUNCTION SEARCH_MAP-----------
//searches through variables and if it finds a key
//matching the input string it returns the value
string searchMap(string findKey) {
    string temp;
    string result = findKey;
    //checks for quotes
    if (result[result.length()-1] == '"')
        findKey = findKey.substr(0, findKey.length()-1);
    for (auto m : variables) {
        temp = m.first;
        string findKey1 = findKey.substr(1);
        if (temp.compare(findKey1)==0)
            result = m.second;
    }
    return result;
}

//--------------FUNCTION PARSE-------------
//seperates user input and puts it in an array
void parse(string userInput, string argv[]) {
    argc = 0;
    string token;

    //remove comments from input
    userInput = userInput.substr(0, userInput.find_first_of("#"));

    //extract words from strings and place in tokens
    while(token != userInput){
        token = userInput.substr(0,userInput.find_first_of(" "));
        userInput = userInput.substr(userInput.find_first_of(" ") + 1);
        argv[argc] = token;
        argc++;
        //if userInput ends in a space
        if (token.compare("")==0)
            argc--;
    }
    //search map for variable
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '^') {
            string temp = searchMap(argv[i]);
            argv[i] = temp;
        }
        //checks for quotes
        if (argv[i][0] == '"') {
            string temp1 = argv[i].substr(1);
            argv[i] = temp1;
        }
        if (argv[i][argv[i].length()-1] == '"') {
            string temp1 = argv[i].substr(0,argv[i].length()-1);
            argv[i] = temp1;
        }
    }

    //if setvar ShowTokens 1 is set: print tokens
    if (printTokens == true) {
        for (int j = 0; j < argc; j++) {
            cout << "TOKEN = " << argv[j] << endl;
        }
    }
}


//----------------FUNCTION SETVAR------------
//if userInput = "setvar variable value"
//maps value to variable
void setVar(string variable, string value) {
    int correctNumTokens = checkNumTokens(setvarTokens);
    //if userInput = "setvar ShowTokens 1"; set printTokens to true
    if (variable.compare("ShowTokens")==0 && value.compare("1")==0){
        printTokens = true;
    }
    if (correctNumTokens == validNumTokens) {
        //check if variable name is valid
        if (isValid(variable)) {
            // map vairable to value
            variables[variable] = value; //note: value is stored as string
        }
    }
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

//-------------FUNCTION SETDIR-----------------
void setDir(string argv[]) {
    int correctNumTokens = checkNumTokens(setdirTokens);
    if (correctNumTokens == validNumTokens) {
        //check if directory is valid
        if (chdir(argv[1].c_str()) < 0) {
            cout << argv[1] << ": not a valid directory." << endl;
        }
            //if directory is valid; set it
        else {
            char buf[BUFFSIZE];
            wd = getcwd(buf, BUFFSIZE);
        }
    }
}

//--------------FUNCTION SHOWPROCS--------------
//this function displays the contents of ProcsMap
void showProcs() {
    if (ProcsMap.empty()) {
        cout << "No background processes." << endl;
        return;
    }
    cout << "Background processes:" << endl;
    for (auto iter : ProcsMap) {
        cout << "pid: " << iter.first << " process: " << iter.second << endl;
    }
}

//------------TEMPORARY FUNCTION SHOW_VARS-------------**************************
void showVars() {
    for (auto i : variables)
        cout << i.first << " " << i.second << endl;
}

//----------------FUNCTION DONE----------------
void done(string argv[]) {
    if (argc > 1) {
        int correctNumTokens = checkNumTokens(doneTokens);
        if (correctNumTokens == validNumTokens) {
            int exitNum = stoi(argv[1]);
            //exit at the status of the number entered
            if (exitNum >= 0)
                exit(exitNum);
            else
                cout << "Parameter to done must be a non-negative integer." << endl;
        }
    }
        // else exit at status 0
    else exit(0);
    return;
}

//--------------FUNCTION PROCESS-------------
void process(string argv[], string userInput) {
    // fork into parent and child processes
    pid_t pid = fork();
    if (pid == 0) {
        //convert argv[] into a useable form
        const char*args[BUFFSIZE];
        for (int i = 0; i < argc-1; i++) {
            args[i] = const_cast<char *>(argv[i+1].c_str());
        }
        //runs a child program
        execvp(args[0],(char* const*) args);
    }
    else {
        if (argv[0].compare("run") == 0)
            waitpid(pid,NULL,0);
            // if command is "fly", record pid
        else if (argv[0].compare("fly") == 0) {
            ProcsMap[pid] = argv[1];
        }
    }

    return;
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
    for (auto iter : ProcsMap) {
        if (waitpid(iter.first, 0, WNOHANG) > 0) {
            cout << "Completed: " << iter.second << endl;
            ProcsMap.erase(iter.first);
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
        //calls correct function based on command
        if (argv[0].compare("setvar")==0)
            setVar(argv[1], argv[2]);
        else if (argv[0].compare("setprompt")==0)
            setPrompt(argv);
        else if (argv[0].compare("setdir")==0)
            setDir(argv);
        else if (argv[0].compare("showprocs")==0)
            showProcs();
        else if (argv[0].compare("showvars")==0)
            showVars();
        else if (argv[0].compare("done")==0)
            done(argv);
        else if (argv[0].compare("run")==0)
            process(argv, userInput);
        else if (argv[0].compare("fly")==0)
            process(argv,userInput);
        else if (argv[0].compare("tovar")==0)
            toVar(argv, userInput);
        else
            cout << "Command not recognized." << endl;
    }
    return 1;
}