#include "treap.h"
#include <iostream>

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

  // t3.toDot(cout) << endl;

  cout << "[Size: " << t2.size() << "] ";
  t2.print(cout) << std::endl;
}
