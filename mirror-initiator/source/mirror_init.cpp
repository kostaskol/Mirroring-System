#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include "mirror_init.h"
#include "help_func.h"
#include "constants.h"
#include <stdexcept>

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
    hf::send_num_blocks(_sockfd, (int) _cservers.size());
    hf::recv_ok(_sockfd);

    // Then, for each content server,
    // send them the *split* data
    for (size_t server = 0; server < _cservers.size(); server++) {
        // There will always be 4 data we are supposed to send to the server for
        // each content-server
        for (size_t s_det = 0; s_det < 4; s_det++) {
            my_vector<my_string> tmp_vec = _cservers.at(server);
            write(_sockfd, tmp_vec.at(s_det).c_str(), 
					tmp_vec.at(s_det).length());
            hf::recv_ok(_sockfd);
        }
    }


    // The initiator will wait for a response from the MirrorServer
    // The response can either be:
    // ERR:<address>:<port>;: Content Server at specified address and port 
	//															is not running
    // OK:<num_files>:<num_bytes>:<average_size_of_files>:
	//											<standard_diviation_of_size>;

    int err_count = 0;
    do {
        my_string resp = "";
        do {
			// We await for a message from the server (we keep the 
			// connection alive). The message can be one of the above.
            char *buffer = new char[1024];
            ssize_t read = recv(_sockfd, buffer, 1023, 0);
            if (read == 0) {
                cerr << "The server has abruptly closed the connection. " 
					 << "Exiting.." << endl;
                exit(-1);
            }
            buffer[read] = '\0';
            resp += buffer;
			delete[] buffer;
            // While the response doesn't contain the character ';' in it,
            // we continue reading from the stream
        } while (!resp.contains(";"));
        hf::send_ok(_sockfd);

		// We don't want ';' to be in the output
        resp.remove(';');

        if (resp.starts_with("ERR")) {
            my_vector<my_string> resp_vec = resp.split(":");
			try {
            	cout << "No content server running at " 
				 	 << resp_vec.at(1) << ":" << resp_vec.at(2) << endl;
			 } catch (runtime_error &e) {
				 cout << "Opps. The server has sent us a bad response (" << resp
				 	  << ")" << endl;
			 }
        } else if (resp.starts_with("OK")) {
            my_vector<my_string> resp_vec = resp.split((char *)":");
            cout << "Statistics: " << endl;
            try {
                cout << "Files transferred " << resp_vec.at(1) << endl;
                cout << "Bytes transferred " << resp_vec.at(2) << endl;
				cout << "Mean size of files " << resp_vec.at(3) << endl;
				cout << "Standard diviation of size " << resp_vec.at(4) << endl;
            } catch(runtime_error &e) {
                cout << "Oops. The server has sent us a bad response (" << resp 
					 << ")" << endl;
                exit(-1);
            }
            break;
        } else {
            cerr << "Undefined response from the server. Exiting..." << endl;
            exit(-1);
        }

    } while (err_count <= (int) _cservers.size()); // Recheck this one!
}
