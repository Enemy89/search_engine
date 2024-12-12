#include "SearchServer.h"

std::mutex updateDate;

SearchServer::SearchServer(InvertedIndex &idx) {
    index = idx;
}

//многопоточная обработка запросов
std::vector<std::vector<RelativeIndex>> SearchServer::search(const std::vector<std::string> &inputQueries, int responsesLimit) {
    std::vector<std::thread> searchThread;
    if(inputQueries.size()>MAX_REQUESTS)
    {
        std::cerr << "The number of requests is more than 1000.";
        resultSearch.resize(MAX_REQUESTS);
    }
    else
        resultSearch.resize(inputQueries.size());
    for(int i =0; i < inputQueries.size() && inputQueries.size() != MAX_REQUESTS ; ++i)
    {
        searchThread.emplace_back(&SearchServer::handleRequest, this, std::ref(inputQueries[i]), i, responsesLimit );
    }
    for(auto & i : searchThread)
        i.join();
    return resultSearch;
}

//поиск и определение релевантности документов для запроса
void SearchServer::handleRequest(const std::string& request, int numRequest, int responsesLimit) {
    RelativeIndex relativeIndex{};
    std::vector<RelativeIndex> result;

    // Разбиваем запрос на слова
    std::istringstream stream(request);
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }

    // Проверяем количество слов в запросе
    if (words.size() > MAX_WORDS_IN_REQUEST) {
        std::cerr << "Incorrect size request: " << numRequest << std::endl;
        return;
    }

    // Создаем множество уникальных слов
    std::unordered_map<std::string, int> wordCounts;
    for (const auto& w : words) {
        wordCounts[w]++;
    }

    // Список документов, содержащих хотя бы одно слово из запроса
    std::set<size_t> docIds;
    std::unordered_map<size_t, float> docRelevance;

    for (const auto& [word, count] : wordCounts) {
        const auto& entries = index.getWordCount(word);
        for (const auto& entry : entries) {
            docIds.insert(entry.docId);
            docRelevance[entry.docId] += entry.count * count;
        }
    }

    // Если документов нет, добавляем пустой результат
    if (docIds.empty()) {
        std::lock_guard<std::mutex> guard(updateDate);
        resultSearch[numRequest] = result;
        return;
    }

    // Находим максимальную абсолютную релевантность
    float maxRelevance = 0;
    for (const auto& [docId, relevance] : docRelevance) {
        if (relevance > maxRelevance) {
            maxRelevance = relevance;
        }
    }

    // Преобразуем абсолютную релевантность в относительную
    for (const auto& [docId, relevance] : docRelevance) {
        relativeIndex.docId = docId;
        relativeIndex.rank = relevance / maxRelevance;
        result.push_back(relativeIndex);
    }

    // Сортируем документы по убыванию релевантности
    std::sort(result.begin(), result.end(), [](const RelativeIndex& a, const RelativeIndex& b) {
        return a.rank > b.rank;
    });

    // Ограничиваем количество результатов
    if (result.size() > responsesLimit) {
        result.resize(responsesLimit);
    }

    // Сохраняем результаты в общий массив
    std::lock_guard<std::mutex> guard(updateDate);
    resultSearch[numRequest] = result;
}

