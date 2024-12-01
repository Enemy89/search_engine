#include "InvertedIndex.h"

std::mutex addingDate;

// Обновление базы документов
void InvertedIndex::updateDocumentBase(const std::vector<std::string>& inputDocuments) {
    std::vector<std::thread> updateThread;
    for (int i = 0; i < inputDocuments.size(); ++i) {
        std::fstream file;
        file.open(inputDocuments[i]);
        if (!file) {
            std::cerr << inputDocuments[i] + " file not found \n";
            continue;
        }
        std::string words;
        std::getline(file, words);
        docs.push_back(words);
        file.close();
        updateThread.emplace_back(&InvertedIndex::putFreqDictionary, this, words, i);
    }
    for(auto & i : updateThread)
        i.join();
}

//заполнение частотного словаря
void InvertedIndex::putFreqDictionary(const std::string& inWords, int i) {
    Entry entry{};
    std::vector<std::string> tokens;
    std::istringstream words(inWords);
    std::string oneWord;

    // Разбиваем строку на слова и сразу исключаем пустые или слишком длинные
    while (std::getline(words, oneWord, ' ')) {
        if (!oneWord.empty() && oneWord.length() <= 100) {
            tokens.push_back(oneWord);
        } else if (oneWord.length() > 100) {
            std::cerr << "Incorrect size words in file: " << i << " word: "<< oneWord<< std::endl;
        }
    }

    // Проверяем, не превышает ли количество слов лимит
    if (tokens.size() > 1000) {
        std::cerr << "Incorrect size file:" << i << std::endl;
        return;
    }

    // Обрабатываем токены
    for (const auto &word : tokens) {
        std::lock_guard<std::mutex> guard(addingDate);

        auto it = freqDictionary.find(word);
        if (it == freqDictionary.end()) {
            // Если слово не найдено, добавляем новую запись
            entry.docId = i;
            entry.count = 1;
            freqDictionary[word] = {entry};
        } else {
            // Если слово найдено, проверяем документы
            auto &entries = it->second;
            auto found = std::find_if(entries.begin(), entries.end(),
                                      [i](const Entry &e) { return e.docId == i; });

            if (found != entries.end()) {
                // Слово уже встречалось в этом документе, увеличиваем счетчик
                found->count++;
            } else {
                // Слово не встречалось в этом документе, добавляем новую запись
                entry.docId = i;
                entry.count = 1;
                entries.push_back(entry);
            }
        }
    }
}

std::map<std::string, std::vector<Entry>> InvertedIndex::getFreqDictionary() {
    return freqDictionary;
}

//проверка количество вхождения слова в частотный словарь
std::vector<Entry> InvertedIndex::getWordCount(const std::string& word) const {
    auto it = freqDictionary.find(word);
    if(freqDictionary.count(word))
        return it->second;
    else
    {
        std::vector<Entry> emptyVector;
        return emptyVector;
    }
}
