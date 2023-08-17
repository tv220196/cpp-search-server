#include <iostream>
#include "document.h"

using namespace std;

Document::Document(const int& create_server_id, const double& create_server_relevance, const int& create_server_rating)
    : id(create_server_id)
    , relevance(create_server_relevance)
    , rating(create_server_rating)
{
}

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}