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
#include "constants.h"
#include "content_server.h"
#include "help_func.h"

using namespace std;


content_server::content_server(cmd_parser *parser) {
    _path = parser->get_path_name();
    _port = parser->get_port();
    _sockfd = 0;
    _init = false;
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
    do {

        int client_fd;
        socklen_t len = sizeof(struct sockaddr_in);
        if ((client_fd = accept(_sockfd, (sockaddr *) &client, &len)) < 0) {
            perror("Accept");
            exit(-1);
        }

        int pid = fork();
        if (pid == 0) {
            char *buffer = new char[1024];
            ssize_t read = recv(client_fd, buffer, 1023, 0);
            buffer[read] = '\0';
            my_string msg = buffer;
            cout << "Got message: " << msg << endl;
            my_vector<my_string> cmd = msg.split((char *) ":");
            try {
                if (cmd.at(0) == "LIST") {
                    _do_list(client_fd);
                    try {
                        _h_table.insert_key(cmd.at(1), cmd.at(2).to_int());
                    } catch (runtime_error &e) {
                        cerr << "User hasn't provided an ID or a delay" << endl;
                    }
                } else if (cmd.at(0) == "FETCH") {
                    _do_fetch(client_fd, cmd.at(1), cmd.at(2));
                }
            } catch (runtime_error &e) {
                cout << "Caught exception!" << endl;
            }

            cout << "Closing the connnection!" << endl;
            close(client_fd);
        }
    } while(true);
}

void content_server::_do_list(int clientfd) {
    my_vector<my_string> list;
    cout << "Looking in path " << _path << endl;
    _get_list(_path, &list);
    cout << "There are " << list.size() << " files in this directory" << endl;
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
        hf::recv_ok(clientfd);
        uint offs = 0;
        // For each part of the file name
        // send at most BLOCK_STR_SIZE characters
        cout << "Name will be " << fname.length() << " bytes" << endl;
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

void content_server::_do_fetch(int clientfd, my_string path, my_string id) {
    int delay;
    bool ex = _h_table.get_key(id, &delay);
    if (!ex) {
        cerr << "Provided ID (" << id << ") doesn't exist in the hash table" << endl;
        return;
    }
    sleep((uint) delay);
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

    for (int file = 0; file < list.size(); file++) {
        // Send the name of each file
        my_string fname = list.at((int) file);
        int loops = (int) fname.length() / BLOCK_STR_SIZE;
        // Send the number of bytes that the client should read
        // and have them expect that number of bytes
        hf::send_num_blocks(clientfd, (int) fname.length());
        hf::recv_ok(clientfd);
        uint offs = 0;
        // For each part of the file name
        // send at most BLOCK_STR_SIZE characters
        for (int file_part = 0; file_part < loops + 1; file_part++) {
            int curr_len = (fname.length() - offs) > BLOCK_STR_SIZE
                           ? BLOCK_STR_SIZE : ((int) fname.length() - offs);
            my_string str_part = fname.substr(offs, curr_len);
            if (send(clientfd, str_part.c_str(), (uint16_t) curr_len, 0) == 0) {
              cout << "The client has closed the connection" << endl;
            }
            hf::recv_ok(clientfd);
            offs += curr_len;
        }

        ifstream inp(fname.c_str(), ios::binary | ios::in);
        inp.seekg(0, ios::end);
        int file_length = (int) inp.tellg();
        inp.seekg(0, ios::beg);

        int blocks = file_length / BLOCK_RAW_SIZE;

        hf::send_num_blocks(clientfd, blocks);
        hf::recv_ok(clientfd);

        for (int block = 0; block < blocks + 1; block++) {
            char *buffer = new char[BLOCK_RAW_SIZE];
            inp.read(buffer, BLOCK_RAW_SIZE);
            send(clientfd, buffer, BLOCK_RAW_SIZE, 0);
            hf::recv_ok(clientfd);
            cout << "Sent part " << block + 1 << "/" << blocks + 1 << " for file " << fname << endl;

        }

    }
    cout << "Finished Sending Data!" << endl;
}

void content_server::_get_list(my_string path, my_vector<my_string> *v) {
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

void content_server::_get_contents_cond(my_vector<my_string> *v, my_string path, my_string cond) {
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
