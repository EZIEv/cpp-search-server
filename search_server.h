#pragma once
#include "document.h"
#include "string_processing.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;


class SearchServer {
public:
    explicit SearchServer(const std::string& text) 
        : SearchServer(SplitIntoWords(text)) {}

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& word_collection);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename PredicateFunc>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, PredicateFunc predicate_func) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    int GetDocumentId(int index) const;

    int GetDocumentCount() const;

private:
    struct QueryWords {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::map<std::string, std::map<int, double>> documents_;
    std::map<int, int> documents_ratings_;
    std::map<int, DocumentStatus> documents_statuses_; 
    int document_count_ = 0;
    std::set<std::string> stop_words_;
    std::vector<int> document_ids_;

    static bool IsWordCorrect(const std::string& word);

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    QueryWords ParseQuery(const std::string& text) const;

    template <typename PredicateFunc>
    std::vector<Document> FindAllDocuments(const QueryWords& query_words, PredicateFunc predicate_func) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);
};


template <typename PredicateFunc>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, PredicateFunc predicate_func) const {
    const QueryWords query_words = ParseQuery(raw_query);
    std::vector<Document> result = FindAllDocuments(query_words, predicate_func);

    std::sort(result.begin(), result.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
        result.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return result;
}


template <typename StringCollection>
SearchServer::SearchServer(const StringCollection& word_collection) {
    for (const std::string& word : word_collection) {
        if (!IsWordCorrect(word)) {
            throw std::invalid_argument("Stop words are incorrect");
        }
        stop_words_.insert(word);
    }
}


template <typename PredicateFunc>
std::vector<Document> SearchServer::FindAllDocuments(const QueryWords& query_words, PredicateFunc predicate_func) const {
    std::vector<Document> matched_documents_vector;
    std::map<int, double> matched_documents;

    for (const auto& [word, documents_id_tf] : documents_) {
        if (query_words.plus_words.contains(word)) {
            double word_idf = std::log(static_cast<double>(document_count_) / documents_.at(word).size());
            for (const auto& [document_id, word_tf] : documents_id_tf) {
                if (predicate_func(document_id, documents_statuses_.at(document_id), documents_ratings_.at(document_id))) {
                    matched_documents[document_id] += word_idf * word_tf;
                }
            }
        }
    }

    for (const std::string& minus_word : query_words.minus_words) {
        if (documents_.contains(minus_word)) {
            for (const auto& [document_id, word_tf] : documents_.at(minus_word)) {
                if (matched_documents.contains(document_id)) {
                    matched_documents.erase(document_id);
                }
            }
        }
    }

    for (const auto& [document_id, relevance] : matched_documents) {
        matched_documents_vector.push_back(Document{ document_id, relevance, documents_ratings_.at(document_id) });
    }

    return matched_documents_vector;
}