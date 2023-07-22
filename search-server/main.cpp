#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
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
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    int GetDocumentCount() const {
        return static_cast<int>(document_count_);
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        document_count_ += 1.;
        const vector<string> words = SplitIntoWordsNoStop(document);
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

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryPlusAndMinusWords(raw_query);
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
                        return { query_plus_words_in_document, document_status_.at(document_id) };
                    }
                }
            }
        }
        return { query_plus_words_in_document, document_status_.at(document_id) };
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryPlusAndMinusWords(raw_query);
        auto matched_documents = FindAllDocuments(query_plus_and_minus_words, status);

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

    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key_mapper) const {
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryPlusAndMinusWords(raw_query);
        auto matched_documents = FindAllDocuments(query_plus_and_minus_words, key_mapper);

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

private:
    double document_count_ = 0.;

    map<string, map<int, double>> inverted_index_;

    set<string> stop_words_;

    map<int, int> average_document_rating_;

    map<int, DocumentStatus> document_status_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        return static_cast<signed int>(accumulate(ratings.begin(), ratings.end(), 0)) / static_cast<signed int>(ratings.size());
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

    double CountIdf(const map<int, double>& inverted_index_id_tf) const {
        return log(document_count_ / static_cast<double>(inverted_index_id_tf.size()));
    }

    vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, DocumentStatus status) const {
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
            if (document_status_.at(id) == status) {
                matched_documents.push_back({ id, relev, average_document_rating_.at(id) });
            }
        }
        return matched_documents;
    }

    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, KeyMapper key_mapper) const {
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
            if (key_mapper(id, document_status_.at(id), average_document_rating_.at(id))) {
                matched_documents.push_back({ id, relev, average_document_rating_.at(id) });
            }
        }
        return matched_documents;
    }
};

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
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
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