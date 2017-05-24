#include <pthread.h>
#include "queue.h"
#include "my_vector.h"
#include "my_string.h"
#include "hash_table.h"
#include "help_func.h"
#include "constants.h"

class content_manager {
private:
	pthread_mutex_t *_q_mtx, *_h_mtx, *_e_mtx, *_f_mtx;
	pthread_cond_t *_e_cond, *_f_cond;
	queue<int> *_q;
	hash_table<int> *_h_table;
	my_string _path;
	bool *_empty;
	bool *_full;
	bool _debug;
	
	void _get_contents_cond(my_vector<my_string> *v, my_string path, 
		my_string cond);

    void _get_list(my_string path, my_vector<my_string> *v);

    void _do_list(int clientfd);

    void _do_fetch(int clientfd, my_string path);
public:
	content_manager(pthread_mutex_t *mtx, pthread_mutex_t *h_mtx,
		pthread_mutex_t *e_mtx, pthread_mutex_t *f_mtx,
		pthread_cond_t *e_cond, pthread_cond_t *f_cond,
		queue<int> *q, hash_table<int> *hash, my_string path, bool *empty, 
		bool *full, bool debug);
		
	content_manager(const content_manager &)=delete;
	
	void run();
};
