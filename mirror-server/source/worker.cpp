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
	pthread_mutex_t *bytes_mtx, pthread_mutex_t *files_mtx,
	pthread_mutex_t *d_mtx, pthread_mutex_t *q_done_mtx, 
	pthread_mutex_t *ack_mtx, pthread_cond_t *q_done_cond,
	pthread_cond_t *e_cond, pthread_cond_t *f_cond, pthread_cond_t *ack_cond, 
	bool *done, int *q_done,
	bool *empty, bool *full, bool *ack, int *bytes_total, int *files_total, 
	my_string path, bool *debug) {
    _q = q;
    _path = path;
    _done = done;
    _e_mtx = e_mtx;
    _f_mtx = f_mtx;
    _rw_mtx = rw_mtx;
	_b_mtx = bytes_mtx;
	_file_mtx = files_mtx;
	_q_done_mtx = q_done_mtx;
	_ack_mtx = ack_mtx;
	
	_e_cond = e_cond;
	_f_cond = f_cond;
	_q_done_cond = q_done_cond;
	_ack_cond = ack_cond;
	
	_full = full;
	_empty = empty;
	_q_done = q_done;
	_done = done;
	_ack = ack;
	
	_bytes_total = bytes_total;
	_files_total = files_total;
}


stats *worker::run() {
    cout << "DEBUG --::-- Worker " << pthread_self() << " started!" << endl;
	while (true) {
	
		// TODO: Still need to figure out how to know that it's the end the session
		bool end = false;
	    do {
			my_string details;
			// We'll need to do a broadcast here
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
				// Signal the full flag
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
		            cerr << "Worker thread #" << pthread_self() << " Malformed string from queue: " << details << endl;
		            continue;
		        }


		        _make_con(addr, port);
		        if (!_fetch(path, addr, port, id)) {
					cout << "Fetch returned false. Continuing" << endl;
				}
				
				close(_sockfd);
				continue;
			} else {
				// If the mirror managers aren't going to put anything else 
				// into the queue, we just pop from it until empty
				pthread_mutex_unlock(_e_mtx);
				do {
					pthread_mutex_lock(_rw_mtx);
					{
						
						if (_q->empty()) end = true;
						else details = _q->pop();
					}
					pthread_mutex_unlock(_rw_mtx);
					if (end) break;
					
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
					
			   } while (true);
		   }
		} while (!end);
		// At this point we are done with the mirror-initiator request
		cerr << "Worker #" << pthread_self() << " done with queue" << endl;
		pthread_mutex_lock(_q_done_mtx);
		{
			(*_q_done)++;
			pthread_cond_signal(_q_done_cond);
		}
		pthread_mutex_unlock(_q_done_mtx);
		
		// Wait for the master thread to wake before continuing
		cerr << "Thread " << pthread_self() << " Waiting on acknowledgement" << endl;
		pthread_mutex_lock(_ack_mtx);
		{
			while (!*_ack)
				pthread_cond_wait(_ack_cond, _ack_mtx);
		}
		cerr << "Thread #" << pthread_self() << " Waited on acknowledgement" << endl;
		pthread_mutex_unlock(_ack_mtx);
	}
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
    for (int file = 0; file < len; file++) {
        // Read amount of bytes to be sent
		my_string tmp_name = _path;
		tmp_name += "/";
		tmp_name += addr; tmp_name += "-"; tmp_name += port;
		tmp_name += path;
		hf::mk_path(tmp_name);
		
        ofstream outp(tmp_name.c_str(), ios::binary | ios::out);
        buffer = new char[1024];
        read = recv(_sockfd, buffer, 1023, 0);
        buffer[read] = '\0';
        int max_bytes = atoi(buffer);
        delete[] buffer;
        hf::send_ok(_sockfd);
		int bytes = 0;
        int bytes_overall = 0;
		do {
			buffer = new char[1024];
			read = recv(_sockfd, buffer, 1024, 0);
			bytes += read;
			bytes_overall += read;
			outp.write(buffer, read);
			delete[] buffer;
		} while (bytes_overall < max_bytes);
		
		hf::send_ok(_sockfd);
		
		pthread_mutex_lock(_b_mtx);
		{
			*_bytes_total += bytes;
		}
		pthread_mutex_unlock(_b_mtx);
		
		pthread_mutex_lock(_file_mtx);
		{
			(*_files_total)++;
		}
		pthread_mutex_unlock(_file_mtx);
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
    my_vector<my_string> vec = str.split(':');
    *path = vec.at(0);
    *addr = vec.at(1);
    *port = vec.at(2).to_int();
    *id = vec.at(3).to_int();
}
