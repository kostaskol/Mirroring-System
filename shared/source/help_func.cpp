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
        recv(sockfd, buf, 2, 0);
        buf[2] = '\0';
        bool res = !strcmp(buf, "OK");
        delete[] buf;
        return res;
    }

    void read_fname(int sockfd, my_string *buffer, int max) {
        int overall = 0;
        my_string complete_buf = "";
        int max_bytes = max > 1023 ? 1023 : max;
        do {
            char *buf = new char[1024];
            max_bytes -= overall;
            ssize_t read = recv(sockfd, buf, (uint16_t) max_bytes, 0);
            buf[read] = '\0';
            complete_buf += buf;
            overall += read;
            hf::send_ok(sockfd);
            delete[] buf;
        } while (overall < max);
        *buffer = complete_buf;
    }

    my_string get_dir_path(my_string path) {
		cout << "Dir path " << path << endl;
        my_vector<my_string> path_vec = path.split((char *) "/");
        path_vec.remove_at(path_vec.size() - 1);
        my_string tmp_path = path_vec.join('/');
        return tmp_path;
    }


}
