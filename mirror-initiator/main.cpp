#include <iostream>
#include "cmd_parser.h"
#include "constants.h"
#include "mirror_init.h"

int main(int argc, char **argv) {
    cmd_parser cmd(MODE_MI);
    cmd.parse(argc, argv);

    mirror_init initiator(&cmd);
    initiator.init();
    initiator.run();
}