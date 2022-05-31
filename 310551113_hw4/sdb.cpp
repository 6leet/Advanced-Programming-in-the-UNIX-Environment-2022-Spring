#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

using namespace std;

void usage() {
    fprintf(stderr, "usage: ./hw4 [-s script] [program]");
}

void parser(int argc, char *argv[], string &script, string &program) {
    char opt;
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 's':
                script = string(optarg);
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        program = string(argv[optind]);
    }
}

/*
int setbreak(..., int &status) { // old status
    if (status) { // check status

    }
    ... // main stuff
    status = new_status // transform
}
*/

pid_t load(string program, int &status) { // TODO: status transform
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        sleep(5);
        char* arg[3] = {strdup(program.c_str()), NULL};
        execvp(arg[0], arg); // exit itself
        return 0;
    } else {
        // int status;
        // waitpid(pid, &status, 0);
        return pid;
    }
    return 0;
}

void middleware(vector<string> cmd, int &status) {
    if (cmd[0] == "load") {
        load(cmd[1], status);
    }
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

pid_t init(string script, string program, int &status) {
    char buf[1024];
    vector<string> cmd;
    while (program.empty()) {
        scanf("%s", buf);
        cmd = sdb_parser(string(buf));
        if (cmd[0] == "load" && cmd.size() >= 2) { // >= or == ?
            program = string(cmd[1]);
            break;
        } else { // not loaded, all commands instead of `load` should be ignored or return error code
            ;
        }
    }
    return load(program, status);
}

void core(string script, string program) {
    printf("script = %s, program = %s\n", script.c_str(), program.c_str());

    int status = 0, wait_status;
    pid_t child = init(script, program, status);
    waitpid(child, &wait_status, 0);
    printf("parent catch child, pid = %d\n", child);
}

int main(int argc, char *argv[]) {
    string script, program;
    parser(argc, argv, script, program);
    core(script, program);
}