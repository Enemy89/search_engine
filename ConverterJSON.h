#ifndef CONVERTERJSON_H
#define CONVERTERJSON_H

#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <iomanip>
#define VERSION_APP "0.1"

namespace fs = std::filesystem;
using json = nlohmann::json;

class ConverterJSON {
private:
    std::string name;
    std::string version;
    int maxResponses;
    int timeUpdate;
    std::vector<std::string> files;
    nlohmann::json objJson;

public:
    ConverterJSON() = default;
    //значение имени движка из config
    std::string getName();
    //значение времени обновления из config
    int getTimeUpdate();
    //список документов для поиска
    std::vector<std::string> getTextDocuments();
    //максимальное количество ответов на один запрос
    int getResponsesLimit() const;
    //список запросов
    std::vector<std::string> getRequests();
    //запись ответов
    void putAnswers(const std::vector<std::vector<std::pair<int, float>>>& answers);

    // Вспомогательные методы (проверка и парсинг config)
    bool loadConfig();
};

#endif // CONVERTERJSON_H
