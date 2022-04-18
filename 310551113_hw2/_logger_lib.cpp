#include <string>
#include <dlfcn.h>
#include "utils.hpp"

using namespace std;

template<typename Func, typename... Args>
auto call_from_libc(Func _, string func_name, Args... args) {
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    static Func libc_func = (Func) dlsym(handle, func_name.c_str());
    return libc_func(args...);
}

int chmod(const char *pathname, mode_t mode) {
    log_func_name(__func__);
    log_arg("file_name", pathname);
    log_arg("mode", mode, true);
    int ret = call_from_libc(chmod, __func__, pathname, mode);
    log_ret(ret, false);
    return ret;
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    log_func_name(__func__);
    log_arg("file_name", pathname);
    log_arg("", owner);
    log_arg("", group, true);
    int ret = call_from_libc(chown, __func__, pathname, owner, group);
    log_ret(ret, false);
    return ret;
}

int close(int fd) {
    log_func_name(__func__);
    log_arg("fd", fd, true);
    int ret = call_from_libc(close, __func__, fd);
    log_ret(ret, false);
    return ret;
}

int creat(const char *path, mode_t mode) {
    log_func_name(__func__);
    log_arg("file_name", path);
    log_arg("mode", mode, true);
    int ret = call_from_libc(creat, __func__, path, mode);
    log_ret(ret, false);
    return ret;
}

int creat64(const char *path, mode_t mode) {
    log_func_name(__func__);
    log_arg("file_name", path);
    log_arg("mode", mode, true);
    int ret = call_from_libc(creat64, __func__, path, mode);
    log_ret(ret, false);
    return ret;
}

int fclose(FILE *stream) {
    log_func_name(__func__);
    log_arg("file_pointer", stream, true);
    int ret = call_from_libc(fclose, __func__, stream);
    log_ret(ret, false);
    return ret;
}

FILE* fopen(const char *pathname, const char *mode) {
    log_func_name(__func__);
    log_arg("file_name", pathname);
    log_arg("mode", mode, true);
    FILE* ret = call_from_libc(fopen, __func__, pathname, mode);
    log_ret(ret, true);
    return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    log_func_name(__func__);
    log_arg("", ptr);
    log_arg("", size);
    log_arg("", nmemb);
    log_arg("file_pointer", stream, true);
    size_t ret = call_from_libc(fread, __func__, ptr, size, nmemb, stream);
    log_ret(ret, false);
    return ret;
}

size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream) {
    log_func_name(__func__);
    log_arg("", ptr);
    log_arg("", size);
    log_arg("", nitems);
    log_arg("file_pointer", stream, true);
    size_t ret = call_from_libc(fwrite, __func__, ptr, size, nitems, stream);
    log_ret(ret, false);
    return ret;
}

int open(const char *pathname, int flags, mode_t mode) {
    int ret = call_from_libc(open, __func__, pathname, flags, mode);
    log_func_name(__func__);
    log_arg("file_name", pathname);
    log_arg("flag", flags);
    log_arg("mode", mode, true);
    log_ret(ret, false);
    return ret;
}

int open64(const char *pathname, int flags, mode_t mode) {
    log_func_name(__func__);
    log_arg("file_name", pathname);
    log_arg("flag", flags);
    log_arg("mode", mode, true);
    int ret = call_from_libc(open64, __func__, pathname, flags, mode);
    log_ret(ret, false);
    return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
    log_func_name(__func__);
    log_arg("fd", fd);
    log_arg("", buf);
    log_arg("", count, true);
    int ret = call_from_libc(read, __func__, fd, buf, count);
    log_ret(ret, false);
    return ret;
}

int rename(const char *oldpath, const char *newpath) {
    log_func_name(__func__);
    log_arg("file_name", oldpath);
    log_arg("file_name", newpath, true);
    int ret = call_from_libc(rename, __func__, oldpath, newpath);
    log_ret(ret, false);
    return ret;
}

FILE* tmpfile() {
    log_func_name(__func__);
    FILE* ret = call_from_libc(tmpfile, __func__);
    log_ret(ret, true);
    return ret;
}

FILE* tmpfile64() {
    log_func_name(__func__);
    FILE* ret = call_from_libc(tmpfile64, __func__);
    log_ret(ret, true);
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
    log_func_name(__func__);
    log_arg("fd", fd);
    log_arg("", buf);
    log_arg("", count, true);
    int ret = call_from_libc(write, __func__, fd, buf, count);
    log_ret(ret, false);
    return ret;
}