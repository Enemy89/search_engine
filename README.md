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
    скачайте библиотеку [nlohmann/json](https://github.com/nlohmann/json) и поместите ее в корень проекта.

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
        "name": "SearchEngine",    // Имя вашего локального поискового движка, может быть любым
        "version": "1.0",    // Версия движка, должна совпадать с версией приложения для запуска движка
        "max_responses": 5,    // Максимальное количество ответов на запрос
        "time_update": 60    // Время обновления индекса в секундах
    },
    "files": [
        "resources/file1.txt",    // Путь к первому текстовому файлу для индексации
        "resources/file2.txt"    // Путь ко второму текстовому файлу для индексации
    ]
}
```

## requests.json

Файл запросов должен быть в формате JSON и содержать:

```json
{
    "requests": [
        "example query",    // Пример первого запроса
        "another example"    // Пример второго запроса
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
        "request1": {    //порядковы номер запроса в порядке размещения в файле requests.json
            "result": "true",    //показатель успешности поиска: true - ответ на запрос успешно найден, false - в докумнетах нет релевантных данных
            "relevance": [
                {
                    "docid": 1,    // порядковый номер документа, в порядке размещения в файле config.json
                    "rank": 0.75    //рейтинг релевантности документа
                },
                {
                    "docid": 2,    / порядковый номер документа, в порядке размещения в файле config.json
                    "rank": 0.5    //рейтинг релевантности документа
                }
            ]
        },
        "request2": {    //порядковы номер запроса в порядке размещения в файле requests.json
            "result": "false"    //показатель успешности поиска: true - ответ на запрос успешно найден, false - в докумнетах нет релевантных данных
        }
    }
}
```
## Ограничения  

### 1. Прекращение работы поисковой системы:  
Программа прекратит свою работу при следующих условиях:  

- **Отсутствие файла `config.json`:** Файл конфигурации обязателен для запуска системы. Если его нет, приложение не запустится.  
- **Отсутствие поля `config` в файле `config.json`:** Если файл существует, но в нем нет поля `config`, приложение не сможет прочитать необходимые настройки.  
- **Несовпадение версии поискового движка:** Если версия приложения, указанная в `config.json`, не совпадает с текущей версией поискового движка, программа не запустится.  

### 2. С продолжением работы поисковой системы:  
В случае, если указанные ниже ошибки встречаются, программа продолжит свою работу, но будет выдавать предупреждения или ошибки в процессе:  

- **Отсутствие файлов для индексации:** Если в папке `resources` не будет текстовых файлов для индексации, система не сможет выполнить поиск по ним, но продолжит работу с доступными документами.  
- **Отсутствие файла `requests.json`:** Если файл с запросами не найден, программа не сможет выполнить поиск по запросам, но продолжит работу без них.  
- **Превышение количества слов в файле для индексации:** Если в одном из файлов для индексации содержится более 1000 слов, система проигнорирует этот файл, но продолжит индексацию других документов.  
- **Превышение количества символов в слове (более 100) в файле для индексации:** Если в одном из документов встречается слово длиной более 100 символов, это слово будет проигнорировано, а индексация продолжится с остальными словами.  
- **Превышение количества запросов в файле `requests.json`:** Если количество запросов в файле `requests.json` превышает 1000, программа ограничит количество запросов до 1000, игнорируя остальные.  
- **Превышение количества слов в запросе (более 10) в файле `requests.json`:** Если запрос в файле содержит более 10 слов, он будет проигнорирован, а программа продолжит обработку оставшихся запросов.  
