#ifndef JMS_MY_VECTOR_H
#define JMS_MY_VECTOR_H

#include <iostream>
#include <cstdlib>
class my_string;
// Simple resizable array template class
// that includes some of std::vector's operations
template <typename T>
class my_vector {
private:
    size_t _size;
    size_t _capacity;
    int _last;
    T *_entries;
    void _enlarge();

public:
    my_vector(size_t capacity = 1);

    my_vector(const my_vector<T> &other);

    my_vector(const my_vector<T> &other, int start, int end);

    ~my_vector();

    void push(T ent);

    void clear();

    my_string join(char j);

    T &at(int index);

    T *at_p(int index);

    bool in(T ent);

    void set_at(int index, T ent);

    void remove_at(int index);

    bool remove(T ent);

    size_t size();

    T &operator[](int index);

    void print();

    my_vector<T> operator=(const my_vector &other);

    bool operator==(const my_vector &other) {
        return true;
    }

    friend std::ostream &operator<<(std::ostream &out, my_vector vec) {
        out << vec._size;
        return out;
    }

    my_vector<T> sublist(int start, int length);
};


#endif //JMS_MY_VECTOR_H
