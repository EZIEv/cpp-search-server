#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
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
    istringstream stream(text);
    string word;
    while (stream >> word) {
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
    Document() = default;

    Document(int id_num, double relevance_num, int rating_num)
    : id(id_num), relevance(relevance_num), rating(rating_num) {
    };

    int id = 0;
    double relevance = 0;
    int rating = 0;
};

class SearchServer {
public:
    explicit SearchServer(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            if (!IsWordCorrect(word)) {
                throw invalid_argument("Stop words are incorrect"s);
            }
            stop_words_.insert(word);
        }
    }

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& word_collection) {
        for (const string& word : word_collection) {
            if (!IsWordCorrect(word)) {
                throw invalid_argument("Stop words are incorrect"s);
            }
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || documents_statuses_.contains(document_id) || !IsValidQuery(document)) {
            throw invalid_argument("Document ID or document content are incorrect"s);
        }

        vector<string> document_words = SplitIntoWordsNoStop(document);
        double word_tf = 1.0 / document_words.size();
        documents_ratings_[document_id] = ComputeAverageRating(ratings);
        documents_statuses_[document_id] = status;
        for (const string& word : document_words) {
            documents_[word][document_id] += word_tf;
        }
        document_ids_.push_back(document_id);
        ++document_count_;

        return;
    }

    template <typename PredicateFunc>
    vector<Document> FindTopDocuments(const string& raw_query, PredicateFunc predicate_func) const {
        if (!IsValidQuery(raw_query)) {
            throw invalid_argument("Query is incorrect"s);
        }

        const QueryWords query_words = ParseQuery(raw_query);
        vector<Document> result = FindAllDocuments(query_words, predicate_func);

        sort(result.begin(), result.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus document_status, int rating) { return document_status == DocumentStatus::ACTUAL; });
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (!IsValidQuery(raw_query)) {
            throw invalid_argument("Query is incorrect"s);
        }

        vector<string> plus_words;
        QueryWords query_words = ParseQuery(raw_query);
        tuple<vector<string>, DocumentStatus> result;

        for (const string& word : query_words.minus_words) {
            if (documents_.contains(word) && documents_.at(word).contains(document_id)) {
                result =  tuple(plus_words, documents_statuses_.at(document_id));
                return result;
            }
        }

        for (const string& word : query_words.plus_words) {
            if (documents_.contains(word) && documents_.at(word).contains(document_id)) {
                plus_words.push_back(word);
            }
        }

        sort(plus_words.begin(), plus_words.end());
        result = tuple(plus_words, documents_statuses_.at(document_id));
        return result;
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >= static_cast<int>(document_ids_.size())) {
            throw out_of_range("ID of document is incorrrect"s);
        }

        return document_ids_.at(index);
    }

    int GetDocumentCount() const {
        return document_count_;
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
    vector<int> document_ids_;

    static bool IsWordCorrect(const string& word) {
        for (const char c : word) {
            if (c >= '\0' && c < ' ') {
                return false;
            }
        }
        return true;
    }

    static bool IsValidQuery(const string& query) {
        istringstream query_stream(query);
        
        string word;
        while (query_stream >> word) {
            if (word.size() == 1 && word[0] == '-') {
                return false;
            }
            if (word[0] == '-' && word[1] == '-') {
                return false;
            }
            if (!IsWordCorrect(word)) {
                return false;
            }
        }

        return true;
    }

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
        SearchServer server(""s);
        ASSERT_EQUAL(server.GetDocumentCount(), 0);
        server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
        vector<Document> found_docs0;
        found_docs0 = server.FindTopDocuments("dogs"s);
        ASSERT_EQUAL(found_docs0.size(), 1u);
        const Document& doc0 = found_docs0[0];
        ASSERT_EQUAL(doc0.id, doc0_id);
        ASSERT_EQUAL(server.GetDocumentCount(), 1);
        server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
        vector<Document> found_docs1;
        found_docs1 = server.FindTopDocuments("bus"s);
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
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> found_docs;
        found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> result;
        result = server.FindTopDocuments("in"s);
        ASSERT_HINT(result.empty(),
                    "Stop words must be excluded from documents"s);
    }

    {
        SearchServer server(vector<string> {"in"s, "the"s} );
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> result;
        result = server.FindTopDocuments("in"s);
        ASSERT_HINT(result.empty(),
                    "Stop words must be excluded from documents"s);
    }

    {
        SearchServer server(set<string> {"in"s, "the"s} );
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> result;
        result = server.FindTopDocuments("in"s);
        ASSERT_HINT(result.empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestConsideringMinusWordsInQuery() {
    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> result;
        result = server.FindTopDocuments("cat in"s);
        ASSERT_EQUAL(result.size(), 1u);
        result = server.FindTopDocuments("cat -in"s);
        ASSERT_HINT(result.empty(),
                    "Minus words must be considered in the query"s);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<Document> result;
        result = server.FindTopDocuments("cat in"s);
        ASSERT_EQUAL(result.size(), 1u);
        result = server.FindTopDocuments("cat -in"s);
        ASSERT_HINT(!result.empty(),
                    "Minus words must not affect on stop words"s);
    }
}

void TestDocumentsMatchingSearchRequest() {
    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        tuple<vector<string>, DocumentStatus> result;
        result = server.MatchDocument("cat in the city"s, 1);
        ASSERT(result == tuple(vector<string> {"cat"s, "city"s, "in"s, "the"s}, DocumentStatus::ACTUAL));
        result = server.MatchDocument("cat the city"s, 1);
        ASSERT(result == tuple(vector<string> {"cat"s, "city"s, "the"s}, DocumentStatus::ACTUAL));
        result = server.MatchDocument("cat -in the city"s, 1);
        ASSERT(result == tuple(vector<string> {}, DocumentStatus::ACTUAL));
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::BANNED, ratings);
        tuple<vector<string>, DocumentStatus> result;
        result = server.MatchDocument("cat in the city"s, 1);
        ASSERT(result == tuple(vector<string> {"cat"s, "city"s}, DocumentStatus::BANNED));
        result = server.MatchDocument("cat the"s, 1);
        ASSERT(result == tuple(vector<string> {"cat"s}, DocumentStatus::BANNED));
        result = server.MatchDocument("cat -in the city"s, 1);
        ASSERT(result == tuple(vector<string> {"cat"s, "city"s}, DocumentStatus::BANNED));
        result = server.MatchDocument("cat -in the -city"s, 1);
        ASSERT(result == tuple(vector<string> {}, DocumentStatus::BANNED));
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
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        vector<Document> find_docs;
        find_docs = server.FindTopDocuments("the cat help with new bus find"s);
        ASSERT_HINT(find_docs[0].id == 3 && find_docs[1].id == 5 && find_docs[2].id == 4 &&
                    find_docs[3].id == 1, 
                    "Documents without accounting stop words are sorted by non-decreasing relevance"s);
    }

    {
        SearchServer server("in the with"s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        vector<Document> find_docs;
        find_docs = server.FindTopDocuments("the cat help with new bus find"s);
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
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        vector<Document> find_docs;
        find_docs = server.FindTopDocuments("the cat help with new bus find not dogs"s);
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
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, status_by_id[doc_id0], ratings0);
        server.AddDocument(doc_id1, content1, status_by_id[doc_id1], ratings1);
        server.AddDocument(doc_id2, content2, status_by_id[doc_id2], ratings2);
        server.AddDocument(doc_id3, content3, status_by_id[doc_id3], ratings3);
        server.AddDocument(doc_id4, content4, status_by_id[doc_id4], ratings4);
        vector<Document> find_docs0;
        find_docs0 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        for (const auto& doc : find_docs0) {
            ASSERT_HINT(doc.id % 2 == 0, "Find documents must have even ids"s);
        }
        vector<Document> find_docs1;
        find_docs1 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                            [](int document_id, DocumentStatus status, int rating) { return rating > 0; });
        for (const auto& doc : find_docs1) {
            ASSERT_HINT(doc.rating > 0, "Find documents must have positive ratings"s);
        }
        vector<Document> find_docs2;
        find_docs2 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                            [](int document_id, DocumentStatus status, int rating) { return rating == 0; });
        for (const auto& doc : find_docs2) {
            ASSERT_HINT(doc.rating == 0, "Find documents must have zero ratings"s);
        }
        vector<Document> find_docs3;
        find_docs3 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                        [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        for (const auto& doc : find_docs3) {
            ASSERT_HINT(status_by_id[doc.id] == DocumentStatus::ACTUAL, "Find documents must have actual statuses"s);
        }
        vector<Document> find_docs4;
        find_docs4 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                            [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; });
        for (const auto& doc : find_docs4) {
            ASSERT_HINT(status_by_id[doc.id] == DocumentStatus::BANNED, "Find documents must have actual statuses"s);
        }
        vector<Document> find_docs5;
        find_docs5 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                            [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
        ASSERT_HINT(find_docs5.empty(), "Find documents must have irrelevant statuses"s);
    }

    {
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, status_by_id[doc_id0], ratings0);
        server.AddDocument(doc_id1, content1, status_by_id[doc_id1], ratings1);
        server.AddDocument(doc_id2, content2, status_by_id[doc_id2], ratings2);
        server.AddDocument(doc_id3, content3, status_by_id[doc_id3], ratings3);
        server.AddDocument(doc_id4, content4, status_by_id[doc_id4], ratings4);
        vector<Document> find_docs6;
        find_docs6 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
                                            [](int document_id, DocumentStatus status, int rating) { return rating > 100; });
        ASSERT(find_docs6.empty());
        vector<Document> find_docs7;
        find_docs7 = server.FindTopDocuments("the cat help with new bus find not dogs"s,
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
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::BANNED, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings3);
        vector<Document> result;
        result = server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(result[0].id, 1);
        result = server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(result[0].id, 2);
        result = server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(result[0].id, 3);
        result = server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(result[0].id, 4);
    }

    {
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::REMOVED, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings3);
        vector<Document> result;
        result = server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::BANNED);
        ASSERT(result.empty());
        result = server.FindTopDocuments("the cat help with new bus find not dogs"s, DocumentStatus::IRRELEVANT);
        ASSERT(result.empty());
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
        SearchServer server(""s);
        server.AddDocument(doc_id0, content0, DocumentStatus::ACTUAL, ratings0);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
        vector<Document> find_docs;
        find_docs = server.FindTopDocuments("the cat help with new bus find not dogs"s);
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

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();
    cerr << "All tests have been passed"s << endl;
    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document &document :
         search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                        [](int document_id, DocumentStatus status, int rating) {
                                            return document_id % 2 == 0;
                                        }))  //
    {
        PrintDocument(document);
    }
}