#include <cmath>
#include <numeric>

#include "search_server.h"
#include "string_processing.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_string)
    :SearchServer(SplitIntoWords(stop_words_string))
{
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(document_id_.size());
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    if (document_id < 0 || documents_.count(document_id) == 1) {
        throw invalid_argument("incorrect id"s);
    }
    const vector<string> words = SplitIntoWordsNoStop(document);
    document_id_.push_back(document_id);
    map<string, double> words_tf;
    double word_proportion = 1. / static_cast<double>(words.size());
    for (const string& word : words) {
        inverted_index_[word][document_id] += word_proportion;
    }
    documents_[document_id] = { ComputeAverageRating(ratings), status };
}

SearchServer::QueryPlusAndMinusWords SearchServer::FindQueryPlusAndMinusWords(const string& text) const {
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

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {//(2.8.6)кортеж содержит пересечение плюс-слов запроса и слов документа с указанным айди
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
                    return tuple<vector<string>, DocumentStatus>{ query_plus_words_in_document, documents_.at(document_id).document_status };
                }
            }
        }
    }
    return tuple<vector<string>, DocumentStatus>{ query_plus_words_in_document, documents_.at(document_id).document_status };
}
    
vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int id, DocumentStatus document_status, int average_document_rating) {
        return status == document_status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentId(int index) const {
    return document_id_.at(index);
}
    
bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(text)) {
            throw invalid_argument("query includes special characters");
        }
        if (!IsValidByMinus(word)) {
            throw invalid_argument("incorrect using minuses");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

bool SearchServer::IsValidWord(const string& text) {
    return none_of(text.begin(), text.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsValidByMinus(const string& word) {
    if (word.size() == 1 && word[0] == '-') {
        return false;
    }
    if (word.size() >= 2 && word[0] == '-' && word[1] == '-') {
        return false;
    }
    return true;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    else {
        return static_cast<signed int>(accumulate(ratings.begin(), ratings.end(), 0)) / static_cast<signed int>(ratings.size());
    }
}

double SearchServer::CountIdf(const map<int, double>& inverted_index_id_tf) const {
    return log(static_cast<double>(document_id_.size()) / static_cast<double>(inverted_index_id_tf.size()));
}

vector<Document> SearchServer::FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words, DocumentStatus status) const {
    return FindAllDocuments(query_plus_and_minus_words, [status](int id, DocumentStatus document_status, int average_document_rating) {
        return status == document_status;
        });
}