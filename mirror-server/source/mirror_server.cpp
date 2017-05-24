#include "mirror_server.h"
#include "constants.h"
#include "worker.h"
#include <iostream>
#include <sys/socket.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cmath>

using namespace std;

mirror_server::mirror_server(cmd_parser *args) {
    _port = args->get_port();
    _outp_path = args->get_outp_path();
    _worker_num = args->get_thread_num();
    if (_worker_num < 1) {
        cerr << "Bad input for thread number: " << _worker_num << endl;
        exit(-1);
    }
    _workers = new pthread_t[_worker_num];
    _mtx_init();
}

void mirror_server::_mtx_init() {
    if (pthread_mutex_init(&_rw_mtx, nullptr)
            || pthread_mutex_init(&_done_mtx, nullptr)
            || pthread_mutex_init(&_full_mtx, nullptr)
            || pthread_mutex_init(&_empty_mtx, nullptr)
			|| pthread_cond_init(&_cond, nullptr)) {
        cerr << "Mutex creation failed. Exiting..." << endl;
        exit(-1);
    }
}

void mirror_server::init() {
    struct sockaddr_in server;

    if ((_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket: ");
        exit(-1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t) _port);
    server.sin_addr.s_addr = INADDR_ANY;

    bzero(&server.sin_zero, 8);

    socklen_t len = sizeof(struct sockaddr_in);

    if ((bind(_sockfd, (sockaddr*) &server, len)) < 0) {
        perror("Bind");
        exit(-1);
    }



    if ((listen(_sockfd, 5)) < 0) {
        perror("Listen");
        exit(-1);
    }
}

void mirror_server::run() {
    sockaddr_in client;
	do {
	    socklen_t len = sizeof(struct sockaddr_in);
		int clientfd;
	    if ((clientfd = accept(_sockfd, (sockaddr *) &client, &len)) < 0) {
	        perror("accept");
	        exit(-1);
	    }

	    char *buffer = new char[1024];
	    ssize_t read = recv(clientfd, buffer, 1023, 0);
	    buffer[read] = '\0';

	    send(clientfd, "OK", 2, 0);
	    int vecs = atoi(buffer);

	    delete[] buffer;
	    my_vector<my_vector<my_string>> vec;
	    for (int i = 0; i < vecs; i++) {
	        my_vector<my_string> tmp;
	        for (int j = 0; j < 4; j++) {
	            buffer = new char[1024];
	            read = recv(clientfd, buffer, 1023, 0);
	            buffer[read] = '\0';
	            send(clientfd, "OK", 2, 0);
	            tmp.push(buffer);
	            delete[] buffer;

	        }
	        vec.push(tmp);
	    }


	    pthread_mutex_unlock(&_rw_mtx);
	    pthread_mutex_unlock(&_full_mtx);
	    pthread_mutex_lock(&_empty_mtx);
	    _managers = new pthread_t[vec.size()];
	    for (int i = 0; i < (int) vec.size(); i++) {
	        my_vector<my_string> tmp_vec = vec.at(i);
	        mirror_manager *man = new mirror_manager(tmp_vec, i, &_data_queue, 
				&_empty_mtx, &_full_mtx, &_rw_mtx, &_done_mtx, &_cond, 
				&_tid, &_down_serv);
	        pthread_attr_t attr;
	        pthread_attr_init(&attr);
	        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	        pthread_create(&_managers[i], &attr, start_manager, man);
	    }
		
		if (_down_serv.size() != 0) {
			cout << "Some Servers where down: " << endl;
			_down_serv.print();
		}

	    // Workers will loop until done becomes true
	    bool done = false;

	    for (int i = 0; i < _worker_num; i++) {
	        worker *w = new worker(&_data_queue, &_empty_mtx, &_full_mtx,
	                               &_rw_mtx, _outp_path, &done);
	        pthread_attr_t attr;
	        pthread_attr_init(&attr);
	        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	        pthread_create(&_workers[i], &attr, start_worker, w);
	    }

	    for (int count = 0; count < (int) vec.size(); count++) {
	        pthread_join(_managers[count], nullptr);
	    }

	    cout << "DEBUG --::-- All mirrorManagers died" << endl;
	    done = true;

	    cout << "DEBUG --::-- Unblocking and joining worker threads!" << endl;
	    // Wherever workers may be stuck, we unblock them
	    for (int i = 0; i < _worker_num; i++) {
	        pthread_mutex_unlock(&_empty_mtx);
	        pthread_mutex_unlock(&_rw_mtx);
	    }
	    int bytes = 0, files = 0;
	    for (int i = 0; i < _worker_num; i++) {
	        void *tmp;
	        pthread_join(_workers[i], &tmp);
	        cout << "DEBUG --::-- worker thread #" << i << " died" << endl;
	        stats *st = (stats *) tmp;
	        bytes += st->bytes;
	        files += st->files;
	        delete st;
	    }

	    my_string msg = "OK:";
	    msg += files;
	    msg += ":";
	    msg += bytes;
		msg += ":";
		if (files != 0)
			msg += (bytes / files);
		else 
			msg += 0;
		msg += ":";
		if (files != 0)
			msg += ((int) sqrt(bytes / files));
		else 
			msg += 0;
	    msg += ";";
	    send(clientfd, msg.c_str(), msg.length(), 0);
	    close(clientfd);
		cout << "Session ended" << endl;
	} while (true);
}

void *start_manager(void *arg) {
    mirror_manager *man = (mirror_manager *) arg;
    man->init();
    man->run();
    delete man;
    pthread_exit(0);
}

void *start_worker(void *arg) {
    worker *w = (worker *) arg;
    stats *statistics = w->run();
    delete w;
    pthread_exit(statistics);
}
