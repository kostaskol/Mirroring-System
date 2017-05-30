#include "cmd_parser.h"
#include "my_string.h"
#include "my_vector.h"
#include "constants.h"
#include <iostream>
using namespace std;

cmd_parser::cmd_parser(int mode) {
    _mode = mode;
	_search = false;
}

void err(my_string msg, char *exec_name) {
    cerr << msg << ". Run " << exec_name << " --help for help" << endl;
    exit(-1);
}

void cmd_parser::parse(int argc, char **argv) {
    if (argc == 2) {
        if (my_string(argv[1]) == "--help") {
            _helper(argv[0]);
        } else {
            my_string err_msg = "Unknown command line argument ";
            err_msg += argv[1];
            err(err_msg, argv[0]);
        }
    }

    switch (_mode) {
        case MODE_CS: {
            if (argc != 7 && argc != 8)
                err("Invalid number of command line arguments", argv[0]);
            for (int arg = 1; arg < argc; arg++) {
                my_string argument = argv[arg];
                if (argument == "-p") {
                    if (arg + 1 < argc) {
                        _port = atoi(argv[++arg]);
                    } else {
                        err("Error while parsing command line arguments", 
							argv[0]);
                    }
                } else if (argument == "-d") {
                    if (arg + 1 < argc) {
                        _path_name = argv[++arg];
                    } else {
                        err("Error while parsing command line arguments", 
							argv[0]);
                    }
                } else if (argument == "-w") {
					if (arg + 1 < argc) {
						_t_num = atoi(argv[++arg]);
					} else {
						err("Error while parsing command line arguments", 
							argv[0]);
					}
				} else {
                    my_string err_msg = "Unknown command line argument ";
                    err_msg += argv[1];
                    err(err_msg, argv[0]);
                }
            }
            break;
        }

        case MODE_MI: {
          if (argc != 7 && argc != 8) {
            err("Invalid number of command line arguments", argv[0]);
          }
            for (int arg = 1; arg < argc; arg++) {
                my_string argument = argv[arg];
                if (argument == "-n") {
                    if (arg + 1 < argc) {
                        _address = argv[++arg];
                    } else {
                        err("Error while parsing command line arguments", 
							argv[0]);
                    }
                } else if (argument == "-p") {
                    if (arg + 1 < argc) {
                        _port = atoi(argv[++arg]);
                    } else {
                        err("Error while parsing command line arguments", 
							argv[0]);
                    }
                } else if (argument == "-s") {
                    if (arg + 1 < argc) {
                        my_string tmp_str = argv[++arg];
                        my_vector<my_string> com_vec = tmp_str.split(',');
                        for (int i = 0; i < (int) com_vec.size(); i++) {
                            _cserver_details.push(com_vec.at(i).split(':'));
                        }
                    } else {
                        err("Error while parsing command line arguments", argv[0]);
                    }
				} else {
                    my_string err_msg = "Unknown command line argument ";
                    err_msg += argv[1];
                    err(err_msg, argv[0]);
                }
            }
            break;
        }

        case MODE_MS: {
            for (int arg = 1; arg < argc; arg++) {
                my_string argument = argv[arg];
                if (argument == "-p") {
                    if (arg + 1 < argc) {
                        _port = atoi(argv[++arg]);
                    } else {
                        err("Error while parsing command line arguments", argv[0]);
                    }
                } else if (argument == "-m") {
                    if (arg + 1 < argc) {
                        _outp_path = argv[++arg];
                    } else {
                        err("Error while parsing command line arguments", argv[0]);
                    }
                } else if (argument == "-w") {
                    if (arg + 1 < argc) {
                        _t_num = atoi(argv[++arg]);
                    } else {
                        err("Error while parsing command line arguments", argv[0]);
                    }
				} else if (argument == "-s") {
					_search = true;
				} else {
                    my_string err_msg = "Unknown command line argument ";
                    err_msg += argv[1];
                    err(err_msg, argv[0]);
                }
            }
            break;
        }

        default: {
            cout << "Unknown mode" << endl;
            exit(-1);
        }
    }
}


void cmd_parser::_helper(char *exec_name) {
    cout << "Help for " << exec_name << endl;
    cout << "Valid options: " << endl;
    switch (_mode) {
        case MODE_MS: {
            cout << "-p: Specifies the port that the server will be" 
				 << " listening to" << endl;
            cout << "-m: Specifies the directory name in which the mirrored " 
				 << "contents will be stored" << endl;
            cout << "\t[Note] Only the last directory of the path will be " 
				 << "created automatically." << endl;
            cout << "-w: Specifies the number of worker threads";
			cout << "-s: Enables the search functionality. " 
				 << "(The server will search through the file list to " 
				 << "find the requested file)" << endl;
            break;
        }

        case MODE_MI: {
            cout << "-n: Specifies the address of the MirrorServer" << endl;
            cout << "-p: Specifies the port to which the MirrorServer is " 
				 << "listening" << endl;
            cout << "-s: Specifies the address, port, directory and delay " 
				 << "of the ContentServers" << endl;
            cout << "\t[Note] The format *must* be the following:" << endl;
            cout << "\t\t<ContentServerAddress1>:<ContentServerPort1>:" 
				 << "<Directory or filename1>:<Delay1>,"
            	 << "<ContentServerAddress2>:<ContentServerPort2>:" 
				 << "<Directory or filename2>:<Delay2>,..." << endl;
            break;
        }

        case MODE_CS: {
            cout << "-p: Specifies the port that the ContentServer " 
				 << "should be listening to" << endl;
            cout << "-d: Directory or Filename that the ContentServer " 
				 << "will make available to *all* valid requests" << endl;
			cout << "-w: Number of threads that will handle requests" << endl;
            break;
        }

        default: {
            cerr << "Unknown mode" << endl;
        }
    }
    exit(0);
}

int cmd_parser::get_port() { return _port; }

my_string cmd_parser::get_path_name() { return _path_name; }

my_string cmd_parser::get_address() { return _address; }

my_vector<my_vector<my_string>> cmd_parser::get_cserver_details() { 
	return _cserver_details; 
}

my_string cmd_parser::get_outp_path() { return _outp_path; }

int cmd_parser::get_thread_num() { return _t_num; }

bool cmd_parser::is_search() { return _search; }
