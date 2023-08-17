#include <iostream>
#include <vector>

#include "search_server.h"
#include "test_example_functions.h"

using namespace std;

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const string& test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

template <typename A>
void Assert(const A& a) {
    if (a == false) {
        cerr << "Assertion failed"s << endl;
        abort();
    }
}

template <typename T, typename U>
void AssertEqual(const T& t, const U& u) {
    if (t != u) {
        cerr << "Assertion failed: "s << t << " != "s << u << endl;
        abort();
    }
}

template <typename T, typename U>
void AssertEqualHint(const T& t, const U& u, const string& hint) {
    if (t != u) {
        cerr << "Assertion failed: "s << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename A>
void AssertHint(const A& a, const string& hint) {
    if (a == false) {
        cerr << "Assertion failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(a) Assert(a)
#define ASSERT_EQUAL(a, b) AssertEqual(a, b)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualHint(a, b, hint)
#define ASSERT_HINT(a, hint) AssertHint(a, hint)
#define RUN_TEST(func) RunTestImpl(func, #func)

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

//Тест проверяет, что добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestFindDocumentByWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_HINT(!found_docs.empty(),
            "Found documents must include the document with word from the document"s);
    }
}

//Тест проверяет, что документы, содержащие минус-слова из поискового запроса, не должны включаться в результаты поиска.
void TestDoNotOutputDocumentsWithMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in -the"s);
        ASSERT_HINT(found_docs.empty(),
            "Documents which include minus-words must be excluded"s);
    }
}

//Тест проверяет cоответствие документов поисковому запросу
void TestComplianceDocumentsAndQuery() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_HINT(!found_docs.empty(),
            "Found documents must include the document with word from the document"s);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in -the"s);
        ASSERT_HINT(found_docs.empty(),
            "Documents which include minus-words must be excluded"s);
    }
}

//Тест проверяет сортировку документов по релевантности
void TestSortByRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, 1,
        "Documents must be sorted by relevance"s);
}

//Тест проверяет вычисление рейтинга документов
void TestCalculateAverageRating() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.rating, 2,
            "Document average rating is incorrect"s);
    }
}

//Тест проверяет фильтрацию результатов поиска с использованием предиката, задаваемого пользователем
void TestSearchWithPredicateFilter() {
    SearchServer server("и в на"s);
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, 2,
        "Search with predicate filter doesn't work"s);
}

//Тест проверяет поиск документов, имеющих заданный статус
void TestSearchWithStatusFilter() {
    SearchServer server;
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, 3,
        "Search with status filter doesn't work"s);
}

//Тест проверяет корректное вычисление релевантности найденных документов
void TestCalculateRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    const Document& doc0 = found_docs[1];
    ASSERT_HINT((abs(doc0.relevance - 0.173287) < 1e-6),
        "Relevance is incorrect"s);
}

// Запуск тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestFindDocumentByWords);
    RUN_TEST(TestDoNotOutputDocumentsWithMinusWords);
    RUN_TEST(TestComplianceDocumentsAndQuery);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestCalculateAverageRating);
    RUN_TEST(TestSearchWithPredicateFilter);
    RUN_TEST(TestSearchWithStatusFilter);
    RUN_TEST(TestCalculateRelevance);
}