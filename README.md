# Search Engine  

## Описание  
Search Engine — это многопоточное приложение, которое реализует:  
- Индексацию текстовых документов.  
- Обработку пользовательских запросов.  
- Выдачу релевантных результатов в порядке их ранжирования.  

Результаты поиска сохраняются в формате JSON, в файл `answers.json`.  

---

## Возможности  
- Загрузка конфигурации и списка документов из файла `config.json`.  
- Чтение текстовых документов из папки `resources`.  
- Обработка запросов из файла `requests.json`.  
- Генерация файла с результатами поиска `answers.json`, содержащего список документов, отсортированных по релевантности.  

---

## Требования  
Для работы приложения требуется:  
- Компилятор с поддержкой C++17.  
- Библиотека [nlohmann/json](https://github.com/nlohmann/json) для работы с JSON-файлами.  
- Многопоточная библиотека `std::thread` (входит в стандарт C++).  

---

## Установка

1. **Клонирование репозитория:**

    ```bash
    git clone <ссылка на репозиторий>
    cd <папка проекта>
    ```

2. **Установка библиотеки nlohmann/json:**

    Если используете пакетный менеджер vcpkg:

    ```bash
    vcpkg install nlohmann-json
    ```

    Укажите путь к библиотеке при компиляции:

    ```bash
    g++ -std=c++17 -pthread -o SearchEngine main.cpp ConverterJSON.cpp InvertedIndex.cpp SearchServer.cpp -I<путь_к_vcpkg>/installed/x64-linux/include -ljsoncpp
    ```

    Альтернативный способ:
    Скачайте библиотеку [nlohmann/json](https://github.com/nlohmann/json) и поместите ее в корень проекта.

3. **Создание необходимых файлов и директорий:**

    Убедитесь, что в директории проекта существует папка `resources`. В эту папку необходимо поместить текстовые документы для индексации.
    В папке проекта создайте файл `requests.json` для запросов и файл `config.json` для конфигурации. Пример формата файлов приведён ниже.

4. **Компиляция проекта:**

    ```bash
    g++ -std=c++17 -pthread -o SearchEngine main.cpp ConverterJSON.cpp InvertedIndex.cpp SearchServer.cpp -Iinclude
    ```

5. **Запуск приложения:**

    ```bash
    ./SearchEngine
    ```

# Конфигурация

## config.json

Файл конфигурации должен быть в формате JSON и содержать:

```json
{
    "config": {
        "name": "SearchEngine",
        "version": "1.0",
        "max_responses": 5,
        "time_update": 60
    },
    "files": [
        "resources/file1.txt",
        "resources/file2.txt"
    ]
}
```

## requests.json

Файл запросов должен быть в формате JSON и содержать:

```json
{
    "requests": [
        "пример первого запроса",
        "пример второго запроса"
    ]
}
```
## Как это работает

Программа читает настройки из `config.json` и загружает список текстовых файлов из папки `resources`.
Выполняется индексация документов. Частотный словарь создаётся в многопоточном режиме.
Запросы из файла `requests.json` обрабатываются, и для каждого вычисляется список релевантных документов.
Результаты поиска записываются в файл `answers.json`.

## Пример файла answers.json

```json
{
    "answers": {
        "request1": {
            "result": "true",
            "relevance": [
                {"docid": 1, "rank": 0.75},
                {"docid": 2, "rank": 0.5}
            ]
        },
        "request2": {
            "result": "false"
        }
    }
}
