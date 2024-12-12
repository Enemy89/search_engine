#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <future>
#include <map>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

struct Entry
{
    size_t docId, count;

    bool operator ==(const Entry& other) const {
        return (docId == other.docId &&
                count == other.count);
    }
};

class InvertedIndex {
private:
    std::vector<std::string> docs;
    std::map<std::string, std::vector<Entry>> freqDictionary;
    static constexpr int MAX_WORD_LENGTH = 100;
    static constexpr int MAX_TOKENS = 1000;
public:
    InvertedIndex()=default;
    //список документов
    void updateDocumentBase(const std::vector<std::string>& inputDocuments);
    //токен - документ
    void putFreqDictionary(const std::string& inWords, int i);
    //частотный словарь
    std::map<std::string, std::vector<Entry>> getFreqDictionary();
    //частота вхождения
    std::vector<Entry> getWordCount(const std::string& word) const;
};

#endif // INVERTEDINDEX_H
