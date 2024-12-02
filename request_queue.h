#pragma once
#include "document.h"
#include "search_server.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

class RequestQueue {
public:
    RequestQueue(const SearchServer& search_server) : search_server_(search_server) {}

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        bool is_empty;
        std::uint64_t time;
    };
    const SearchServer& search_server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    std::uint64_t cur_time_ = 1;
    std::uint64_t no_result_requests_ = 0;
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);

    requests_.push_back({ result.empty(), cur_time_ });
    if (result.empty()) {
        ++no_result_requests_;
    }
    if (cur_time_ > min_in_day_) {
        if (requests_.front().is_empty) {
            --no_result_requests_;
        }
        requests_.pop_front();
    }
    ++cur_time_;

    return result;
}