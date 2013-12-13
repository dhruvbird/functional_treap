#include "treap.h"
#include <iostream>
#include <iterator>
#include <assert.h>

using namespace std;
using namespace dhruvbird::functional;

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
  cout << endl;

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
  for (auto it820 = t3.lower_bound(820); it820 != it820End; ++it820) {
    cout << *it820 << ", ";
  }
  cout << endl;

  auto it770 = t3.upper_bound(770);
  cout << "begin..UB[770]: ";
  for (auto it = t3.begin(); it != it770; ++it) {
    cout << *it << ", ";
  }
  cout << endl;

  assert(t3.exists(300));
  assert(t3.exists(305));

  assert(!t3.exists(401));
  assert(!t3.exists(1001));

}
