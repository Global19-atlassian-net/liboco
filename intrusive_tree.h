#ifndef INTRUSIVE_TREE_H
#define INTRUSIVE_TREE_H

#include "do_not_copy.h"
#include "intrusive_link.h"

#include <cassert>
#include <cstddef>
#include <algorithm>

template <class X>
class intrusive_tree_link : private intrusive_link<X> {
public:
  typedef intrusive_tree_link type;
  template <class T, typename intrusive_tree_link<T>::type T::*link,
            bool (T::*predicate)(const T &) const>
    friend class intrusive_tree;

  bool bound() const {
    assert(p.p || (!l.p && !r.p));
    return p.p;
  }

private:
  bool red() const { return !black(); }
  bool black() const { return intptr_t(p.p) & 0x1; }

  void toggle() { p.p = (X*)(intptr_t(p.p) ^ 0x1); }
  X* up() const { return (X*)(intptr_t(p.p) & ~0x1); }

  intrusive_link<X> p, l, r;
};

template <class T, typename intrusive_tree_link<T>::type T::*link,
          bool (T::*predicate)(const T &) const>
class intrusive_tree : public do_not_copy {
public:
  intrusive_tree() : root_(NULL) { }
  ~intrusive_tree() { assert(empty()); }

  bool empty() const { return !root_; }

  intrusive_tree & insert(T* t) {
    assert(valid());
    assert(!is_bound(t));

    T ** i = &root_;
    while (*i) {
      (t->*link).p.p = *i;
      i = (t->*predicate)(**i)
        ? &((*i)->*link).l.p
        : &((*i)->*link).r.p
        ;
    }
    *i = t;

    assert(is_red(t));

    T* n = t;
    while (true) {
      assert(n);

      T* p = parent_(n);
      if (!p) {
        assert(is_red(n));
        if (is_red(n))
          toggle(n);
        break;
      }

      if (is_black(p))
        break;

      T* g = parent_(p);
      if (!g)
        break;

      T* u = peer(p);
      if (!u || is_black(u)) {
        if (is_right(n) && is_left(p)) {
          assert(is_black(g));
          assert(is_red(p));
          assert(is_red(n));

          rotate_left(p);

          assert(is_black(g));
          assert(is_red(p));
          assert(is_red(n));

          using std::swap;
          swap(n,p);
        } else if (is_left(n) && is_right(p)) {
          assert(is_black(g));
          assert(is_red(p));
          assert(is_red(n));

          rotate_right(p);

          assert(is_black(g));
          assert(is_red(p));
          assert(is_red(n));

          using std::swap;
          swap(n,p);
        }

        assert(!is_left(n) || is_left(p));
        assert(!is_right(n) || is_right(p));

        assert(is_black(g));
        assert(is_red(p));
        assert(is_red(n));

        if (is_left(n))
          rotate_right(g);
        else // if (is_right(n))
          rotate_left(g);

        toggle(g);
        toggle(p);

        assert(is_red(g));
        assert(is_black(p));
        assert(is_red(n));

        break;
      }

      assert(is_black(g));
      assert(is_red(p));
      assert(is_red(u));

      toggle(g);
      toggle(p);
      toggle(u);

      n = g;
    }

    assert(is_member(t));
    assert(is_bound(t));
    assert(!empty());
    assert(valid());

    return *this;
  }

  T* remove(T* t) {
    assert(valid());
    assert(!empty());
    assert(is_bound(t));
    assert(is_member(t));

    static int i = 0;
    if (left_(t) && right_(t))
      node_swap(t, (++i % 2) ? rightest_(left_(t)): leftest_(right_(t)));

    assert(!left_(t) || !right_(t));

    T* c = left_(t) ? take_left(t)
         : right_(t) ? take_right(t)
         : NULL;

    if (c)
      replace(t, c);

    if (is_red(t)) {
      if (!c)
        unlink(t);
      return t;
    }

    toggle(t);

    if (is_red(c)) {
      assert(c);
      toggle(c);
      return t;
    }

    T* n = c ? c : t;
    while (!is_root(n)) {
      T* p = parent_(n);
      T* s = peer(n);

      if (is_red(s)) {
        assert(is_black(p));
        toggle(p);
        toggle(s);

        if (is_left(n))
          rotate_left(p);
        else // if (is_right(n))
          rotate_right(p);

        p = parent_(n);
        s = peer(n);
      }

      T* sl = s ? left_(s) : NULL;
      T* sr = s ? right_(s) : NULL;

      if (is_red(p) && is_black(s) && is_black(sl) && is_black(sr)) {
        assert(s);
        toggle(s);
        toggle(p);
        break;
      }

      if (is_red(p) || is_red(sl) || is_red(sr)) {
        assert(is_black(s));
        if (is_left(n) && is_black(sr)) {
          assert(is_red(sl));
          toggle(s);
          toggle(sl);
          rotate_right(s);
        } else if (is_right(n) && is_black(sl)) {
          assert(is_red(sr));
          toggle(s);
          toggle(sr);
          rotate_left(s);
        }

        p = parent_(n);
        s = peer(n);
        sl = left_(s);
        sr = right_(s);

        if (is_red(p)) {
          toggle(p);
          if (is_black(s))
            toggle(s);
        }

        if (is_left(n)) {
          if (is_red(sr))
            toggle(sr);
          rotate_left(p);
        } else /* if (is_right(n)) */ {
          if (is_red(sl))
            toggle(sl);
          rotate_right(p);
        }

        break;
      }

      assert(is_black(p));
      assert(is_black(s));
      assert(is_black(sl));
      assert(is_black(sr));

      assert(s);
      toggle(s);
      n = p;
    }

    if (!c)
      unlink(t);

    assert(!is_bound(t));
    assert(valid());

    return t;
  }

  void swap(intrusive_tree & that) {
    using std::swap;
    swap(this->root_, that.root_);
  }

  bool is_member(const T* n) const {
    assert(n);
    return !empty() && eldest_(n) == root_;
  }

  T* root() const {
    assert(!empty());
    return root_;
  }

  T* parent(const T* n) const {
    assert(is_member(n));
    return parent_(n);
  }

  T* left(const T* n) const {
    assert(is_member(n));
    return left_(n);
  }

  T* right(const T* n) const {
    assert(is_member(n));
    return right_(n);
  }

  T* eldest(const T* n) const {
    assert(is_member(n));
    return eldest_(n);
  }

  T* leftest(const T* n) const {
    assert(is_member(n));
    return leftest_(n);
  }

  T* rightest(const T* n) const {
    assert(is_member(n));
    return rightest_(n);
  }

  T* min() const { return leftest(root()); }
  T* max() const { return rightest(root()); }

  T* next(const T* n) const {
    assert(is_member(n));

    if (T* r = right_(n))
      return leftest_(r);

    do {
      T* p = parent_(n);
      if (is_left(n))
        return p;
      n = p;
    } while (!is_root(n));
    
    return NULL;
  }

  T* prev(const T* n) const {
    assert(is_member(n));

    if (T* r = left_(n))
      return rightest_(r);

    do {
      T* p = parent_(n);
      if (is_right(n))
        return p;
      n = p;
    } while (!is_root(n));
    
    return NULL;
  }

private:
  T * root_;

  bool is_red(const T* n) const { return n && (n->*link).red(); }
  bool is_black(const T* n) const { return !n || (n->*link).black(); }
  bool is_bound(const T* n) const { assert(n); return (n->*link).bound(); }

  void toggle(T* n) const { assert(n); (n->*link).toggle(); }

  T* parent_(const T* n) const { assert(n); return (n->*link).up(); }
  T* left_(const T* n) const { assert(n); return (n->*link).l.p; }
  T* right_(const T* n) const { assert(n); return (n->*link).r.p; }

  T* eldest_(const T* n) const {
    assert(n);
    while (const T* c = parent_(n))
      n = c;
    return const_cast<T*>(n);
  }

  T* leftest_(const T* n) const {
    assert(n);
    while (const T* c = left_(n))
      n = c;
    return const_cast<T*>(n);
  }

  T* rightest_(const T* n) const {
    assert(n);
    while (const T* c = right_(n))
      n = c;
    return const_cast<T*>(n);
  }

  T* peer(const T* n) const {
    assert(!is_root(n));
    return is_left(n) ? right_(parent_(n)) : left_(parent_(n));
  }

  bool is_root(const T* n) const {
    assert(parent_(n) || n == root_);
    return !empty() && n == root_;
  }

  bool is_left(const T* n) const {
    assert(!is_root(n));
    return n == left_(parent_(n));
  }

  bool is_right(const T* n) const {
    assert(!is_root(n));
    return n == right_(parent_(n));
  }

  void unlink(T* n) {
    assert(n);
    assert(is_member(n));
    assert(!left_(n));
    assert(!right_(n));

    if (is_root(n))
      link_root(NULL);
    else if (is_left(n))
      link_left(parent_(n), NULL);
    else // if (is_right(n))
      link_right(parent_(n), NULL);

    bool b = is_black(n);
    (n->*link).p.p = NULL;
    if (b)
      toggle(n);
  }

  void link_root(T* n) {
    root_ = n;
    if (n)
      link_parent(NULL, n);
  }

  void link_left(T* p, T* c) {
    assert(p);
    if (c)
      link_parent(p, c);
    (p->*link).l.p = c;
  }

  void link_right(T* p, T* c) {
    assert(p);
    if (c)
      link_parent(p, c);
    (p->*link).r.p = c;
  }

  void link_parent(T* p, T* c) {
    assert(c);
    bool black = is_black(c);
    (c->*link).p.p = p;
    if (black)
      toggle(c);
  }

  void rotate_left(T* p) {
    T* g = parent_(p);
    T* n = right_(p);
    T* c = left_(n);
    if (is_root(p))
      link_root(n);
    else if (is_left(p))
      link_left(g, n);
    else // if (is_right(p))
      link_right(g, n);

    link_left(n, p);
    link_right(p, c);
  }

  void rotate_right(T* p) {
    T* g = parent_(p);
    T* n = left_(p);
    T* c = right_(n);
    if (is_root(p))
      link_root(n);
    else if (is_left(p))
      link_left(g, n);
    else // if (is_right(p))
      link_right(g, n);

    link_right(n, p);
    link_left(p, c);
  }

  void root_swap(T* n) {
    assert(n);

    if (is_left(n))
      link_left(parent_(n), root_);
    else // if (is_right(n))
      link_right(parent_(n), root_);

    link_root(n);
  }

  void left_swap(T* foo, T* bar) {
    T* fc = left_(foo);
    T* bc = left_(bar);

    link_left(foo, bc);
    link_left(bar, fc);
  }

  void right_swap(T* foo, T* bar) {
    T* fc = right_(foo);
    T* bc = right_(bar);

    link_right(foo, bc);
    link_right(bar, fc);
  }

  void parent_swap(T* foo, T* bar) {
    assert(!is_root(foo));
    assert(!is_root(bar));

    T* fp = parent_(foo);
    T* bp = parent_(bar);

    bool fl = is_left(foo);
    bool bl = is_left(bar);

    if (fl)
      link_left(fp, bar);
    else // if (fr)
      link_right(fp, bar);

    if (bl)
      link_left(bp, foo);
    else // if (br)
      link_right(bp, foo);
  }

  void node_swap(T* foo, T* bar) {
    if (is_root(foo))
      root_swap(bar);
    else if (is_root(bar))
      root_swap(foo);
    else
      parent_swap(foo, bar);

    if (is_black(foo) != is_black(bar)) {
      toggle(bar);
      toggle(foo);
    }

    left_swap(foo, bar);
    right_swap(foo, bar);
  }

  T* take_left(T* t) {
    T* c = left_(t);
    assert(c);
    link_left(t, NULL);
    link_parent(NULL, c);
    return c;
  }

  T* take_right(T* t) {
    T* c = right_(t);
    assert(c);
    link_right(t, NULL);
    link_parent(NULL, c);
    return c;
  }

  void replace(T* o, T* n) {
    if (is_root(o)) {
      unlink(o);
      link_root(n);
    } else if (is_left(o)) {
      T* p = parent_(o);
      unlink(o);
      link_left(p, n);
    } else /* if (is_right(t)) */ {
      T* p = parent_(o);
      unlink(o);
      link_right(p, n);
    }
  }

  unsigned depth(const T* t) const {
    if (!t)
      return 1;
    assert(depth(left_(t)) == depth(right_(t)));
    return (is_black(t) ? 1 : 0) + depth(left_(t));
  }

  bool valid(const T* t) const {
    if (!t)
      return true;

    T* l = left_(t);
    T* r = right_(t);

    if (is_red(t)) {
      if (l && !is_black(l))
        return false;
      if (r && !is_black(r))
        return false;
    }

    if (l && (t->*predicate)(*l))
      return false;
    if (r && (r->*predicate)(*t))
      return false;

    return valid(l) && valid(r);
  }

  bool valid() const {
    if (empty())
      return true;
    if (!is_black(root_))
      return false;
    if (depth(left_(root_)) != depth(right_(root_)))
      return false;
    return valid(root_);
  }

};

#endif//INTRUSIVE_TREE
