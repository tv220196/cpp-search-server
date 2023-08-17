#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "document.h"
#include "paginator.h"
#include "request_queue.h"
#include "search_server.h"
#include "test_example_functions.h"

using namespace std;

int main() {
    /*//тестирование
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
    */

    /*//проверка корректности ввода
    try {
        vector<string> stop_words = { "and"s, "in"s, "\tat"s };
        SearchServer search_server(stop_words);
    }
    catch (const invalid_argument& e) {
        cout << "Error: "s << e.what() << endl;
    }
    try {
        vector<string> stop_words = { "and"s, "in"s, "at"s };
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
    */

    /*//вывод документов по страницам
    SearchServer search_server({ "and with"s });
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }*/

    //кол-во запросов с нулевым результатом
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;//1437
    
}