#include "queue.h"
#include <iostream>
#include "constants.h"
#include "my_string.h"
#include <stdexcept>

using namespace std;

template <typename T>
queue<T>::queue(int max) : _head(nullptr), _last(nullptr), _size(0), _max(max){}

template <typename T>
// We only delete head because the whole queue will be deleted recursively
queue<T>::~queue() {
    queue_node *curr;
    while(_head) {
        curr = _head;
        _head = _head->get_next();
        delete curr;
    }

    /*
    for (size_t i = 0; i < _size; i++) {
        curr_next = curr->get_next();
        delete curr;
        curr = curr_next;
    }
    */
}

template <typename T>
void queue<T>::push(T ent) {
    if (full()) throw runtime_error("Queue is full");
    if (_head == nullptr) {
        _last = _head = new queue_node(ent);
        _size++;
        return;
    }

    queue_node *curr = new queue_node(ent, nullptr, _last);
    _last->set_next(curr);
    _last = curr;
    _size++;
}

template <typename T>
T queue<T>::pop() {
    if (_head == nullptr) {
        throw runtime_error("queue is empty");
    }

    T tmp = _head->get_entry();
    queue_node *curr = _head->get_next();
    if (curr != nullptr)
        curr->set_prev(nullptr);
    delete _head;
    _head = curr;
    _size--;
    return tmp;
}

template <typename T>
bool queue<T>::full() { return (int) _size == _max; }

template <typename T>
T &queue<T>::peek() {
    if (_head == nullptr)
        throw runtime_error("queue is empty");

    return _head->get_entry();
}

template <typename T>
void queue<T>::print() {
    queue_node *curr = _head;
    for (size_t i = 0; i < _size; i++) {
        cout << curr->get_entry() << ", ";
        curr = curr->get_next();
    }
}

template <typename T>
bool queue<T>::empty() {
    return _size == 0;
}

template <typename T>
size_t queue<T>::size() { return _size; }

template class queue<int>;
template class queue<my_string>;
