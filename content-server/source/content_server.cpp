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
#include "content_server.h"
#include "content_manager.h"

using namespace std;


content_server::content_server(cmd_parser *parser) {
    _path = parser->get_path_name();
    _port = parser->get_port();
	_thread_num = parser->get_thread_num();
    _sockfd = 0;
    _init = false;
	
	pthread_mutex_init(&_q_mtx, nullptr);
	pthread_mutex_init(&_h_mtx, nullptr);
	pthread_mutex_init(&_f_mtx, nullptr);
	pthread_mutex_init(&_e_mtx, nullptr);
	
	pthread_mutex_lock(&_e_mtx);
	
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
    if (!_init) {
        cout << "Content Server was not initialised. Exiting.." << endl;
        exit(-1);
    }

    struct sockaddr_in client;

    if ((listen(_sockfd, 5)) < 0) {
        perror("Listen");
        exit(-1);
    }
	// Create <thread_num> threads that will handle all connections
	pthread_t *tids = new pthread_t[_thread_num];
	for (int i = 0; i < _thread_num; i++) {
		content_manager *man = new content_manager(&_q_mtx, &_h_mtx, &_e_mtx,
							&_f_mtx, &_queue, &_h_table, _path);
							
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
		pthread_create(&tids[i], &attr, manager_starter, man);
	}
	// What *could* happen is:
	// have 5 threads that will all listen to this socket and they will all 
	// handle the requests (no need for a queue) 
	// Branch and try
    do {
        int client_fd;
        socklen_t len = sizeof(struct sockaddr_in);
        if ((client_fd = accept(_sockfd, (sockaddr *) &client, &len)) < 0) {
            perror("Accept");
            exit(-1);
        }
		
	
		// Wait until the queue is not full
		cout << "Master : Waiting on full" << endl;
		bool full;
		pthread_mutex_lock(&_f_mtx);
		{
			// Lock the queue
			cout << "Master : Waiting on queue" << endl;
			pthread_mutex_lock(&_q_mtx);
			{
				// Push the new client fd into the queue
				_queue.push(client_fd);
				// Check whether the last push filled the queue
				// If it has, stop (ourselves) from pushing into the queue
				// Otherwise, allow pushing
				full = _queue.full();
			}
			cout << "Master : Unlocking queue" << endl;
			pthread_mutex_unlock(&_q_mtx);
		}
		cout << "Master unlocking empty" << endl;
		pthread_mutex_unlock(&_e_mtx);
		if (full) {
			cout << "Master locking full" << endl;
			pthread_mutex_lock(&_f_mtx);
			cout << "Master locked full" << endl;
		} else {
			cout << "Master unlocking full" << endl;
			pthread_mutex_unlock(&_f_mtx);
		}
        
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
