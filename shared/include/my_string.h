#ifndef MYSTRING_H_
#define MYSTRING_H_

#include <iostream>
#include "my_vector.h"

// Simple string class
// that includes some of the usual std::string operations
class my_string {
private:
    char *_str;
    size_t _len;
public:
    my_string();
    my_string(char *str);
    my_string(const char *str);
    my_string(const my_string &other);
    my_string(char c);
    my_string(const int num);

    virtual ~my_string();

    const char *c_str();
	
	std::string cpp_str();

    size_t length();

    void clear();

    my_vector<my_string> split(char *delim);
	
	my_vector<my_string> split(char delim);
	
	my_vector<my_string> split(const char *delim);
	
    my_string substr(int start, int length);

    int to_int();

    void remove(char c);

    void replace(char old, char n);

    bool contains(char *str);

    bool contains(const char *str);

    bool contains(const my_string &other);

    void remove_substr(const my_string &other);

    void remove_substr(const char *str);

    void remove_substr(char *str);

    bool starts_with(char *str);

    bool starts_with(const my_string &other);

    bool starts_with(const char *str);

    my_string &operator=(const my_string& other);
    my_string &operator=(const char *str);
    my_string &operator=(char *str);

    my_string &operator+=(const my_string& other);
    my_string &operator+=(const char *str);
    my_string &operator+=(const char c);
    my_string &operator+=(int num);

    bool operator==(const my_string& other);
    bool operator==(const char *str);
    bool operator!=(const my_string &other);
    bool operator!=(const char *str);

    char operator[](int index);

    friend std::ostream &operator<<(std::ostream &out, my_string str);
};

#endif /* MYSTRING_H_ */
