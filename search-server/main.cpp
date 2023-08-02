#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
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
    Document(const int& create_server_id, const double& create_server_relevance, const int& create_server_rating)
        : id(create_server_id)
        , relevance(create_server_relevance)
        , rating(create_server_rating)
    {
    }
    Document() = default;
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words_container) {
        for (const string& word : stop_words_container) {
            if (!IsValidWord(word)) {
                throw invalid_argument("stop words include special characters"s);
            }
            stop_words_.insert(word);
        }
    }
    explicit SearchServer(const string& stop_words_string)
    {
        for (const string& word : SplitIntoWords(stop_words_string)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("stop words include special characters"s);
            }
            stop_words_.insert(word);
        }
    }
    SearchServer() = default;

    int GetDocumentCount() const {
        return static_cast<int>(document_count_);
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0 || document_status_.count(document_id) == 1) {
            throw invalid_argument("incorrect id"s);
        }
        if (!IsValidWord(document)) {
            throw invalid_argument("document includes special characters"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        document_count_ += 1.;
        document_id_.push_back(document_id);
        map<string, double> words_tf;
        double word_proportion = 1. / static_cast<double>(words.size());
        for (const string& word : words) {
            inverted_index_[word][document_id] += word_proportion;
        }
        document_status_[document_id] = status;
        if (ratings.empty()) {
            average_document_rating_[document_id] = 0;
        }
        else {
            average_document_rating_[document_id] = ComputeAverageRating(ratings);
        }
    }

    struct QueryPlusAndMinusWords {
        set<string> plus_words;
        set<string> minus_words;
    };

    QueryPlusAndMinusWords FindQueryPlusAndMinusWords(const string& text) const {
        QueryPlusAndMinusWords query_plus_and_minus_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                query_plus_and_minus_words.minus_words.insert(word.substr(1, word.size() - 1));
            }
            else {
                query_plus_and_minus_words.plus_words.insert(word);
            }
        }
        return query_plus_and_minus_words;
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {//(2.8.6)кортеж содержит пересечение плюс-слов запроса и слов документа с указанным айди
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryPlusAndMinusWords(raw_query);
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("query includes special characters");
        }
        if (!IsValidByMinus(raw_query)) {
            throw invalid_argument("incorrect using minuses");
        }
        vector<string> query_plus_words_in_document;
        for (const string& word : query_plus_and_minus_words.plus_words) {
            if (inverted_index_.count(word) != 0) {
                map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    if (id == document_id) {
                        query_plus_words_in_document.push_back(word);
                    }
                }
            }
        }
        for (const string& word : query_plus_and_minus_words.minus_words) {
            if (inverted_index_.count(word) != 0) {
                map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    if (id == document_id) {
                        query_plus_words_in_document.clear();
                        return tuple<vector<string>, DocumentStatus>{ query_plus_words_in_document, document_status_.at(document_id) };
                    }
                }
            }
        }
        return tuple<vector<string>, DocumentStatus>{ query_plus_words_in_document, document_status_.at(document_id) };
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        if (!IsValidWord(raw_query)) {
            throw invalid_argument("query includes special characters");
        }
        if (!IsValidByMinus(raw_query)) {
            throw invalid_argument("incorrect using minuses");
        }
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryPlusAndMinusWords(raw_query);
        auto matched_documents = FindAllDocuments(query_plus_and_minus_words, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return (lhs.relevance > (EPSILON + rhs.relevance)) ||
                    (abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status](int id, DocumentStatus document_status, int average_document_rating) {
            return status == document_status;
            });
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >= static_cast<int>(document_count_)) {
            throw out_of_range("incorrect id"s);
        }
        return document_id_[index];
    }

private:
    set<string> stop_words_;
    double document_count_ = 0.;
    vector<int> document_id_;
    map<int, int> average_document_rating_;
    map<int, DocumentStatus> document_status_;
    map<string, map<int, double>> inverted_index_;

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

    static bool IsValidWord(const string& text) {
        return none_of(text.begin(), text.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    static bool IsValidByMinus(const string& text) {
        for (int i = 0; i < text.size() - 2; ++i) {
            if (text[i] == '-' && text[i + 1] == '-') {
                return false;
            }
            if (text[i] == '-' && text[i + 1] == ' ') {
                return false;
            }
        }
        if (text[text.size() - 1] == '-') {
            return false;
        }
        return true;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        return static_cast<signed int>(accumulate(ratings.begin(), ratings.end(), 0)) / static_cast<signed int>(ratings.size());
    }

    double CountIdf(const map<int, double>& inverted_index_id_tf) const {
        return log(document_count_ / static_cast<double>(inverted_index_id_tf.size()));
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, DocumentPredicate document_predicate) const {
        map<int, double> document_id_relevance;
        for (const string& word : query_plus_and_minus_words.plus_words) {
            double query_word_idf = 0;
            if (inverted_index_.count(word) != 0) {
                map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                query_word_idf = CountIdf(inverted_index_id_tf);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    document_id_relevance[id] += tf * query_word_idf;
                }
            }
        }
        for (const string& word : query_plus_and_minus_words.minus_words) {
            if (inverted_index_.count(word) != 0) {
                map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    document_id_relevance.erase(id);
                }
            }
        }
        vector<Document> matched_documents;
        for (const auto& [id, relev] : document_id_relevance) {
            if (document_predicate(id, document_status_.at(id), average_document_rating_.at(id))) {
                matched_documents.push_back({ id, relev, average_document_rating_.at(id) });
            }
        }
        return matched_documents;
    }

    vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, DocumentStatus status) const {
        return FindAllDocuments(query_plus_and_minus_words, [status](int id, DocumentStatus document_status, int average_document_rating) {
            return status == document_status;
            });
    }
};

//Макросы ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST 

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

// -------- Начало модульных тестов поисковой системы ----------

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
    const vector<int> ratings = { 1, 2, 3 };//рейтинг 2
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

// --------- Окончание модульных тестов поисковой системы -----------

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

int main() {

    TestSearchServer();
    cout << "Search server testing finished"s << endl;

    try {
        vector<string> stop_words = { "и"s, "в"s, "\tна"s };
        SearchServer search_server(stop_words);
    }
    catch (const invalid_argument& e) {
        cout << "Error: "s << e.what() << endl;
    }
    try {
        vector<string> stop_words = { "и"s, "в"s, "на"s };
        SearchServer search_server(stop_words);
        try {
            search_server.AddDocument(1, "пушистый кот и пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
            search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        }
        catch (const invalid_argument& e) {
            cout << "Error: "s << e.what() << endl;
        }
        try {
            search_server.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        }
        catch (const invalid_argument& e) {
            cout << "Error: "s << e.what() << endl;
        }
        try {
            search_server.AddDocument(3, "\t"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        }
        catch (const invalid_argument& e) {
            cout << "Error: "s << e.what() << endl;
        }
        try {
            auto documents_without_error = search_server.FindTopDocuments("пушистый"s);
            for (const Document& document : documents_without_error) {
                PrintDocument(document);
            }
            auto documents_with_error = search_server.FindTopDocuments("--пушистый"s);
            for (const Document& document : documents_with_error) {
                PrintDocument(document);
            }
        }
        catch (const invalid_argument& e) {
            cout << "Error: "s << e.what() << endl;
        }
    }
    catch (const invalid_argument& e) {
        cout << "Error: "s << e.what() << endl;
    }

}