# Functional Treap

An implementation of a Treap that uses path-copying to implement
mutating operations.

The public interface somewhat mimics the API exposed by
[std::multimap\<T\>](http://en.cppreference.com/w/cpp/container/multimap).

### Details

This is an implementation of a functional treap data structure. Every
mutating operation (insert/delete/update) doesn't mutate the existing
treap, but instead returns a new copy of the treap, with the mutation
applied. This means that every mutating operation on a treap
<b>must</b> capture the return value since that is the latest version
of the treap.

Since no existing version is mutated, it is entirely safe to perform
operations concurrently on your copy of the treap from multiple
threads. A treap internally holds an std::shared_ptr\<\> to the root
node. This automatically reclaims memory for unused (past) versions of
the treap, so memory management isn't an issue.

This treap also provides <b>random access iterators</b> (you heard
that right!) that cost O(log n) per random
increment/decrement. Incrementing using operator++/-- is O(1)
amortized, but incrementing/decrementing by 1 using operator+=/-= is
not (and costs O(log n) per increment/decrement). This is a design
decision. Using the random access iterator, you can (quickly) perform
operations like computing the number of elements between 2 given
iterators.
