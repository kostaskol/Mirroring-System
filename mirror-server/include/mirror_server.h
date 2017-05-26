#ifndef MIRROR_SERVER_MIRROR_SERVER_H
#define MIRROR_SERVER_MIRROR_SERVER_H

#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "queue.h"
#include "cmd_parser.h"
#include "mirror_manager.h"

struct stats {
    int bytes;
    int files;
};

class mirror_server {
private:
    int _port;
    int _worker_num;
    my_string _outp_path;
    pthread_t *_workers;
    queue<my_string> _data_queue;
    pthread_t *_managers;
    int _sockfd;
    pthread_mutex_t _rw_mtx, _f_mtx, _e_mtx, _q_done_mtx, _bytes_mtx, 
	_file_mtx, _done_mtx, _ack_mtx;
    pthread_cond_t _e_cond, _f_cond, _q_done_cond, _ack_cond;
	
	int _bytes_recvd, _files_recvd;	
	
	bool _empty, _full, _done, _ack;
	
	int _q_done;
	
	bool _debug;
	bool _search;
	my_vector<my_string> _down_serv;

    void _mtx_init();
public:
    mirror_server(cmd_parser *args);

    void init();

    void run();

};

void *start_manager(void *arg);
void *start_worker(void *arg);
#endif //MIRROR_SERVER_MIRROR_SERVER_H
