#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <cassert>
#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <set>
#include <vector>
#include <thread>
#include <mutex>

char chars[26 + 26 + 10];
std::multiset<std::string> top30;
std::mutex m;

void do_read(int times) {
    while (times-- > 0) {
        char buf[512 * 1024];
        int fd = open("/dev/top30", O_RDONLY);
        size_t done = read(fd, buf, sizeof(buf));
        close(fd);

        std::multiset<std::string> got_top;
        std::string line;
        std::string bufStr = std::string(buf, done);
        std::istringstream ss(bufStr);
        std::vector<std::string> heap;
        while (getline(ss, line)) {
            got_top.insert(line);
            heap.push_back(line);
        }

        for (size_t j = 1; j < heap.size(); ++j) {
            if (heap[j] > heap[(j - 1) / 2]) {
                std::cerr << "FAIL HEAP" << std::endl;
                abort();
            }
        }

        assert(got_top.size() == heap.size());
    }
 }

void do_write(int times) {
    while (times-- > 0) {
        std::string s;
        int len = 10 + random() % 300;
        for (int j = 0; j < len; ++j) s += chars[rand() % sizeof(chars)];

        int fd = open("/dev/top30", O_WRONLY);
        write(fd, s.c_str(), s.length());
        close(fd);

        if (s.length() > 255) s.resize(255);

        std::lock_guard<std::mutex> guard(m);
        top30.insert(s);
        if (top30.size() > 30) {
            top30.erase(--top30.rbegin().base());
        }
    }
 }

int main(int argc, char** argv) {
    int counter = 0;
    for (int i = 0; i < 26; ++i) chars[counter++] = 'a' + i;
    for (int i = 0; i < 26; ++i) chars[counter++] = 'A' + i;
    for (int i = 0; i < 10; ++i) chars[counter++] = '0' + i;

    srand(time(0));
    for (size_t it = 0; it < 1000; ++it) {
        std::vector< std::thread > t;
        for (size_t i = 0; i < 50; ++i) {
            t.emplace_back(do_write, 50);
        }
        for (size_t i = 0; i < 50; ++i) {
            t.emplace_back(do_read, 50);
        }
        for (size_t i = 0; i < t.size(); ++i) {
            t[i].join();
        }

        char buf[512 * 1024];
        int fd = open("/dev/top30", O_RDONLY);
        size_t done = read(fd, buf, sizeof(buf));
        close(fd);

        std::multiset<std::string> got_top;
        std::string line;
        std::string bufStr = std::string(buf, done);
        std::istringstream ss(bufStr);
        std::vector<std::string> heap;
        while (getline(ss, line)) {
            got_top.insert(line);
            heap.push_back(line);
        }

        for (size_t j = 1; j < heap.size(); ++j) {
            if (heap[j] > heap[(j - 1) / 2]) {
                std::cerr << "FAIL HEAP" << std::endl;
                abort();
            }
        }

        if (got_top.size() != top30.size()) {
            std::cerr << "FAIL SIZE" << " Expected: " << top30.size() << ", Got: " << got_top.size() << std::endl;
            return 1;
        }

        if (got_top != top30) {
            std::cerr << "FAIL" << std::endl;
            std::cerr << "Expected\n";
            for (auto s: top30) {
                std::cerr << s << "\n";
            }
            std::cerr << "\nGot:\n";
            for (auto s: got_top) {
                std::cerr << s << "\n";
            }
            return 1;
        }

        std::cerr << "Iter #" << it << " is OK\n";
    }

    return 0;
}
