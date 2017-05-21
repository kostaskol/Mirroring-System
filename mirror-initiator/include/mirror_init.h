#ifndef MIRROR_INITIATOR_MIRROR_INIT_H
#define MIRROR_INITIATOR_MIRROR_INIT_H


#include "my_string.h"
#include "cmd_parser.h"

/**
 *	Mirror Initiator class which handles all of the initiator's operations
 * (Send the data to the Mirror Server, await for its response and print 
 *  that data to the user)
 */
class mirror_init {
private:
    my_vector<my_vector<my_string>> _cservers;
    my_string _address;
    int _port;
    bool _init;
    int _sockfd;
public:
    mirror_init(cmd_parser *args);

    void init();

    void run();
};


#endif //MIRROR_INITIATOR_MIRROR_INIT_H
