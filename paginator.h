#pragma once

#include <iostream>
#include <iterator>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end, size_t size) {
        size_ = size;
        begin_ = begin;
        end_ = end;
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }

private:
    size_t size_;
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t size) {
        while (std::distance(begin, end) >= size) {
            pages_.push_back({ begin, begin + size, size });
            std::advance(begin, size);
        }
        if (begin != end) {
            pages_.push_back({ begin, end, static_cast<size_t>(std::distance(end, begin)) });
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& page) {
    Iterator it = page.begin();
    while (it != page.end()) {
        output << *it;
        ++it;
    }

    return output;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}