all: mt_test read write

mt_test: mt_test.cpp
	g++ -x c++ -std=c++11 mt_test.cpp -o mt_test -pthread -std=c++11

read: read.c
	gcc -x c read.c -o read
	
write: write.c
	gcc -x c write.c -o write

clean:
	rm mt_test read write
