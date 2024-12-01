#include "search_server.h"

#include <numeric>


void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document ID is less than 0");
    }
    if (documents_statuses_.contains(document_id)) {
        throw std::invalid_argument("Document with this ID already added");
    }

    std::vector<std::string> document_words = SplitIntoWordsNoStop(document);
    double word_tf = 1.0 / document_words.size();
    documents_ratings_[document_id] = ComputeAverageRating(ratings);
    documents_statuses_[document_id] = status;
    for (const std::string& word : document_words) {
        documents_[word][document_id] += word_tf;
    }
    document_ids_.push_back(document_id);
    ++document_count_;

    return;
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
}


std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, [](int document_id, DocumentStatus document_status, int rating) { return document_status == DocumentStatus::ACTUAL; });
}


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    std::vector<std::string> plus_words;
    QueryWords query_words = ParseQuery(raw_query);
    std::tuple<std::vector<std::string>, DocumentStatus> result;

    for (const std::string& word : query_words.minus_words) {
        if (documents_.contains(word) && documents_.at(word).contains(document_id)) {
            result = std::tuple(plus_words, documents_statuses_.at(document_id));
            return result;
        }
    }

    for (const std::string& word : query_words.plus_words) {
        if (documents_.contains(word) && documents_.at(word).contains(document_id)) {
            plus_words.push_back(word);
        }
    }

    std::sort(plus_words.begin(), plus_words.end());
    result = std::tuple(plus_words, documents_statuses_.at(document_id));
    return result;
}


int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index >= static_cast<int>(document_ids_.size())) {
        throw std::out_of_range("ID of document is incorrrect");
    }

    return document_ids_.at(index);
}


int SearchServer::GetDocumentCount() const {
    return document_count_;
}


bool SearchServer::IsWordCorrect(const std::string& word) {
    for (const char c : word) {
        if (c >= '\0' && c < ' ') {
            return false;
        }
    }
    return true;
}


bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}


std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsWordCorrect(word)) {
            throw std::invalid_argument("The word \"" + word + "\" contains forbidden symbol");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }

    return words;
}


SearchServer::QueryWords SearchServer::ParseQuery(const std::string& text) const {
    QueryWords query_words;

    for (const std::string& word : SplitIntoWordsNoStop(text)) {
        if (word[0] == '-') {
            if (word.size() == 1) {
                throw std::invalid_argument("The minus word consists only from minus");
            }
            if (word[1] == '-') {
                throw std::invalid_argument("The minus word is misspelled");
            }
            query_words.minus_words.insert(word.substr(1));
        }
        else {
            query_words.plus_words.insert(word);
        }
    }

    return query_words;
}


int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.size() == 0) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}