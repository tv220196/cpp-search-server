#pragma once

#include <deque>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> documents_found = search_server_.FindTopDocuments(raw_query, document_predicate);
        CreateRequestsDeque(documents_found);
        return documents_found;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        bool no_documents_found;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int no_result_requests_ = 0;
    const SearchServer& search_server_;
    
    void CreateRequestsDeque(std::vector<Document> documents_found);
};