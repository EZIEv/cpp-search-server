#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

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

vector<int> ReadNumbersSplitedWithSpace() {
    vector<int> numbers;
    int n = 0;
    cin >> n;
    for (int i = 0; i < n; ++i) {
        int k;
        cin >> k;
        numbers.push_back(k);
    }
    ReadLine();
    return numbers;
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

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        vector<string> document_words = SplitIntoWordsNoStop(document);
        double word_tf = 1.0 / document_words.size();
        documents_ratings_[document_id] = ComputeAverageRating(ratings);
        documents_statuses_[document_id] = status;
        for (const string& word : document_words) {
            documents_[word][document_id] += word_tf;
        }
        ++document_count_;
    }

    template <typename PredicateFunc>
    vector<Document> FindTopDocuments(const string& raw_query, PredicateFunc predicate_func) const {
        const QueryWords query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, predicate_func);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    int GetDocumentCount() const {
        return document_count_;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        vector<string> plus_words;
        QueryWords query_words = ParseQuery(raw_query);

        for (const string& word : query_words.minus_words) {
            if (documents_.contains(word) && documents_.at(word).contains(document_id)) {
                return tuple(plus_words, documents_statuses_.at(document_id));
            }
        }

        for (const string& word : query_words.plus_words) {
            if (documents_.contains(word) && documents_.at(word).contains(document_id)) {
                plus_words.push_back(word);
            }
        }

        sort(plus_words.begin(), plus_words.end());
        return tuple(plus_words, documents_statuses_.at(document_id));
    }

private:
    struct QueryWords {
        set<string> plus_words;
        set<string> minus_words;
    };

    map<string, map<int, double>> documents_;
    map<int, int> documents_ratings_;
    map<int, DocumentStatus> documents_statuses_; 
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

    template <typename PredicateFunc>
    vector<Document> FindAllDocuments(const QueryWords& query_words, PredicateFunc predicate_func) const {
        vector<Document> matched_documents_vector;
        map<int, double> matched_documents;

        for (const auto& [word, documents_id_tf] : documents_) {
            if (query_words.plus_words.contains(word)) {
                double word_idf = log(static_cast<double>(document_count_) / documents_.at(word).size());
                for (const auto& [document_id, word_tf] : documents_id_tf) {
                    if (predicate_func(document_id, documents_statuses_.at(document_id), documents_ratings_.at(document_id))) {
                        matched_documents[document_id] += word_idf * word_tf;
                    }
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
            matched_documents_vector.push_back(Document{ document_id, relevance, documents_ratings_.at(document_id) });
        }

        return matched_documents_vector;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.size() == 0) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }
};

/*SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        string document = ReadLine();
        vector<int> ratings = ReadNumbersSplitedWithSpace();
        search_server.AddDocument(document_id, document, ratings);
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();

    for (const auto& [document_id, relevance, rating] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "s
            << "relevance = "s << relevance << ", "s
            << "rating = "s << rating << " }"s << endl;
    }
}
*/

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}