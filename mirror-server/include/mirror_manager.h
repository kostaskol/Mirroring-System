#ifndef MIRROR_SERVER_MIRROR_MANAGER_H
#define MIRROR_SERVER_MIRROR_MANAGER_H


#include "my_string.h"
#include "queue.h"

typedef bool (my_string::*resolver)(const char *);
class mirror_manager {
private:
    queue<my_string> *_q;
    my_string _addr;
    int _port;
    my_string _path;
    int _delay;
    bool _init;
    int _sockfd;
	bool _debug;
	int _id;
	// Thread variables
    pthread_mutex_t *_rw_mtx, *_f_mtx, *_e_mtx;
    pthread_cond_t *_e_cond, *_f_cond;
	
	resolver _resolver;
	
	bool *_full, *_empty;
public:
    mirror_manager(my_vector<my_string> details, int id, queue<my_string> *q,
                   pthread_mutex_t *e_mtx, pthread_mutex_t *f_mtx,
                   pthread_mutex_t *rw_mtx, pthread_cond_t *e_cond, pthread_cond_t *f_cond, 
				   bool *full, bool *empty, bool debug, bool search);

    bool init();

    void run();
	
	my_string get_addr();
	
	int get_port();
};


#endif //MIRROR_SERVER_MIRROR_MANAGER_H
