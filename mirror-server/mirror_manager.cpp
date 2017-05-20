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

using namespace std;

mirror_manager::mirror_manager(my_vector<my_string> details, int id,
                               queue<my_string> *q, pthread_mutex_t *e_mtx,
                               pthread_mutex_t *f_mtx, pthread_mutex_t *rw_mtx,
                               pthread_mutex_t *d_mtx, pthread_cond_t *cond,
                               pthread_t *tid) {
    _id = id;
    cout << "Got vector: " << endl;
    details.print();
    _addr = details.at(CS_ADDR);
    cout << "Address is: " << _addr << endl;
    _port = details.at(CS_PORT).to_int();
    _path = details.at(CS_DIR);
    _delay = details.at(CS_DELAY).to_int();
    _init = false;
    _q = q;
    _rw_mtx = rw_mtx;
    _full_mtx = f_mtx;
    _empty_mtx = e_mtx;
    _done_mtx = d_mtx;
    _cond = cond;
    _tid = tid;
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
        cerr << "No such host: " << _addr << endl;
        close(_sockfd);
        return false;
    }

    remote_server.sin_family = AF_INET;
    bcopy(server->h_addr, &remote_server.sin_addr.s_addr, server->h_length);
    remote_server.sin_port = htons((uint16_t) _port);

    bzero(&remote_server.sin_zero, 8);

    if ((connect(_sockfd, (struct sockaddr *) &remote_server, sizeof(struct sockaddr_in))) < 0) {
        perror("Manager: Connect");
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
        if (!fname.starts_with(_path)) {
            cout << "File name " << fname << " doesn't start with " << _path << endl;
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
        cout << "MirrorServer #" << _id << ": Waiting on full_mtx" << endl;
        pthread_mutex_lock(_full_mtx);
        cout << "MirrorServer #" << _id << ": Waiting on rw_mtx" << endl;
        pthread_mutex_lock(_rw_mtx);
        cout << "MirrorServer #" << _id << ": Pushing to queue" << endl;
        _q->push(full_name);
        // If this was the last item that could be added to the queue
        // we
        if (_q->full()) pthread_mutex_lock(_full_mtx);
        else pthread_mutex_unlock(_full_mtx);
        // We unlock the empty mutex when we push something onto the queue
        cout << "MirrorServer #" << _id << ": Unlocking rw_mtx" << endl;
        pthread_mutex_unlock(_rw_mtx);
        cout << "MirrorServer #" << _id << ": Unlocking empty_mtx" << endl;
        pthread_mutex_unlock(_empty_mtx);
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

    pthread_exit(nullptr);
}