#include <iostream>
#include "SearchServer.h"
#include "InvertedIndex.h"
#include "ConverterJSON.h"


int main() {
    ConverterJSON converterJson;
    InvertedIndex invertedIndex;

    if (!converterJson.loadConfig()) {
        std::cerr << "Failed to load configuration." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "Starting "<< converterJson.getName() << std::endl;

    std::vector <std::string > listFiles = converterJson.getTextDocuments();
    invertedIndex.updateDocumentBase(listFiles);
    SearchServer searchServer(invertedIndex);
    std::vector <std::string > listQuery = converterJson.getRequests();
    std::vector<std::vector<RelativeIndex>> listRelativeIndex = searchServer.search(listQuery, converterJson.getResponsesLimit());
    std::vector<std::vector<std::pair<int, float>>>answers;
    for(auto & i : listRelativeIndex)
    {
        std::pair <int, float> pairResult;
        std::vector<std::pair<int,float>> answersPair;
        for(auto & j : i)
        {
            pairResult = std::make_pair(j.docId,j.rank);
            answersPair.push_back(pairResult);
        }
        answers.push_back(answersPair);
    }
    converterJson.putAnswers(answers);

    std::cout<<"Successfully. The file answers.json formed."<<std::endl;
    std::cin.get();
    return 0;
}
