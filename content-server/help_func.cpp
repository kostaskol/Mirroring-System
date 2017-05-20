#include <dirent.h>
#include "help_func.h"
#include <sys/socket.h>
#include "constants.h"
#include <iostream>
#include <cstring>
#include "constants.h"

using namespace std;

namespace hf {
    void send_num_blocks(int sockfd, int loops) {
        my_string loop_str = loops;
        send(sockfd, loop_str.c_str(), loop_str.length(), 0);
    }

    int recv_num_blocks(int sockfd) {
        char *buffer = new char[BLOCK_SIZE];
        ssize_t read = recv(sockfd, buffer, BLOCK_STR_SIZE, 0);
        buffer[read] = '\0';
        int loops = atoi(buffer);
        send(sockfd, "OK", 2, 0);
        delete[] buffer;
        return loops;
    }

    void send_ok(int sockfd) {
        send(sockfd, "OK", 2, 0);
    }

    bool recv_ok(int sockfd) {
        char *buf = new char[3];
        if (recv(sockfd, buf, 2, 0) == 0) {
          cout << "The other side has closed the connection!" << endl;
          delete[] buf;
          return false;
        }
        buf[2] = '\0';
        bool res = !strcmp(buf, "OK");
        delete[] buf;
        return res;
    }


}
