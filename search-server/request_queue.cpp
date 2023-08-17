#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server)
    :search_server_(search_server)
{
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    vector<Document> documents_found = search_server_.FindTopDocuments(raw_query, status);
    CreateRequestsDeque(documents_found);
    return documents_found;
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    vector<Document> documents_found = search_server_.FindTopDocuments(raw_query);
    CreateRequestsDeque(documents_found);
    return documents_found;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_;
}

void RequestQueue::CreateRequestsDeque(vector<Document> documents_found) {
    if (requests_.size() == min_in_day_) {
        if (requests_.front().no_documents_found == true) {
            --no_result_requests_;
        }
        requests_.pop_front();
    }

    if (documents_found.empty()) {
        requests_.push_back({ true });
        ++no_result_requests_;
    }
    else {
        requests_.push_back({ false });
    }

}