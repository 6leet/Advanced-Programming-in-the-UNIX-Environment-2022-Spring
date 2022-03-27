#include <iostream>
#include <string>
#include <regex>
#include <map>
#include <dirent.h>
#include <sys/types.h>

using namespace std;

struct Filter {
    regex command, filename, type;
    Filter(int argc, char *argv[]) {
        command = regex(".*");
        filename = regex(".*");
        type = regex(".*");
        if (argc > 1 && argc % 2 == 0) {
            exit(-1);
        }
        for (int i = 1; i < argc; i++) {
            string arg = string(argv[i]);
            if (arg[0] == '-') {
                if (arg.substr(1, arg.size() - 1) == "c") {
                    command = regex(argv[++i]);
                } else if (arg.substr(1, arg.size() - 1) == "f") {
                    filename = regex(argv[++i]);
                } else if (arg.substr(1, arg.size() - 1) == "t") {
                    type = regex(argv[++i]);
                } else {
                    exit(-1);
                }
            } else {
                exit(-1);
            }
        }
    }
    bool filt(string _c, string _f, string _t) {
        return regex_search(_c, command) && regex_search(_f, filename) && regex_match(_t, type);
    }
};

struct File {
    string fd, type, node, name;
    File() {
        fd = type = node = name = "";
    }
};

struct Process {
    string command, pid, user;
    vector<File> files;
    Process() {
        command = pid = user = "";
        files.clear();
    }
    Process(string _p) {
        command = user = "";
        pid = _p;
        files.clear();
    }
};

bool is_number(string s) {
    for (int i = 0; i < s.size(); i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}

void iterateProc(string procPath, vector<Process> &processes) {
    DIR *dp = opendir(procPath.c_str());
    if (dp == NULL) {
        cerr << "can't open /proc.\n";
        exit(-1);
    } else {
        struct dirent *direntp = readdir(dp);
        while (direntp != NULL) {
            cout << direntp->d_name << '\n';
        }
    }
}

int main(int argc, char *argv[]) {
    Filter f(argc, argv);
    vector<Process> processes;
    iterateProc("/proc", processes);
}