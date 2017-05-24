#ifndef CONTENT_SERVER_CMD_PARSER_H
#define CONTENT_SERVER_CMD_PARSER_H


#include "my_string.h"

class cmd_parser {
private:
    int _mode;

    // General
    int _port;
	bool _debug;

    // For ContentServer
    my_string _path_name;

    // For MirrorInitiator
    my_string _address;
    my_vector<my_vector<my_string>> _cserver_details;

    // For MirrorServer
    my_string _outp_path;
    int _t_num;

    void _helper(char *);
public:
    cmd_parser(int mode);
    cmd_parser(const cmd_parser&)=delete;

    void parse(int argc, char **argv);

    int get_port();

    my_string get_path_name();

    my_string get_address();

    my_vector<my_vector<my_string>> get_cserver_details();

    my_string get_outp_path();

    int get_thread_num();
	
	bool is_debug();

    cmd_parser &operator=(const cmd_parser &)=delete;
};


#endif //CONTENT_SERVER_CMD_PARSER_H
