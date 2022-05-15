#include <cstdio>
#include <limits.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sstream>

namespace logger {
using namespace std;

string log_buf = "";

string quote(string s) {
    for (int i = 0; i < s.size(); i++) {
        if (!isprint(s[i])) {
            s[i] = '.';
        }
    }
    return '\"' + s + '\"';
}

string safe_readlink(string link_path) {
    char buf[1024];
    ssize_t len;
    if ((len = readlink(link_path.c_str(), buf, sizeof(buf) - 1)) != -1) {
        buf[len] = '\0';
        return string(buf);
    }
    return link_path;
}

string resolve(string type, string file_name) {
    char buf[4096] = {0};
    if (realpath(file_name.c_str(), buf) != NULL) {
        return quote(buf);
    }
    return quote(file_name);
}

string resolve(string type, FILE* file_pointer) {
    string link_path = "/proc/self/fd/" + to_string(file_pointer->_fileno);
    return quote(safe_readlink(link_path));
}

string resolve(string type, const void* arg) {
    return quote(string((const char *)arg));
}

string resolve(string type, int arg) {
    if (type == "fd") {
        string link_path = "/proc/self/fd/" + to_string(arg);
        return quote(safe_readlink(link_path));
    } else if (type == "mode" || type == "flag") {
        char buf[1024] = {0};
        sprintf(buf, "%03o", arg);
        return buf;
    } else {
        return to_string(arg);
    }
}

void log_func_name(string func_name) {
    log_buf = "";
    // dprintf(FD, "%s\n", func_name.c_str());
    char buf[4096] = {0};
    sprintf(buf, "[logger] %s(", func_name.c_str());
    log_buf += string(buf);
}

void log_arg(string type, auto arg, bool is_last=false) {
    char buf[4096] = {0};
    sprintf(buf, "%s", resolve(type, arg).c_str());
    log_buf += string(buf);
    if (!is_last) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, ", ");
        log_buf += string(buf);
    }
}

void log_ret(auto ret, bool is_ptr) {
    char buf[4096] = {0};
    if (is_ptr) {
        sprintf(buf, ") = %p\n", ret);
    } else {
        sprintf(buf, ") = %ld\n", ret);
    }
    // dprintf(FD, "%s", log_buf.c_str());
}
}
