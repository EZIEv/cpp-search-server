#pragma once
#include "document.h"
#include "search_server.h"

#include <iostream>
#include <string>

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "(" << line << "): " << func << ": ";
        std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
        std::cerr << t << " != " << u << ".";
        if (!hint.empty()) {
            std::cerr << " Hint: " << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const std::string& test_name) {
    func();
    std::cerr << test_name << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

void TestAddDocument();

void TestExcludeStopWordsFromAddedDocumentContent();

void TestConsideringMinusWordsInQuery();

void TestDocumentsMatchingSearchRequest();

void TestSortFindDocumentsByRelevance();

void TestComputationOfDocumentsRating();

void TestFilterDocumentsByPredicateFunc();

void TestFindDocumentsByStatus();

void TestComputationOfDocumentRelevance();

void TestSearchServer();