#include <pthread.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <signal.h>
#include "content_manager.h"


using namespace std;

content_manager::content_manager(pthread_mutex_t *mtx, pthread_mutex_t *h_mtx,
	pthread_mutex_t *e_mtx, pthread_mutex_t *f_mtx,
	pthread_cond_t *e_cond, pthread_cond_t *f_cond,
	queue<int> *q, hash_table<int> *hash, my_string path, bool *empty, 
	bool *full, bool debug) {
	_q_mtx = mtx;
	_h_mtx = h_mtx;
	_e_mtx = e_mtx;
	_f_mtx = f_mtx;
	_e_cond = e_cond;
	_f_cond = f_cond;
	_q = q;
	_h_table = hash;
	_path = path;
	_empty = empty;
	_full = full;
	_debug = debug;
}

void content_manager::run() {
	cout << "Slave full " << _full << endl;
	signal(SIGPIPE, SIG_IGN);
	while (true) {
		int client_fd;
		// Lock the stop flag mutex
		cout << "Thread #" << pthread_self() << " Waiting on empty mutex" << endl;
		pthread_mutex_lock(_e_mtx);
		{
			// Wait until the queue isn't empty (flag raised by the master
			// thread, lowered by a worker thread)
			while (*_empty) {
				cout << "Waiting on empty" << endl;
				pthread_cond_wait(_e_cond, _e_mtx);
			}
			// Don't release the mutex yet. First get the element in the queue
			// and then release it
		}
		
		// The empty mutex is still locked here
		{
			pthread_mutex_lock(_q_mtx); 
			{
				try {
					client_fd = _q->pop();
				} catch (runtime_error &e) {
					cout << "Caught exception" << endl;
					continue;
				}

				// Check if we just popped the last element of the queue
				// If we have, set _empty to true
				// (We already hold the empty mutex at this point)
				*_empty = _q->empty();
				// if (_q->empty()) *_empty = true;
			}
			pthread_mutex_unlock(_q_mtx);
		}
		pthread_mutex_unlock(_e_mtx);
		
		// Acquire the full mutex
		pthread_mutex_lock(_f_mtx);
		{
			// Lower the full flag
			*_full = false;
			pthread_cond_signal(_f_cond);
		}
		pthread_mutex_unlock(_f_mtx);
		
		
		// This is where the main transfer will occur
		char *buffer = new char[1024];
		// First, we receive the request message from the server
		ssize_t read = recv(client_fd, buffer, 1023, 0);
		buffer[read] = '\0';
		my_string msg = buffer;
		if (_debug) cout << "Got message " << msg << endl;
		// Figure out the components of the message
		my_vector<my_string> cmd = msg.split(":");
		try {
			if (cmd.at(0) == "LIST") {
				my_string id, delay;
				try {
					id = cmd.at(1);
					delay = cmd.at(2);
				} catch (runtime_error &e) {
					if (_debug) cerr << "Client hasn't provided an ID" 
									<< " or a delay" << endl;
					close(client_fd);
					continue;
				}
				// If it's a LIST request
				// Lock the hashtable mutex and 
				// map the delay to the given ID
				pthread_mutex_lock(_h_mtx); 
				{
					_h_table->insert_key(id, delay.to_int());
				}
				pthread_mutex_unlock(_h_mtx);
				
				// Send the list to the client
				_do_list(client_fd);
				cout << "Thread #" << pthread_self() << " done with list" << endl;
			} else if (cmd.at(0) == "FETCH") {
				// If it's a FETCH request
				int delay;
				bool ex;
				try {
					my_string id = cmd.at(2);
				} catch (runtime_error &e) {
					cerr << "User has sent us a malformed message "  << msg << endl;
					cmd.print();
					continue;
				}
				// Find the delay for the given ID
				cout << "Thread #" << pthread_self() << " Locking h_mtx" << endl;
				pthread_mutex_lock(_h_mtx);
				{
					ex = _h_table->get_key(cmd.at(2), &delay);
				}
				cout << "Thread #" << pthread_self() << " unlocking h_mtx" << endl;
				pthread_mutex_unlock(_h_mtx);
				if (!ex) {
					if (_debug) 
					cerr << "Provided ID (" << cmd.at(2) 
						 << ") doesn't exist in the hash table" << endl;
					close(client_fd);
					continue;
				}
				
				// Perform the given delay
				// If it's 0 (the Client doesn't want us to wait)
				// skip the call to sleep entirely
				cout << "Thread #" << pthread_self() << " Sleeping" << endl;
				if (delay != 0) sleep(delay);
				
				// Send the files to the client
				cout << "Thread #" << pthread_self() << " Fetching!" << endl;
				_do_fetch(client_fd, cmd.at(1));
				cout << "Thread #" << pthread_self() << " done with fetch" << endl;
			}
		} catch (runtime_error &e) {
			continue;
		}

		if (_debug) cout << "Thread #" << pthread_self() << " Closing the connnection!" << endl;
		close(client_fd);
	}
}



void content_manager::_do_list(int clientfd) {
    my_vector<my_string> list;
    cout << "Looking in path " << _path << endl;
    _get_list(_path, &list);
    // Send the number of files to the client
    hf::send_num_blocks(clientfd, (int) list.size());
    hf::recv_ok(clientfd);
    // Send all of the file paths
    for (size_t file_no = 0; file_no < list.size(); file_no++) {
        my_string fname = list.at((int) file_no);
        int loops = (int) fname.length() / BLOCK_STR_SIZE;
        // Send the number of bytes that the client should read
        // and have them expect that number of bytes
        hf::send_num_blocks(clientfd, (int) fname.length());
		
		// Make sure that the client has received the request
        hf::recv_ok(clientfd);
        uint offs = 0;
        // For each part of the file name
        // send at most BLOCK_STR_SIZE characters
        cout << "Sending name " << fname << endl;
        for (int file_part = 0; file_part < loops + 1; file_part++) {
            int curr_len = (fname.length() - offs) > BLOCK_STR_SIZE
                           ? BLOCK_STR_SIZE : ((int) fname.length() - offs);
            my_string str_part = fname.substr(offs, curr_len);
            send(clientfd, str_part.c_str(), (uint16_t) curr_len, 0);
            hf::recv_ok(clientfd);
            offs += curr_len;
        }
    }
    cout << "Finished Sending List!" << endl;
}

void content_manager::_do_fetch(int clientfd, my_string path) {
    my_vector<my_string> corr = path.split((char *)"/");
    corr.remove(".");
    corr.remove("..");
    corr.remove("~");
    path = corr.join('/');
    my_vector<my_string> list;
    my_string tmp_path = _path;
    tmp_path += "/";
    tmp_path += path;
    _get_contents_cond(&list, tmp_path, path);
    //_get_list(tmp_path, &list);

    hf::send_num_blocks(clientfd, (int) list.size());
    hf::recv_ok(clientfd);

    for (int file = 0; file < (int) list.size(); file++) {
        // Send the name of each file
        my_string fname = list.at((int) file);
        // Send the number of bytes that the client should read
        // and have them expect that number of bytes
        // For each part of the file name
        // send at most BLOCK_STR_SIZE characters

        ifstream inp(fname.c_str(), ios::binary | ios::in);
        inp.seekg(0, ios::end);
        int file_length = (int) inp.tellg();
        inp.seekg(0, ios::beg);

        int blocks = file_length / BLOCK_RAW_SIZE;

        hf::send_num_blocks(clientfd, blocks);
        hf::recv_ok(clientfd);
		
		cout << "Sending file " << fname << endl;
        for (int block = 0; block < blocks + 1; block++) {
            char *buffer = new char[BLOCK_RAW_SIZE];
            inp.read(buffer, BLOCK_RAW_SIZE);
            send(clientfd, buffer, BLOCK_RAW_SIZE, 0);
            hf::recv_ok(clientfd);
			/*if (block == blocks) {
				cout << "\rPart [" << block + 1 << "/" << blocks + 1 << "]" << endl;
			} else {
				cout << "\rPart [" << block + 1 << "/" << blocks + 1 << "]" << flush;
			}*/
        }

    }
    cout << "Finished Sending Data!" << endl;
}

void content_manager::_get_list(my_string path, my_vector<my_string> *v) {
    // Fills out the my_vector<my_string> object
    // with the contents + paths of the path assigned (recursively)

    // We should first check whether the input path is a simple file,
    // in which case we insert it into the list an return

    struct stat s;
    stat(path.c_str(), &s);
    if (S_ISDIR(s.st_mode)) {
        DIR *dir;
        struct dirent *entry;
        if ((dir = opendir(path.c_str())) != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_DIR) {
                    if (strcmp(entry->d_name, ".") != 0
                        && strcmp(entry->d_name, "..") != 0) {
                        my_string tmp = path;
                        tmp += "/";
                        tmp += entry->d_name;
                        _get_list(tmp, v);
                    }
                } else {
                    my_string tmp = path;
                    tmp += "/";
                    tmp += entry->d_name;
                    // We want the client to receive paths relative to our position
                    // this means that we need to remove the path of the directory
                    // we make available
                    // from the LIST response
                    tmp.remove_substr(_path);
                    v->push(tmp);
                }
            }
            closedir(dir);
        }
    } else if (S_ISREG(s.st_mode)) {
        v->push(path);
    }
}

void content_manager::_get_contents_cond(my_vector<my_string> *v, 
	my_string path, my_string cond) {
    struct stat s;
    stat(path.c_str(), &s);
    if (S_ISDIR(s.st_mode)) {
        DIR *dir;
        struct dirent *entry;
        if ((dir = opendir(path.c_str())) != NULL) {
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_type == DT_DIR) {
                    if (strcmp(entry->d_name, ".") != 0
                        && strcmp(entry->d_name, "..") != 0) {
                        my_string tmp = path;
                        tmp += "/";
                        tmp += entry->d_name;
                        _get_contents_cond(v, tmp, cond);
                    }
                } else {
                    my_string tmp = path;
                    tmp += "/";
                    tmp += entry->d_name;
                    cout << "Tmp = " << tmp << ". Cond = " << cond << endl;
                    if (tmp.contains(cond)) {
                        v->push(tmp);
                    }
                }
            }
            closedir(dir);
        }
    } else if (S_ISREG(s.st_mode)) {
        if (path.contains(cond)) {
            v->push(path);
        }
    }
}
