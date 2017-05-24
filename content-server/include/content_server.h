#ifndef CONTENT_SERVER_CONTENT_SERVER_H
#define CONTENT_SERVER_CONTENT_SERVER_H


#include "my_string.h"
#include "cmd_parser.h"
#include "hash_table.h"
#include "queue.h"

class content_server {
private:
    my_string _path;
    int _port;
    int _sockfd;
    bool _init;
	queue<int> _queue;
	pthread_mutex_t _q_mtx, _h_mtx, _f_mtx, _e_mtx, _stp_mtx;
	
	int _thread_num;
	
	bool _debug;

    hash_table<int> _h_table;
public:
    content_server(cmd_parser *parser);

    void init();

    void run();
};

void *manager_starter(void *arg);

#endif //CONTENT_SERVER_CONTENT_SERVER_H
