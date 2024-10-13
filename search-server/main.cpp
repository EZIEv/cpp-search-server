#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        vector<string> document_words = SplitIntoWordsNoStop(document);
		double word_tf = 1.0 / document_words.size();
        for (const string& word : document_words) {
            documents_[word][document_id] += word_tf;
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const QueryWords query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

private:
    struct QueryWords {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, map<int, double>> documents_;
    int document_count_ = 0;
    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }

        return words;
    }

    QueryWords ParseQuery(const string& text) const {
        QueryWords query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                query_words.minus_words.insert(word.substr(1));
            }
            else {
                query_words.plus_words.insert(word);
            }
        }

        return query_words;
    }

    vector<Document> FindAllDocuments(const QueryWords& query_words) const {
        vector<Document> matched_documents_vector;
        map<int, double> matched_documents;

        for (const auto& [word, documents_id_tf] : documents_) {
            if (query_words.plus_words.contains(word)) {
                double word_idf = log(static_cast<double>(document_count_) / documents_.at(word).size());
                for (const auto& [document_id, word_tf] : documents_id_tf) {
                    matched_documents[document_id] += word_idf * word_tf;
                }
            }
        }

        for (const string& minus_word : query_words.minus_words) {
            if (documents_.contains(minus_word)) {
                for (const auto& [document_id, word_tf] : documents_.at(minus_word)) {
                    if (matched_documents.contains(document_id)) {
                        matched_documents.erase(document_id);
                    }
                }
            }

        }

        for (const auto& [document_id, relevance] : matched_documents) {
            matched_documents_vector.push_back(Document{ document_id, relevance });
        }

        return matched_documents_vector;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();

    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}