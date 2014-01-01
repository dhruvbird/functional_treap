#include "treap.h"
#include <iostream>
#include <iterator>
#include <algorithm>
#include <assert.h>

using namespace std;
using namespace dhruvbird::functional;

// #define VERBOSE_TREAPS 1 // To enable

void test_construct() {
    Treap<int> t;
    assert(t.size() == 0);
    assert(t.empty());
}

template <typename C, typename Seq>
void insert_sequence(C &container, Seq const &seq) {
    for (auto &elem : seq) {
        container = container.insert(elem);
    }
}

void test_insertion() {
    {
        Treap<int> t;
        MockTreap<int> mt;
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        insert_sequence(t, seq);
        insert_sequence(mt, seq);

        assert(t.size() == mt.size());
        assert(std::equal(t.begin(), t.end(), mt.begin()));
        assert(std::equal(mt.begin(), mt.end(), t.begin()));
    }

    {
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        Treap<int> t(seq.begin(), seq.end());
        MockTreap<int> mt(seq.begin(), seq.end());

        assert(t.size() == mt.size());
        assert(std::equal(t.begin(), t.end(), mt.begin()));
        assert(std::equal(mt.begin(), mt.end(), t.begin()));
    }

    {
        Treap<int> t;
        MockTreap<int> mt;
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        std::sort(seq.begin(), seq.end());
        insert_sequence(t, seq);
        insert_sequence(mt, seq);

        assert(t.size() == mt.size());
        assert(std::equal(t.begin(), t.end(), mt.begin()));
        assert(std::equal(mt.begin(), mt.end(), t.begin()));
    }

    {
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        std::sort(seq.begin(), seq.end());
        Treap<int> t(seq.begin(), seq.end());
        MockTreap<int> mt(seq.begin(), seq.end());

        assert(t.size() == mt.size());
        assert(std::equal(t.begin(), t.end(), mt.begin()));
        assert(std::equal(mt.begin(), mt.end(), t.begin()));
    }
}

void test_deletion() {
    {
        Treap<int> t;
        MockTreap<int> mt;
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        insert_sequence(t, seq);
        insert_sequence(mt, seq);

        std::random_shuffle(seq.begin(), seq.end());
        for (auto &elem : seq) {
            t = t.erase(elem);
            mt = mt.erase(elem);
            assert(t.size() == mt.size());
            assert(std::equal(t.begin(), t.end(), mt.begin()));
            assert(std::equal(mt.begin(), mt.end(), t.begin()));
        }
    }

    {
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        Treap<int> t(seq.begin(), seq.end());
        MockTreap<int> mt(seq.begin(), seq.end());

        assert(t.size() == mt.size());

        std::random_shuffle(seq.begin(), seq.end());
        for (auto &elem : seq) {
            t = t.erase(elem);
#if VERBOSE_TREAPS == 1
            t.toDot(std::cerr) << endl;
#endif
            auto x = std::bind(&Treap<int>::print, t);
            mt = mt.erase(elem);
            assert(t.size() == mt.size());

#if VERBOSE_TREAPS == 1
            cerr << "Treap: ";
            std::copy(t.begin(), t.end(),
                      std::ostream_iterator<int>(cerr, ", "));
            cerr << endl;

            cerr << "Mock Treap: ";
            std::copy(mt.begin(), mt.end(),
                      std::ostream_iterator<int>(cerr, ", "));
            cerr << endl;
#endif
            assert(std::equal(t.begin(), t.end(), mt.begin()));
            assert(std::equal(mt.begin(), mt.end(), t.begin()));
        }
    }

    {
        Treap<int> t;
        MockTreap<int> mt;
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        std::sort(seq.begin(), seq.end());
        insert_sequence(t, seq);
        insert_sequence(mt, seq);

        std::random_shuffle(seq.begin(), seq.end());
        for (auto &elem : seq) {
            t = t.erase(elem);
            mt = mt.erase(elem);
            assert(t.size() == mt.size());
            assert(std::equal(t.begin(), t.end(), mt.begin()));
            assert(std::equal(mt.begin(), mt.end(), t.begin()));
        }
    }

    {
        vector<int> seq(20);
        copy_n(RNGIterator(6271), seq.size(), seq.begin());
        std::sort(seq.begin(), seq.end());
        Treap<int> t(seq.begin(), seq.end());
        MockTreap<int> mt(seq.begin(), seq.end());

        assert(t.size() == mt.size());

        std::random_shuffle(seq.begin(), seq.end());
        for (auto &elem : seq) {
            t = t.erase(elem);
            auto x = std::bind(&Treap<int>::print, t);
            mt = mt.erase(elem);
            assert(t.size() == mt.size());
            assert(std::equal(t.begin(), t.end(), mt.begin()));
            assert(std::equal(mt.begin(), mt.end(), t.begin()));
        }
    }
}

int main() {
    test_construct();
    test_insertion();
    test_deletion();
}
