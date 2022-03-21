#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <vector>

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

void iterateProcess(string procPath, Process &proc) {
    filesystem::path path{procPath};
    for (auto const& entry : filesystem::directory_iterator{path}) {
        cout << filesystem::absolute(entry.path()).string() << ": ";
        if (entry.is_directory()) {
            cout << "directory\n";
        } else if (entry.is_symlink()) {
            cout << "symlink\n";
        }
    }
    cout << '\n';
}

void iterateBase(string basePath) {
    vector<Process> processes;
    filesystem::path path{basePath};
    for (auto const& entry : filesystem::directory_iterator{path}) {
        if (entry.is_directory()) {
            string procPathR = filesystem::relative(entry.path(), path).string();
            string procPathA = filesystem::absolute(entry.path()).string();
            if (isNumber(procPathR)) {
                Process proc(stoi(procPathR));
                iterateProcess(procPathA, proc);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setFilter(argc, argv);
    iterateBase("/proc");
}
