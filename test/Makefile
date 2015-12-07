all: mt_test read write

mt_test: mt_test.cpp
	g++ -x c++ -std=c++11 mt_test.cpp -o mt_test -pthread -O2 -Wall

read: read.c
	gcc -x c read.c -o read -O2 -Wall
	
write: write.c
	gcc -x c write.c -o write -O2 -Wall

clean:
	rm mt_test read write
