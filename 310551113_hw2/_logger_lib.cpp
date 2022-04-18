#include <string>
#include <dlfcn.h>

using namespace std;

template<typename Func, typename... Args>
auto call_from_libc(Func _, string func_name, Args... args) {
    void *handle = dlopen("libc.so.6", RTLD_LAZY);
    static Func libc_func = (Func) dlsym(handle, func_name.c_str());
    return libc_func(args...);
}

int fclose(FILE *stream) {
    int ret = call_from_libc(fclose, __func__, stream);
    return ret;
}

FILE* fopen(const char *pathname, const char *mode) {
    FILE* ret = call_from_libc(fopen, __func__, pathname, mode);
    return ret;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t ret = call_from_libc(fread, __func__, ptr, size, nmemb, stream);
    return ret;
}

size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream) {
    size_t ret = call_from_libc(fwrite, __func__, ptr, size, nitems, stream);
    return ret;
}