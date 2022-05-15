#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

string usage_massage = string("usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]") +
                        "\n\t-p: set the path to logger.so, default = ./logger.so" +
                        "\n\t-o: print output to file, print to \"stderr\" if no file specified" +
                        "\n\t--: separate the arguments for logger and for the command\n";

void cmd_parser(int argc, char *argv[], string &so_path, string &output_file, vector<string> &cmd) {
    char opt;
    while ((opt = getopt(argc, argv, "p:o:")) != -1) {
        switch (opt) {
            case 'p':
                so_path = string(optarg);
                break;
            case 'o':
                output_file = string(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "%s", usage_massage.c_str());
                exit(EXIT_FAILURE);
        }
    }
    for (int i = optind; i < argc; i++) {
        cmd.push_back(string(argv[i]));
    }
}

string concat_cmd(string so_path, vector<string> &cmd) {
    string logger_cmd = "LD_PRELOAD=" + so_path + " ";
    for (int i = 0; i < cmd.size(); i++) {
        logger_cmd += cmd[i] + ' ';
    }
    return logger_cmd;
}

void output(string output_file) {
    if (output_file.size() != 0) {
        int fd = open(output_file.c_str(), O_RDWR | O_CREAT, 0644);
        dup2(fd, 5);
        close(fd);
    } else {
        dup2(2, 5);
    }
}

int main(int argc, char *argv[]) {
    string so_path = "./logger.so", output_file = "";
    vector<string> cmd;
    cmd_parser(argc, argv, so_path, output_file, cmd);

    output(output_file);
    system(concat_cmd(so_path, cmd).c_str());

    return 0;
}
