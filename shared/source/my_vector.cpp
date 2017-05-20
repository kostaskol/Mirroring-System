#include <stdexcept>
#include "my_vector.h"
#include "my_string.h"

using namespace std;

template <typename T>
my_vector<T>::my_vector(size_t capacity) : _size(0), _capacity(capacity), _last(0) {
    _entries = new T[_capacity];
}

template <typename T>
my_vector<T>::my_vector(const my_vector<T> &other)
        : _size(other._size),
          _capacity(other._capacity) {

    _entries = new T[_capacity];

    for (size_t i = 0; i < _size; i++) {
        _entries[i] = other._entries[i];
    }
    _last = other._last;
}

template <typename T>
my_vector<T>::my_vector(const my_vector<T> &other, int start, int end) {
    if (start < 0 || end > (int) other._size) return;
    _capacity = other._capacity;
    _size = (size_t) end - start;
    _last = other._last;
    _entries = new T[_capacity];
    int cur = 0;
    for (size_t i = (size_t) start; i < (size_t) end; i++) {
        _entries[cur++] = other._entries[i];
    }
}

template <typename T>
my_vector<T> my_vector<T>::sublist(int start, int length) {
    if (start + length > (int) _size) {
        throw runtime_error("Bad indices");
    }

    my_vector<T> tmp;
    for (size_t i = (size_t) start; i <= (size_t) start + length; i++) {
        tmp.push(_entries[i]);
    }

    return tmp;
}

template <typename T>
my_vector<T>::~my_vector() {
    delete[] _entries;
}

template <typename T>
void my_vector<T>::push(T ent) {
    if (++_size >= (size_t) _capacity) {
        _enlarge();
    }

    _entries[_last++] = ent;
}

template <>
my_string my_vector<my_string>::join(char j) {
    my_string tmp = "";
    for (size_t i = 0; i < _size - 1; i++) {
        tmp += _entries[i];
        tmp += j;
    }
    tmp += _entries[_size - 1];

    return tmp;
}

template <typename T>
bool my_vector<T>::in(T ent) {
    for (size_t i = 0; i < _size; i++) {
        if (_entries[i] == ent) {
            return true;
        }
    }
    return false;
}



template <typename T>
void my_vector<T>::print() {
    for (size_t i = 0; i < _size; i++) {
        cout << _entries[i] << endl;
    }
}

template <typename T>
T &my_vector<T>::at(int index) {
    if (index >= (int) _size) {
        throw std::runtime_error("Index out of range:");
    }

    return _entries[index];
}



template <typename T>
T *my_vector<T>::at_p(int index) {
    if (index >= (int) _size) {
        throw std::runtime_error("Index out of range");
    }

    return &_entries[index];
}

template <typename T>
void my_vector<T>::_enlarge() {
    T *tmp = new T[_capacity * 2];
    for (size_t i = 0; i < _size; i++) {
        tmp[i] = _entries[i];
    }

    delete[] _entries;

    _entries = tmp;

    _capacity *= 2;
}

template <typename T>
void my_vector<T>::clear() {
    if (_size != 0) delete[] _entries;

    _size = 0;

    _last = 0;

    _capacity = 1;
    _entries = new T[_capacity];
}

template <typename T>
void my_vector<T>::set_at(int index, T ent) {
    if (index < 0 || index > (int) _size) {
        throw runtime_error("Bad index: " + index);
    }

    _entries[index] = ent;
}


template <typename T>
bool my_vector<T>::remove(T ent) {
    T *tmp = new T[_capacity];
    int j = 0;
    bool found = false;
    for (size_t i = 0; i < _size; i++) {
        if (_entries[i] == ent) {
            found = true;
            continue;
        }

        tmp[j++] = _entries[i];
    }

    if (found) {
        delete[] _entries;
        _entries = tmp;

        _size--;
        _last--;
    } else {
        delete[] tmp;
    }

    return found;
}

template <typename T>
void my_vector<T>::remove_at(int index) {
    if (index >= (int) _size || index < 0) {
        throw runtime_error("Bad index: " + index);
    }
    T *tmp_ent = new T[_capacity];

    int j = 0;
    for (size_t i = 0; i < _size; i++) {
        if ((int) i != index) {
            tmp_ent[j++] = _entries[i];
        }
    }

    delete[] _entries;

    _entries = tmp_ent;

    _size--;
    _last--;
}

template <typename T>
size_t my_vector<T>::size() { return _size; }

template <typename T>
my_vector<T> my_vector<T>::operator=(const my_vector &other) {
    delete[] _entries;
    _size = other._size;
    _capacity = other._capacity;
    _entries = new T[_capacity];
    for (size_t i = 0; i < _capacity; i++) {
        _entries[i] = other._entries[i];
    }

    return *this;
}

template class my_vector<my_string>;
template class my_vector<int>;
template class my_vector<my_vector<my_string>>;
