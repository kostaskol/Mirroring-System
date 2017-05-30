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
	bool *full) {
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
}

void content_manager::run() {
	signal(SIGPIPE, SIG_IGN);
	while (true) {
		int client_fd;
		// Lock the empty flag mutex
		pthread_mutex_lock(_e_mtx);
		{
			// Wait until the queue isn't empty (flag raised by the master
			// thread, lowered by a worker thread)
			while (*_empty) {
				pthread_cond_wait(_e_cond, _e_mtx);
			}
			// Don't release the mutex yet. First get the element in the queue
			// and then release it
			pthread_mutex_lock(_q_mtx); 
			{
				try {
					client_fd = _q->pop();
				} catch (runtime_error &e) {
					cerr << "Manager #" << pthread_self() 
						 << " queue was empty " << endl;
					continue;
				}

				// Check if we just popped the last element of the queue
				// If we have, set _empty to true
				// (We already hold the empty mutex at this point)
				*_empty = _q->empty();
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
		delete[] buffer;
		// Figure out the components of the message
		my_vector<my_string> cmd = msg.split(':');
		try {
			if (cmd.at(0) == "LIST") {
				my_string id, delay;
				try {
					id = cmd.at(1);
					delay = cmd.at(2);
				} catch (runtime_error &e) {
					cerr << "Client hasn't provided an ID" 
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
				cout << "Thread #" << pthread_self() 
					 << " done with list" << endl;
			} else if (cmd.at(0) == "FETCH") {
				// If it's a FETCH request
				int delay;
				bool ex;
				try {
					my_string id = cmd.at(2);
				} catch (runtime_error &e) {
					cerr << "User has sent us a malformed message "  
						 << msg << endl;
					cmd.print();
					continue;
				}
				// Find the delay for the given ID
				pthread_mutex_lock(_h_mtx);
				{
					ex = _h_table->get_key(cmd.at(2), &delay);
				}
				pthread_mutex_unlock(_h_mtx);
				if (!ex) {
					cerr << "Provided ID (" << cmd.at(2) 
						 << ") doesn't exist in the hash table" << endl;
					close(client_fd);
					continue;
				}
				
				// Perform the given delay
				// If it's 0 (the Client doesn't want us to wait)
				// skip the call to sleep entirely
				if (delay != 0) sleep(delay);
				
				// Send the files to the client
				_do_fetch(client_fd, cmd.at(1));
			}
		} catch (runtime_error &e) {
			continue;
		}

		cout << "Closing the connnection" << endl;
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
        cout << "Sending file path " << fname << endl;
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
    my_vector<my_string> corr = path.split(':');
	/* As a very basic means of security
	 * we don't allow the user to access files outside of the 
	 * available folder
	 * (
	 * Namely: 
	 * '..': which would take us to the previous folder
	 * '~': which would unfold to a path relative to $HOME
	 * )
	 * It is obvious that the user will most likely
	 * not get what they want (unless the "corrected" path agrees with the
	 * 							provided one)
	 */
	// Remove unwanted strings from the path
    corr.remove(".");
    corr.remove("..");
    corr.remove("~");
	// Create the path again
    path = corr.join('/');
    my_vector<my_string> list;
    my_string tmp_path = _path;
    tmp_path += "/";
    tmp_path += path;
	
	// Ask for a list of all of the files that match the path
	// [Note] This remains from a previous version
	// where the client was allowed to request entire directories
	// however the time it would take to change it
	// is simply not worth it
    _get_contents_cond(&list, tmp_path, path);

    // Send the name of each file
    my_string fname = list.at(0);
    // Send the number of bytes that the client should read
    // and have them expect that number of bytes
    // For each part of the file name
    // send at most BLOCK_STR_SIZE characters


	// Figure out the file's size
    ifstream inp(fname.c_str(), ios::binary | ios::in);
    inp.seekg(0, ios::end);
    int file_length = (int) inp.tellg();
    inp.seekg(0, ios::beg);

	// We will loop <blocks> times to get the entire file
    int blocks = file_length / BLOCK_RAW_SIZE;

    hf::send_num_blocks(clientfd, file_length);
    hf::recv_ok(clientfd);
	
	cout << "Sending file " << fname << endl;
    for (int block = 0; block < blocks + 1; block++) {
        char *buffer = new char[BLOCK_RAW_SIZE];
        inp.read(buffer, BLOCK_RAW_SIZE);
		size_t read = inp.gcount();
        send(clientfd, buffer, read, 0);
		if (block == blocks) {
			// Use printf here because it's thread safe
			printf("\rPart [%d/%d]\n", block + 1, blocks + 1);
		} else {
			printf("\rPart [%d/%d]", block + 1, blocks + 1);
			fflush(stdout);
		}
		delete[] buffer;
    }
	hf::recv_ok(clientfd);

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
