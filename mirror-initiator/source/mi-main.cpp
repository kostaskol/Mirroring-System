#include "cmd_parser.h"
#include "constants.h"
#include "mirror_init.h"

int main(int argc, char **argv) {
	// Parse the command line arguments
    cmd_parser cmd(MODE_MI);
    cmd.parse(argc, argv);

    mirror_init initiator(&cmd);
	// Explicitly initialise the mirror-initiator (connect to the server)
    initiator.init();
	// Run the mirror-initiator
    initiator.run();
}
