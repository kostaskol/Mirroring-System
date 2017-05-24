#ifndef MIRROR_SERVER_WORKER_H
#define MIRROR_SERVER_WORKER_H

#include <pthread.h>
#include "my_string.h"
#include "queue.h"
#include "mirror_server.h"

class worker {
private:
    queue<my_string> *_q;
    pthread_mutex_t *_empty_mtx, *_full_mtx, *_rw_mtx;
    my_string _path;
    int _sockfd;
    bool *_done;
    int _bytes_recvd, _files_recvd;

    void _get_info(my_string *path, my_string *addr, int *port, int *id, my_string &vec);
    bool _make_con(my_string addr, int port);
    bool _fetch(my_string path, my_string addr, int port, int id);
public:
    worker(queue<my_string> *q, pthread_mutex_t *e_mtx,
           pthread_mutex_t *f_mtx, pthread_mutex_t *rw_mtx, my_string path, 
		   bool *done);

    stats *run();

};


#endif //MIRROR_SERVER_WORKER_H
