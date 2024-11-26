#ifndef SEARCH_SERVER_H
#define SEARCH_SERVER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "InvertedIndex.h"

struct RelativeIndex
{
    size_t docId;
    float rank;

    // Данный оператор необходим для проведения тестовых сценариев
    bool operator ==(const RelativeIndex& other) const {
        return (docId == other.docId && rank == other.rank);
    }
};

class SearchServer {
private:
    InvertedIndex index;
    std::vector<std::vector<RelativeIndex>> resultSearch;

public:
    explicit SearchServer(InvertedIndex &idx);
    //регулирующая функция поиска (многопоточная обработка)
    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string> &inQueries, int responsesLimit);
    //основная функция поиска и определения релевантности (сама логика)
    void handleRequest(const std::string& request, int i, int responsesLimit);
};

#endif // SEARCH_SERVER_H
