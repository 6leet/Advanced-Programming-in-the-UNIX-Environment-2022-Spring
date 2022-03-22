#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <vector>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <sstream>

using namespace std;

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
        cout << command << '\t' << pid << '\t' << user << '\n';
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
        } else if (procEntryF == "status") {
            proc.user = getProcUser(procEntryA);
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
                processes.push_back(proc);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setFilter(argc, argv);
    vector<Process> processes;
    iterateBase("/proc", processes);


    for (int i = 0; i < processes.size(); i++) {
        processes[i].show();
    }
}
