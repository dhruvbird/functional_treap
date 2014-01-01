all: driver test

driver: driver.cpp treap.h
	g++ -std=c++0x -g driver.cpp -o driver

test: test.cpp treap.h
	g++ -std=c++0x -g test.cpp -o test
