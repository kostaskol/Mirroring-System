#include <dirent.h>
#include "help_func.h"
#include <sys/socket.h>
#include "constants.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include "constants.h"

using namespace std;

namespace hf {
    void send_num_blocks(int sockfd, int loops) {
		// Send an integer over TCP
        my_string loop_str = loops;
        send(sockfd, loop_str.c_str(), loop_str.length(), 0);
    }

    int recv_num_blocks(int sockfd) {
		// Receives an integer over TCP
		// (also sends the acknowledgement signal)
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
		// Reads a string over TCP
		// (splits it into 1024 bytes blocks)
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
	
	void mk_path(my_string path) {
		// Creates all of the directories in a path
		// NOTE: It doesn't create the last
		// part of the path (even if it is a directory)
		my_vector<my_string> parts = path.split("/");
		for (int part = 0; part < (int) parts.size() - 1; part++) {
			my_string path_so_far = "";
			for (int i = 0; i <= part; i++) {
				path_so_far += parts.at(i);
				path_so_far += "/";
			}
			mkdir(path_so_far.c_str(), 0777);
		}
	}

}
