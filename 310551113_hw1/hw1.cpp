//  todo: 
//      1. how to get COMMAND if permission is denied? (not by read_symlink, not by reading `cmdline` file)
//      2. 
#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <vector>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

struct MaxLength {
    int command;
    int pid;
    int user;
    int fd;
    int type;
    int node;
    int name;
    MaxLength() {
        command = 7;
        pid = 3;
        user = 4;
        fd = 2;
        type = 4;
        node = 4;
        name = 4;
    }
};

MaxLength maxlen;

struct File {
    string fd;
    string type;
    string node;
    string name;
    File() {
        fd = "";
        type = "";
        node = "";
        name = "";
    }
};

struct Process {
    string command;
    int pid;
    string user;
    vector<File> files;
    Process() {
        command = "";
        pid = -1;
        user = "";
        files.clear();
    }
    Process(int _pid) {
        command = "";
        pid = _pid;
        user = "";
        files.clear();
    }
    void show() {
        for (int i = 0; i < files.size(); i++) {
            cout << setw(maxlen.command + 4) << left << command 
                << setw(maxlen.pid + 4) << left << pid 
                << setw(maxlen.user + 4) << left << user 
                << setw(maxlen.fd + 4) << left << files[i].fd
                << setw(maxlen.type) << left << files[i].type
                << setw(maxlen.node) << left << files[i].node
                << setw(maxlen.name) << left << files[i].name
                << '\n';
        }
    }
};

struct Filter {
    regex command;
    regex filename;
    regex type;
    Filter() {
        command = regex(".*");
        filename = regex(".*");
        type = regex(".*");
    }
    bool process(string _command, string _filename, string _type) {
        return regex_search(_command, command) && regex_search(_filename, filename) && regex_match(_type, type);
    }
};

void updateMax(Process &proc) {
    maxlen.command = max(maxlen.command, int(proc.command.size()));
    maxlen.pid = max(maxlen.pid, int(to_string(proc.pid).size()));
    maxlen.user = max(maxlen.user, int(proc.user.size()));

    for (int i = 0; i < proc.files.size(); i++) {
        maxlen.fd = max(maxlen.fd, int(proc.files[i].fd.size()));
        maxlen.type = max(maxlen.type, int(proc.files[i].type.size()));
        maxlen.node = max(maxlen.node, int(proc.files[i].node.size()));
        maxlen.name = max(maxlen.name, int(proc.files[i].name.size()));
    }
}

Filter setFilter(int argc, char *argv[]) {
    Filter f;
    for (int i = 0; i < argc; i++) {
        if (string(argv[i]) == "-c") {
            f.command = regex(argv[i++]);
        } else if (string(argv[i]) == "-t") {
            f.filename = regex(argv[i++]);
        } else if (string(argv[i]) == "-f") {
            f.type = regex(argv[i++]);
        }
    }
    return f;
}

bool isNumber(string s) {
    for (int i = 0; i < s.size(); i++) {
        if (!isdigit(s[i])) {
            return false;
        }
    }
    return true;
}

string getCommand(string procEntryA) {
    ifstream file(procEntryA);
    string line;
    getline(file, line);
    stringstream ss(line);
    string command;
    while (getline(ss, command, '\0')) {
        break;
    }
    return command;
}

uid_t getProcUid(string procEntryA) { // "proc/[pid]/status"
    ifstream file(procEntryA);
    string line;
    while (getline(file, line)) {
        if (line.substr(0, 3) == "Uid") {
            stringstream ss(line);
            string buf;
            while (ss >> buf) {
                if (isNumber(buf)) {
                    return stoi(buf);
                }
            }
            break;
        }
    }
    return -1;
}

string getProcUser(string procEntryA) {
    uid_t uid = getProcUid(procEntryA);
    struct passwd *pwd;
    pwd = getpwuid(uid);
    return pwd->pw_name;
}

void iterateProcess(string procPath, Process &proc) {
    filesystem::path path{procPath};
    for (auto const& entry : filesystem::directory_iterator{path}) {
        string procEntryA = filesystem::absolute(entry.path()).string();
        string procEntryF = filesystem::absolute(entry.path()).filename();
        if (procEntryF == "cmdline") {
            // COMMAND
            proc.command = getCommand(procEntryA);
        }
        if (procEntryF == "exe") {             
            // FD: txt
            File file;
            file.fd = "txt";
            file.type = "REG";
            file.name = filesystem::read_symlink(entry.path()).string();
            proc.files.push_back(file);
        } else if (procEntryF == "status") { 
            // USER
            proc.user = getProcUser(procEntryA);
            
        } else if (procEntryF == "cwd") { 
            // FD: current working directory
            File file;
            file.fd = "cwd";
            file.type = "DIR";
            file.name = filesystem::read_symlink(entry.path()).string();
            proc.files.push_back(file);
        }
    }
}

void iterateBase(string basePath, vector<Process> &processes) {
    filesystem::path path{basePath};
    for (auto const& entry : filesystem::directory_iterator{path}) {
        if (entry.is_directory()) {
            string procPathR = filesystem::relative(entry.path(), path).string();
            string procPathA = filesystem::absolute(entry.path()).string();
            if (isNumber(procPathR)) {
                Process proc(stoi(procPathR));
                iterateProcess(procPathA, proc);
                updateMax(proc);
                processes.push_back(proc);
            }
        }
    }
}

void output(vector<Process> processes) {
    cout << setw(maxlen.command + 4) << left << "COMMAND" 
        << setw(maxlen.pid + 4) << left << "PID" 
        << setw(maxlen.user + 4) << left << "USER" 
        << setw(maxlen.fd + 4) << left << "FD"
        << setw(maxlen.type) << left << "TYPE"
        << setw(maxlen.node) << left << "NODE"
        << setw(maxlen.name) << left << "NAME"
        << '\n';
    for (int i = 0; i < processes.size(); i++) {
        processes[i].show();
    } 
}

int main(int argc, char *argv[]) {
    setFilter(argc, argv);
    vector<Process> processes;
    iterateBase("/proc", processes);
    output(processes);
}
