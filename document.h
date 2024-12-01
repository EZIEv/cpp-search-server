#pragma once

#include <iostream>

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    Document() = default;

    Document(int id_num, double relevance_num, int rating_num);

    int id = 0;
    double relevance = 0;
    int rating = 0;
};

std::ostream& operator<<(std::ostream& output, const Document& document);