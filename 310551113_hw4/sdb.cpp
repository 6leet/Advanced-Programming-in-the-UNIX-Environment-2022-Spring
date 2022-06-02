#include <stdio.h>
#include <iomanip>
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
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

// ---head of utils.cpp---

#define LLSIZE 8
#define EPSIZE 8
#define WORD 8

#define NOT_LOADED 0
#define LOADED 1
#define RUNNING 2

#define ERR_WRONGSTATUS -1
#define ERR_LACKARG -2
#define ERR_UNDEFINED -3
#define ERR_BADWAIT -4
#define ERR_NULL -5
#define ERR_BADREG -6

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
    {"vmmap", RUNNING},
    {"set", RUNNING},
    {"si", RUNNING},
    {"start", LOADED}
};

map<string, int> reg_offset = {
    {"r15", R15},
    {"r14", R14},
    {"r13", R13},
    {"r12", R12},
    {"rbp", RBP},
    {"rbx", RBX},
    {"r11", R11},
    {"r10", R10},
    {"r9", R9},
    {"r8", R8},
    {"rax", RAX},
    {"rcx", RCX},
    {"rdx", RDX},
    {"rsi", RSI},
    {"rdi", RDI},
    {"orig_rax", ORIG_RAX},
    {"rip", RIP},
    {"cs", CS},
    {"eflags", EFLAGS},
    {"rsp", RSP},
    {"ss", SS},
    {"fs_base", FS_BASE},
    {"gs_base", GS_BASE},
    {"ds", DS},
    {"es", ES},
    {"fs", FS},
    {"gs", GS}
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

void _help() {
    string help_msg = string("- break {instruction-address}: add a break point\n") + 
                    "- cont: continue execution\n" +
                    "- delete {break-point-id}: remove a break point\n" + 
                    "- disasm addr: disassemble instructions in a file or a memory region\n" +
                    "- dump addr: dump memory content\n" +
                    "- exit: terminate the debugger\n" +
                    "- get reg: get a single value from a register\n" +
                    "- getregs: show registers\n" +
                    "- help: show this message\n" +
                    "- list: list break points\n" +
                    "- load {path/to/a/program}: load a program\n" +
                    "- run: run the program\n" +
                    "- vmmap: show memory layout\n" +
                    "- set reg val: get a single value to a register\n" +
                    "- si: step into instruction\n" +
                    "- start: start the program and stop at the first instruction";
    writemsg(help_msg);
}

int lackarg() {
    debug("Not enough input arguments");
    return ERR_LACKARG;
}

int undefined(string command) {
    debug("Undefined command: \"" + command + "\".  Try \"help\".");
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

string hexify(unsigned long long x) {
    stringstream ss;
    ss << hex << x;
    return "0x" + ss.str();
}

unsigned long long toull(string val) {
    if (val.size() > 2 && val[1] == 'x') {
        return strtoull(val.c_str(), NULL, 16);
    } else {
        return strtoull(val.c_str(), NULL, 10);
    }
}

void padzero(string &s, int n) {
    s.insert(s.begin(), 16 - s.length(), '0');
}

// ---end of utils.cpp---

// ---head of handler.hpp--

string get_entrypoint();

// break
int cont();
// delete
// disasm
int dump(string, int);
void quit(); // exit
int get(string);
int getregs();
int help();
// list
int load(string);
int run();
int vmmap();
int set(string, string);
int si();
int start();

// ---head of handler.cpp---

/*
return if WIFSTOPPED ?
int setbreak(..., int &status) { // old status
    if (status) { // check status

    }
    ... // main stuff
    status = new_status // transform
}
*/

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

int cont() {
    if (!checkstatus("cont")) return ERR_WRONGSTATUS;

    ptrace(PTRACE_CONT, conf.child, 0, 0);

    conf.status = RUNNING;
    return waitstatus();
}

int dump(string addr /* hex */, int bytes=80) {
    long ret;
    unsigned char *ptr = (unsigned char *) &ret;
    string layout, ascii;

    unsigned long long lladdr = strtoull(addr.c_str(), NULL, 16);
    int head = 0x1;
    while (bytes) {
        ret = ptrace(PTRACE_PEEKTEXT, conf.child, lladdr, 0);
        if (head) {
            layout = "\t" + hexify(lladdr) + ":"; ascii = "";
        }

        char buf[1024];
        sprintf(buf, " %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x %2.2x",
                ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
        layout += string(buf);

        for (int i = 0; i < WORD; i++) {
            if (ptr[i] > 127 || ptr[i] < 30) ptr[i] = '.';
            ascii += ptr[i];
        }

        if (!head) {
            writemsg(layout + " |" + ascii + "|");
        }

        lladdr += 8; bytes -= 8;
        head ^= 0x1;
    }
    return 0;
}

void quit() { // exit
    exit(0);
}

int get(string reg) {
    if (!checkstatus("get")) return ERR_WRONGSTATUS;

    if (reg_offset.find(reg) == reg_offset.end()) {
        return ERR_BADREG;
    }
    unsigned long long regval;
    regval = ptrace(PTRACE_PEEKUSER, conf.child, reg_offset[reg] * LLSIZE, 0);
    writemsg(reg + " = " + to_string(regval) + " (" + hexify(regval) + ")");
    return 0;
}

int getregs() {
    if (!checkstatus("getregs")) return ERR_WRONGSTATUS;

    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, conf.child, 0, &regs) == 0) {
        char buf[1024];
        sprintf(buf, 
            "RAX %llx\tRBX %llx\tRCX %llx\tRDX %llx\nR8  %llx\tR9  %llx\tR10 %llx\tR11 %llx\nR12 %llx\tR13 %llx\tR14 %llx\tR15 %llx\nRDI %llx\tRSI %llx\tRBP %llx\tRSP %llx\nRIP %llx\tFLAGS %016llx", 
            regs.rax, regs.rbx, regs.rcx, regs.rdx, regs.r8, regs.r9, regs.r10, regs.r11, regs.r12, regs.r13, regs.r14, regs.r15, regs.rdi, regs.rsi, regs.rbp, regs.rsp, regs.rip, regs.eflags
        );
        writemsg(buf);
        return 0;
    }
    return ERR_NULL;
}

int help() {
    _help();
    return 0;
}

int load(string program) {
    if (!checkstatus("load")) return ERR_WRONGSTATUS;

    conf.program = program;
    string entrypoint = get_entrypoint();
    debug("program '" + conf.program + "' loaded. entry point " + entrypoint);

    conf.status = LOADED;
    return 0;
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

int vmmap() {
    if (!checkstatus("vmmap")) return ERR_WRONGSTATUS;

    string maps = "/proc/" + to_string(conf.child) + "/maps", line;
    ifstream file(maps.c_str());
    while (getline(file, line)) {
        stringstream ss(line);
        string address, perms, offset, dev, node, pathname;
        ss >> address >> perms >> offset >> dev >> node >> pathname;

        // address preprocess
        size_t idx = address.find("-");
        string begin = address.substr(0, idx), end = address.substr(idx + 1, address.size() - 1 - idx);
        padzero(begin, 16); padzero(end, 16);

        // perms preprocess
        perms = perms.substr(0, perms.size() - 1);

        // offset preprocess
        unsigned long long off = strtoull(offset.c_str(), NULL, 16);

        char buf[1024];
        sprintf(buf, "%s-%s %s %-8llx\t%s", begin.c_str(), end.c_str(), perms.c_str(), off, pathname.c_str());
        writemsg(buf);
    }
}

int set(string reg, string val) {
    if (!checkstatus("set")) return ERR_WRONGSTATUS;

    unsigned long long regval = toull(val);
    if (ptrace(PTRACE_POKEUSER, conf.child, reg_offset[reg] * LLSIZE, regval) != 0) errquit("poketext");
    return 0;
}

int si() {
    if (!checkstatus("si")) return ERR_WRONGSTATUS;

    if (ptrace(PTRACE_SINGLESTEP, conf.child, 0, 0) < 0) errquit("ptrace singlestep");
    return waitstatus();
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

        int ret = waitstatus();
        ptrace(PTRACE_SETOPTIONS, conf.child, 0, PTRACE_O_EXITKILL);
        debug("pid " + to_string(conf.child));

        conf.status = RUNNING;
        return ret;
    }
    return ERR_NULL;
}

// ---end of handler.cpp---

// ---head of core.cpp---

int middleware(vector<string> cmd) {
    if (cmd[0] == "break" || cmd[0] == "b") {
        if (cmd.size() < 2) return lackarg();

    } else if (cmd[0] == "cont" || cmd[0] == "c") {
        return cont();
    } else if (cmd[0] == "delete") {
        if (cmd.size() < 2) return lackarg();

    } else if (cmd[0] == "disasm" || cmd[0] == "d") {
        if (cmd.size() < 2) return lackarg();

    } else if (cmd[0] == "dump" || cmd[0] == "x") {
        return dump(cmd[1]);
    } else if (cmd[0] == "exit" || cmd[0] == "q") {
        quit();
    } else if (cmd[0] == "get" || cmd[0] == "g") {
        if (cmd.size() < 2) return lackarg();
        return get(cmd[1]);
    } else if (cmd[0] == "getregs") {
        return getregs();
    } else if (cmd[0] == "help" || cmd[0] == "h") {
        return help();
    } else if (cmd[0] == "list" || cmd[0] == "l") {

    } else if (cmd[0] == "load") {
        if (cmd.size() < 2) return lackarg();
        return load(cmd[1]);
    } else if (cmd[0] == "run" || cmd[0] == "r") {
        return run();
    } else if (cmd[0] == "vmmap" || cmd[0] == "m") {
        return vmmap();
    } else if (cmd[0] == "set" || cmd[0] == "s") {
        if (cmd.size() < 3) return lackarg();
        return set(cmd[1], cmd[2]);
    } else if (cmd[0] == "si") {
        return si();
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

// ---end of core.cpp---