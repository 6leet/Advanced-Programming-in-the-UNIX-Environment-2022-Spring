#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <regex>
#include <map>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

map<string, int> maxlen;
vector<string> columns{"COMMAND", "PID", "USER", "FD", "TYPE", "NODE", "NAME"};

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

void init_maxlen() {
    for (int i = 0; i < columns.size(); i++) {
        maxlen[columns[i]] = columns[i].size();
    }
}

void update_maxlen(Process &process) {
    for (int i = 0; i < process.files.size(); i++) {
        File file = process.files[i];
        vector<string> cols{process.command, process.pid, process.user, file.fd, file.type, file.node, file.name};
        for (int j = 0; j < columns.size(); j++) {
            maxlen[columns[j]] = max(maxlen[columns[j]], int(cols[j].size()));
        }
    }
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

uid_t get_uid(string pid_path, int &err) {
    err = 0;
    ifstream file(pid_path + "/status");
    if (!file) {
        err = 1;
        return -1;
    } else {
        string line;
        while (getline(file, line)) {
            if (line.substr(0, 3) == "Uid") {
                stringstream ss(line);
                string buf;
                while (ss >> buf) {
                    if (is_number(buf)) {
                        return stoi(buf);
                    }
                }
                break;
            }
        }
        err = 1;
        return -1;
    }
}

string get_user(string pid_path, int &err) {
    err = 0;
    uid_t uid = get_uid(pid_path, err);
    if (err == 1) return "";
    struct passwd *pwd = getpwuid(uid);
    if (pwd == NULL) {
        err = 1;
        return "";
    } else {
        return pwd->pw_name;
    }
}

string safe_readlink(string link_path, int &err) {
    err = 0;
    char buf[1024];
    ssize_t len;
    if ((len = readlink(link_path.c_str(), buf, sizeof(buf) - 1)) != -1) {
        buf[len] = '\0';
        return string(buf);
    } else {
        if (errno == EACCES) {
            err = -1; // permission denied
            return link_path + " (Permission denied)";
        } else {
            err = 1;
            return "";
        }
    }
}

string get_from_stat(string file_path, string target, bool through_link, int &err) {
    struct stat buf;
    int _stat;
    if (through_link) { // the file linked by softlink
        _stat = stat(file_path.c_str(), &buf);
    } else { // softlink itself
        _stat = lstat(file_path.c_str(), &buf);
    }
    if (_stat == -1) { // error
        if (errno = EACCES) {
            err = -1; // permission denied
            if (target == "type") return "unknown";
            else return "";
        } else {
            err = 1;
            return "";
        }
    }
    if (target == "type") {
        switch (buf.st_mode & S_IFMT) {
            case S_IFCHR:  return "CHR";
            case S_IFDIR:  return "DIR";
            case S_IFIFO:  return "FIFO";
            case S_IFREG:  return "REG";
            case S_IFSOCK: return "SOCK";
            default:       return "unknown";
        }
    } else if (target == "node") {
        return to_string(buf.st_ino);
    } else if (target == "mode") {
        string mode = "";
        if (buf.st_mode & S_IRUSR) {
            mode = "r";
        }
        if (buf.st_mode & S_IWUSR) {
            if (mode == "r" ) {
                mode = "u";
            } else {
                mode = "w";
            }
        }
        return mode;
    } else {
        return "";
    }
}

File get_special_file(string file_path, string fd, int &err) {
    err = 0;
    File file;
    file.fd = fd;
    file.name = safe_readlink(file_path, err);
    if (err == 1) return File();
    file.type = get_from_stat(file_path, "type", true, err);
    if (err == 1) return File();
    file.node = get_from_stat(file_path, "node", true, err);
    if (err == 1) return File();
    return file;
}

vector<File> get_maps(string map_path, int &err) {
    err = 0;
    ifstream file(map_path);
    vector<File> map_files;
    if (!file) {
        err = 1;
        return vector<File>();
    } else {
        string line;
        while (getline(file, line)) {
            File map_file;
            map_file.fd = "mem";
            stringstream ss(line);
            string buf;
            int i = 0;
            while (ss >> buf) {
                if (i == 5) {
                    map_file.name = buf;
                    break;
                }
                i++;
            }
            map_file.type = get_from_stat(file_path, "type", false, err);
            if (err == 1) return vector<File>();
            map_file.node = get_from_stat(file_path, "node", false, err);
            if (err == 1) return vector<File>();
        }
    }
}

vector<File> iterate_fd(string fd_path, int &err) {
    err = 0;
    DIR *dp = opendir(fd_path.c_str());
    vector<File> fd_files;
    if (dp == NULL) {
        return vector<File>();
    } else {
        struct dirent *dir;
        while ((dir = readdir(dp)) != NULL) {
            File fd_file;
            string link(dir->d_name);
            if (is_number(link)) {
                string mode = get_from_stat(fd_path + "/" + link, "mode", false, err);
                if (err == 1) return vector<File>();
                fd_file.fd = link + mode;
                fd_file.type = get_from_stat(fd_path + "/" + link, "type", true, err);
                if (err == 1) return vector<File>();
                fd_file.node = get_from_stat(fd_path + "/" + link, "node", true, err);
                if (err == 1) return vector<File>();
                fd_file.name = safe_readlink(fd_path + "/" + link, err);
                if (err == 1) return vector<File>();
                fd_files.push_back(fd_file);
            }
        }
        return fd_files;
    }
}

int iterate_pid(string pid_path, Process &process) {
    int err;

    process.command = get_command(pid_path, err);
    if (err == 1) return 1;

    process.user = get_user(pid_path, err);
    if (err == 1) return 1;

    // cwd, rtd, txt
    vector<string> fds{"cwd", "rtd", "txt"};
    vector<string> filenames{"cwd", "root", "exe"};
    for (int i = 0; i < fds.size(); i++) {
        process.files.push_back(get_special_file(pid_path + "/" + filenames[i], fds[i], err));
        if (err == 1) return err;
    }

    // map
    vector<File> map_files = get_maps(pid_path + "/maps", err);
    if (err == 1) return err;
    process.files.insert(process.files.end(), map_files.begin(), map_files.end());

    // fd
    vector<File> fd_files = iterate_fd(pid_path + "/fd", err);
    process.files.insert(process.files.end(), fd_files.begin(), fd_files.end());

    return 0;
}

int iterate_proc(string proc_path, vector<Process> &processes) {
    DIR *dp = opendir(proc_path.c_str());
    if (dp == NULL) {
        cerr << "can't open /proc.\n";
        exit(1);
    } else {
        struct dirent *dir;
        while ((dir = readdir(dp)) != NULL) {
            string pid(dir->d_name);
            if (is_number(pid)) {
                Process process(pid);
                int err = iterate_pid(proc_path + "/" + pid, process);
                if (err != 1) {
                    processes.push_back(process);
                    update_maxlen(process);
                }
            }
        }
    }
}

void output(vector<Process> processes) {
    for (int i = 0; i < columns.size(); i++) {
        cout << setw(maxlen[columns[i]] + 2) << left << columns[i];
    }
    cout << '\n';

    for (int i = 0; i < processes.size(); i++) {
        Process process = processes[i];
        for (int j = 0; j < process.files.size(); j++) {
            File file = process.files[j];
            vector<string> cols{process.command, process.pid, process.user, file.fd, file.type, file.node, file.name};
            for (int k = 0; k < cols.size(); k++) {
                cout << setw(maxlen[columns[k]] + 2) << left << cols[k];
            }
            cout << '\n';
        }
    }
}

int main(int argc, char *argv[]) {
    Filter f(argc, argv);
    init_maxlen();
    vector<Process> processes;
    iterate_proc("/proc", processes);
    output(processes);
}