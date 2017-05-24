#ifndef QUEUE_H_
#define QUEUE_H_

#include <cstring>

// To be used only as pointer. Copying doesn't work
template <typename T>
class queue {
private:
    class queue_node {
    private:
        T _entry;
        queue_node *_next;
        queue_node *_prev;
    public:
        queue_node(T ent, queue_node *next = nullptr, queue_node *prev = nullptr) {
            _entry = ent;
            _next = next;
            _prev = prev;
        }

        queue_node(const queue_node &other) {
            _entry = other._entry;
            if (other._next != nullptr)
                _next = new queue_node(other._next->_entry);

            if (other._prev != nullptr)
                _prev = new queue_node(other._prev->_entry);
        }

        ~queue_node() {
            // Do nothing here
        }

        queue_node *get_next() { return _next; }

        queue_node *get_prev() { return _prev; }

        void set_next(queue_node *next) {
            _next = next;
        }

        void add_next(queue_node *next) {
            _next = new queue_node(next->_entry);
        }

        void set_prev(queue_node *prev) {
            _prev = prev;
        }

        void add_prev(queue_node *prev) {
            _prev = new queue_node(prev->_entry);
        }

        T &get_entry() { return _entry; }

        queue_node &operator=(queue_node &other) {
            _entry = other._entry;
            _next = new queue_node(other._next->_entry);
            _prev = new queue_node(other._prev->_entry);
            return *this;
        }
    };

    queue_node *_head;
    queue_node *_last;
    size_t _size;
	int _max;
public:
    queue(int max = 256);
    queue(const queue&)=delete;
    ~queue();

    void push(T ent);

    bool empty();

    T pop();

    size_t size();

    T &peek();

    bool full();

    queue &operator=(const queue &other)=delete;

    void print();
};

#endif /* QUEUE_H_ */
