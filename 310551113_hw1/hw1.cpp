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
        cout << setw(maxlen.command + 4) << left << command 
            << setw(maxlen.pid + 4) << left << pid 
            << setw(maxlen.user + 4) << left << user 
            << '\n';
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
        if (procEntryF == "exe") { // COMMAND
            proc.command = filesystem::read_symlink(entry.path()).filename();
            maxlen.command = max(maxlen.command, int(proc.command.size()));
        } else if (procEntryF == "status") {
            proc.user = getProcUser(procEntryA);
            maxlen.user = max(maxlen.user, int(proc.user.size()));
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
                maxlen.pid = max(maxlen.pid, int(to_string(proc.pid).size()));
                iterateProcess(procPathA, proc);
                processes.push_back(proc);
            }
        }
    }
}

void output(vector<Process> processes) {
    cout << setw(maxlen.command + 4) << left << "COMMAND" 
        << setw(maxlen.pid + 4) << left << "PID" 
        << setw(maxlen.user + 4) << left << "USER" 
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
