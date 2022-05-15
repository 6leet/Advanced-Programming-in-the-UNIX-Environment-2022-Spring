#include "utils.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <climits>
#include <gnu/lib-names.h>
#include <string>
#include <dlfcn.h>
#include <unistd.h>
#include <sstream>
#include <cstring>

#define FD 5

using namespace std;
using namespace logger;

template<typename Func, typename... Args>
auto call_from_libc(Func _, string func_name, Args... args) {
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    static Func libc_func = (Func) dlsym(handle, func_name.c_str());
    return libc_func(args...);
}

extern "C"
{
    FILE *fopen(const char *pathname, const char *mode);
    size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
    int fclose(FILE *stream);
    int chmod(const char *path, mode_t mode);
    int chown(const char *pathname, uid_t owner, gid_t group);
    int rename(const char *oldpath, const char *newpath);
    int open(const char *pathname, int flags, mode_t mode);
    ssize_t write(int fd, const void *buf, size_t count);
    ssize_t read(int fd, void *buf, size_t count);
    int close(int fd);
    int creat(const char *path, mode_t mode);
    int remove(const char *pathname);
    FILE *tmpfile(void);
}

int chmod(const char *pathname, mode_t mode) {
    int ret = call_from_libc(chmod, __func__, pathname, mode);
    dprintf(FD, "[logger] %s(%s, %s) = %d\n",
            __func__,
            resolve("file_name", pathname).c_str(),
            resolve("mode", mode).c_str(),
            ret
        );
    return ret;
}

int chown(const char *pathname, uid_t owner, gid_t group) {
    int ret = call_from_libc(chown, __func__, pathname, owner, group);
    dprintf(FD, "[logger] %s(%s, %s, %s) = %d\n",
            __func__,
            resolve("file_name", pathname).c_str(),
            resolve("", owner).c_str(),
            resolve("", group).c_str(),
            ret
        );
    return ret;
}

int close(int fd) {
    int ret = call_from_libc(close, __func__, fd);
    dprintf(FD, "[logger] %s(%s) = %d\n",
            __func__,
            resolve("fd", fd).c_str(),
            ret
        );
    return ret;
}

int creat(const char *path, mode_t mode) {
    int ret = call_from_libc(creat, __func__, path, mode);
    dprintf(FD, "[logger] %s(%s, %s) = %d\n",
            __func__,
            resolve("file_name", path).c_str(),
            resolve("mode", mode).c_str(),
            ret
        );
    return ret;
}

int creat64(const char *path, mode_t mode) {
    int ret = call_from_libc(creat64, __func__, path, mode);
    dprintf(FD, "[logger] %s(%s, %s) = %d\n",
            __func__,
            resolve("file_name", path).c_str(),
            resolve("mode", mode).c_str(),
            ret
        );
    return ret;
}

int fclose(FILE *stream) {
    string resolve_stream = resolve("file_pointer", stream);
    int ret = call_from_libc(fclose, __func__, stream);
    dprintf(FD, "[logger] %s(%s) = %d\n",
            __func__,
            resolve_stream.c_str(),
            ret
        );
    return ret;
}

FILE* fopen(const char *pathname, const char *mode) {
    FILE* ret = call_from_libc(fopen, __func__, pathname, mode);
    dprintf(FD, "[logger] %s(%s, %s) = %p\n",
            __func__,
            resolve("file_name", pathname).c_str(),
            resolve("mode", mode).c_str(),
            ret
        );
    return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t ret = call_from_libc(fread, __func__, ptr, size, nmemb, stream);
    dprintf(FD, "[logger] %s(%s, %s, %s, %s) = %d\n",
            __func__,
            resolve("", ptr).c_str(),
            resolve("", size).c_str(),
            resolve("", nmemb).c_str(),
            resolve("file_pointer", stream).c_str(),
            ret
        );
    return ret;
}

size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream) {
    size_t ret = call_from_libc(fwrite, __func__, ptr, size, nitems, stream);
    dprintf(FD, "[logger] %s(%s, %s, %s, %s) = %d\n",
            __func__,
            resolve("", ptr).c_str(),
            resolve("", size).c_str(),
            resolve("", nitems).c_str(),
            resolve("file_pointer", stream).c_str(),
            ret
        );
    return ret;
}

int open(const char *pathname, int flags, mode_t mode) {
    int ret = call_from_libc(open, __func__, pathname, flags, mode);
    dprintf(FD, "[logger] %s(%s, %s, %s) = %d\n",
            __func__,
            resolve("file_name", pathname).c_str(),
            resolve("flag", flags).c_str(),
            resolve("mode", mode).c_str(),
            ret
        );
    return ret;
}

int open64(const char *pathname, int flags, mode_t mode) {
    int ret = call_from_libc(open64, __func__, pathname, flags, mode);
    dprintf(FD, "[logger] %s(%s, %s, %s) = %d\n",
            __func__,
            resolve("file_name", pathname).c_str(),
            resolve("flag", flags).c_str(),
            resolve("mode", mode).c_str(),
            ret
        );
    return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
    int ret = call_from_libc(read, __func__, fd, buf, count);
    dprintf(FD, "[logger] %s(%s, %s, %s) = %d\n",
            __func__,
            resolve("fd", fd).c_str(),
            resolve("", buf).c_str(),
            resolve("", count).c_str(),
            ret
        );
    return ret;
}

int rename(const char *oldpath, const char *newpath) {
    int ret = call_from_libc(rename, __func__, oldpath, newpath);
    dprintf(FD, "[logger] %s(%s, %s) = %d\n",
            __func__,
            resolve("file_name", oldpath).c_str(),
            resolve("file_name", newpath).c_str(),
            ret
        );
    return ret;
}

FILE* tmpfile() {
    FILE* ret = call_from_libc(tmpfile, __func__);
    dprintf(FD, "[logger] %s() = %d\n",
            __func__,
            ret
        );
    return ret;
}

FILE* tmpfile64() {
    FILE* ret = call_from_libc(tmpfile64, __func__);
    dprintf(FD, "[logger] %s() = %d\n",
            __func__,
            ret
        );
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
    int ret = call_from_libc(write, __func__, fd, buf, count);
    dprintf(FD, "[logger] %s(%s, %s, %s) = %d\n",
            __func__,
            resolve("fd", fd).c_str(),
            resolve("", buf).c_str(),
            resolve("", count).c_str(),
            ret
        );
    return ret;
}
