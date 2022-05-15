CC=g++
CFLAGS=-shared -fPIC -fconcepts -Wno-format -std=c++17
TARGET=logger.so

all:
	$(CC) -o $(TARGET) $(CFLAGS) lib.cpp utils.hpp -ldl
	$(CC) -o logger hw2.cpp -std=c++17
