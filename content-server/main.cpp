#include <iostream>
#include "cmd_parser.h"
#include "constants.h"
#include "my_vector.h"
#include "content_server.h"

using namespace std;

int main(int argc, char **argv) {
    cmd_parser cmd(MODE_CS);
    cmd.parse(argc, argv);

    content_server server(&cmd);
    server.init();
    server.run();

}