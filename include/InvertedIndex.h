#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <map>
#include <sstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace fs = std::filesystem;

struct Entry
{
    size_t docId, count;

    // Данный оператор необходим для проведения тестовых сценариев
    bool operator ==(const Entry& other) const {
        return (docId == other.docId &&
                count == other.count);
    }
};

class InvertedIndex {
private:
    std::vector<std::string> docs;
    std::map<std::string, std::vector<Entry>> freqDictionary;
public:
    InvertedIndex()=default;
    //список документов
    void updateDocumentBase(std::vector<std::string> inDocs);
    //токен - документ
    void putFreqDictionary(std::string inWords, int i);
    //частотный словарь
    std::map<std::string, std::vector<Entry>> getFreqDictionary();
    //частота вхождения
    std::vector<Entry> getWordCount(const std::string& word);
};

#endif // INVERTEDINDEX_H