#include "ConverterJSON.h"

std::string ConverterJSON::getName() {
    return name;
}

int ConverterJSON::getTimeUpdate() {
    return timeUpdate;
}

std::vector<std::string> ConverterJSON::getTextDocuments() {
    return files;
}

int ConverterJSON::getResponsesLimit() const {
    return maxResponses;
}

std::string ConverterJSON::getBasePath() {
    // Возвращает путь к исполняемому файлу
    std::string executablePath = std::filesystem::current_path().string();

    // Проверяем, где находится конфиг
    if (std::filesystem::exists(executablePath + "/config.json")) {
        return executablePath + "\\"; // Если конфиг рядом с exe
    } else {
        return "../../"; // Если запускаем из src, вернуться на уровень выше
    }
}

bool ConverterJSON::loadConfig() {
    std::ifstream configFile(getBasePath() + "config.json");

    if (!configFile.is_open()) {
        std::cerr << "Config file is missing." << std::endl;
        return false;
    }

    json configJson;
    try {
        configFile >> configJson;
    } catch (json::parse_error& e) {
        std::cerr << "Config file is empty or contains invalid JSON." << std::endl;
        return false;
    }

    if (configJson.find("config") == configJson.end()) {
        std::cerr << "Config file is empty." << std::endl;
        return false;
    }

    try {
        if (configJson["config"].contains("name")) {
            name = configJson["config"]["name"];
        } else {
            std::cerr << "Missing 'name' in config.json." << std::endl;
            return false;
        }

        if (configJson["config"].contains("version")) {
            version = configJson["config"]["version"];
        } else {
            std::cerr << "Missing 'version' in config.json." << std::endl;
            return false;
        }

        if (configJson["config"].contains("max_responses")) {
            maxResponses = configJson["config"]["max_responses"];
            if (maxResponses <= 0) {
                std::cerr << "Invalid 'max_responses' in config.json. It must be a positive integer." << std::endl;
                return false;
            }
        } else {
            maxResponses = 5; // Устанавливаем значение по умолчанию
        }

        if (version != VERSION_APP) {
            std::cerr << "Config.json has incorrect file version." << std::endl;
            return false;
        }

        if (configJson["config"].contains("time_update")) {
            timeUpdate = configJson["config"]["time_update"];
            if (timeUpdate <= 0) {
                std::cerr << "Invalid 'time_update' in config.json. It must be a positive integer." << std::endl;
                return false;
            }
        } else {
            std::cerr << "Missing 'time_update' in config.json." << std::endl;
            return false;
        }

        // Загружаем список файлов
        files.clear();
        std::string resourcesPath = getBasePath() + "/resources";

        if (!fs::exists(resourcesPath) || !fs::is_directory(resourcesPath)) {
            std::cerr << "Error: Resources path does not exist or is not a directory." << std::endl;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(resourcesPath)) {
            if (entry.is_regular_file()) {
                std::string relativePath = fs::relative(entry.path(), fs::current_path()).string();
                std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
                files.push_back(relativePath);
            }
        }

        configJson["files"] = files;

        // Обновляем конфигурационный файл
        std::ofstream outputConfigFile(getBasePath() + "/config.json");
        if (!outputConfigFile.is_open()) {
            std::cerr << "Error: Unable to write to config file." << std::endl;
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

std::vector<std::string> ConverterJSON::getRequests() {
    std::vector<std::string> listRequests;
    objJson.clear();

    std::ifstream requestsFile(getBasePath() + "/requests.json");
    if (!requestsFile)
        std::cerr << "File requests.json not found."<<std::endl;
    requestsFile >> objJson;
    requestsFile.close();
    for (const auto &i: objJson["requests"])
        listRequests.push_back(i);
    return listRequests;
}

void ConverterJSON::putAnswers(const std::vector<std::vector<std::pair<int, float>>>& answers) {
    std::ofstream answersFile(getBasePath() + "/answers.json");
    nlohmann::json objJson = {{"answers", {}}};

    for (int i = 0; i < answers.size(); ++i)
    {
        std::string requestKey = "request" + std::to_string(i + 1);
        nlohmann::json requestJson;

        if (answers[i].empty())
        {
            requestJson["result"] = "false";
        }
        else
        {
            requestJson["result"] = "true";
            nlohmann::json relevanceJson = nlohmann::json::array();
            for (const auto &pair : answers[i])
            {
                double roundedRank = std::ceil(pair.second * 1000) / 1000.0;
                // Явное округление до 3 знаков
                std::ostringstream stream;
                stream << std::fixed << std::setprecision(3) << roundedRank;
                relevanceJson.push_back({
                                                {"docid", pair.first},
                                                {"rank", std::stod(stream.str())} // Преобразуем обратно в double
                                        });
            }
            requestJson["relevance"] = relevanceJson;
        }
        objJson["answers"][requestKey] = requestJson;
    }

    answersFile << std::setw(4) << objJson << std::endl;
    answersFile.close();
}
