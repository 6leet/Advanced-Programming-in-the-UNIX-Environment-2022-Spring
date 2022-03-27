#include <iostream>
#include <fstream>
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
            exit(1);
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
                    exit(1);
                }
            } else {
                exit(1);
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

string get_command(string pid_path, int &err) {
    err = 0;
    ifstream file(pid_path + "/comm");
    if (!file) {
        err = 1;
        return "";
    } else {
        string command;
        getline(file, command);
        file.close();
        return command;
    }
}

int get_user() {
    return 0;
}

int iterate_pid(string pid_path, Process &process) {
    int err;
    get_command(pid_path, err);
    if (err) return 1;

    get_user();
    if (err) return 1;


}

int iterate_proc(string proc_path, vector<Process> &processes) {
    DIR *dp = opendir(proc_path.c_str());
    if (dp == NULL) {
        cerr << "can't open /proc.\n";
        exit(1);
    } else {
        struct dirent *dir;
        while ((dir = readdir(dp)) != NULL) {
            // cout << direntp->d_name << '\n';
            string pid(dir->d_name);
            if (is_number(pid)) {
                Process process(pid);
                int err = iterate_pid(proc_path + pid, process);
                if (!err) {
                    processes.push_back(process);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    Filter f(argc, argv);
    vector<Process> processes;
    iterate_proc("/proc", processes);
}