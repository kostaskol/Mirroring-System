#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include "mirror_init.h"
#include "help_func.h"
#include "constants.h"

using namespace std;

mirror_init::mirror_init(cmd_parser *args) {
    _port = args->get_port();
    _address = args->get_address();
    _cservers = args->get_cserver_details();
    _init = false;
}

void mirror_init::init() {
    struct sockaddr_in remote_server;
    struct hostent *server;

    if ((_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(-1);
    }

    server = gethostbyname(_address.c_str());
    if (server == nullptr) {
        cerr << "No such host: " << _address << endl;
        exit(-1);
    }

    remote_server.sin_family = AF_INET;
    bcopy(server->h_addr, &remote_server.sin_addr.s_addr, server->h_length);
    remote_server.sin_port = htons((uint16_t) _port);

    bzero(&remote_server.sin_zero, 8);

    if ((connect(_sockfd, (struct sockaddr *) &remote_server, sizeof(struct sockaddr_in))) < 0) {
        perror("Connect");
        exit(-1);
    }
}

void mirror_init::run() {
    // First off, send the server the number of content servers
    // it is supposed to get
    cout << "Hello World" << endl;
    hf::send_num_blocks(_sockfd, (int) _cservers.size());
    hf::recv_ok(_sockfd);

    // Then, for each content server,
    // send them the *split* data
    for (size_t server = 0; server < _cservers.size(); server++) {
        // There will always be 4 data we are supposed to send to the server for
        // each content-server
        for (size_t s_det = 0; s_det < 4; s_det++) {
            my_vector<my_string> tmp_vec = _cservers.at(server);
            write(_sockfd, tmp_vec.at(s_det).c_str(), tmp_vec.at(s_det).length());
            hf::recv_ok(_sockfd);
        }
    }


    // The initiator will wait for a response from the MirrorServer
    // The response can either be:
    // ERR <nums>;: Host #<num-i> doesn't exist
    // OK <num_files> <num_bytes>;: The overall number of files and bytes transferred
    // TODO: If I have time:
    // % <num>;: Percentage of requested files downloaded

    int err_count = 0;
    do {
        my_string resp = "";
        do {
            char *buffer = new char[1024];
            cout << "Reading..." << endl;
            ssize_t read = recv(_sockfd, buffer, 1023, 0);
            if (read == 0) {
                cerr << "The server has abruptly closed the connection. Exiting.." << endl;
                exit(-1);
            }
            buffer[read] = '\0';
            resp += buffer;
            cout << "Read " << buffer << endl;
            // While the response doesn't contain the character ';' in it,
            // we continue reading from the stream
        } while (!resp.contains(";"));
        hf::send_ok(_sockfd);

        resp.remove(';');

        if (resp.contains("ERR")) {
            my_vector<my_string> resp_vec = resp.split((char *)" ");
            for (int i = 0; i < (int) resp_vec.size(); i++) {
                int serv_num = resp_vec.at(i).to_int();
                cout << "Couldn't establish a connection to " << _cservers.at(serv_num).at(0) <<
                        ":" << _cservers.at(serv_num).at(1) << " doesn't exist" << endl;
                err_count++;
            }
        } else if (resp.contains("OK")) {
            my_vector<my_string> resp_vec = resp.split((char *)" ");
            cout << "Statistics: " << endl;
            try {
                cout << "Files transferred " << resp_vec.at(1) << endl;
                cout << "Bytes transferred " << resp_vec.at(2) << endl;
            } catch(runtime_error &e) {
                cout << "Oops. The server has sent us a bad response (" << resp << ")" << endl;
                exit(-1);
            }
            break;
        } else if (resp.contains("%")) {
            my_vector<my_string> resp_vec = resp.split((char *)" ");
            try {
                cout << "Transfer progress: [" << resp_vec.at(1) << "]" << endl;
            } catch (runtime_error &e) {
                cout << "Oops. The server has sent us a bad response (" << resp << ")" << endl;
            }
        } else {
            cerr << "Undefined response from the server. Exiting..." << endl;
            exit(-1);
        }

    } while (err_count <= _cservers.size()); // Recheck this one!
}
