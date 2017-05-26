#include "mirror_manager.h"
#include "constants.h"
#include "help_func.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <stdexcept>

using namespace std;

mirror_manager::mirror_manager(my_vector<my_string> details, int id, 
	queue<my_string> *q, pthread_mutex_t *e_mtx, pthread_mutex_t *f_mtx, 
	pthread_mutex_t *rw_mtx, pthread_cond_t *e_cond, 
	pthread_cond_t *f_cond, bool *full, bool *empty, bool debug, bool search) {
    _id = id;
    _addr = details.at(CS_ADDR);
    _port = details.at(CS_PORT).to_int();
    _path = details.at(CS_DIR);
    _delay = details.at(CS_DELAY).to_int();
    _init = false;
    _q = q;
    _rw_mtx = rw_mtx;
    _f_mtx = f_mtx;
    _e_mtx = e_mtx;
    _e_cond = e_cond;
	_f_cond = f_cond;
	
	_full = full;
	_empty = empty;
		
	if (search)
		_resolver = &my_string::contains;
	else
		_resolver = &my_string::starts_with;
}

bool mirror_manager::init() {
    signal(SIGPIPE, SIG_IGN);
    _init = true;
    struct sockaddr_in remote_server;
    struct hostent *server;

    if ((_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return false;
    }

    cout << "Checking for address: " << _addr << endl;
    server = gethostbyname(_addr.c_str());
    if (server == nullptr) {
        close(_sockfd);
        return false;
    }

    remote_server.sin_family = AF_INET;
    bcopy(server->h_addr, &remote_server.sin_addr.s_addr, server->h_length);
    remote_server.sin_port = htons((uint16_t) _port);

    bzero(&remote_server.sin_zero, 8);

    if ((connect(_sockfd, (struct sockaddr *) &remote_server, sizeof(struct sockaddr_in))) < 0) {
        perror("Manager: Connect");
		cerr << _addr << ":" << _port << endl;
        close(_sockfd);
        return false;
    }

    return true;
}

void mirror_manager::run() {
    my_string cmd = "LIST:";
    cmd += _id;
    cmd += ":";
    cmd += _delay;
    char *buf;
    send(_sockfd, cmd.c_str(), cmd.length(), 0);

    buf = new char[1024];
    ssize_t read = recv(_sockfd, buf, 1023, 0);
    buf[read] = '\0';

    int files = atoi(buf);
    delete[] buf;
    hf::send_ok(_sockfd);

    for (int file = 0; file < files; file++) {
        buf = new char[1024];
        read = recv(_sockfd, buf, 1023, 0);
        buf[read] = '\0';
        int max = atoi(buf);
        delete[] buf;
        hf::send_ok(_sockfd);
        my_string fname;
        hf::read_fname(_sockfd, &fname, max);
        // We check to make sure that the path returned from
        // the content-server matched the one requested by the user
        if (!(fname.*_resolver)(_path.c_str())) {
            // cout << "File name " << fname << " doesn't start with " << _path << endl;
            continue;
        }

        my_string full_name = fname;
        full_name += ":";
        full_name += _addr;
        full_name += ":";
        full_name += _port;
        full_name += ":";
        full_name += _id;
        cout << "Pushing file " << fname << " to queue" << endl;
        //cout << "MirrorServer #" << _id << ": Waiting on full_mtx" << endl;
		pthread_mutex_lock(_f_mtx);
		{
			while (*_full)
				pthread_cond_wait(_f_cond, _f_mtx);
		
			pthread_mutex_lock(_rw_mtx);
			{
				try {
					_q->push(full_name);
				} catch (runtime_error &e) {
					cerr << "Runtime error. Queue is full!" << endl;
				}
				*_full = _q->full();
			}
			pthread_mutex_unlock(_rw_mtx);
			
			pthread_mutex_lock(_e_mtx);
			{
				*_empty = false;
				pthread_cond_signal(_e_cond);
			}
			pthread_mutex_unlock(_e_mtx);
			
		}
		pthread_mutex_unlock(_f_mtx);
    }

    // The thread signals the master thread that it's about to finish
    /*pthread_cond_signal(_cond);
    // Then it locks the done mutex before it sets the tid
    // to its thread id (we don't want another thread to change this before it's done
    pthread_mutex_lock(_done_mtx);
    cout << "Setting Thread id" << endl;
    *_tid = pthread_self();
    // Unlock the done mutex and exit
    pthread_mutex_unlock(_done_mtx);*/

    cout << "DEBUG --::-- MirrorServer #" << _id << " dying!" << endl;
	return;

}

my_string mirror_manager::get_addr() { return _addr; }

int mirror_manager::get_port() { return _port; }
