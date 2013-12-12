/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include <new>
#include <memory>
#include <vector>
#include <sstream>
#include <assert.h>

#include <iostream>
#include <stdio.h>

using namespace std;

namespace dhruvbird { namespace functional {
  enum class ChildDirection {
    LEFT, RIGHT
  };

  template <typename T>
  struct TreapNode {
    T data;
    int heapKey;
    mutable std::shared_ptr<TreapNode> left, right;

    TreapNode(T const &_data,
              int _heapKey,
              std::shared_ptr<TreapNode> _left = nullptr,
              std::shared_ptr<TreapNode> _right = nullptr)
      : data(_data), heapKey(_heapKey),
        left(_left), right(_right) { }

    bool isLeftChildOf(std::shared_ptr<TreapNode> &parent) const {
      return parent->left.get() == this;
    }

    bool isRightChildOf(std::shared_ptr<TreapNode> &parent) const {
      return parent->right.get() == this;
    }

    std::shared_ptr<TreapNode> clone() const {
      auto copy = std::make_shared<TreapNode>
        (this->data,
         this->heapKey,
         this->left,
         this->right);
      return copy;
    }
  };

  template <typename T>
  void rotateLeft(std::shared_ptr<TreapNode<T> > &node,
                  std::shared_ptr<TreapNode<T> > &parent,
                  std::shared_ptr<TreapNode<T> > &grandParent) {
    fprintf(stderr, "rotateLeft(%d[%d], %d[%d])\n", node->data, node->heapKey,
      parent->data, parent->heapKey);

    assert(node->isRightChildOf(parent));
    auto nLeft = node->left;
    node->left = parent;
    parent->right = nLeft;
    if (grandParent.get()) {
      if (parent->isLeftChildOf(grandParent)) {
        grandParent->left = node;
      } else {
        grandParent->right = node;
      }
    }
    node.swap(parent);
  }

  template <typename T>
  void rotateRight(std::shared_ptr<TreapNode<T> > &node,
                   std::shared_ptr<TreapNode<T> > &parent,
                   std::shared_ptr<TreapNode<T> > &grandParent) {
    fprintf(stderr, "rotateRight(%d[%d], %d[%d])\n", node->data, node->heapKey,
      parent->data, parent->heapKey);
    assert(node->isLeftChildOf(parent));
    auto nRight = node->right;
    node->right = parent;
    parent->left = nRight;
    if (grandParent.get()) {
      if (parent->isLeftChildOf(grandParent)) {
        grandParent->left = node;
      } else {
        grandParent->right = node;
      }
    }
    node.swap(parent);
  }

  template <typename T>
  void rotateUp(std::shared_ptr<TreapNode<T> > &node,
                std::shared_ptr<TreapNode<T> > &parent,
                std::shared_ptr<TreapNode<T> > &grandParent) {
    assert(node->isLeftChildOf(parent) || node->isRightChildOf(parent));
#if !defined NDEBUG
    if (node->data < parent->data) {
      assert(node->isLeftChildOf(parent));
    } else if (node->data > parent->data) {
      assert(node->isRightChildOf(parent));
    }
#endif

    if (node->isLeftChildOf(parent)) {
      rotateRight(node, parent, grandParent);
    } else {
      rotateLeft(node, parent, grandParent);
    }
  }

  template <typename T, typename LessThan=std::less<T> >
  class TreapIterator {
    typedef TreapNode<T> NodeType;
    typedef std::shared_ptr<NodeType> NodePtrType;
    typedef std::vector<NodePtrType> PtrsType;
    PtrsType ptrs;

  public:
    TreapIterator() { }
    TreapIterator(PtrsType _ptrs) : ptrs(_ptrs) { }

    TreapIterator& operator++() {
      assert(!ptrs.empty());
      auto tmp = ptrs.back();
      if (tmp->right.get()) {
        tmp = tmp->right;
        ptrs.push_back(tmp);
        while (tmp->left.get()) {
          tmp = tmp->left;
          ptrs.push_back(tmp);
        }
        return *this;
      }
      if (ptrs.size() > 1) {
        size_t ptrx = ptrs.size() - 1;
        // Right-Child
        while (ptrs.size() > 1 && ptrs[ptrx]->isRightChildOf(ptrs[ptrx - 1])) {
          --ptrx;
          ptrs.pop_back();
        }
        if (ptrs.size() > 1 && ptrs[ptrx]->isLeftChildOf(ptrs[ptrx - 1])) {
          ptrs.pop_back();
          return *this;
        }
      }
      assert(ptrs.size() == 1);
      // Clean-up since ptrs.size() == 1
      ptrs.pop_back();
      return *this;
    }

    T const& operator*() const {
      return ptrs.back()->data;
    }

    const T* operator->() const {
      return &(ptrs.back()->data);
    }

    bool operator==(TreapIterator const &rhs) const {
      return this->ptrs == rhs.ptrs;
    }

    bool operator!=(TreapIterator const &rhs) const {
      return !(*this == rhs);
    }

  };

  template <typename T, typename LessThan=std::less<T> >
  class Treap {
    typedef TreapNode<T> NodeType;
    typedef std::shared_ptr<NodeType> NodePtrType;
    mutable NodePtrType root;
    size_t _size;

    NodePtrType insertNode(NodePtrType node) const {
      if (!root.get()) {
        return node;
      }
      LessThan lt;
      NodePtrType tmp = root;
      std::vector<NodePtrType> ptrs;
      ChildDirection dirn = ChildDirection::LEFT;
      std::vector<ChildDirection> dirns;
      while (tmp) {
        ptrs.push_back(tmp->clone());
        dirns.push_back(dirn);
        if (lt(node->data, tmp->data)) {
          dirn = ChildDirection::LEFT;
          tmp = tmp->left;
        } else {
          dirn = ChildDirection::RIGHT;
          tmp = tmp->right;
        }
      }
      tmp = ptrs.back();
      if (lt(node->data, tmp->data)) {
        dirns.push_back(ChildDirection::LEFT);
        tmp->left = node;
      } else {
        dirns.push_back(ChildDirection::RIGHT);
        tmp->right = node;
      }
      ptrs.push_back(node);

      for (size_t i = 1; i < ptrs.size(); ++i) {
        if (dirns[i] == ChildDirection::LEFT) {
          ptrs[i-1]->left = ptrs[i];
        } else {
          ptrs[i-1]->right = ptrs[i];
        }
      }

      size_t ptrx = ptrs.size() - 1;
      // While we have node, parent, grand-parent (i.e. at least 3
      // nodes).
      while (ptrx > 1 && ptrs[ptrx]->heapKey < ptrs[ptrx - 1]->heapKey) {
        rotateUp(ptrs[ptrx], ptrs[ptrx - 1], ptrs[ptrx - 2]);
        --ptrx;
      }
      assert(ptrx > 0);
      if (ptrs[ptrx]->heapKey < ptrs[ptrx - 1]->heapKey) {
        NodePtrType grandParent;
        rotateUp(ptrs[ptrx], ptrs[ptrx - 1], grandParent);
      }
      return ptrs[0];
    }

    template <typename Func>
    void inorder(NodePtrType n, Func f) const {
      // std::cout << "n.get(): " << n.get() << endl;
      if (!n.get()) return;
      inorder(n->left, f);
      f(n->data, n);
      inorder(n->right, f);
    }

  public:
    Treap() : _size(0) { }
    Treap(Treap const &rhs) {
      this->root = rhs.root;
      this->_size = rhs._size;
    }
    Treap& operator=(Treap const &rhs) {
      this->root = rhs.root;
      this->_size = rhs._size;
      return *this;
    }

    size_t size() const {
      return this->_size;
    }

    Treap insert(T const &data) const {
      Treap newTreap(*this);
      const int heapKey = rand() % 100;
      newTreap.root = newTreap.insertNode(std::make_shared<NodeType>(data, heapKey));
      newTreap._size = this->size() + 1;
      return newTreap;
    }

    std::ostream& print(std::ostream &out) const {
      this->inorder(this->root, [&out](T const &data, NodePtrType node) {
          out << data << ", ";
        });
      return out;
    }

    std::ostream& toDot(std::ostream &out) const {
      out << "digraph Treap {\n";
      this->inorder(this->root, [&out](T const &data, NodePtrType node) {
          std::ostringstream buff1, buff2, buff3;
          buff1 << node->data << "(" << node->heapKey << ")";
          if (node->left) {
            buff2 << node->left->data << "(" << node->left->heapKey << ")";
          } else {
            buff2 << node.get() << "L";
            out << "  \"" << buff2.str() << "\"[shape=point]\n";
          }
          out << "  \"" << buff1.str() << "\" -> \"" << buff2.str() << "\"[label=L]\n";
          if (node->right) {
            buff3 << node->right->data << "(" << node->right->heapKey << ")";
          } else {
            buff3 << node.get() << "R";
            out << "  \"" << buff3.str() << "\"[shape=point]\n";
          }
          out << "  \"" << buff1.str() << "\" -> \"" << buff3.str() << "\"[label=R]\n";
        });
      out << "}\n";
      return out;
    }

    TreapIterator<T, LessThan> begin() const {
      std::vector<NodePtrType> ptrs;
      auto tmp = this->root;
      while (tmp) {
        ptrs.push_back(tmp);
        tmp = tmp->left;
      }
      TreapIterator<T, LessThan> it(ptrs);
      return it;
    }

    TreapIterator<T, LessThan> end() const {
      return TreapIterator<T, LessThan>();
    }

  };



}}

