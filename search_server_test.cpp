#include "search_server_test.h"

#include <vector>

using namespace std;


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