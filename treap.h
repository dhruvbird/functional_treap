/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include <new>
#include <memory>
#include <vector>
#include <sstream>
#include <iterator>
#include <assert.h>

#include <iostream>
#include <stdio.h>

using namespace std;

namespace dhruvbird { namespace functional {
  enum class ChildDirection {
    LEFT, RIGHT
  };

  template <typename T, typename LessThan>
  class Treap;

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

    bool isLeftChildOf(std::shared_ptr<TreapNode> const &parent) const {
      return parent->left.get() == this;
    }

    bool isRightChildOf(std::shared_ptr<TreapNode> const &parent) const {
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

  /**
   * Rotate 'node' left around its 'parent'.
   */
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

  /**
   * Rotate 'node' right around its 'parent'.
   */
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

  /**
   * Rotate 'node' up (left or right) around its 'parent'.
   */
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
  class TreapIterator : public std::iterator<std::bidirectional_iterator_tag, const T> {
    typedef TreapNode<T> NodeType;
    typedef std::shared_ptr<NodeType> NodePtrType;
    typedef std::vector<NodePtrType> PtrsType;
    // 'ptrs' stores all node pointers from root up to the current
    // node, following both the left and right children. It doesn't
    // matter which child we follow since finding the successor
    // [operator++] or the predecessor [operator--] can be done
    // entirely by looking at the current node and the path from the
    // root to this node, all of which are stored in 'ptrs'.
    //
    // Storing 'ptrs' allows us to achieve amortized O(1) cost of
    // iterating over the whole collection. If we re-computed the path
    // from the root to the current node every time we wanted to visit
    // the parent node, we would have to incur a cost of O(n log n) to
    // iterate over the entire collection.
    //
    PtrsType ptrs;
    NodePtrType root;
    friend class Treap<T, LessThan>;

    /**
     * Returns the current state of the iterator. i.e. The path from
     * the root node to the current node.
     */
    PtrsType const& getRootToNodePtrs() const {
      return this->ptrs;
    }

  public:
    TreapIterator() { }
    TreapIterator(PtrsType &&_ptrs, NodePtrType _root)
      : ptrs(_ptrs), root(_root) { }

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

    TreapIterator operator++(int) const {
      TreapIterator it = *this;
      ++*this;
      return it;
    }

    TreapIterator& operator--() {
      if (ptrs.empty()) {
        auto tmp = this->root;
        while (tmp) {
          ptrs.push_back(tmp);
          tmp = tmp->right;
        }
        return *this;
      }

      auto tmp = ptrs.back();
      if (tmp->left.get()) {
        tmp = tmp->left;
        ptrs.push_back(tmp);
        while (tmp->right.get()) {
          tmp = tmp->right;
          ptrs.push_back(tmp);
        }
        return *this;
      }
      if (ptrs.size() > 1) {
        size_t ptrx = ptrs.size() - 1;
        // Right-Child
        while (ptrs.size() > 1 && ptrs[ptrx]->isLeftChildOf(ptrs[ptrx - 1])) {
          --ptrx;
          ptrs.pop_back();
        }
        if (ptrs.size() > 1 && ptrs[ptrx]->isRightChildOf(ptrs[ptrx - 1])) {
          ptrs.pop_back();
          return *this;
        }
      }
      // We backed up all the way before the first node. This is an
      // error.
      assert(ptrs.size() != 1);
      return *this;
    }

    TreapIterator operator--(int) const {
      TreapIterator it = *this;
      --*this;
      return it;
    }

    T const& operator*() const {
      return ptrs.back()->data;
    }

    const T* operator->() const {
      return &(ptrs.back()->data);
    }

    bool operator==(TreapIterator const &rhs) const {
      return this->ptrs == rhs.ptrs && this->root == rhs.root;
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

  public:
    typedef TreapIterator<T, LessThan> iterator;
    typedef TreapIterator<T, LessThan> const_iterator;
    typedef T value_type;

  private:
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

    std::vector<NodePtrType>
    clonePtrs(std::vector<NodePtrType> const &ptrs) const {
      std::vector<NodePtrType> clonedPtrs;
      assert(!ptrs.empty());
      clonedPtrs.reserve(ptrs.size());
      clonedPtrs.push_back(ptrs[0]->clone());
      for (size_t i = 1; i < ptrs.size(); ++i) {
        clonedPtrs.push_back(ptrs[i]->clone());
        if (ptrs[i]->isLeftChildOf(ptrs[i-1])) {
          clonedPtrs[i-1]->left = clonedPtrs[i];
        } else {
          clonedPtrs[i-1]->right = clonedPtrs[i];
        }
      }
      return clonedPtrs;
    }

    NodePtrType deleteRootNode() const {
      auto newRoot = this->root;
      if (!newRoot->right) {
        newRoot = newRoot->left;
      } else if (!newRoot->left) {
        newRoot = newRoot->right;
      } else {
        Treap t(this->root);
        auto parts = t.deleteAndGetBegin();
        parts.first->left = this->root->left;
        parts.first->right = parts.second;
        newRoot = parts.first;
      }
      return newRoot;
    }

    std::pair<NodePtrType /* begin */, NodePtrType /* rest */>
    deleteAndGetBegin() const {
      auto first = this->begin();
      auto succPtr = first.getRootToNodePtrs().back()->clone();
      auto newRoot = this->deleteIterator(first);
      return std::make_pair(succPtr, newRoot);
    }

    NodePtrType deleteIterator(iterator const &it) const {
      auto ptrs = this->clonePtrs(it.getRootToNodePtrs());
      assert(!ptrs.empty());

      if (ptrs.size() == 1) { // Delete the root node
        return this->deleteRootNode();
      }

      auto parPtr = ptrs[ptrs.size() - 2];
      auto delPtr = ptrs[ptrs.size() - 1];
      if (!delPtr->left || !delPtr->right) {
        if (!delPtr->left) {
          if (delPtr->isLeftChildOf(parPtr)) {
            parPtr->left = delPtr->right;
          } else {
            parPtr->right = delPtr->right;
          }
        } else {
          if (delPtr->isLeftChildOf(parPtr)) {
            parPtr->left = delPtr->left;
          } else {
            parPtr->right = delPtr->left;
          }
        }
      } else {
        // Has both child nodes.
        Treap t(delPtr->right);
        // The '_size' of 't' doesn't matter.
        auto parts = t.deleteAndGetBegin();
        auto succPtr = parts.first;
        auto newRoot = parts.second;

        succPtr->left = delPtr->left;
        succPtr->right = newRoot;

        if (delPtr->isLeftChildOf(parPtr)) {
          parPtr->left = succPtr;
        } else {
          parPtr->right = succPtr;
        }
      }
      return ptrs[0];
    }

    NodePtrType deleteKey(T const &key) const {
      LessThan lt;
      auto keyIt = this->lower_bound(key);
      if (keyIt == this->end()) {
        return this->root;
      }
      if (!lt(key, *keyIt) && !lt(*keyIt, key)) { // key == *keyIt
        return this->deleteIterator(keyIt);
      }
      return this->root;
    }

    template <typename Func>
    void inorder(NodePtrType n, Func f) const {
      // std::cout << "n.get(): " << n.get() << endl;
      if (!n.get()) return;
      inorder(n->left, f);
      f(n->data, n);
      inorder(n->right, f);
    }

    Treap(NodePtrType _root) : root(_root), _size(0) { }

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

    Treap erase(T const &key) const {
      Treap newTreap(*this);
      newTreap.root = newTreap.deleteKey(key);
      newTreap._size -= (newTreap.root == this->root ? 0 : 1);
      return newTreap;
    }

    Treap erase(iterator const &it) const {
      assert(it != this->end());
      assert(it.root == this->root);
      assert(this->root.get() != nullptr);
      Treap newTreap(*this);
      newTreap.root = newTreap.deleteIterator(it);
      newTreap._size -= 1;
      return newTreap;
    }

    bool exists(T const &key) const {
      NodePtrType tmp = root;
      LessThan lt;
      while (tmp) {
        if (lt(key, tmp->data)) {
          tmp = tmp->left;
        } else if (lt(tmp->data, key)) {
          tmp = tmp->right;
        } else {
          return true;
        }
      }
      return false;
    }

    /**
     * The first position before which we can insert 'key' and remain
     * sorted.
     */
    iterator lower_bound(T const &key) const {
      NodePtrType tmp = root;
      LessThan lt;
      std::vector<NodePtrType> ptrs;
      size_t capSize = 0;
      while (tmp) {
        ptrs.push_back(tmp);
        if (!lt(tmp->data, key)) { // key <= tmp->data
          capSize = ptrs.size();
          tmp = tmp->left;
        } else { // key > tmp->data
          tmp = tmp->right;
        }
      }
      ptrs.resize(capSize);
      return iterator(std::move(ptrs), this->root);
    }

    /**
     * The last position before which we can insert 'key' and remain
     * sorted.
     */
    iterator upper_bound(T const &key) const {
      NodePtrType tmp = root;
      LessThan lt;
      std::vector<NodePtrType> ptrs;
      size_t capSize = 0;
      while (tmp) {
        ptrs.push_back(tmp);
        if (lt(key, tmp->data)) { // key < tmp->data
          capSize = ptrs.size();
          tmp = tmp->left;
        } else { // key >= tmp->data
          tmp = tmp->right;
        }
      }
      ptrs.resize(capSize);
      return iterator(std::move(ptrs), this->root);
    }

    iterator find(T const &key) const {
      iterator it = this->lower_bound(key);
      LessThan lt;
      if (!lt(key, *it) && !lt(*it, key)) {
        return it;
      }
      return this->end();
    }

    std::ostream& print(std::ostream &out) const {
      this->for_each([&out](T const &data, NodePtrType node) {
          out << data << ", ";
        });
      return out;
    }

    std::ostream& toDot(std::ostream &out) const {
      out << "digraph Treap {\n";
      this->for_each([&out](T const &data, NodePtrType node) {
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

    template <typename Func>
    void for_each(Func f) const {
      this->inorder(this->root, f);
    }

    iterator begin() const {
      std::vector<NodePtrType> ptrs;
      auto tmp = this->root;
      while (tmp) {
        ptrs.push_back(tmp);
        tmp = tmp->left;
      }
      return iterator(std::move(ptrs), this->root);
    }

    iterator end() const {
      return iterator({ }, this->root);
    }

  };



}}

