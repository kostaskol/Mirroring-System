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

using namespace std;

worker::worker(queue<my_string> *q, pthread_mutex_t *e_mtx,
               pthread_mutex_t *f_mtx, pthread_mutex_t *rw_mtx, my_string path, bool *done) {
    _q = q;
    _path = path;
    _done = done;
    _empty_mtx = e_mtx;
    _full_mtx = f_mtx;
    _rw_mtx = rw_mtx;
    _bytes_recvd = 0;
    _files_recvd = 0;
}


void worker::run() {
    bool end = false;
    cout << "DEBUG --::-- Worker " << pthread_self() << " started!" << endl;
    do {
        my_string details;
        // We check whether we are done after each mutex lock
        // in case the thread was waiting there
        // While MirrorServers are running, there is the possibility that the queue is empty
        // but the process has not ended (and we simply need to wait)
        if (*_done) break;
        cout << "Worker #" << pthread_self() << ": Waiting on _empty_mtx" << endl;
        pthread_mutex_lock(_empty_mtx);
        if (*_done) break;
        cout << "Worker: Waiting on _rw_mtx" << endl;
        pthread_mutex_lock(_rw_mtx);
        if (*_done) break;
        try {
            details = _q->pop();
            // if (_q->empty()) pthread_mutex_lock(_empty_mtx);
            // else pthread_mutex_unlock(_empty_mtx);
        } catch (runtime_error &e) {
            cerr << "Worker thread #" << pthread_self() << ": Queue threw a runtime error" << endl;
            end = true;
        }
        cout << "Worker: Unlocking rw_mtx" << endl;
        pthread_mutex_unlock(_rw_mtx);
        cout << "Worker: Unlocking full_mtx" << endl;
        pthread_mutex_unlock(_full_mtx);
        cout << "Worker #" << pthread_self() << " popped " << details << endl;

        if (!end) {
            my_string addr, path;
            int port, id;
            try {
                _get_info(&path, &addr, &port, &id, details);
            } catch (runtime_error &e) {
                cerr << "Worker thread #" << pthread_self() << "Malformed string from queue: " << details << endl;
                continue;
            }

            _make_con(addr, port);
            // We now have the address and port of the server
            // and the requested path
            _fetch(path, addr, port, id);
            // We don't need thread safety on _q->empty
            // since it performs a read-only operation
        }
    } while (!end);

    while (!_q->empty()) {
        my_string details;
        // When, however, the MirrorManagers have stopped working,
        // we simply need to be careful of the rw operations on the queue,
        // and stop once we see an empty queue
        cout << "Worker #" << pthread_self() << " locking rw (done)" << endl;
        pthread_mutex_lock(_rw_mtx);
        if (_q->empty()) end = true;
        details = _q->pop();
        // if (_q->empty()) pthread_mutex_lock(_empty_mtx);
        // else pthread_mutex_unlock(_empty_mtx);
        cout << "Worker #" << pthread_self() << ": Unlocking rw_mtx (done)" << endl;
        pthread_mutex_unlock(_rw_mtx);

        if (!end) {
            my_string addr, path;
            int port, id;
            try {
                _get_info(&path, &addr, &port, &id, details);
            } catch (runtime_error &e) {
                cerr << "Worker thread #" << pthread_self() << "Malformed string from queue: " << details << endl;
                continue;
            }

            _make_con(addr, port);
            // We now have the address and port of the server
            // and the requested path
            if (!_fetch(path, addr, port, id)) {
				cout << "Error!" << endl;
			}
            // We don't need thread safety on _q->empty
            // since it performs a read-only operation
        }
    }

    stats *statistics = new stats;
    statistics->bytes = _bytes_recvd;
    statistics->files = _files_recvd;

    cout << "Worker #" << pthread_self() << " dying" << endl;

    pthread_exit(statistics);
}

bool worker::_fetch(my_string path, my_string addr, int port, int id) {
    my_string fetcher = "FETCH:";
    fetcher += path;
    fetcher += ":";
    fetcher += id;
    cout << "Fetching " << fetcher << endl;
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
        cout << "Worker #" << pthread_self() << path << ": No such file or directory at remote server " << addr << ":" << port << endl;
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
        if (fname[0] != '/') {
            tmp_name += "/";
        }
        my_string tmp = fname;
        fname = tmp_name;
        fname += tmp;
        my_string exec = "exec mkdir -p ";
        exec += hf::get_dir_path(fname);
        system(exec.c_str());
        ofstream outp(fname.c_str(), ios::binary | ios::out);
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
    my_vector<my_string> vec = str.split((char *) ":");
    *path = vec.at(0);
    *addr = vec.at(1);
    *port = vec.at(2).to_int();
    *id = vec.at(3).to_int();
}
