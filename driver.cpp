#include "treap.h"
#include <iostream>
#include <iterator>
#include <algorithm>
#include <assert.h>

using namespace std;
using namespace dhruvbird::functional;

template <typename T>
std::vector<typename T::value_type> vector_of(T const &t) {
  return std::vector<typename T::value_type>();
}

int main() {
  Treap<int> t;
  cout << "[Size: " << t.size() << "] ";
  t.print(cout) << std::endl;

  auto t1 = t.insert(10);
  cout << "[Size: " << t1.size() << "] ";
  t1.print(cout) << std::endl;

  auto t2 = t1.insert(201);
  cout << "[Size: " << t2.size() << "] ";
  t2.print(cout) << std::endl;

  auto t3 = t2;
  for (int i = 150; i < 1000; i += 50) {
    t3 = t3.insert(i);
    cout << "[Size: " << t3.size() << "] ";
    t3.print(cout) << std::endl;
  }

  for (int i = 955; i >= 100; i -= 50) {
    t3 = t3.insert(i);
    cout << "[Size: " << t3.size() << "] ";
    t3.print(cout) << std::endl;
  }

  cout<<"Iteration: ";
  for (auto it = t3.begin(); it != t3.end(); ++it) {
    cout << *it << ", ";
  }
  cout << std::endl;

  typedef Treap<int>::iterator iterator_type;

  cout<<"Revr Iter: ";
  auto revEnd = std::reverse_iterator<iterator_type>(t3.begin());
  for (auto it = std::reverse_iterator<iterator_type>(t3.end());
       it != revEnd; ++it) {
    cout << *it << ", ";
  }
  cout << endl;

  // t3.toDot(cout) << endl;

  cout << "[Size: " << t2.size() << "] ";
  t2.print(cout) << std::endl;

  
  auto it255End = t3.end();
  cout << "LB[255]..end: ";
  for (auto it255 = t3.lower_bound(255); it255 != it255End; ++it255) {
    cout << *it255 << ", ";
  }
  cout << endl;

  auto it820End = t3.end();
  cout << "LB[820]..end: ";
  std::copy(t3.lower_bound(820), it820End,
	    std::ostream_iterator<int>(cout, ", "));
  cout << endl;

  auto it770 = t3.upper_bound(770);
  cout << "begin..UB[770]: ";
  std::copy(t3.begin(), it770,
	    std::ostream_iterator<int>(cout, ", "));
  cout << endl;

  assert(t3.exists(300));
  assert(t3.exists(305));

  assert(!t3.exists(401));
  assert(!t3.exists(1001));

  auto t4 = t3.erase(201);
  cout << "[Size: " << t4.size() << "] ";
  t4.print(cout) << std::endl;

  // Remove all the even valued elements from 't3'
  {
    auto toErase = vector_of(t3);
    std::copy_if(t3.begin(), t3.end(), std::back_inserter(toErase),
		 [](int x) { return !(x % 2); });
    auto t5 = t3;
    for (auto x : toErase) {
      t5 = t5.erase(x);
    }
    cout << "[Size: " << t5.size() << "] ";
    t5.print(cout) << std::endl;
  }

  cout<<"t3 -->\n";
  cout << "[Size: " << t3.size() << "] ";
  t3.print(cout) << std::endl;

  assert(t3.exists(555));
  t4 = t3.erase(t3.find(555));
  cout<<"t4 -->\n";
  cout << "[Size: " << t4.size() << "] ";
  t4.print(cout) << std::endl;

  t4 = t4.update(600, 600);
  cout<<"t4 -->\n";
  cout << "[Size: " << t4.size() << "] ";
  t4.print(cout) << std::endl;

  {
    auto first = t4.lower_bound(155);
    auto last = t4.lower_bound(300);
    cout << "[Count (" << 155 << "-" << 300 << "): "
	 << std::distance(first, last) << "]" << endl;

    // t4.toDot(cout) << endl;
  }

  Treap<int> t5(t4.begin(), t4.end());
  cout << "[Size: " << t5.size() << "] ";
  t5.print(cout) << std::endl;

  std::vector<int> ints = { 98, 18, 19, 288, 1, 29, 12 };
  Treap<int> t6(ints.begin(), ints.end());
  cout << "[Size: " << t6.size() << "] ";
  t6.print(cout) << std::endl;

}
