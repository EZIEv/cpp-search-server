#include "document.h"


Document::Document(int id_num, double relevance_num, int rating_num)
    : id(id_num), relevance(relevance_num), rating(rating_num) {}


std::ostream& operator<<(std::ostream& output, const Document& document) {
    output << "{ "
         << "document_id = " << document.id << ", "
         << "relevance = " << document.relevance << ", "
         << "rating = " << document.rating << " }";

    return output;
}