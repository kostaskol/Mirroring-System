#ifndef VARIOUS_TESTS_HELPFUNC_H_H
#define VARIOUS_TESTS_HELPFUNC_H_H

#include "my_string.h"

namespace hf {
    void send_num_blocks(int sockfd, int loops);

    int recv_num_blocks(int sockfd);

    void send_ok(int sockfd);

    bool recv_ok(int sockfd);

    void read_fname(int sockfd, my_string *buffer, int max);

    void mk_path(my_string path);
}

#endif //VARIOUS_TESTS_HELPFUNC_H_H
