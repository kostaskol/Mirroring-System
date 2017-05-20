#ifndef JMS_HASH_TABLE_H
#define JMS_HASH_TABLE_H


#include "my_string.h"

/* A simple hash table
 * Since we are not holding the key in the saved values
 * we keep two parallel index tables
 * The first for keys, the other for the actual entries
 *
 * A my_vector object is used to handle collisions
 *
 * The default size of the hashtable is 30
 */
template <typename T>
class hash_table {

private:
    my_vector<my_string> *_keys;
    my_vector<T> *_entries;
    int _size;

    int _hash_func(my_string key);
public:

    hash_table(int size = 30);
    ~hash_table();

    void insert_key(my_string key, T ent);

    bool get_key(my_string key, T *val);

    void set_key(my_string key, T ent);

    bool remove_key(my_string key);
};


#endif //JMS_HASH_TABLE_H
