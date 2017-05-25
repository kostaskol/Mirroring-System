#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <dirent.h>
#include <stdexcept>
#include <signal.h>
#include "content_server.h"
#include "content_manager.h"

using namespace std;

content_server::content_server(cmd_parser *parser) {
    _path = parser->get_path_name();
    _port = parser->get_port();
	_thread_num = parser->get_thread_num();
    _sockfd = 0;
    _init = false;
	_debug = parser->is_debug();
	pthread_mutex_init(&_q_mtx, nullptr);
	pthread_mutex_init(&_h_mtx, nullptr);
	pthread_mutex_init(&_f_mtx, nullptr);
	pthread_mutex_init(&_e_mtx, nullptr);
	pthread_cond_init(&_e_cond, nullptr);
	pthread_cond_init(&_f_cond, nullptr);
	
	_empty = true;
	_full = false;
}

void content_server::init() {
    _init = true;

    struct sockaddr_in server;

    if ((_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        exit(-1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t) _port);
    server.sin_addr.s_addr = INADDR_ANY;

    bzero(&server.sin_zero, 8);

    socklen_t len = sizeof(struct sockaddr_in);

    if ((bind(_sockfd, (sockaddr *) &server, len)) < 0) {
        perror("Bind");
        exit(-1);
    }
}

void content_server::run() {
	signal(SIGPIPE, SIG_IGN);
    if (!_init) {
        cout << "Content Server was not initialised. Exiting.." << endl;
        exit(-1);
    }

    struct sockaddr_in client;

	// We allow thread_num open connections at a time
    if ((listen(_sockfd, _thread_num)) < 0) {
        perror("Listen");
        exit(-1);
    }
	// Create <thread_num> threads that will handle all connections
	pthread_t *tids = new pthread_t[_thread_num];
	cout << "Master full " << &_full << endl;
	for (int i = 0; i < _thread_num; i++) {
		cout << "Master : " << " creating new thread" << endl;
		content_manager *man = new content_manager(&_q_mtx, &_h_mtx, &_e_mtx,
							&_f_mtx, &_e_cond, &_f_cond, &_queue, &_h_table,
							_path, &_empty, &_full, _debug);
							
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
		pthread_create(&tids[i], &attr, manager_starter, man);
	}

	do {
        int client_fd;
        socklen_t len = sizeof(struct sockaddr_in);
        if ((client_fd = accept(_sockfd, (sockaddr *) &client, &len)) < 0) {
            perror("Accept");
            exit(-1);
        }
		cout << "Thread #" << pthread_self() << " just accepted an fd" << endl; 
	
		// Wait until the queue is not full

		// TODO: Make sure we are safe to unlock it here
		// At this point we are already holding the full mutex
		cout << "Master : Waiting on queue" << endl;
		pthread_mutex_lock(&_q_mtx);
		{
			cout << "Pushing connection" << endl;
			_queue.push(client_fd);
			
			// Make sure that the queue isn't full
			_full = _queue.full();
			cout << "Queue is " << _full << endl;
		}
		pthread_mutex_unlock(&_q_mtx);
		cout << "Locking _e_mtx" << endl;
		pthread_mutex_lock(&_e_mtx);
		{
			cout << "Lowered empty flag" << endl;
			_empty = false;
			pthread_cond_signal(&_e_cond);
		}
		pthread_mutex_unlock(&_e_mtx);
		
		pthread_mutex_lock(&_f_mtx);
		{
			while (_full) {
				pthread_cond_wait(&_f_cond, &_f_mtx);
			}
		}
		pthread_mutex_unlock(&_f_mtx);
        
    } while(true);
}

void *manager_starter(void *arg) {
	pthread_detach(pthread_self());
	content_manager *man = (content_manager*) arg;
	man->run();
	delete man;
	cout << "Thread #" << pthread_self() << " exiting" << endl;
	pthread_exit(nullptr);
}
