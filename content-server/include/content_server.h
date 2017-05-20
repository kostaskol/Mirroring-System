#ifndef CONTENT_SERVER_CONTENT_SERVER_H
#define CONTENT_SERVER_CONTENT_SERVER_H


#include "my_string.h"
#include "cmd_parser.h"
#include "hash_table.h"

class content_server {
private:
    my_string _path;
    int _port;
    int _sockfd;
    bool _init;

    hash_table<int> _h_table;

    void _get_contents_cond(my_vector<my_string> *v, my_string path, my_string cond);

    void _get_list(my_string path, my_vector<my_string> *v);

    void _do_list(int clientfd);

    void _do_fetch(int clientfd, my_string path, my_string id);
public:
    content_server(cmd_parser *parser);

    void init();

    void run();
};


#endif //CONTENT_SERVER_CONTENT_SERVER_H
