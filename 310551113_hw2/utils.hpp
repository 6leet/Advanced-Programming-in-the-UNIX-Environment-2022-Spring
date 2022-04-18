#include <limits.h>
#include <unistd.h>
#include <string>

using namespace std;

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
    printf("[logger] %s(", func_name.c_str());
}

void log_arg(string type, auto arg, bool is_last=false) {
    // if (type == "file_name") {
        // printf("%s", resolve(arg).c_str());
    // } else if (type == "file_pointer") {
        // printf("%s", resolve(arg).c_str());
    // } else if (type == "mode") {
    printf("%s", resolve(type, arg).c_str());
    // }
    if (!is_last) {
        printf(", ");
    }
}

void log_ret(auto ret, bool is_ptr) {
    if (is_ptr) {
        printf(") = %p\n", ret);
    } else {
        printf(") = %ld\n", ret);
    }
}