#include <iostream>
#include <string>
#include <regex>

using namespace std;

struct filter {
    regex command;
    regex filename;
    string type;
    filter() {
        command = regex(".*");
        filename = regex(".*");
        type = "";
    }
};

int main(int argc, char *argv[]) {
    filter f;
    for (int i = 0; i < argc; i++) {
        if (string(argv[i]) == "-c") {
            
        } else if (string(argv[i]) == "-t") {
            // f.filenameRegex = argv[i++];
        } else if (string(argv[i]) == "-f") {
            // f.typeFilter = argv[i++];
        }
    }
    while (true) {
        string s;
        smatch m;
        cin >> s;
        cout << regex_search(s, m, f.command) << '\n';
    }
}
