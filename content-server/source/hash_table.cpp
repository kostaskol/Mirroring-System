#include <cstring>
#include <iostream>
#include <stdexcept>
#include "hash_table.h"
#include "my_string.h"

using namespace std;

template <typename T>
hash_table<T>::hash_table(int size) : _size(size) {
    _keys = new my_vector<my_string>[_size];
    _entries = new my_vector<T>[_size];
}

template <typename T>
hash_table<T>::~hash_table() {
    delete[] _keys;
    delete[] _entries;
}

template <typename T>
void hash_table<T>::insert_key(my_string key, T ent) {
    int index = _hash_func(key) % _size;
    if (index < 0 || index > _size - 1) {
        cerr << "Bad index: " << index << endl;
        return;
    }


	if (!_keys[index].in(key)) {
		_keys[index].push(key);
		_entries[index].push(ent);
	} else {
		set_key(key, ent);
	}
}

template <typename T>
bool hash_table<T>::get_key(my_string key, T *val) {
    int index = _hash_func(key) % _size;
    if (index < 0 || index > _size - 1) {
        throw std::runtime_error("Bad index " + index);
    }

    my_vector<T> tmp_t = _entries[index];
    my_vector<my_string> tmp_str = _keys[index];
    for (size_t i = 0; i < tmp_t.size(); i++) {
        if (tmp_str.at(i) == key) {
            *val = _entries[index].at(i);
            return true;
        }
    }

    return false;
}

template <typename T>
void hash_table<T>::set_key(my_string key, T ent) {
    int index = _hash_func(key) % _size;
    if (index < 0 || index > _size - 1) {
        cerr << "Bad index: " << index << endl;
        return;
    }

    my_vector<my_string> tmp_string = _keys[index];
    for (size_t i = 0; i < tmp_string.size(); i++) {
        if (tmp_string.at(i) == key) {
        	_entries[index].set_at(i, ent);
        	return;
        }
    }
}


template <typename T>
bool hash_table<T>::remove_key(my_string key) {
    int index = _hash_func(key) % _size;

    bool found = false;
    for (size_t i = 0; i < _keys[index].size(); i++) {
        if (_keys[index].at(i) == key) {
            _keys[index].remove_at(i);
            _entries[index].remove_at(i);
            found = true;
        }
    }

    return found;
}

template <typename T>
int hash_table<T>::_hash_func(my_string key) {
    char *str = new char[key.length() + 1];
    char *tmp = str;
    strcpy(str, key.c_str());
    int hash = 5381;
    int c;

    while ((c = *tmp++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    // If we get a negative result, return -result
    delete[] str;
    return hash > 0 ? hash : -hash;
}


template class hash_table<my_string>;
template class hash_table<int>;
