#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
};

class SearchServer {
public:
    double document_count_ = 0.;

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        map<string, double> words_tf;
        double word_proportion = 1. / static_cast<double>(words.size());
        for (const string& word : words) {
            inverted_index_[word][document_id] += word_proportion;
        }
    }

    struct QueryPlusAndMinusWords {
        set<string> plus_words;
        set<string> minus_words;
    };

    QueryPlusAndMinusWords FindQueryAndMinusWords(const string& text) const {
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

    vector<Document> FindTopDocuments(const string& raw_query) const {
        QueryPlusAndMinusWords query_plus_and_minus_words = FindQueryAndMinusWords(raw_query);
        auto matched_documents = FindAllDocuments(query_plus_and_minus_words);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    map<string, map<int, double>> inverted_index_;

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

    double CountIdf(const map<int, double>& inverted_index_id_tf) const {
        return log(document_count_ / static_cast<double>(inverted_index_id_tf.size()));        
    }

    vector<Document> FindAllDocuments(const QueryPlusAndMinusWords& query_plus_and_minus_words) const {
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
            matched_documents.push_back({ id, relev });
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    search_server.document_count_ = static_cast<double>(document_count);
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();

    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}