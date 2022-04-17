#include <iostream>
#include <vector>
#include <dlfcn.h>
#include <unistd.h>

#define args(...) __VA_ARGS__
#define func(return_type, func_name, func_arg, para) \
    return_type func_name(func_arg) { \
        return_type ret = call_from_libc(func_name, #func_name, para); \
        cout << "call " << #func_name << '\n'; \
        return ret; \
    } \

using namespace std;

template<typename Func, typename... Args>
auto call_from_libc(Func, string func_name, Args... args) {
    cout << "func_name: " << func_name << '\n';
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    Func libc_func = (Func) dlsym(handle, func_name.c_str());
    return libc_func(args...);
}

extern "C" {
func(int, chmod, args(const char *pathname, mode_t mode), args(pathname, mode))
func(int, chown, args(const char *pathname, uid_t owner, gid_t group), args(pathname, owner, group))
func(int, close, args(int fd), args(fd))
func(int, creat, args(const char *path, mode_t mode), args(path, mode))
func(int, fclose, args(FILE *stream), args(stream))
func(FILE *, fopen, args(const char *pathname, const char *mode), args(pathname, mode))
func(size_t, fread, args(void *ptr, size_t size, size_t nmemb, FILE *stream), args(ptr, size, nmemb, stream))
func(size_t, fwrite, args(const void *ptr, size_t size, size_t nitems, FILE *stream), args(ptr, size, nitems, stream))
func(int, open, args(const char *pathname, int flags, mode_t mode), args(pathname, flags, mode))
func(ssize_t, read, args(int fd, void *buf, size_t count), args(fd, buf, count))
// func(int, remove, args(const char *pathname), args(pathname))
func(int, rename, args(const char *oldpath, const char *newpath), args(oldpath, newpath))
// func(FILE *, tmpfile, args(void), args())
// FILE * tmpfile(void) { call_from_libc(tmpfile, "tmpfile", void); cout << "call " << "tmpfile" << '\n'; }
func(ssize_t, write, args(int fd, const void *buf, size_t count), args(fd, buf, count))
}