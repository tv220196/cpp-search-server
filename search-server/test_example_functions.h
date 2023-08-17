#pragma once

#include <string>

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const std::string& test_name);

template <typename A>
void Assert(const A& a);

template <typename T, typename U>
void AssertEqual(const T& t, const U& u);

template <typename T, typename U>
void AssertEqualHint(const T& t, const U& u, const std::string& hint);

template <typename A>
void AssertHint(const A& a, const std::string& hint);

#define ASSERT(a) Assert(a)
#define ASSERT_EQUAL(a, b) AssertEqual(a, b)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualHint(a, b, hint)
#define ASSERT_HINT(a, hint) AssertHint(a, hint)
#define RUN_TEST(func) RunTestImpl(func, #func)

void TestExcludeStopWordsFromAddedDocumentContent();
void TestFindDocumentByWords();
void TestDoNotOutputDocumentsWithMinusWords();
void TestComplianceDocumentsAndQuery();
void TestSortByRelevance();
void TestCalculateAverageRating();
void TestSearchWithPredicateFilter();
void TestSearchWithStatusFilter();
void TestCalculateRelevance();
void TestSearchServer();