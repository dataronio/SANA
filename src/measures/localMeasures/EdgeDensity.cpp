#include <vector>
#include <iostream>
#include "EdgeDensity.hpp"

using namespace std;

EdgeDensity::EdgeDensity(Graph* G1, Graph* G2, const vector<double>& distWeights) : LocalMeasure(G1, G2, "edged") {
    vector<double> normWeights(distWeights);
    normalizeWeights(normWeights);
    this->distWeights = normWeights;

    string fileName = autogenMatricesFolder+G1->getName()+"_"+
        G2->getName()+"_edged_"+to_string(normWeights.size());
    for (double w : normWeights)
        fileName += "_" + extractDecimals(w, 3);
    fileName += ".bin";

    loadBinSimMatrix(fileName);
}

void EdgeDensity::initSimMatrix() {
    uint n1 = G1->getNumNodes();
    uint n2 = G2->getNumNodes();
    uint k = distWeights.size();
    vector<vector<ushort> > densities1 (n1, vector<ushort> (k+1));
    vector<vector<ushort> > densities2 (n2, vector<ushort> (k+1));
    for (ushort i = 0; i < n1; i++) {
        densities1[i] = G1->numEdgesAround(i, k);
        for (ushort j = 1; j < k; j++) {
            densities1[i][j] += densities1[i][j-1];
        } 
    }
    for (ushort i = 0; i < n2; i++) {
        densities2[i] = G2->numEdgesAround(i, k);
        for (ushort j = 1; j < k; j++) {
            densities2[i][j] += densities2[i][j-1];
        }
    }
    sims = vector<vector<float> > (n1, vector<float> (n2, 0));
    for (uint h = 0; h < k; h++) {
        if (distWeights[h] > 0) {
            for (uint i = 0; i < n1; i++) {
                for (uint j = 0; j < n2; j++) {
                    if (densities1[i][h] < densities2[j][h]) {
                        sims[i][j] += ((double) densities1[i][h]/densities2[j][h]) * distWeights[h];
                    }
                    else {
                        sims[i][j] += ((double) densities2[j][h]/densities1[i][h]) * distWeights[h];
                    }
                }
            }
        }
    }
}

EdgeDensity::~EdgeDensity() {
}