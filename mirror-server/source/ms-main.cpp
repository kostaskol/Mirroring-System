#include <iostream>
#include "cmd_parser.h"
#include "constants.h"
#include "mirror_server.h"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
    cmd_parser cmd(MODE_MS);
    cmd.parse(argc, argv);

    mirror_server server(&cmd);
    server.init();
    server.run();

}