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
#include <ctime>


void do_read(int times) {
    while (times-- > 0) {
        char buf[512 * 1024];
        int fd = open("/dev/top30", O_RDONLY);
        size_t done = read(fd, buf, sizeof(buf));
        close(fd);

        std::multiset<std::string> got_top;
        std::string line;
        std::istringstream ss(std::string(buf, done));
        std::vector<std::string> heap;
        while (getline(ss, line)) {
            got_top.insert(line);
            heap.push_back(line);
        }

        for (size_t j = 1; j < heap.size(); ++j) {
            if (heap[j] > heap[(j - 1) / 2]) {
                std::cerr << "FAIL HEAP" << std::endl;
                std::terminate();
            }
        }

        assert(got_top.size() == heap.size());
    }
 }

void do_write(int times, const std::vector<std::string>& words) {
    while (times-- > 0) {
        int fd = open("/dev/top30", O_WRONLY);
        const std::string& s = words[times];
        if (write(fd, s.c_str(), s.length()) != (int)s.length()) {
            std::terminate();
        }
        close(fd);
    }
 }

int main(int argc, char** argv) {
    std::multiset<std::string> top30;

    char chars[26 + 26 + 10];
    int counter = 0;
    for (int i = 0; i < 26; ++i) chars[counter++] = 'a' + i;
    for (int i = 0; i < 26; ++i) chars[counter++] = 'A' + i;
    for (int i = 0; i < 10; ++i) chars[counter++] = '0' + i;

    srand(time(0));
    for (size_t it = 0; it < 20; ++it) {
        clock_t start_time = clock();

        std::vector< std::vector<std::string> > words;
        for (size_t i = 0; i < 50; ++i) {
            words.emplace_back();
            for (size_t j = 0; j < 5000; ++j) {
                std::string s;
                int len = 10 + random() % 300;
                for (int j = 0; j < len; ++j) s += chars[rand() % sizeof(chars)];
                words.back().emplace_back(s);
            }
        }

        std::vector< std::thread > t;
        for (size_t i = 0; i < words.size(); ++i) {
            t.emplace_back(do_write, words[i].size(), std::ref(words[i]));
            t.emplace_back(do_read, words[i].size());
        }
        for (size_t i = 0; i < t.size(); ++i) {
            t[i].join();
        }

        for (size_t i = 0; i < words.size(); ++i) {
            for (auto s : words[i]) {
                if (s.length() > 255) {
                    s.resize(255);
                }
                top30.insert(s);
                if (top30.size() > 30) {
                    top30.erase(--top30.rbegin().base());
                }
            }
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

        std::cerr << "Iter #" << it << " is OK (" << double(clock() - start_time) / CLOCKS_PER_SEC << "s)\n";
    }

    return 0;
}
