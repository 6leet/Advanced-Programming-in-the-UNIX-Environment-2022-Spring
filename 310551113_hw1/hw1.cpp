//  todo: 
//      1. COMMAND should be a filename or an absolute path?
//      2. duplicate process (the first one and the last one)
//      3. Because a running process in the system could be terminated at any time, your program may encounter a race condition that an earlier check is successful for /proc/<pid>,, 
//         but latter accesses to files in /proc/<pid> directory are failed. Your program should be able to handle cases like this properly.
#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

struct Filter {
    regex command;
    regex filename;
    regex type;
    Filter() {
        command = regex(".*");
        filename = regex(".*");
        type = regex(".*");
    }
    bool process(string _command) {
        return regex_search(_command, command);
    }
    bool file(string _filename, string _type) {
        return regex_search(_filename, filename) && regex_match(_type, type);
    }
};

Filter f;

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
            cout << setw(maxlen.command + 2) << left << command 
                << setw(maxlen.pid + 2) << left << pid 
                << setw(maxlen.user + 2) << left << user 
                << setw(maxlen.fd + 2) << left << files[i].fd
                << setw(maxlen.type + 2) << left << files[i].type
                << setw(maxlen.node + 2) << left << files[i].node
                << setw(maxlen.name + 2) << left << files[i].name
                << '\n';
        }
    }
};

set<string> inodePool;

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
            f.command = regex(argv[++i]);
        } else if (string(argv[i]) == "-f") {
            f.filename = regex(argv[++i]);
        } else if (string(argv[i]) == "-t") {
            f.type = regex(argv[++i]);
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

string getCommand(string procEntryA) { // "proc/[pid]/cmdline" & split by '\0'
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

string processCommand(string command) {
    int slash = command.find('/');
    while (slash != -1) {
        command = command.substr(slash + 1, command.size() - slash - 1);
        slash = command.find('/');
        cout << slash << '\n';
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

string getMode(string filename) {
    // int fd = open(filename.c_str(), O_RDONLY);
    // if (fd < 0) {
    //     cerr << "open\n";
    //     return "x";
    // }
    struct stat buf;
    if (stat(filename.c_str(), &buf) < 0) {
        cerr << "fstat\n";
        return "x";
    }
    // close(fd);
    string mode = "";
    if (buf.st_mode & S_IRUSR) {
        mode = "r";
    }
    if (buf.st_mode & S_IWUSR) {
        if (mode == "r") {
            mode = "u";
        } else {
            mode = "w";
        }
    }
    return to_string(buf.st_mode);
}

int getInode(string filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        cerr << "open\n";
        return -1;
    }
    struct stat buf;
    if (fstat(fd, &buf) < 0) {
        cerr << "fstat\n";
        return -1;
    }
    close(fd);
    return buf.st_ino;
}

string extractInode(string filename) {
    int s = filename.find('[') + 1, t = filename.find(']') - 1;
    return filename.substr(s, t - s + 1);
}

void safeReadSymlink(filesystem::path filePath, string type, File &file) {
    try {
        file.type = type;
        file.name = filesystem::read_symlink(filePath).string();
        string filePathA = filesystem::absolute(filePath).string();
        int del = file.name.find("(deleted)");
        if (del) {
            file.name = file.name.substr(0, del);
        }
        if (type == "FIFO" || type == "SOCK") {
            file.node = to_string(getInode(filePathA));
            file.fd += getMode(filePathA);
            if (file.node == "-1") {
                file.node = extractInode(file.name);
            }
        } else {
            file.node = to_string(getInode(file.name));
            file.fd += getMode(file.name);
            inodePool.insert(file.node);
        }
    } catch (exception &e) {
        file.type = "unknown";
        file.name = filesystem::absolute(filePath).string() + " (Permission denied)";
    }
}

void getMappedFiles(string procEntryA, Process &proc) {
    ifstream file(procEntryA);
    string line;
    while (getline(file, line)) {
        File file;
        file.fd = "mem";
        file.type = "REG";
        stringstream ss(line);
        string buf;
        int i = 0;
        while (ss >> buf) {
            if (i % 7 == 4) { // inode
                if (inodePool.find(buf) != inodePool.end() || buf == "0") {
                    break;
                } else {
                    inodePool.insert(buf);
                    file.node = buf;
                }
            } else if (i % 7 == 5) { // filename
                file.name = buf;
            } else if (i % 7 == 6) { // deleted or not
                if (buf.find("deleted") != -1) {
                    file.fd = "DEL";
                }
            }
            i++;
        }
        if (file.node != "") {
            if (f.file(file.name, file.type)) {
                proc.files.push_back(file);
            }
        }
    }
}

void iterateFd(filesystem::path fdPath, Process &proc) {
    for (auto const& entry : filesystem::directory_iterator{fdPath}) {
        File file;
        file.fd = filesystem::absolute(entry.path()).filename();
        file.name = filesystem::read_symlink(entry.path()).string();
        if (entry.is_directory()) {
            file.type = "DIR";
            safeReadSymlink(entry.path(), "DIR", file);
        } else if (entry.is_regular_file()) {
            file.type = "REG";
            safeReadSymlink(entry.path(), "REG", file);
        } else if (entry.is_character_file()) {
            file.type = "CHR";
            safeReadSymlink(entry.path(), "CHR", file);
        } else if (entry.is_fifo()) {
            file.type = "FIFO";
            safeReadSymlink(entry.path(), "FIFO", file);
        } else if (entry.is_socket()) {
            file.type = "SOCK";
            safeReadSymlink(entry.path(), "SOCK", file);
        } else if (entry.is_other()) {
            file.type = "unknown";
            safeReadSymlink(entry.path(), "unknown", file);
        }
        if (f.file(file.name, file.type)) {
            proc.files.push_back(file);
        }
    }
}

void iterateProcess(filesystem::path procPath, Process &proc) {
    for (auto const& entry : filesystem::directory_iterator{procPath}) {
        string procEntryA = filesystem::absolute(entry.path()).string();
        string procEntryF = filesystem::absolute(entry.path()).filename();
        if (procEntryF == "cmdline") {
            // COMMAND
            proc.command = getCommand(procEntryA);
            proc.command = processCommand(proc.command);
        } else if (procEntryF == "status") { 
            // USER
            proc.user = getProcUser(procEntryA);
        } else if (procEntryF == "cwd") { 
            // FD: current working directory
            File file;
            file.fd = "cwd";
            safeReadSymlink(entry.path(), "DIR", file);
            if (f.file(file.name, file.type)) {
                proc.files.push_back(file);
            }
        } else if (procEntryF == "exe") {             
            // FD: txt
            File file;
            file.fd = "txt";
            safeReadSymlink(entry.path(), "REG", file);
            if (f.file(file.name, file.type)) {
                proc.files.push_back(file);
            }
        } else if (procEntryF == "root") {
            // FD: rtd
            File file;
            file.fd = "rtd";
            safeReadSymlink(entry.path(), "DIR", file);
            if (f.file(file.name, file.type)) {
                proc.files.push_back(file);
            }
        } else if (procEntryF == "maps") {
            // FD: mem
            getMappedFiles(procEntryA, proc);
        } else if (procEntryF == "fd") {
            // Fd: [0-9][rwu]
            try {
                iterateFd(entry.path(), proc);
            } catch (exception &e) {
                File file;
                file.fd = "NOFD";
                file.name = filesystem::absolute(entry.path()).string() + " (permission denied)";
                if (f.file(file.name, file.type)) {
                    proc.files.push_back(file);
                }
            }
        }
    }
}

void iterateBase(string path, vector<Process> &processes) {
    filesystem::path basePath{path};
    for (auto const& entry : filesystem::directory_iterator{basePath}) {
        try {
            if (entry.is_directory()) {
            string procPathR = filesystem::relative(entry.path(), basePath).string();
            string procPathA = filesystem::absolute(entry.path()).string();
                if (isNumber(procPathR)) {
                    inodePool.clear();
                    Process proc(stoi(procPathR));
                    iterateProcess(entry.path(), proc);
                    if (f.process(proc.command)) {
                        updateMax(proc);
                        processes.push_back(proc);
                    }
                }
            }
        } catch (exception &e) {
            continue;
        }
    }
}

void output(vector<Process> processes) {
    cout << setw(maxlen.command + 2) << left << "COMMAND" 
        << setw(maxlen.pid + 2) << left << "PID" 
        << setw(maxlen.user + 2) << left << "USER" 
        << setw(maxlen.fd + 2) << left << "FD"
        << setw(maxlen.type + 2) << left << "TYPE"
        << setw(maxlen.node + 2) << left << "NODE"
        << setw(maxlen.name + 2) << left << "NAME"
        << '\n';

    for (int i = 0; i < processes.size(); i++) {
        processes[i].show();
    } 
}

int main(int argc, char *argv[]) {
    f = setFilter(argc, argv);
    vector<Process> processes;
    iterateBase("/proc", processes);
    output(processes);
}
