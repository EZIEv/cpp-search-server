#include <algorithm>
#include <cmath>
#include <cstdlib>
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

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const string& test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

void TestAddDocument() {
    const int doc0_id = 1;
    const string content0 = "dogs better than cats"s;
    const vector<int> ratings0 = {5, 5 ,5};
    const int doc1_id = 2;
    const string content1 = "new bus stop"s;
    const vector<int> ratings1 = {1, 1, 1};
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        const auto found_docs0 = server.FindTopDocuments("dogs"s);
        ASSERT_EQUAL(found_docs0.size(), 1u);
        const Document& doc0 = found_docs0[0];
        ASSERT_EQUAL(doc0.id, doc0_id);
        ASSERT_EQUAL(server.GetDocumentCount(), 1);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        const auto found_docs1 = server.FindTopDocuments("bus"s);
        ASSERT_EQUAL(found_docs1.size(), 1u);
        const Document& doc1 = found_docs1[0];
        ASSERT_EQUAL(doc1.id, doc1_id);
        ASSERT_EQUAL(server.GetDocumentCount(), 2);
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestConsideringMinusWordsInQuery() {
    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.FindTopDocuments("cat in"s).size(), 1u);
        ASSERT_HINT(server.FindTopDocuments("cat -in"s).empty(),
                    "Minus words must be considered in the query"s);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.FindTopDocuments("cat in"s).size(), 1u);
        ASSERT_HINT(!server.FindTopDocuments("cat -in"s).empty(),
                    "Minus words must not affect on stop words"s);
    }
}

void TestDocumentsMatchingSearchRequest() {
    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.MatchDocument("cat in the city"s, 1) == tuple(vector<string> {"cat"s, "city"s, "in"s, "the"s}, DocumentStatus::ACTUAL));
        ASSERT(server.MatchDocument("cat the city"s, 1) == tuple(vector<string> {"cat"s, "city"s, "the"s}, DocumentStatus::ACTUAL));
        ASSERT(server.MatchDocument("cat -in the city"s, 1) == tuple(vector<string> {}, DocumentStatus::ACTUAL));
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        ASSERT(server.MatchDocument("cat in the city"s, 1) == tuple(vector<string> {"cat"s, "city"s}, DocumentStatus::BANNED));
        ASSERT(server.MatchDocument("cat the"s, 1) == tuple(vector<string> {"cat"s}, DocumentStatus::BANNED));
        ASSERT(server.MatchDocument("cat -in the city"s, 1) == tuple(vector<string> {"cat"s, "city"s}, DocumentStatus::BANNED));
        ASSERT(server.MatchDocument("cat -in the -city"s, 1) == tuple(vector<string> {}, DocumentStatus::BANNED));
    }
}

void TestSortFindDocumentsByRelevance() {
    const int doc_id0 = 1;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 2;
    const string content1 = "dogs better than cats"s;
    const vector<int> ratings1 = {1, 2, 3};
    const int doc_id2 = 3;
    const string content2 = "new bus stop"s;
    const vector<int> ratings2 = {1, 2, 3};
    const int doc_id3 = 4;
    const string content3 = "find the key"s;
    const vector<int> ratings3 = {1, 2, 3};
    const int doc_id4 = 5;
    const string content4 = "help with homework"s;
    const vector<int> ratings4 = {1, 3, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        const auto find_docs = server.FindTopDocuments("the cat help with new bus find"s);
        ASSERT_HINT(find_docs[0].id == 3 && find_docs[1].id == 5 && find_docs[2].id == 4 &&
                    find_docs[3].id == 1, 
                    "Documents without accounting stop words are sorted by non-decreasing relevance"s);
    }

    {
        SearchServer server;
        server.SetStopWords("in the with"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        const auto find_docs = server.FindTopDocuments("the cat help with new bus find"s);
        ASSERT_HINT(find_docs[0].id == 3 && find_docs[1].id == 1 && find_docs[2].id == 4 &&
                    find_docs[3].id == 5, 
                    "Documents with accounting stop words are sorted by non-decreasing relevance"s);
    }
}

void TestComputationOfDocumentsRating() {
    const int doc_id0 = 1;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {};
    const int doc_id1 = 2;
    const string content1 = "dogs better than cats"s;
    const vector<int> ratings1 = {1};
    const int doc_id2 = 3;
    const string content2 = "new bus stop"s;
    const vector<int> ratings2 = {0, 0, 0};
    const int doc_id3 = 4;
    const string content3 = "find the key"s;
    const vector<int> ratings3 = {-1, 2, 0};
    const int doc_id4 = 5;
    const string content4 = "help with homework"s;
    const vector<int> ratings4 = {5, 6, 15};
    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        const auto find_docs = server.FindTopDocuments("the cat help with new bus find not dogs"s);
        ASSERT_EQUAL(find_docs[0].rating, (5 + 6 + 15) / 3);
        ASSERT_EQUAL(find_docs[1].rating, 0);
        ASSERT_EQUAL(find_docs[2].rating, (-1 + 2) / 3);
        ASSERT_EQUAL(find_docs[3].rating, 0);
        ASSERT_EQUAL(find_docs[4].rating, 1);
    }
}

void TestFilterDocumentsByPredicateFunc() {
    const int doc_id0 = 1;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 2;
    const string content1 = "dogs better than cats"s;
    const vector<int> ratings1 = {0, 0, 0};
    const int doc_id2 = 3;
    const string content2 = "new bus stop"s;
    const vector<int> ratings2 = {-1, -2, -3};
    const int doc_id3 = 4;
    const string content3 = "find the key"s;
    const vector<int> ratings3 = {5, 5, 5};
    const int doc_id4 = 5;
    const string content4 = "help with homework"s;
    const vector<int> ratings4 = {1, 0, 1};
    map<int, DocumentStatus> status_by_id = {{1, DocumentStatus::ACTUAL},
                                            {2, DocumentStatus::ACTUAL},
                                            {3, DocumentStatus::BANNED},
                                            {4, DocumentStatus::ACTUAL},
                                            {4, DocumentStatus::ACTUAL}};
    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, status_by_id[doc_id0], ratings0);
        server.AddDocument(doc_id1, content1, status_by_id[doc_id1], ratings1);
        server.AddDocument(doc_id2, content2, status_by_id[doc_id2], ratings2);
        server.AddDocument(doc_id3, content3, status_by_id[doc_id3], ratings3);
        server.AddDocument(doc_id4, content4, status_by_id[doc_id4], ratings4);
        const auto find_docs0 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        for (const auto& doc : find_docs0) {
            ASSERT_HINT(doc.id % 2 == 0, "Find documents must have even ids"s);
        }
        const auto find_docs1 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
        for (const auto& doc : find_docs1) {
            ASSERT_HINT(doc.rating > 0, "Find documents must have positive ratings"s);
        }
        const auto find_docs2 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return rating == 0; });
        for (const auto& doc : find_docs2) {
            ASSERT_HINT(doc.rating == 0, "Find documents must have zero ratings"s);
        }
        const auto find_docs3 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        for (const auto& doc : find_docs3) {
            ASSERT_HINT(status_by_id[doc.id] == DocumentStatus::ACTUAL, "Find documents must have actual statuses"s);
        }
        const auto find_docs4 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
        for (const auto& doc : find_docs4) {
            ASSERT_HINT(status_by_id[doc.id] == DocumentStatus::BANNED, "Find documents must have actual statuses"s);
        }
        const auto find_docs5 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
        ASSERT_HINT(find_docs5.empty(), "Find documents must have irrelevant statuses"s);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, status_by_id[doc_id0], ratings0);
        server.AddDocument(doc_id1, content1, status_by_id[doc_id1], ratings1);
        server.AddDocument(doc_id2, content2, status_by_id[doc_id2], ratings2);
        server.AddDocument(doc_id3, content3, status_by_id[doc_id3], ratings3);
        server.AddDocument(doc_id4, content4, status_by_id[doc_id4], ratings4);
        const auto find_docs6 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return rating > 100; });
        ASSERT(find_docs6.empty());
        const auto find_docs7 = server.FindTopDocuments("the cat help with new bus find not dogs"s, 
                                                       [](int document_id, DocumentStatus status, int rating) { return document_id == 100; });
        ASSERT(find_docs7.empty());
    }
}

void TestFindDocumentsByStatus() {
    const int doc_id0 = 1;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {};
    const int doc_id1 = 2;
    const string content1 = "dogs better than cats"s;
    const vector<int> ratings1 = {1};
    const int doc_id2 = 3;
    const string content2 = "new bus stop"s;
    const vector<int> ratings2 = {0, 0, 0};
    const int doc_id3 = 4;
    const string content3 = "find the key"s;
    const vector<int> ratings3 = {-1, 2, 0};
    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings3);
        ASSERT_EQUAL(server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::ACTUAL)[0].id, 1);
        ASSERT_EQUAL(server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::BANNED)[0].id, 2);
        ASSERT_EQUAL(server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::IRRELEVANT)[0].id, 3);
        ASSERT_EQUAL(server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::REMOVED)[0].id, 4);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::REMOVED, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings3);
        ASSERT(server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::BANNED).empty());
        ASSERT(server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::IRRELEVANT).empty());
    }
}

void TestComputationOfDocumentRelevance() {
    const int doc_id0 = 1;
    const string content0 = "cat in the city"s;
    const vector<int> ratings0 = {1, 2, 3};
    const int doc_id1 = 2;
    const string content1 = "dogs better than cats"s;
    const vector<int> ratings1 = {1, 2, 3};
    const int doc_id2 = 3;
    const string content2 = "new bus stop"s;
    const vector<int> ratings2 = {1, 2, 3};
    const int doc_id3 = 4;
    const string content3 = "find the key"s;
    const vector<int> ratings3 = {1, 2, 3};
    const int doc_id4 = 5;
    const string content4 = "help with homework"s;
    const vector<int> ratings4 = {1, 3, 3};
    map<int, double> relevance_by_id = {{1, 1.0 / 4 * log(5.0 / 2) + 1.0 / 4 * log(5.0 / 1)},
                                        {2, 1.0 / 4 * log(5.0 / 1)},
                                        {3, 1.0 / 3 * log(5.0 / 1) + 1.0 / 3 * log(5.0 / 1)},
                                        {4, 1.0 / 3 * log(5.0 / 2) + 1.0 / 3 * log(5.0 / 1)},
                                        {5, 1.0 / 3 * log(5.0 / 1) + 1.0 / 3 * log(5.0 / 1)}};
    {
        SearchServer server;
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        const auto find_docs = server.FindTopDocuments("the cat help with new bus find not dogs"s);
        for (const auto& doc : find_docs) {
            ASSERT(abs(doc.relevance - relevance_by_id[doc.id]) < EPSILON);
        }
    }
}

void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestConsideringMinusWordsInQuery);
    RUN_TEST(TestDocumentsMatchingSearchRequest);
    RUN_TEST(TestSortFindDocumentsByRelevance);
    RUN_TEST(TestComputationOfDocumentsRating);
    RUN_TEST(TestFilterDocumentsByPredicateFunc);
    RUN_TEST(TestFindDocumentsByStatus);
    RUN_TEST(TestComputationOfDocumentRelevance);
}

int main() {
    TestSearchServer();
    cerr << "Search server testing finished"s << endl;
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