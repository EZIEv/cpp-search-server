#include "read_input_functions.h"

#include <iostream>


std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}


int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}


std::vector<int> ReadNumbersSplitedWithSpace() {
    std::vector<int> numbers;
    int n = 0;
    std::cin >> n;
    for (int i = 0; i < n; ++i) {
        int k;
        std::cin >> k;
        numbers.push_back(k);
    }
    ReadLine();
    return numbers;
}