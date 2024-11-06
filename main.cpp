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

// Структура данных для хранения результата работы findDocumentIdsForTokens
using RequestData = std::unordered_map<std::string, std::vector<std::vector<int>>>;


class ConverterJSON {
private:
    std::string name;
    std::string version;
    int maxResponses;
    int timeUpdate;
    std::vector<std::string> files;
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

        handleUserQueryAndSave();
    }

private:
    //метод загрузки и проверки конфига и обновления
    bool loadConfig() {
        std::ifstream configFile("../config.json");

        if (!configFile.is_open()) {
            std::cerr << "Config file is missing." << std::endl;
            return false;
        }

        //пытаемся записать содержимое из файла
        json configJson;
        try {
            configFile >> configJson;
        } catch (json::parse_error& e) {
            std::cerr << "Config file is empty or contains invalid JSON." << std::endl;
            return false;
        }

        //проверяем на пустоту
        if (configJson.find("config") == configJson.end()) {
            std::cerr << "config file is empty" << std::endl;
            return false;
        }

        //десериализация и проверка
        try {
            if (configJson["config"].find("name") != configJson["config"].end()) {
                name = configJson["config"]["name"];
            } else {
                return false;
            }

            if (configJson["config"].find("version") != configJson["config"].end()) {
                version = configJson["config"]["version"];
            } else {
                return false;
            }

            if (configJson["config"].find("max_responses") != configJson["config"].end()) {
                maxResponses = configJson["config"]["max_responses"];
                if (maxResponses <= 0) {
                    std::cerr << "Invalid max_responses in config.json. It must be a positive integer." << std::endl;
                    return false;
                }
            } else { //если не указано максмальное число, то устанавливаем 5 и аписываем в конфиг
                maxResponses = 5;
                configJson["config"]["max_responses"] = maxResponses;
            }

            //проверка соответствия версии конфига и приложения
            if (version != APP_VERSION) {
                std::cerr << "Сonfig.json has incorrect file version" << std::endl;
                return false;
            }

            if (configJson["config"].find("time_update") != configJson["config"].end()) {
                timeUpdate = configJson["config"]["time_update"];
                if (timeUpdate <= 0) {
                    std::cerr << "Invalid time_update in config.json. It must be a positive integer." << std::endl;
                    return false;
                }
            } else {
                return false;
            }

            // Очистка и заполнение списка файлов
            files.clear();
            std::string resourcesPath = "../resources";

            if (!fs::exists(resourcesPath) || !fs::is_directory(resourcesPath)) {
                std::cerr << "Error: Resources path does not exist or is not a directory" << std::endl;
                return false;
            } else {
                // Устанавливаем начальный ID для документов
                int nextDocID = 1;
                configJson["document_id"] = json::object();  // Очищаем документ_id, чтобы пересоздать его по порядку

                // Добавляем новые и существующие файлы в список и присваиваем новые document_id
                for (const auto &entry : fs::directory_iterator(resourcesPath)) {
                    if (entry.is_regular_file()) {
                        std::string relativePath = fs::relative(entry.path(), fs::current_path()).string();
                        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
                        files.push_back(relativePath);

                        // Присваиваем последовательный document_id
                        configJson["document_id"][relativePath] = nextDocID++;
                    }
                }

                // Сохраняем обновленный список файлов
                configJson["files"] = files;
            }

            //записываем измеенения
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
        std::cout << "Config successfully loaded and saved." << std::endl;
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

            // Сравниваем с интервалом времени, если текущее время - время последнего изменения базы больше. чем время обновления из конфига, то обновляем
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
            for (const auto& [filePath, docID] : configJson["document_id"].items()) {
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
    
private:
    // Функция для поиска ID токена в базе индексов
    int findTermID(const json& indexJson, const std::string& token) {
        if (indexJson.contains("term_index") && indexJson["term_index"].contains("term_to_id")) {
            const auto& termToID = indexJson["term_index"]["term_to_id"];
            auto it = termToID.find(token);
            if (it != termToID.end()) {
                return it.value(); // Возвращаем ID терма
            }
        }
        return 0; // Возвращаем 0, если терм не найден
    }

// Функция для добавления пользовательского запроса в файл requests.json
    void saveUserQueryToFile(const std::vector<std::string>& tokens) {
        json requestsJson;
        json indexJson;

        // Загрузка базы индексов из файла index.json
        std::ifstream indexFile("../index.json");
        if (indexFile.is_open()) {
            indexFile >> indexJson;
            indexFile.close();
        } else {
            std::cerr << "Error: Unable to open index.json" << std::endl;
            return;
        }

        // Проверка на существование файла requests.json
        std::ifstream requestFile("../requests.json");
        if (requestFile.is_open()) {
            if (requestFile.peek() == std::ifstream::traits_type::eof()) {
                requestsJson = json::object();
            } else {
                try {
                    requestFile >> requestsJson;
                } catch (json::parse_error& e) {
                    std::cerr << "Error reading requests.json: " << e.what() << std::endl;
                    requestsJson = json::object();
                }
            }
            requestFile.close();
        } else {
            requestsJson = json::object();
        }

        // Проверка количества запросов
        if (requestsJson.size() >= 1000) {
            auto oldestKey = requestsJson.begin().key();
            requestsJson.erase(oldestKey);
        }

        // Генерация ключа для нового запроса
        std::string requestKey = "requests" + std::to_string(requestsJson.size() + 1);

        // Формирование записи запроса с учетом id и позиции токенов
        json requestEntry = json::array();
        for (size_t i = 0; i < tokens.size(); ++i) {
            int tokenId = findTermID(indexJson, tokens[i]);
            requestEntry.push_back({ {"id", tokenId}, {"token", tokens[i]}, {"position", static_cast<int>(i)} });
        }
        requestsJson[requestKey] = requestEntry;

        // Сохранение обновленного JSON в файл
        std::ofstream outputRequestFile("../requests.json");
        if (outputRequestFile.is_open()) {
            outputRequestFile << requestsJson.dump(4);
            outputRequestFile.close();
        } else {
            std::cerr << "Error: Unable to write to requests.json" << std::endl;
        }
    }

// Основная функция для обработки запроса пользователя и его сохранения
    void handleUserQueryAndSave() {
        std::string query;

        while (true) {
            std::cout << "Please enter your search query (1 to 10 words) or type 'break' to exit: ";
            std::getline(std::cin, query);

            if (query == "break") {
                std::cout << "Exiting query input." << std::endl;
                break;
            }

            std::vector<std::string> tokens = tokenize(query);
            toLowercase(tokens);
            removeStopWords(tokens);

            if (tokens.size() < 1 || tokens.size() > 10) {
                std::cerr << "Invalid query: Please enter between 1 and 10 valid words." << std::endl;
            } else {
                saveUserQueryToFile(tokens);
                break;
            }
        }
    }
public:
    RequestData findDocumentIdsForTokens() {
        json requestsJson;
        json indexJson;
        RequestData requestDocIds;

        // Загрузка данных из файлов requests.json и index.json
        std::ifstream requestsFile("../requests.json");
        if (requestsFile.is_open()) {
            if (requestsFile.peek() == std::ifstream::traits_type::eof()) {
                std::cerr << "Error: requests.json is empty" << std::endl;
                return {};
            }
            requestsFile >> requestsJson;
            requestsFile.close();
        } else {
            std::cerr << "Error: Unable to open requests.json" << std::endl;
            return {};
        }

        std::ifstream indexFile("../index.json");
        if (indexFile.is_open()) {
            if (indexFile.peek() == std::ifstream::traits_type::eof()) {
                std::cerr << "Error: index.json is empty" << std::endl;
                return {};
            }
            indexFile >> indexJson;
            indexFile.close();
        } else {
            std::cerr << "Error: Unable to open index.json" << std::endl;
            return {};
        }

        // Создание и сортировка вектора для каждого токена
        for (const auto& [requestKey, tokens] : requestsJson.items()) {
            std::vector<std::vector<int>> docIdList;

            for (const auto& token : tokens) {
                int tokenId = token["id"];
                std::vector<int> documentIds;

                if (tokenId == 0) {
                    continue;
                }

                // Ищем tokenId в positional_index и собираем id документов
                if (indexJson.contains("positional_index") &&
                    indexJson["positional_index"].contains(std::to_string(tokenId))) {

                    for (const auto& [docId, positionInfo] : indexJson["positional_index"][std::to_string(tokenId)].items()) {
                        documentIds.push_back(std::stoi(docId));
                    }

                    // Сортируем documentIds по возрастанию
                    std::sort(documentIds.begin(), documentIds.end());
                }

                docIdList.push_back(documentIds);
            }

            requestDocIds[requestKey] = docIdList;
        }

        return requestDocIds;
    }

// Новая функция для подсчета совпадений
    void countDocumentMatches(const std::unordered_map<std::string, std::vector<std::vector<int>>>& requestData) {
        for (const auto& [requestKey, docIdLists] : requestData) {
            std::cout << "Processing " << requestKey << ":\n";
            if (docIdLists.size() <= 1) {
                std::cout << "No matches to count." << std::endl;
                continue;
            }

            std::unordered_map<int, int> docCount;

            // Подсчитываем вхождения каждого id документа
            for (const auto& docIds : docIdLists) {
                for (int docId : docIds) {
                    docCount[docId]++;
                }
            }

            // Переносим данные из docCount в вектор пар для сортировки
            std::vector<std::pair<int, int>> sortedDocCount(docCount.begin(), docCount.end());

            // Сортируем по убыванию количества совпадений
            std::sort(sortedDocCount.begin(), sortedDocCount.end(),
                      [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                          return a.second > b.second;
                      });

            // Выводим отсортированный результат
            for (const auto& [docId, count] : sortedDocCount) {
                std::cout << "Document ID: " << docId << " - Matches: " << count << "\n";
            }
        }
    }

    void calculatePositionDifference(const json& requestsJson, const json& indexJson) {
        for (const auto& [requestKey, tokens] : requestsJson.items()) {
            if (tokens.size() < 2) {
                std::cout << requestKey << ": Not enough tokens for comparison." << std::endl;
                continue;
            }

            std::cout << "Processing " << requestKey << ":\n";

            // Получаем ID токенов из запроса
            int firstTokenId = tokens[0]["id"];
            int secondTokenId = tokens[1]["id"];

            // Проверяем наличие этих токенов в индексе
            if (!indexJson["positional_index"].contains(std::to_string(firstTokenId)) ||
                !indexJson["positional_index"].contains(std::to_string(secondTokenId))) {
                std::cout << "Tokens not found in index for " << requestKey << std::endl;
                continue;
            }

            // Получаем документы, где присутствуют оба токена
            const auto& firstTokenDocs = indexJson["positional_index"][std::to_string(firstTokenId)];
            const auto& secondTokenDocs = indexJson["positional_index"][std::to_string(secondTokenId)];

            // Проходим по каждому документу, где присутствуют оба токена
            for (const auto& [docId, firstPositions] : firstTokenDocs.items()) {
                if (secondTokenDocs.contains(docId)) {
                    // Получаем позиции первого и второго токенов в текущем документе
                    const std::vector<int> firstPositionsVec = firstPositions["positions"].get<std::vector<int>>();
                    const std::vector<int> secondPositionsVec = secondTokenDocs[docId]["positions"].get<std::vector<int>>();

                    // Вычисляем минимальное значение разницы по модулю
                    int minPositionDiff = INT_MAX;
                    for (int firstPos : firstPositionsVec) {
                        for (int secondPos : secondPositionsVec) {
                            int diff = abs(firstPos - secondPos);
                            minPositionDiff = std::min(minPositionDiff, diff);
                        }
                    }

                    std::cout << "Document ID: " << docId << " - Minimum Position Difference: " << minPositionDiff << "\n";
                }
            }
        }
    }

    //заглушка-персер джейсона, надо будет переделать на адекватную сериализацию-десериализацию
    std::pair<json, json> loadJsonFiles() {
        json requestsJson;
        json indexJson;

        // Загрузка requests.json
        std::ifstream requestsFile("../requests.json");
        if (requestsFile.is_open()) {
            if (requestsFile.peek() != std::ifstream::traits_type::eof()) {
                requestsFile >> requestsJson;
            } else {
                std::cerr << "Error: requests.json is empty" << std::endl;
            }
            requestsFile.close();
        } else {
            std::cerr << "Error: Unable to open requests.json" << std::endl;
        }

        // Загрузка index.json
        std::ifstream indexFile("../index.json");
        if (indexFile.is_open()) {
            if (indexFile.peek() != std::ifstream::traits_type::eof()) {
                indexFile >> indexJson;
            } else {
                std::cerr << "Error: index.json is empty" << std::endl;
            }
            indexFile.close();
        } else {
            std::cerr << "Error: Unable to open index.json" << std::endl;
        }

        return {requestsJson, indexJson};
    }


    std::unordered_set<std::string> stopWords = {"the", "is", "at", "which", "on", "and", "a", "to", "ah"};

};

int main() {
    ConverterJSON converter;
    // Загрузка JSON-файлов
    auto [requestsJson, indexJson] = converter.loadJsonFiles();

    // Вызов функции для расчета минимальных разниц позиций
    converter.calculatePositionDifference(requestsJson, indexJson);

    // Запуск функции для получения documentIds
    RequestData requestDocIds = converter.findDocumentIdsForTokens();

    // Подсчет совпадений
    converter.countDocumentMatches(requestDocIds);
    return 0;
}
