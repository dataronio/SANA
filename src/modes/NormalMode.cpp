#include "NormalMode.hpp"
#include <cassert>
#include <utility>
#include <iostream>
#include "../utils/utils.hpp"
#include "../utils/FileIO.hpp"
#include "../arguments/measureSelector.hpp"
#include "../arguments/MethodSelector.hpp"
#include "../arguments/GraphLoader.hpp"
#include "../Report.hpp"

void NormalMode::run(ArgumentParser& args) {
    createFolders();

    //before loading graphs, check that the user did not forget to provide the execution time/iter
    //this is just to detect this common mistake early
    if (args.strings["-method"] == "sana") MethodSelector::validateTimeOrIterLimit(args);

    pair<Graph, Graph> graphs = GraphLoader::initGraphs(args);
    Graph G1 = graphs.first;
    Graph G2 = graphs.second;

    MeasureCombination M;
    measureSelector::initMeasures(M, G1, G2, args); 
    Method* method;
    method = MethodSelector::initMethod(G1, G2, args, M);
    Alignment A = method->runAndPrintTime();
    A.printDefinitionErrors(G1,G2);
    assert(A.isCorrectlyDefined(G1, G2) and "Resulting alignment is not correctly defined");

    bool longReport = (args.bools["-multi-iteration-only"] ? false : true);

    Report::saveReport(G1, G2, A, M, method, args.strings["-o"], longReport);
    Report::saveLocalMeasures(G1, G2, A, M, method, args.strings["-localScoresFile"]);
    delete method;
}

string NormalMode::getName() { return "NormalMode"; }

void NormalMode::createFolders() {
    FileIO::createFolder("matrices"); //are all these still used? -Nil
    FileIO::createFolder("tmp");
    FileIO::createFolder("alignments");
    FileIO::createFolder("go");
    FileIO::createFolder("go/autogenerated");
    FileIO::createFolder("networks");
    FileIO::createFolder(AUTOGENEREATED_FILES_FOLDER);
}
