#include <iostream>
#include <string>
#include <vector>

using namespace std;

string usage_massage = string("usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]") + 
                        "\n\t-p: set the path to logger.so, default = ./logger.so" + 
                        "\n\t-o: print output to file, print to \"stderr\" if no file specified" + 
                        "\n\t--: separate the arguments for logger and for the command";

int cmd_parser(int argc, char *argv[], string &so_path, string &output_file, vector<string> &cmd) {
    bool is_cmd = false;
    for (int i = 1; i < argc; i++) {
        string str_arg = string(argv[i]);
        if (str_arg == "-p") {
            so_path = string(argv[++i]);
        } else if (str_arg == "-o") {
            output_file = string(argv[++i]);
        } else if (str_arg == "--") {
            is_cmd = true;
        } else {
            if (is_cmd) {
                cmd.push_back(string(argv[i]));
            } else {
                cout << usage_massage << '\n';
                return 1;
            }
        }
    }
    return 0;
}

string concat_cmd(string so_path, vector<string> &cmd) {
    string logger_cmd = "LD_PRELOAD=" + so_path + " ";
    for (int i = 0; i < cmd.size(); i++) {
        logger_cmd += cmd[i];
    }
    return logger_cmd;
}

int main(int argc, char *argv[]) {
    string so_path = "./logger.so", output_file = "";
    vector<string> cmd;
    if (cmd_parser(argc, argv, so_path, output_file, cmd) == 1) {
        return 0;
    }

    system(concat_cmd(so_path, cmd).c_str());
}