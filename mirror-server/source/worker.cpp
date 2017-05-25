#include "worker.h"
#include "help_func.h"
#include "constants.h"
#include "mirror_server.h"
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <stdexcept>

using namespace std;

worker::worker(queue<my_string> *q, pthread_mutex_t *e_mtx, 
	pthread_mutex_t *f_mtx, pthread_mutex_t *rw_mtx, 
	pthread_cond_t *e_cond, pthread_cond_t *f_cond, bool *done, 
	bool *empty, bool *full, my_string path, bool *debug) {
    _q = q;
    _path = path;
    _done = done;
    _e_mtx = e_mtx;
    _f_mtx = f_mtx;
    _rw_mtx = rw_mtx;
	
	_e_cond = e_cond;
	_f_cond = f_cond;
	
	_full = full;
	_empty = empty;
	
	_done = done;
	
    _bytes_recvd = 0;
    _files_recvd = 0;
}


stats *worker::run() {
    bool end = false;
    cout << "DEBUG --::-- Worker " << pthread_self() << " started!" << endl;
	
	// TODO: Still need to figure out how to know that it's the end the session
    do {
		my_string details;
		// We'll need to do a broadcast here
		// TODO: Maybe I should lock both empty and done here
		pthread_mutex_lock(_e_mtx);
		{
			while (*_empty && !*_done)
				pthread_cond_wait(_e_cond, _e_mtx);
		}
		if (!*_done) {
			{
				pthread_mutex_lock(_rw_mtx);
				{
					details = _q->pop();
					*_empty = _q->empty();
				}
				pthread_mutex_unlock(_rw_mtx);
			}
			pthread_mutex_unlock(_e_mtx);
			
			pthread_mutex_lock(_f_mtx);
			{
				*_full = false;
				pthread_cond_signal(_f_cond);
			}
			pthread_mutex_unlock(_f_mtx);
	        my_string addr, path;
	        int port, id;
	        try {
	            _get_info(&path, &addr, &port, &id, details);
	        } catch (runtime_error &e) {
	            cerr << "Worker thread #" << pthread_self() << "Malformed string from queue: " << details << endl;
	            continue;
	        }


			// This is standard so it remains
	        _make_con(addr, port);
	        _fetch(path, addr, port, id);
			
			close(_sockfd);
		} else {
			// If the mirror managers aren't going to put anything else 
			// into the queue, we just pop from it until empty
			pthread_mutex_unlock(_e_mtx);
			end = false;
			do {
				pthread_mutex_lock(_rw_mtx);
				{
					try {
						details = _q->pop();
					} catch (runtime_error &e) {
						end = true;
					}
				}
				pthread_mutex_unlock(_rw_mtx);
				
				my_string addr, path;
			   int port, id;
			   try {
				   _get_info(&path, &addr, &port, &id, details);
			   } catch (runtime_error &e) {
				   cerr << "Worker thread #" << pthread_self() 
				   	    << "Malformed string from queue: " << details << endl;
				   continue;
			   }


			   // This is standard so it remains
			   _make_con(addr, port);
			   _fetch(path, addr, port, id);
			   
			   close(_sockfd);
				
			} while (!end);
		}

	} while (!end);
	
    stats *statistics = new stats;
    statistics->bytes = _bytes_recvd;
    statistics->files = _files_recvd;

    cout << "Worker #" << pthread_self() << " dying" << endl;

    return statistics;
}

bool worker::_fetch(my_string path, my_string addr, int port, int id) {
    my_string fetcher = "FETCH:";
    fetcher += path;
    fetcher += ":";
    fetcher += id;
    cout << "Thread #" << pthread_self() << " fetching " << fetcher << endl;
    send(_sockfd, fetcher.c_str(), fetcher.length(), 0);

    // Receive amount of files that will be transfered
    char *buffer = new char[1024];
    ssize_t read = recv(_sockfd, buffer, 1023, 0);
	if (read == 0) {
		cout << "Remote Server has closed the connection!" << endl;
		return false;
	}
    buffer[read] = '\0';
    my_string len_str = buffer;
    delete[] buffer;
    int len = len_str.to_int();
    if (len == 0) {
        cout << "Worker #" << pthread_self() << path << ": No such file" 
			 << " or directory at remote server " << addr << ":" << port << endl;
		
        return false;
    }
    hf::send_ok(_sockfd);
    _files_recvd += len;
    for (int file = 0; file < len; file++) {
        // Read amount of bytes to be sent
        buffer = new char[1024];
        read = recv(_sockfd, buffer, 1023, 0);
        buffer[read] = '\0';
        hf::send_ok(_sockfd);
        int max = atoi(buffer);
        delete[] buffer;
        my_string fname;
        hf::read_fname(_sockfd, &fname, max);
        
		my_string tmp_name = _path;
		tmp_name += "/";
		tmp_name += addr; tmp_name += "-"; tmp_name += port;
		tmp_name += path;
		hf::mk_path(tmp_name);
		
        ofstream outp(tmp_name.c_str(), ios::binary | ios::out);
        buffer = new char[1024];
        read = recv(_sockfd, buffer, 1023, 0);
        buffer[read] = '\0';
        int max_blocks = atoi(buffer);
        delete[] buffer;
        hf::send_ok(_sockfd);
        for (int block = 0; block < max_blocks + 1; block++) {
            buffer = new char[1024];
            read = recv(_sockfd, buffer, 1024, 0);
            _bytes_recvd += read;
            outp.write(buffer, read);
            delete[] buffer;
            hf::send_ok(_sockfd);
        }
    }
	cout << "Thread #" << pthread_self() << " fetched" << endl;
    return true;
}


bool worker::_make_con(my_string addr, int port) {
    struct sockaddr_in remote_server;
    struct hostent *server;

    if ((_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return false;
    }

    server = gethostbyname(addr.c_str());
    if (server == nullptr) {
        cerr << "No such host: " << addr << endl;
        close(_sockfd);
        return false;
    }

    remote_server.sin_family = AF_INET;
    bcopy(server->h_addr, &remote_server.sin_addr.s_addr, server->h_length);
    remote_server.sin_port = htons((uint16_t) port);

    bzero(&remote_server.sin_zero, 8);

    if ((connect(_sockfd, (struct sockaddr *) &remote_server, sizeof(struct sockaddr_in))) < 0) {
        perror("Manager: Connect");
        close(_sockfd);
        return false;
    }

    return true;
}

void worker::_get_info(my_string *path, my_string *addr, int *port, int *id, my_string &str) {
    my_vector<my_string> vec = str.split(":");
    *path = vec.at(0);
    *addr = vec.at(1);
    *port = vec.at(2).to_int();
    *id = vec.at(3).to_int();
}
