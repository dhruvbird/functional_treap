/* -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <queue>
#include <set>
#include <sstream>
#include <vector>

#include <assert.h>
#include <stdio.h>

using namespace std;

namespace dhruvbird { namespace functional {
  enum class ChildDirection {
    LEFT, RIGHT
  };

  const int _treap_random_seed = 6781;

  template <typename T, typename LessThan>
  class Treap;

  template <typename T>
  struct TreapNode {
    T data;
    int heapKey;
    size_t subtreeSize;
    mutable std::shared_ptr<TreapNode> left, right;

    TreapNode(T const &_data,
              int _heapKey,
              size_t _subtreeSize,
              std::shared_ptr<TreapNode> _left = nullptr,
              std::shared_ptr<TreapNode> _right = nullptr)
      : data(_data), heapKey(_heapKey), subtreeSize(_subtreeSize),
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
         this->subtreeSize,
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
#if 0
    fprintf(stderr, "rotateLeft(%d[%d], %d[%d])\n", node->data, node->heapKey,
      parent->data, parent->heapKey);
#endif
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
#if 0
    fprintf(stderr, "rotateRight(%d[%d], %d[%d])\n", node->data, node->heapKey,
      parent->data, parent->heapKey);
#endif
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
    // Note: Comparators are broken here.
    if (node->data < parent->data) {
      assert(node->isLeftChildOf(parent));
    } else if (parent->data < node->data) {
      assert(node->isRightChildOf(parent));
    }
#endif

    if (node->isLeftChildOf(parent)) {
      rotateRight(node, parent, grandParent);
    } else {
      rotateLeft(node, parent, grandParent);
    }
    // 'parent' and 'node' are now swapped. First set the size on
    // 'node' and then on 'parent'.
    node->subtreeSize = (node->left ? node->left->subtreeSize : 0) +
      (node->right ? node->right->subtreeSize : 0) + 1;
    parent->subtreeSize = (parent->left ? parent->left->subtreeSize : 0) +
      (parent->right ? parent->right->subtreeSize : 0) + 1;
  }

  class RNGIterator : public std::iterator<std::forward_iterator_tag, const int> {
    unsigned int seed;
    int rno;

  public:
    RNGIterator (unsigned int seed_) : seed(seed_), rno(0) {
      ++(*this);
    }

    RNGIterator& operator++() {
      this->rno = rand_r(&this->seed);
      return *this;
    }

    RNGIterator operator++(int) {
      RNGIterator ret = *this;
      ++(*this);
      return ret;
    }

    int operator*() const {
      return this->rno;
    }

  };

  template <typename T, typename LessThan=std::less<T> >
  class TreapIterator : public std::iterator<std::random_access_iterator_tag, const T> {
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

    /**
     * Returns the rank of the iterator '*this'. i.e. It returns the
     * number of elements < the element pointed to by '*this'.
     */
    size_t rank() const {
      if (!this->root) return 0;
      if (this->ptrs.empty()) {
        /* This is the end() iterator */
        return this->root->subtreeSize;
      }

      /* Stores the # of elements >= *this */
      size_t count = 0;
      count = ptrs[0]->subtreeSize;
      for (size_t i = 1; i < ptrs.size(); ++i) {
        if (ptrs[i]->isRightChildOf(ptrs[i-1])) {
          // Everything to the right is kosher.
          count -= ptrs[i-1]->subtreeSize;
          count += ptrs[i]->subtreeSize;
        }
      }
      count -= (ptrs.back()->left ? ptrs.back()->left->subtreeSize : 0);
      // Return # of elements < *this
      return root->subtreeSize - count;
    }

    /**
     * Moves the current iterator to point to the rank'th
     * element. Ranks start from 0. The k'th element is the k'th
     * smallest element in the treap.
     */
    void moveToRank(const size_t rank) {
      assert(rank > 0 && this->root || rank == 0);
      if (!this->root) return;
      assert(rank <= this->root->subtreeSize);
      if (rank == this->root->subtreeSize) {
        // Move to end()
        this->ptrs.clear();
        return;
      }
      PtrsType path(1, this->root);
      size_t currRank = this->root->left ? this->root->left->subtreeSize : 0;
      while (currRank != rank) {
        if (currRank < rank) {
          path.push_back(path.back()->right);
          currRank += 1;
        } else {
          path.push_back(path.back()->left);
          currRank -= path.back()->subtreeSize;
        }
        currRank += path.back()->left ? path.back()->left->subtreeSize : 0;
      }
      ptrs = std::move(path);
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

    TreapIterator& operator+=(const off_t offset) const {
      this->moveToRank(static_cast<off_t>(this->rank()) + offset);
      return *this;
    }

    TreapIterator& operator-=(const off_t offset) const {
      this->moveToRank(static_cast<off_t>(this->rank()) - offset);
      return *this;
    }

    /**
     * Cost: O(log n)
     */
    off_t operator-(TreapIterator const& other) const {
      const off_t otherRank = other.rank();
      const off_t thisRank = this->rank();
      return thisRank - otherRank;
    }

    T const& operator[](off_t offset) const {
      if (offset == 0) return **this;
      auto other = *this;
      other += offset;
      return *other;
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

  /**
   * This is an implementation of a functional treap data
   * structure. Every mutating operation (insert/delete/update)
   * doesn't mutate the existing treap, but instead returns a new copy
   * of the treap, with the mutation applied. This means that every
   * mutating operation on a treap *must* capture the return value
   * since that is the latest version of the treap.
   *
   * Since no existing version is mutated, it is entirely safe to
   * perform operations concurrently on your copy of the treap from
   * multiple threads. A treap internally holds an std::shared_ptr<>
   * to the root node. This automatically reclaims memory for unused
   * (past) versions of the treap, so memory management isn't an
   * issue.
   *
   * This treap also provides _random access iterators_ (you heard
   * that right!) that cost O(log n) per random
   * increment/decrement. Incrementing using operator++/-- is O(1)
   * amortized, but incrementing/decrementing by 1 using operator+=/-=
   * is not (and costs O(log n) per increment/decrement). This is a
   * design decision. Using the random access iterator, you can
   * (quickly) perform operations like computing the number of
   * elements between 2 given iterators.
   *
   */
  template <typename T, typename LessThan=std::less<T> >
  class Treap {
    typedef TreapNode<T> NodeType;
    typedef std::shared_ptr<NodeType> NodePtrType;
    mutable NodePtrType root;
    unsigned int seed;

  public:
    typedef TreapIterator<T, LessThan> iterator;
    typedef TreapIterator<T, LessThan> const_iterator;
    typedef T value_type;

  private:
    /**
     * Inserts a new node 'node' into the tree rooted at this->root,
     * and returns a pointer to the new root. The existing tree is
     * un-modified, and a new root to node path is created. Nodes that
     * are unchanged are shared between the older and the newer tree.
     *
     */
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
        ptrs.back()->subtreeSize++;
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

    /**
     * Inserts a new node 'node' into the tree rooted at this->root,
     * and returns a pointer to the new root. The existing tree is
     * *modified*.
     *
     */
    NodePtrType insertNodeNoClone(NodePtrType node) const {
      if (!root.get()) {
        return node;
      }
      LessThan lt;
      NodePtrType tmp = root;
      std::vector<NodePtrType> ptrs;
      while (tmp) {
        ptrs.push_back(tmp);
        ptrs.back()->subtreeSize++;
        if (lt(node->data, tmp->data)) {
          tmp = tmp->left;
        } else {
          tmp = tmp->right;
        }
      }
      tmp = ptrs.back();
      if (lt(node->data, tmp->data)) {
        tmp->left = node;
      } else {
        tmp->right = node;
      }
      ptrs.push_back(node);

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

    /**
     * Clone the path present in 'ptrs' and return a list of cloned
     * nodes with their left and right pointers set to the new nodes
     * (if appropriate).
     *
     */
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

    /**
     * Delete the current tree's root node and return a pointer to the
     * new root node. Uses path copying to copy the root ro node path,
     * and the older tree remains unmodified.
     *
     */
    NodePtrType deleteRootNode() const {
      auto newRoot = this->root;
      if (!newRoot->right) {
        newRoot = newRoot->left;
      } else if (!newRoot->left) {
        newRoot = newRoot->right;
      } else {
        newRoot = this->root->clone();
        auto leftChild = newRoot->left;
        newRoot->left = nullptr;
        auto rightChild = newRoot->right;
        Treap t(rightChild);
        auto parts = t.deleteAndGetBegin();
        parts.first->left = leftChild;
        parts.first->right = parts.second;
        newRoot = parts.first;
        // Set subtreeSize and heapKey for 'newRoot'
        newRoot->subtreeSize = this->root->subtreeSize - 1;
        newRoot->heapKey = this->root->heapKey;
      }
      return newRoot;
    }

    std::pair<NodePtrType /* begin */, NodePtrType /* rest */>
    deleteAndGetBegin() const {
      auto first = this->begin();
      auto ptrs = first.getRootToNodePtrs();
      auto succPtr = ptrs.back()->clone();
      succPtr->subtreeSize = 1;
      succPtr->left = succPtr->right = nullptr;
      auto newRoot = this->deleteIterator(first);
      return std::make_pair(succPtr, newRoot);
    }

    NodePtrType deleteIterator(iterator const &it) const {
      auto ptrs = this->clonePtrs(it.getRootToNodePtrs());
      assert(!ptrs.empty());

      if (ptrs.size() == 1) { // Delete the root node
        return this->deleteRootNode();
      }

      for (auto &ptr : ptrs) {
        ptr->subtreeSize--;
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
        auto parts = t.deleteAndGetBegin();
        auto succPtr = parts.first;
        auto newRoot = parts.second;

        succPtr->left = delPtr->left;
        succPtr->right = newRoot;
        succPtr->subtreeSize = (succPtr->left ? succPtr->left->subtreeSize : 0) +
          (succPtr->right ? succPtr->right->subtreeSize : 0) + 1;

        // FIXME: succPtr->heapKey
        succPtr->heapKey = parPtr->heapKey;

        if (delPtr->isLeftChildOf(parPtr)) {
          parPtr->left = succPtr;
        } else {
          parPtr->right = succPtr;
        }
        // No need to update parPtr->subtreeSize since it has already
        // been decremented by 1.
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
    void levelorder(NodePtrType n, Func f) const {
      if (!n.get()) return;
      std::queue<NodePtrType> q;
      q.push(n);
      while (!q.empty()) {
        auto top = q.front();
        q.pop();
        if (top->left) q.push(top->left);
        if (top->right) q.push(top->right);
        f(top->data, top);
      }
    }

    template <typename Func>
    void inorder(NodePtrType n, Func f) const {
      // std::cout << "n.get(): " << n.get() << endl;
      if (!n.get()) return;
      inorder(n->left, f);
      f(n->data, n);
      inorder(n->right, f);
    }

    Treap(NodePtrType _root) :
      root(_root), seed(_treap_random_seed) { }

    size_t countGte(iterator const &it) const {
      size_t count = 0;
      auto const &ptrs = it.getRootToNodePtrs();
      if (ptrs.empty()) {
        return 0;
      }
      count = ptrs[0]->subtreeSize;
      for (size_t i = 1; i < ptrs.size(); ++i) {
        if (ptrs[i]->isRightChildOf(ptrs[i-1])) {
          // Everything to the right is kosher.
          count -= ptrs[i-1]->subtreeSize;
          count += ptrs[i]->subtreeSize;
        }
      }
      count -= (ptrs.back()->left ? ptrs.back()->left->subtreeSize : 0);
      return count;
    }

    template <typename Iter>
    void fillNodes(Iter first, Iter last,
                   std::vector<NodePtrType> &nodes) const {
      for (; first != last; ++first) {
        nodes.push_back(std::make_shared<NodeType>(*first, 0, 1));
      }
    }

    /**
     * Precondition: last - first > 0
     */
    template <typename Iter>
    void assignSorted(Iter first, Iter last) {
      std::vector<NodePtrType> allNodes;
      this->fillNodes(first, last, allNodes);

      std::vector<NodePtrType> nodes[2];

#define NODE_GET(IDX) ((IDX) < nodes[1].size() ? nodes[1][IDX] : nullptr)
      size_t start = 1;
      size_t hjump = 2;
      while (nodes[0].size() != 1) {
        nodes[0].swap(nodes[1]);
        nodes[0].clear();
        size_t ctr = 0;
#if 0
        fprintf(stderr, "New Set [start: %d] [hjump: %d]\n", start, hjump);
#endif

        for (size_t i = start - 1; i < allNodes.size(); i += hjump) {
          auto &nn = allNodes[i];
          nn->left = NODE_GET(ctr); ctr++;
          nn->right = NODE_GET(ctr); ctr++;
          nn->subtreeSize = (nn->left ? nn->left->subtreeSize : 0) +
            (nn->right ? nn->right->subtreeSize : 0) + 1;
          // cerr << nn->subtreeSize << ", ";
          nodes[0].push_back(nn);
        }
        // Copy residual nodes from nodes[1] to nodes[0].
        while (ctr < nodes[1].size()) {
          nodes[0].push_back(nodes[1][ctr++]);
        }
        start *= 2;
        hjump *= 2;
        // cerr << std::endl;
      }
#undef NODE_GET
      this->root = nodes[0][0];
      std::vector<int> allHeapKeys;
      // Assign heap keys.
      const int seed = 8271;
      std::copy_n(RNGIterator(seed), this->size(),
                  std::back_inserter(allHeapKeys));
      std::transform(allHeapKeys.begin(), allHeapKeys.end(),
                     allHeapKeys.begin(),
                     [&] (int rno) { return rno % (this->size() * 12 + 1); });
      std::sort(allHeapKeys.begin(), allHeapKeys.end());
      auto heapKeysIt = allHeapKeys.begin();
      this->levelorder(this->root, [&heapKeysIt] (T const&, NodePtrType &node) {
          node->heapKey = *heapKeysIt;
          ++heapKeysIt;
        });
    }

  public:
    Treap() : seed(_treap_random_seed) { }
    Treap(Treap const &rhs) {
      this->root = rhs.root;
      this->seed = rhs.seed;
    }
    /* No MOVE semantics since this is an immutable data structure. */
    Treap& operator=(Treap const &rhs) {
      this->root = rhs.root;
      this->seed = rhs.seed;
      return *this;
    }
    /**
     * Bulk load from a possibly sorted set.
     */
    template <typename Iter>
    Treap(Iter first, Iter last) : seed(_treap_random_seed) {
      // Do we have at least 2 elements?
      if (first == last) return;
      auto f = first;
      ++f;
      if (f == last) {
        // Single element
        const int heapKey = rand_r(&this->seed) % (this->size() * 12 + 1);
        this->root = std::make_shared<NodeType>(*f, heapKey, 1);
        return;
      }

      // Invariant: last - first > 1.
      //
      // Check if input range is sorted.
      if (std::adjacent_find(first, last,
                             [] (typename Iter::value_type const &lhs,
                                 typename Iter::value_type const &rhs) {
                               return LessThan()(rhs, lhs);
                             }) != last) {
        // Unsorted: O(n log n)
        for (; first != last; ++first) {
          const int heapKey = rand_r(&this->seed) % (this->size() * 12 + 1);
          this->root = this->insertNodeNoClone(std::make_shared<NodeType>(*first, heapKey, 1));
        }
      } else {
        // Sorted: O(n)
        this->assignSorted(first, last);
      }
    }

    size_t size() const {
      return this->root ? this->root->subtreeSize : 0;
    }

    bool empty() const {
      return this->root ? false : true;
    }

    Treap insert(T const &data) const {
      Treap newTreap(*this);
      auto _seed = this->seed;
      const int heapKey = rand_r(&_seed) % (this->size() * 12 + 1);
      newTreap.root = newTreap.insertNode(std::make_shared<NodeType>(data, heapKey, 1));
      newTreap.seed = _seed;
      return newTreap;
    }

    /**
     * Erases the first found element with KEY == key. Returns a new
     * treap with the element removed. The first found element isn't
     * necessarily the first element inserted with KEY == key.
     */
    Treap erase(T const &key) const {
      Treap newTreap(*this);
      newTreap.root = newTreap.deleteKey(key);
      return newTreap;
    }

    /**
     * Erases the element pointed to by iterator 'it'. Returns a new
     * treap with the element removed.
     */
    Treap erase(iterator const &it) const {
      assert(it != this->end());
      assert(it.root == this->root);
      assert(this->root.get() != nullptr);
      Treap newTreap(*this);
      newTreap.root = newTreap.deleteIterator(it);
      return newTreap;
    }

    /**
     * Update the Treap and replace oldKey with newKey. It is assumed
     * that newKey fits in *exactly* the same place as oldKey. If not,
     * this will violate the BST properties.
     */
    Treap update(T const &oldKey, T const &newKey) const {
      assert(!LessThan()(oldKey, newKey) && !LessThan()(newKey, oldKey));
      auto it = this->find(oldKey);
      if (it == this->end()) {
        return *this;
      }
      auto ptrs = this->clonePtrs(it.getRootToNodePtrs());
      ptrs.back()->data = newKey;
      Treap newTreap(ptrs[0]);
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

    /**
     * Count the number of elements with KEY == key.
     *
     * Complexity: O(log n)
     *
     */
    size_t count(T const& key) const {
      iterator first = this->lower_bound(key);
      iterator last = this->upper_bound(key);
      return last - first;
    }

    std::ostream& print(std::ostream &out) const {
      this->for_each([&out](T const &data, NodePtrType node) {
          out << data << ", ";
        });
      return out;
    }

    /**
     * Print a graphviz consumable tree representation of this
     * treap. Values in parenthesis represent heap keys and
     * subtree-sizes.
     *
     * Node format: Key(heapKey,subtreeSize)
     *
     */
    std::ostream& toDot(std::ostream &out) const {
      out << "digraph Treap {\n";
      this->for_each([&out](T const &data, NodePtrType node) {
          std::ostringstream buff1, buff2, buff3;
          buff1 << node->data << "(" << node->heapKey << "," << node->subtreeSize << ")";
          if (node->left) {
            buff2 << node->left->data << "(" << node->left->heapKey << "," << node->left->subtreeSize << ")";
          } else {
            buff2 << node.get() << "L";
            out << "  \"" << buff2.str() << "\"[shape=point]\n";
          }
          out << "  \"" << buff1.str() << "\" -> \"" << buff2.str() << "\"[label=L]\n";
          if (node->right) {
            buff3 << node->right->data << "(" << node->right->heapKey << "," << node->right->subtreeSize << ")";
          } else {
            buff3 << node.get() << "R";
            out << "  \"" << buff3.str() << "\"[shape=point]\n";
          }
          out << "  \"" << buff1.str() << "\" -> \"" << buff3.str() << "\"[label=R]\n";
        });
      out << "}\n";
      return out;
    }

    /**
     * Apply function 'f' to every element in the treap (in sorted
     * order).
     */
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

  template <typename T, typename LessThan=std::less<T> >
  class MockTreap {
    typedef std::multiset<T, LessThan> impl_type;
    mutable impl_type impl;

  public:
    typedef typename impl_type::const_iterator iterator;
    typedef typename impl_type::const_iterator const_iterator;
    typedef T value_type;

    MockTreap() { }

    template <typename Iter>
    MockTreap(Iter first, Iter last)
      : impl(first, last) { }

    size_t size() const {
      return this->impl.size();
    }

    MockTreap insert(T const &data) const {
      MockTreap other(*this);
      other.impl.insert(data);
      return other;
    }

    MockTreap erase(iterator it) const {
      MockTreap other(*this);
      other.impl.swap(this->impl);
      other.impl.erase(it);
      return other;
    }

    MockTreap erase(T const &key) const {
      auto it = this->find(key);
      if (it != this->end()) {
        return this->erase(it);
      }
      return *this;
    }

    bool exists(T const &key) const {
      return this->find(key) != this->end();
    }

    MockTreap update(T const &oldKey, T const &newKey) const {
      MockTreap other(*this);
      auto it = other.impl.find(oldKey);
      if (it != other.impl.end()) {
        other.impl.erase(it);
        other.impl.insert(newKey);
      }
      return other;
    }

    iterator lower_bound(T const &key) const {
      return this->impl.lower_bound(key);
    }

    iterator upper_bound(T const &key) const {
      return this->impl.upper_bound(key);
    }

    iterator find(T const &key) const {
      return this->impl.find(key);
    }

    size_t count(T const& key) const {
      return this->impl.count(key);
    }

    template <typename Func>
    void for_each(Func f) const {
      for (auto it = this->begin(); it != this->end(); ++it) {
        f(*it, nullptr);
      }
    }

    iterator begin() const {
      return this->impl.begin();
    }

    iterator end() const {
      return this->impl.end();
    }
  };


}}
