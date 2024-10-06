#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string APP_VERSION = "0.1";

class ConverterJSON {
public:
    ConverterJSON() {
        if (!loadConfig()) {
            std::cerr << "Failed to load configuration." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::cout << "Starting " << name << std::endl;

        // Проверяем наличие файла индекса и создаем его, если он отсутствует
        if (fs::exists("../index.json")) {
            manageIndex();
        }

        readUserRequest();
    }

private:
    //метод загрузки и проверки конфига
    bool loadConfig() {
        std::ifstream configFile("../config.json");

        if (!configFile.is_open()) {
            std::cerr << "config file is missing" << std::endl;
            return false;
        }

        json configJson;
        try {
            configFile >> configJson;
        } catch (json::parse_error& e) {
            std::cerr << "config file is empty or contains invalid JSON" << std::endl;
            return false;
        }

        if (configJson.find("config") == configJson.end()) {
            std::cerr << "config file is empty" << std::endl;
            return false;
        }

        try {
            name = configJson["config"]["name"];
            version = configJson["config"]["version"];

            if (configJson["config"].find("max_responses") != configJson["config"].end()) {
                maxResponses = configJson["config"]["max_responses"];
                if (maxResponses <= 0) {
                    std::cerr << "Invalid max_responses in config.json. It must be a positive integer." << std::endl;
                    return false;
                }
            } else {
                maxResponses = 5;
                configJson["config"]["max_responses"] = maxResponses; // Создание ключа в JSON
            }

            timeUpdate = configJson["config"]["time_update"];

            if (version != APP_VERSION) {
                std::cerr << "config.json has incorrect file version" << std::endl;
                return false;
            }

            if (timeUpdate <= 0) {
                std::cerr << "Invalid time_update in config.json. It must be a positive integer." << std::endl;
                return false;
            }

            // Очистка и заполнение списка файлов
            files.clear();
            std::string resourcesPath = "../resources";

            if (!fs::exists(resourcesPath) || !fs::is_directory(resourcesPath)) {
                std::cerr << "Error: Resources path does not exist or is not a directory" << std::endl;
                return false;
            }

            // Переменная для хранения следующего document_id
            int nextDocID = 1;
            if (configJson.find("document_ids") != configJson.end()) {
                nextDocID = configJson["document_ids"].size() + 1;  // Начинаем с последнего ID + 1
            } else {
                configJson["document_ids"] = json::object();  // Если нет, создаем пустой объект для ID
            }

            // Карта для хранения document_id
            std::unordered_map<std::string, int> documentIDMap;
            for (const auto& [filePath, docID] : configJson["document_ids"].items()) {
                documentIDMap[filePath] = docID.get<int>();
            }

            for (const auto& entry : fs::directory_iterator(resourcesPath)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = fs::relative(entry.path(), fs::current_path()).string();
                    std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
                    files.push_back(relativePath);

                    // Присваиваем document_id, если его еще нет
                    if (documentIDMap.find(relativePath) == documentIDMap.end()) {
                        documentIDMap[relativePath] = nextDocID++;
                    }
                }
            }

            // Сохраняем обновленные document_ids в config.json
            for (const auto& [filePath, docID] : documentIDMap) {
                configJson["document_ids"][filePath] = docID;
            }

            // Сохраняем обновленный список файлов
            configJson["files"] = files;

            std::ofstream outputConfigFile("../config.json");
            if (!outputConfigFile.is_open()) {
                std::cerr << "Error: Unable to write to config file" << std::endl;
                return false;
            }

            outputConfigFile << configJson.dump(4);
            outputConfigFile.close();

        } catch (json::type_error& e) {
            std::cerr << "Error reading configuration: " << e.what() << std::endl;
            return false;
        }

        return true;
    }


    //Метод для создания/обновления индекса токенов
    void manageIndex() {
        std::string indexFilePath = "../index.json";
        bool indexExists = fs::exists(indexFilePath);

        if (indexExists) {
            // Получаем текущее время
            auto currentTime = std::chrono::system_clock::now();

            // Получаем время последнего изменения файла
            auto lastWriteTime = fs::last_write_time(indexFilePath);

            // Преобразуем в time_t
            std::time_t current_time_t = std::chrono::system_clock::to_time_t(currentTime);
            std::time_t last_write_time_t = std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now() - (fs::file_time_type::clock::now() - lastWriteTime));

            // Вычисляем разницу в секундах
            double elapsedSeconds = std::difftime(current_time_t, last_write_time_t);

            // Сравниваем с интервалом времени
            if (elapsedSeconds >= timeUpdate) {
                createIndex();
            }
        } else {
            // Если индекс не существует, создаем новый индекс
            createIndex();
        }
    }

    // Метод для токенизации текста
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::stringstream ss(text);
        std::string word;

        while (ss >> word) {
            // Удаление пунктуации в начале и конце каждого слова
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            tokens.push_back(word);
        }

        return tokens;
    }

    // Метод для приведения слов к нижнему регистру
    void toLowercase(std::vector<std::string>& tokens) {
        for (auto& word : tokens) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        }
    }

private:
    // Метод удаления стоп-слов
    void removeStopWords(std::vector<std::string>& tokens) {
        tokens.erase(std::remove_if(tokens.begin(), tokens.end(),[this](const std::string& word)
        {return stopWords.find(word) != stopWords.end();}),tokens.end());
    }


    //Метод создания id токенов
    void indexTokens(std::unordered_map<std::string, int>& termIdMap, std::vector<std::string>& tokens, int& nextTermId) {
        for (const auto& word : tokens) {
            if (termIdMap.find(word) == termIdMap.end()) {
                termIdMap[word] = nextTermId++;
            }
        }
    }

    //Метод создания инвертированного индекса (частота появления в документах)
    void createInvertedIndex(std::unordered_map<int, std::vector<std::pair<int, int>>>& invertedIndex,
                             const std::unordered_map<std::string, int>& termIdMap,
                             const std::vector<std::string>& tokens,
                             int documentId) {
        for (const auto& word : tokens) {
            int termId = termIdMap.at(word);

            auto& docList = invertedIndex[termId];
            auto it = std::find_if(docList.begin(), docList.end(), [&](const std::pair<int, int>& p) {
                return p.first == documentId;
            });

            if (it != docList.end()) {
                it->second++;  // Увеличиваем частоту
            } else {
                docList.emplace_back(documentId, 1);  // Новый документ, частота = 1
            }
        }
    }

    //Метод создания позиционного индекса
    void createPositionalIndex(std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& positionalIndex,
                               const std::unordered_map<std::string, int>& termIdMap,
                               const std::vector<std::string>& tokens,
                               int documentId) {
        for (int i = 0; i < tokens.size(); ++i) {
            const std::string& word = tokens[i];
            int termId = termIdMap.at(word);

            positionalIndex[termId][documentId].push_back(i);  // Добавляем позицию
        }
    }

    //Метод для построения индекса основной
    void createIndex() {
        std::unordered_map<std::string, int> termIdMap;
        int nextTermId = 1;  // Следующий ID для нового терма

        // Инвертированный и позиционный индексы
        std::unordered_map<int, std::vector<std::pair<int, int>>> invertedIndex;
        std::unordered_map<int, std::unordered_map<int, std::vector<int>>> positionalIndex;

        // Мапа для сопоставления документов и их ID
        std::unordered_map<std::string, int> documentIdMap;

        // Загружаем document_id из config.json
        std::ifstream configFile("../config.json");
        if (configFile.is_open()) {
            json configJson;
            configFile >> configJson;
            for (const auto& [filePath, docID] : configJson["document_ids"].items()) {
                documentIdMap[filePath] = docID.get<int>();
            }
            configFile.close();
        }

        for (const auto& filePath : files) {
            std::ifstream inputFile(filePath);
            if (!inputFile.is_open()) {
                std::cerr << "Error: Unable to open file " << filePath << std::endl;
                continue;
            }

            std::string content((std::istreambuf_iterator<char>(inputFile)),
                                (std::istreambuf_iterator<char>()));
            inputFile.close();

            // Токенизация текста
            std::vector<std::string> tokens = tokenize(content);

            // Приведение к нижнему регистру
            toLowercase(tokens);

            // Удаление стоп-слов
            removeStopWords(tokens);

            // Получение document_id
            int documentId = documentIdMap[filePath];

            // Индексация токенов
            indexTokens(termIdMap, tokens, nextTermId);

            // Создание инвертированного индекса
            createInvertedIndex(invertedIndex, termIdMap, tokens, documentId);

            // Создание позиционного индекса
            createPositionalIndex(positionalIndex, termIdMap, tokens, documentId);
        }

        // Сохранение индексов (основной, инвертированный, позиционный)
        saveIndex(termIdMap, invertedIndex, positionalIndex);
    }

    //Сериализация
    void saveIndex(const std::unordered_map<std::string, int>& termIdMap,
                   const std::unordered_map<int, std::vector<std::pair<int, int>>>& invertedIndex,
                   const std::unordered_map<int, std::unordered_map<int, std::vector<int>>>& positionalIndex) {
        json indexJson;

        // Создаем term_index
        json termIndexJson;
        json termToIdJson;
        json idToTermJson;

        for (const auto& [term, id] : termIdMap) {
            termToIdJson[term] = id;
            idToTermJson[std::to_string(id)] = term;
        }

        termIndexJson["term_to_id"] = termToIdJson;
        termIndexJson["id_to_term"] = idToTermJson;
        indexJson["term_index"] = termIndexJson;

        // Создаем inverted_index
        json invertedIndexJson;
        for (const auto& [termId, docList] : invertedIndex) {
            json docListJson;
            for (const auto& [documentId, frequency] : docList) {
                docListJson.push_back({{"document_id", documentId}, {"frequency", frequency}});
            }
            invertedIndexJson[std::to_string(termId)] = docListJson;
        }
        indexJson["inverted_index"] = invertedIndexJson;

        // Создаем positional_index
        json positionalIndexJson;
        for (const auto& [termId, docMap] : positionalIndex) {
            json docMapJson;
            for (const auto& [documentId, positions] : docMap) {
                docMapJson[std::to_string(documentId)] = {{"positions", positions}};
            }
            positionalIndexJson[std::to_string(termId)] = docMapJson;
        }
        indexJson["positional_index"] = positionalIndexJson;

        // Сохраняем индекс в index.json
        std::ofstream indexFile("../index.json");
        if (!indexFile.is_open()) {
            std::cerr << "Error: Unable to write to index file" << std::endl;
            return;
        }
        indexFile << indexJson.dump(4);
        indexFile.close();
    }


    // Метод для построения индекса с добавлением ID для каждого терма
    /*void createIndex() {
        // Мапа для хранения термов и их ID
        std::unordered_map<std::string, int> termIdMap;
        int nextTermId = 1;  // Следующий ID для нового терма

        // Инвертированный индекс
        std::unordered_map<int, std::vector<std::pair<int, int>>> invertedIndex;

        // Мапа для сопоставления документов и их ID
        std::unordered_map<std::string, int> documentIdMap;

        // Загружаем document_id из config.json
        std::ifstream configFile("../config.json");
        if (configFile.is_open()) {
            json configJson;
            configFile >> configJson;
            for (const auto& [filePath, docID] : configJson["document_ids"].items()) {
                documentIdMap[filePath] = docID.get<int>();
            }
            configFile.close();
        }

        // Основной индекс: std::unordered_map<term_id, std::unordered_map<document_id, frequency>>
        std::unordered_map<int, std::unordered_map<int, int>> index;

        for (const auto& filePath : files) {
            std::ifstream inputFile(filePath);
            if (!inputFile.is_open()) {
                std::cerr << "Error: Unable to open file " << filePath << std::endl;
                continue;
            }

            std::string content((std::istreambuf_iterator<char>(inputFile)),
                                (std::istreambuf_iterator<char>()));
            inputFile.close();

            // Токенизация текста
            std::vector<std::string> tokens = tokenize(content);

            // Приведение к нижнему регистру
            toLowercase(tokens);

            // Удаление стоп-слов
            removeStopWords(tokens);

            // Получение document_id для текущего файла
            int documentId = documentIdMap[filePath];

            // Обработка каждого токена
            for (const auto& word : tokens) {
                // Проверяем, есть ли у терма уже свой term_id
                if (termIdMap.find(word) == termIdMap.end()) {
                    termIdMap[word] = nextTermId++;
                }
                int termId = termIdMap[word];

                // Обновление основного индекса
                index[termId][documentId]++;

                // Обновление инвертированного индекса
                auto& docList = invertedIndex[termId];
                auto it = std::find_if(docList.begin(), docList.end(), [&](const std::pair<int, int>& p) {
                    return p.first == documentId;
                });
                if (it != docList.end()) {
                    // Документ уже есть, увеличиваем частоту
                    it->second++;
                } else {
                    // Новый документ, добавляем его с частотой 1
                    docList.emplace_back(documentId, 1);
                }
            }
        }

        // Сохранение основного индекса в index.json
        json indexJson;
        for (const auto& [termId, fileMap] : index) {
            json fileJson;
            for (const auto& [docId, count] : fileMap) {
                fileJson[std::to_string(docId)] = count;
            }
            indexJson[std::to_string(termId)] = fileJson;
        }

        std::ofstream indexFile("../index.json");
        if (!indexFile.is_open()) {
            std::cerr << "Error: Unable to write to index file" << std::endl;
            return;
        }

        indexFile << indexJson.dump(4);
        indexFile.close();

        // Сохранение инвертированного индекса в inverted_index.json
        json invertedIndexJson;
        for (const auto& [termId, docList] : invertedIndex) {
            json docJson;
            for (const auto& [docId, freq] : docList) {
                docJson.push_back({docId, freq});
            }
            invertedIndexJson[std::to_string(termId)] = docJson;
        }

        std::ofstream invertedIndexFile("../inverted_index.json");
        if (!invertedIndexFile.is_open()) {
            std::cerr << "Error: Unable to write to inverted index file" << std::endl;
            return;
        }

        invertedIndexFile << invertedIndexJson.dump(4);
        invertedIndexFile.close();
    }*/


private:
    // Метод для запроса у пользователя и записи в requests.json
    void readUserRequest() {
        // Читаем существующие запросы из файла requests.json
        std::ifstream requestsFile("../requests.json");
        json requestsJson;
        if (requestsFile.is_open()) {
            try {
                requestsFile >> requestsJson;
            } catch (json::parse_error& e) {
                std::cerr << "Error reading requests.json: " << e.what() << std::endl;
            }
            requestsFile.close();
        }

        // Получаем текущий список запросов
        std::vector<std::string> requests;
        if (requestsJson.find("requests") != requestsJson.end()) {
            requests = requestsJson["requests"].get<std::vector<std::string>>();
        }

        // Проверяем, если запросов больше 1000, удаляем самый старый
        if (requests.size() >= 1000) {
            requests.erase(requests.begin());
        }

        std::string userInput;
        bool validInput = false;

        while (!validInput) {
            std::cout << "Enter a search query (1-10 words): ";
            std::getline(std::cin, userInput);

            std::istringstream iss(userInput);
            std::vector<std::string> words{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

            if (words.size() >= 1 && words.size() <= 10) {
                validInput = true;
            } else {
                std::cout << "Invalid query. Please enter between 1 and 10 words." << std::endl;
            }
        }

        // Добавляем новый запрос в список
        requests.push_back(userInput);

        // Сохраняем обновленный список запросов в файл
        json updatedRequestsJson;
        updatedRequestsJson["requests"] = requests;

        std::ofstream outputRequestsFile("../requests.json");
        if (!outputRequestsFile.is_open()) {
            std::cerr << "Error: Unable to write to requests.json" << std::endl;
        } else {
            outputRequestsFile << updatedRequestsJson.dump(4);
            outputRequestsFile.close();
        }
    }

    std::string name;
    std::string version;
    int maxResponses;
    int timeUpdate;
    std::vector<std::string> files;
    std::unordered_set<std::string> stopWords = {"the", "is", "at", "which", "on", "and", "a", "to", "ah"};
};

int main() {
    ConverterJSON converter;
    return 0;
}
