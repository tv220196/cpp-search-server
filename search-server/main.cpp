/*
//1.1.9
// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>
using namespace std;

int main() {
    int incl_3 = 0;
    for (int i = 1; i <= 1000; ++i) {
        string num = to_string(i);
        bool flag = false;
        for (const char& c : num) {
            if (c == '3') {
                flag = true;
            }
        }
        if (flag) {
            ++incl_3;
        }
    }
    cout << incl_3 << endl;
}
*/

//1.5.1
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
        for (const string& word : words) {//можно эффективнее. посчитать вес одного слова до цикла, потом при проходе в цикле по всем словам прибавлять посчитанное значение к ячейке слово-идентификатор
            //words_tf[word] += 1. / static_cast<double>(words.size());
            words_tf[word] += word_proportion;
        }
        for (const string& word : words) {
            inverted_index_[word].insert({ document_id, words_tf[word] });
        }
    }

    /*set<string> FindMinusWordsV1(const string& text) const {
        vector<string> minus_words_v;*/
        /*string word;
        for (const char c : text) {//у вас есть метод сплит, который делит строку на слова
            if (c == ' ') {
                if (!word.empty() && word[0] == '-') {
                    minus_words_v.push_back(word);
                }
                word.clear();
            }
            else {
                word += c;
            }
        }
        if (!word.empty() && word[0] == '-') {
            minus_words_v.push_back(word);
        }*/
        /*for (const string& word : SplitIntoWords(text)) {
            if (word[0] == '-') {
                minus_words_v.push_back(word);
            }
        }
        set<string> minus_words;
        for (const string& word : minus_words_v) {//строка - это массив символов. а в массиве можно работать с индексами
            string minus_word;*/
            /*for (const char& c : word) {
                if (c != '-') {
                    minus_word += c;
                }
            }*/
            /*for (int i = 1; i < word.size(); ++i) {
                minus_word += word[i];
            }
            minus_words.insert(minus_word);
            minus_word.clear();
        }
        return minus_words;
    }*/

    set<string> FindMinusWords(const set<string>& query_words) const {
        set<string> minus_words;
        for (const string& word : query_words) {
            if (word[0] == '-') {
                string minus_word;
                for (int i = 1; i < word.size(); ++i) {
                    minus_word += word[i];
                }
                minus_words.insert(minus_word);
                minus_word.clear();
            }
        }
        return minus_words;
    }


    struct QueryAndMinusWords {
        set<string> query_words;
        set<string> minus_words;
    };

    vector<Document> FindTopDocuments(const string& raw_query) const {
        //set<string> minus_words = FindMinusWordsV1(raw_query);
        //const set<string> query_words = ParseQuery(raw_query);//133-134 попробуйте этот функционал объединить. 
                                                              //в итоге вы должны вернуть не набор слов, а структуру с двумя наборами слов, которую и передать одним объектом в FindAllDocuments. 
                                                              //на данный момент вы дважды работаете с одной и той же строкой
        QueryAndMinusWords query_and_minus_words;
        query_and_minus_words.query_words = ParseQuery(raw_query);
        query_and_minus_words.minus_words = FindMinusWords(query_and_minus_words.query_words);

        //auto matched_documents = FindAllDocumentsV1(query_words, minus_words);
        auto matched_documents = FindAllDocuments(query_and_minus_words);

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

    /*vector<string> SplitIntoWordsNoStopV1(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word) && word[0] != '-') {
                words.push_back(word);
            }
        }
        return words;
    }*/

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string& text) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }
        return query_words;
    }

    /*set<string> ParseQueryV1(const string& text) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStopV1(text)) {
            query_words.insert(word);
        }
        return query_words;
    }*/

    double FindIdf(const map<int, double>& inverted_index_id_tf) const {
        double query_word_idf;
        query_word_idf = log(document_count_ / static_cast<double>(inverted_index_id_tf.size()));
        return query_word_idf;
    }

    /*vector<Document> FindAllDocumentsV1(const set<string>& query_words, const set<string>& minus_words) const {
        map<int, double> document_id_relevance;
        for (const string& word : query_words) {
            double query_word_idf = 0;
            if (inverted_index_.count(word) != 0) {
                map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                //query_word_idf = log(document_count_ / static_cast<double>(inverted_index_id_tf.size()));//формулу расчета ИДФ стоит вынести в отдельную функцию (именованная функция - это понятнее, чем сложная формула)
                query_word_idf = FindIdf(inverted_index_id_tf);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    document_id_relevance[id] += tf * query_word_idf;
                }
            }
        }
        for (const string& word : minus_words) {
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
    }*/
    vector<Document> FindAllDocuments(const QueryAndMinusWords& query_and_minus_words) const {
        map<int, double> document_id_relevance;
        for (const string& word : query_and_minus_words.query_words) {
            double query_word_idf = 0;
            if (inverted_index_.count(word) != 0) {
                map<int, double> inverted_index_id_tf = inverted_index_.at(word);
                query_word_idf = FindIdf(inverted_index_id_tf);
                for (const auto& [id, tf] : inverted_index_id_tf) {
                    document_id_relevance[id] += tf * query_word_idf;
                }
            }
        }
        for (const string& word : query_and_minus_words.minus_words) {
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