#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#define EPSIZE 8

#define NOT_LOADED 0
#define LOADED 1
#define RUNNING 2

#define ERR_WRONGSTATUS -1
#define ERR_LACKARG -2
#define ERR_UNDEFINED -3
#define ERR_BADWAIT -4
#define ERR_NULL -5

using namespace std;

struct sdb_config {
    string script;
    string program;
    pid_t child;
    int status = NOT_LOADED;
} conf;

vector<string> status_str = {"NOT LOADED", "LOADED", "RUNNING"};

map<string, int> status_map = {
    {"break", RUNNING},
    {"cont", RUNNING},
    {"delete", RUNNING},
    {"disasm", RUNNING},
    {"dump", RUNNING},
    {"get", RUNNING},
    {"getregs", RUNNING},
    {"load", NOT_LOADED},
    {"run", LOADED},
    {"vmmap", RUNNING},
    {"set", RUNNING},
    {"si", RUNNING},
    {"start", LOADED}
};

void errquit(string msg) {
    perror(msg.c_str());
    exit(-1);
}

void writemsg(string msg) {
    cout << msg << '\n';
}

void debug(string msg) {
    string debug_msg = "** " + msg;
    writemsg(debug_msg);
}

void usage() {
    errquit("usage: ./hw4 [-s script] [program]");
}

int lackarg() {
    debug("Not enough input arguments");
    return ERR_LACKARG;
}

int undefined(string command) {
    writemsg("Undefined command: \"" + command + "\".  Try \"help\".");
    return ERR_UNDEFINED;
}

int waitstatus() { // stop & exit ? others?
    int wait_status;
    if (waitpid(conf.child, &wait_status, 0) < 0) errquit("waitpid");
    if (WIFEXITED(wait_status)) {
        debug("child process " + to_string(conf.child) + " terminiated normally (code 0)");
        conf.status = LOADED;
        return 0;
    } else if (WIFSTOPPED(wait_status)) {
        return 0;
    } else { // others
        return ERR_BADWAIT;
    }
}

bool checkstatus(string command) {
    if (conf.status ^ status_map[command]) {
        debug("state must be " + status_str[status_map[command]] + ", current status: " + status_str[conf.status]);
        return false;
    }
    return true;
}

string get_entrypoint() {
    ifstream file(conf.program, ifstream::binary);
    if (!file) {
        errquit("ifstream");
    }

    file.seekg(24);
    char buf[32], hex[32];
    file.read(buf, EPSIZE);
    file.close();

    int sig_i = -1;
    for (int i = 0; i < EPSIZE; i++) {
        if (buf[EPSIZE - 1 - i] != 0 && sig_i < 0) {
            sig_i = i * 2;
        }
        sprintf(hex + i * 2, "%02x\n", (uint8_t)buf[EPSIZE - 1 - i]);
    }
    hex[EPSIZE * 2] = '\0';
    return "0x" + string(hex + sig_i);
}

/*
return if WIFSTOPPED ?
int setbreak(..., int &status) { // old status
    if (status) { // check status

    }
    ... // main stuff
    status = new_status // transform
}
*/

int cont() {
    if (!checkstatus("cont")) return ERR_WRONGSTATUS;

    ptrace(PTRACE_CONT, conf.child, 0, 0);

    conf.status = RUNNING;
    return waitstatus();
}

void quit() { // exit
    exit(0);
}

int start() {
    if (!checkstatus("start")) return ERR_WRONGSTATUS;

    pid_t pid = fork();
    if (pid < 0) {
        errquit("fork");
    } else if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) errquit("ptrace@child");
        char* arg[3] = {strdup(conf.program.c_str()), NULL};
        execvp(arg[0], arg);
        errquit("execvp");
    } else {
        conf.child = pid;
        conf.status = LOADED;

        ptrace(PTRACE_SETOPTIONS, conf.child, 0, PTRACE_O_EXITKILL);

        conf.status = RUNNING;
        debug("pid " + to_string(conf.child));
        return waitstatus();;
    }
    return ERR_NULL;
}

int run() {
    if (conf.status == NOT_LOADED) {
        debug("state must be LOADED or RUNNING, current status: NOT LOADED");
        return ERR_WRONGSTATUS;
    } else if (conf.status == RUNNING) {
        debug("program " + conf.program + " is already running");
        return cont();
    } else { // sdb_status == LOADED
        int ret = start();
        return (ret < 0) ? ret : cont();
    }
}

int load(string program) {
    if (!checkstatus("load")) return ERR_WRONGSTATUS;

    conf.program = program;
    string entrypoint = get_entrypoint();
    debug("program '" + conf.program + "' loaded. entry point " + entrypoint);

    conf.status = LOADED;
    return 0;
}

int middleware(vector<string> cmd) {
    if (cmd[0] == "break" || cmd[0] == "b") {
        if (cmd.size() < 2) return lackarg();
    } else if (cmd[0] == "cont") {
        return cont();
    } else if (cmd[0] == "exit" || cmd[0] == "q") {
        quit();
    } else if (cmd[0] == "load") {
        if (cmd.size() < 2) return lackarg();
        return load(cmd[1]);
    } else if (cmd[0] == "run") {
        return run();
    } else if (cmd[0] == "start") {
        return start();
    } else {
        return undefined(cmd[0]);
    }
    return ERR_NULL;
}

vector<string> sdb_parser(string command) {
    vector<string> cmd;
    stringstream ss(command);
    string buf;
    while (ss >> buf) {
        cmd.push_back(buf);
    }
    return cmd;
}

void arg_parser(int argc, char *argv[]) {
    char opt;
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 's':
                conf.script = string(optarg);
                break;
            default:
                usage();
        }
    }
    if (optind < argc) {
        load(argv[optind]);
    }
}

void core() {
    string command;
    vector<string> cmd;
    while (true) {
        cout << "sdb> ";
        getline(cin, command);
        cmd = sdb_parser(command);
        middleware(cmd);
    }
}

int main(int argc, char *argv[]) {
    arg_parser(argc, argv);
    core();
}