#include "InvertedIndex.h"

std::mutex addingDate;

// Обновление базы документов
void InvertedIndex::updateDocumentBase(const std::vector<std::string>& inputDocuments) {
    std::vector<std::future<void>> futures;

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

        // Запуск задачи асинхронно
        futures.push_back(std::async(std::launch::async, &InvertedIndex::putFreqDictionary, this, words, i));
    }

    // Ожидание завершения всех задач
    for (auto& future : futures) {
        future.get();
    }
}

// Заполнение частотного словаря
void InvertedIndex::putFreqDictionary(const std::string& inWords, int i) {
    Entry entry{};
    std::vector<std::string> tokens;
    std::istringstream words(inWords);
    std::string oneWord;

    // Разбиваем строку на слова и исключаем пустые или слишком длинные
    while (std::getline(words, oneWord, ' ')) {
        if (!oneWord.empty() && oneWord.length() <= MAX_WORD_LENGTH) {
            tokens.push_back(oneWord);
        } else if (oneWord.length() > MAX_WORD_LENGTH) {
            std::cerr << "Incorrect size words in file: " << i << " word: " << oneWord << std::endl;
        }
    }

    // Проверяем, не превышает ли количество слов лимит
    if (tokens.size() > MAX_TOKENS) {
        std::cerr << "Incorrect size file: " << i << std::endl;
        return;
    }

    // Локальный словарь для текущей обработки
    std::unordered_map<std::string, std::vector<Entry>> localFreqDictionary;

    // Обрабатываем токены
    for (const auto& word : tokens) {
        auto& entries = localFreqDictionary[word];
        auto found = std::find_if(entries.begin(), entries.end(),
                                  [i](const Entry& e) { return e.docId == i; });

        if (found != entries.end()) {
            found->count++;
        } else {
            entry.docId = i;
            entry.count = 1;
            entries.push_back(entry);
        }
    }

    // Сливаем локальные данные с глобальным словарём
    {
        std::lock_guard<std::mutex> guard(addingDate);
        for (const auto& [word, entries] : localFreqDictionary) {
            auto& globalEntries = freqDictionary[word];
            for (const auto& localEntry : entries) {
                auto found = std::find_if(globalEntries.begin(), globalEntries.end(),
                                          [docId = localEntry.docId](const Entry& e) {
                                              return e.docId == docId;
                                          });

                if (found != globalEntries.end()) {
                    found->count += localEntry.count;
                } else {
                    globalEntries.push_back(localEntry);
                }
            }
        }
    }
}

// Получение частотного словаря
std::map<std::string, std::vector<Entry>> InvertedIndex::getFreqDictionary() {
    return freqDictionary;
}

// Проверка количества вхождения слова в частотный словарь
std::vector<Entry> InvertedIndex::getWordCount(const std::string& word) const {
    auto it = freqDictionary.find(word);
    if (freqDictionary.count(word)) {
        return it->second;
    } else {
        return {};
    }
}