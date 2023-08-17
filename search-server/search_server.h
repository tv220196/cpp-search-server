#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include "document.h"

using namespace std::literals::string_literals;

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words_container) {
        for (const std::string& word : stop_words_container) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("stop words include special characters"s);
            }
            stop_words_.insert(word);
        }
    }
    explicit SearchServer(const std::string& stop_words_string);
    SearchServer() = default;

    int GetDocumentCount() const;

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    struct QueryPlusAndMinusWords {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    QueryPlusAndMinusWords FindQueryPlusAndMinusWords(const std::string& text) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryPlusAndMinusWords(raw_query);
        auto matched_documents = FindAllDocuments(query_plus_and_minus_words, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return (lhs.relevance > (EPSILON + rhs.relevance)) ||
                    (std::abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    int GetDocumentId(int index) const;

private:
    std::set<std::string> stop_words_;
    std::vector<int> document_id_;
    struct DocumentData {
        int average_document_rating;
        DocumentStatus document_status;
    };
    std::map<int, DocumentData> documents_;
    std::map<std::string, std::map<int, double>> inverted_index_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static bool IsValidWord(const std::string& text);
    static bool IsValidByMinus(const std::string& word);

    static int ComputeAverageRating(const std::vector<int>& ratings);
    double CountIdf(const std::map<int, double>& inverted_index_id_tf) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, DocumentPredicate document_predicate) const {
        std::map<int, double> document_id_relevance;
        for (const std::string& word : query_plus_and_minus_words.plus_words) {
            double query_word_idf = 0;
            if (inverted_index_.count(word) != 0) {
                std::map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                query_word_idf = CountIdf(inverted_index_id_tf);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    document_id_relevance[id] += tf * query_word_idf;
                }
            }
        }
        for (const std::string& word : query_plus_and_minus_words.minus_words) {
            if (inverted_index_.count(word) != 0) {
                std::map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    document_id_relevance.erase(id);
                }
            }
        }
        std::vector<Document> matched_documents;
        for (const auto& [id, relev] : document_id_relevance) {
            if (document_predicate(id, documents_.at(id).document_status, documents_.at(id).average_document_rating)) {
                matched_documents.push_back({ id, relev, documents_.at(id).average_document_rating });
            }
        }
        return matched_documents;
    }
    std::vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, DocumentStatus status) const;
};
