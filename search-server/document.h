#pragma once

struct Document {
    Document(const int& create_server_id, const double& create_server_relevance, const int& create_server_rating);
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

void PrintDocument(const Document& document);
