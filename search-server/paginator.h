#pragma once

#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "document.h"

using namespace std::literals::string_literals;

template <typename Iterator>
class IteratorRange {
public:
    explicit IteratorRange(Iterator container_begin, Iterator container_end)
        : begin_(container_begin)
        , end_(container_end)
    {
    }
    IteratorRange() = default;

    const Iterator begin() const {
        return begin_;
    }
    const Iterator end() const {
        return end_;
    }
private:
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator container_begin, Iterator container_end, size_t page_size)
        : begin_(container_begin)
        , end_(container_end)
        , page_size_(page_size)
    {
        GetPagesIterators();
    }
    Paginator() = default;

    const auto begin() const {
        return iterator_ranges_.begin();
    }
    const auto end() const {
        return iterator_ranges_.end();
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t page_size_;
    std::vector<IteratorRange<Iterator>> iterator_ranges_;

    void GetPagesIterators() {
        Iterator start = begin_;
        Iterator end;
        if (static_cast<int>(distance(start, end_)) < page_size_) {
            iterator_ranges_.push_back(IteratorRange(start, end_));
        }
        else {
            for (int i = 0; i < (static_cast<size_t>(distance(begin_, end_)) / page_size_); ++i) {
                end = next(start, page_size_);
                iterator_ranges_.push_back(IteratorRange(start, end));
                start = end;
            }
            if (static_cast<int>(distance(start, end_)) != 0) {
                iterator_ranges_.push_back(IteratorRange(start, end_));
            }
        }
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

std::ostream& operator<<(std::ostream& os, const Document& document) {
    os << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s;
    return os;
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& page) {
    for (auto document = page.begin(); document != page.end(); ++document) {
        os << *document;
    }
    return os;
}
