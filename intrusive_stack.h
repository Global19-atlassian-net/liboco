#ifndef INTRUSIVE_STACK_H
#define INTRUSIVE_STACK_H

#include "do_not_copy.h"
#include "intrusive_link.h"

#include <cassert>
#include <cstddef>

template <class X>
struct intrusive_stack_link : private intrusive_link<X> {
  typedef intrusive_stack_link type;
  template <class T, typename intrusive_stack_link<T>::type T::*link>
    friend class intrusive_stack;

  bool bound() const { return intrusive_link<X>::p; }
};

template <class T, typename intrusive_stack_link<T>::type T::*link>
class intrusive_stack : public do_not_copy {
public:
  intrusive_stack() : head(reinterpret_cast<T*>(0xbad)) { }
  ~intrusive_stack() { assert(empty()); }

  bool empty() const { return head == reinterpret_cast<const T*>(0xbad); }

  intrusive_stack & push(T* t) {
    assert(!(t->*link).bound());

    (t->*link).p = head;
    head = t;

    assert((t->*link).bound());
    assert(!empty());
    return *this;
  }

  T* peek() const {
    assert(!empty());
    return head;
  }
  
  T* pop() {
    assert(!empty());

    T* t = head;
    assert((t->*link).bound());

    head = (t->*link).p;
    (t->*link).p = NULL;

    assert(!(t->*link).bound());
    return t;
  }

private:
  T * head;
};

#endif//INTRUSIVE_STACK_H
