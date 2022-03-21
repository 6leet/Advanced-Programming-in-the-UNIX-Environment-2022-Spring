#include <iostream>
#include <string>
#include <regex>
#include <filesystem>

using namespace std;

struct filter {
    regex command;
    regex filename;
    regex type;
    filter() {
        command = regex(".*");
        filename = regex(".*");
        type = regex(".*");
    }
    bool process(string _command, string _filename, string _type) {
        return regex_search(_command, command) && regex_search(_filename, filename) && regex_match(_type, type);
    }
};

filter setFilter(int argc, char *argv[]) {
    filter f;
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

void iterate() {
    filesystem::path path{"/proc"};
    for (auto const& entry : filesystem::directory_iterator{path}) {
        if (entry.is_directory()) {
            cout << filesystem::relative(entry.path(), path).string() << '\n';
        }
    }
}

int main(int argc, char *argv[]) {
    setFilter(argc, argv);
    iterate();
}
