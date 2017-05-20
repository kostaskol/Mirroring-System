#ifndef MIRROR_SERVER_MIRROR_MANAGER_H
#define MIRROR_SERVER_MIRROR_MANAGER_H


#include "my_string.h"
#include "queue.h"

class mirror_manager {
private:
    queue<my_string> *_q;
    my_string _addr;
    int _port;
    my_string _path;
    int _delay;
    int _id;
    bool _init;
    int _sockfd;
    pthread_mutex_t *_rw_mtx, *_full_mtx,
            *_empty_mtx, *_done_mtx;
    pthread_cond_t *_cond;
    pthread_t *_tid;

public:
    mirror_manager(my_vector<my_string> details, int id, queue<my_string> *q,
                   pthread_mutex_t *e_mtx, pthread_mutex_t *f_mtx,
                   pthread_mutex_t *rw_mtx, pthread_mutex_t *d_mtx, pthread_cond_t *cond, pthread_t *tid);

    bool init();

    void run();
};


#endif //MIRROR_SERVER_MIRROR_MANAGER_H
