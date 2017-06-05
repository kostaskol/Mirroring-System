#ifndef MIRROR_SERVER_WORKER_H
#define MIRROR_SERVER_WORKER_H

#include <pthread.h>
#include "my_string.h"
#include "queue.h"
#include "mirror_server.h"

class worker {
private:
    queue<my_string> *_q;
    pthread_mutex_t *_e_mtx, *_f_mtx, *_rw_mtx, *_b_mtx, *_file_mtx, 
		*_q_done_mtx, *_ack_mtx, *_spread_mtx;
		
	pthread_cond_t *_e_cond, *_f_cond, *_q_done_cond, *_ack_cond;
	
	bool *_empty, *_full, *_done, *_ack;
	
	int *_q_done;
    my_string _path;
    int _sockfd;
	int *_bytes_total, *_files_total;
	
	my_vector<int> *_spread;
	
    void _get_info(my_string *path, my_string *addr, int *port, int *id, my_string &vec);
    bool _make_con(my_string addr, int port);
    bool _fetch(my_string path, my_string addr, int port, int id);
public:
    worker(queue<my_string> *q, pthread_mutex_t *e_mtx, 
	pthread_mutex_t *f_mtx, pthread_mutex_t *rw_mtx, 
	pthread_mutex_t *bytes_mtx, pthread_mutex_t *files_mtx,
	pthread_mutex_t *d_mtx, pthread_mutex_t *q_done_mtx, 
	pthread_mutex_t *ack_mtx, pthread_mutex_t *spread_mtx, 
	pthread_cond_t *q_done_cond, pthread_cond_t *e_cond, pthread_cond_t *f_cond,
	pthread_cond_t *ack_cond, bool *done, int *q_done, bool *empty, bool *full, 
	bool *ack, int *bytes_total, int *files_total, my_string path, 
	my_vector<int> *spread);

    void run();

};


#endif //MIRROR_SERVER_WORKER_H
