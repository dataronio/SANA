#include "Alignment.hpp"
#include "Graph.hpp"
#include "utils/utils.hpp"

using namespace std;

Alignment::Alignment() {}
Alignment::Alignment(const vector<uint>& mapping): A(mapping) {}
Alignment::Alignment(const Alignment& alig): A(alig.A) {}
Alignment::Alignment(const Graph& G1, const Graph& G2, const vector<array<string, 2>>& edgeList) {
    assert(G1.getNumNodes() == edgeList.size());
    uint n1 = G1.getNumNodes();
    uint n2 = G2.getNumNodes();
    A = vector<uint>(n1, n2); //n2 used to denote invalid index
    for (const auto& edge : edgeList) {
        string nodeG1 = edge[0], nodeG2 = edge[1];
        A[G1.getNameIndex(nodeG1)] = G2.getNameIndex(nodeG2);
    }
    printDefinitionErrors(G1,G2);
    assert(isCorrectlyDefined(G1, G2));
}

uint Alignment::size() const { return A.size(); }
vector<uint> Alignment::asVector() const { return A; }
const vector<uint>* Alignment::getVector() const { return &A; }
uint& Alignment::back() { return A.back(); }
uint& Alignment::operator[] (uint node) { return A[node]; }
const uint& Alignment::operator[](const uint node) const { return A[node]; }

Alignment Alignment::loadEdgeList(const Graph& G1, const Graph& G2, const string& fileName) {
    vector<string> edges = fileToStrings(fileName);
    vector<array<string, 2>> edgeList;
    edgeList.reserve(edges.size()/2);
    for (uint i = 0; i < edges.size(); i += 2) {
        edgeList.push_back({edges[i], edges[i+1]});
    }
    return Alignment(G1, G2, edgeList);
}

Alignment Alignment::loadEdgeListUnordered(const Graph& G1, const Graph& G2, const string& fileName) {
    vector<string> edges = fileToStrings(fileName);
    vector<array<string, 2>> edgeList;
    edgeList.reserve(edges.size()/2);
    for (uint i = 0; i < edges.size(); i += 2) {
        string name1 = edges[i], name2 = edges[i+1];
        //check if G1 contains a node named name1. If not, G2 must contain it and name2 must be in G1
        //(note: may not work as intended if graphs have overlapping names -Nil)
        if (G1.hasNodeName(name1)) {
            assert(G2.hasNodeName(name2));
            edgeList.push_back({name1, name2});
        } else {
            assert(G1.hasNodeName(name2));
            assert(G2.hasNodeName(name1));
            edgeList.push_back({name2, name1});
        }
    }
    return Alignment(G1, G2, edgeList);
}

Alignment Alignment::loadPartialEdgeList(const Graph& G1, const Graph& G2,
                                    const string& fileName, bool byName) {
    vector<string> edges = fileToStrings(fileName);
    vector<array<string, 2>> edgeList;
    edgeList.reserve(edges.size()/2);
    for (uint i = 0; i < edges.size(); i += 2) {
        edgeList.push_back({edges[i], edges[i+1]});
    }
    uint n1 = G1.getNumNodes(), n2 = G2.getNumNodes();
    vector<uint> A(n1, n2); //n2 used to note invalid value
    for (const auto& edge : edgeList) {
        string nodeG1 = edge[0], nodeG2 = edge[1];
        if (not byName) {
            A[atoi(nodeG1.c_str())] = atoi(nodeG2.c_str());
        } else {
            bool nodeG1Misplaced = false;
            bool nodeG2Misplaced = false;
            if (not G1.hasNodeName(nodeG1)) {
                cout << nodeG1 << " not in G1 " << G1.getName();
                if (G2.hasNodeName(nodeG1)) {
                    cout << ", but it is in G2. Will switch if appropriate." << endl;
                    nodeG1Misplaced = true;
                } else {
                    cout << endl;
                    continue;
                }
            }
            if (not G2.hasNodeName(nodeG2)) {
                cout << nodeG2 << " not in G2 " << G2.getName();
                if (G1.hasNodeName(nodeG2)) {
                    cout << ", but it is in G1. Will switch if appropriate." << endl;
                    nodeG2Misplaced = true;
                } else {
                    cout << endl;
                    continue;
                }
            }
            if (nodeG1Misplaced and nodeG2Misplaced) {
                swap(nodeG1, nodeG2);
                cout << nodeG1 << " and " << nodeG2 << " swapped." << endl;
            }
            A[G1.getNameIndex(nodeG1)] = G2.getNameIndex(nodeG2);
        }
    }
    vector<bool> G2AssignedNodes(n2, false);
    for (uint i = 0; i < n1; i++) {
        if (A[i] != n2) {
            if (G2AssignedNodes[A[i]]) throw runtime_error("two G1 nodes map to the same G2 node");
            G2AssignedNodes[A[i]] = true;
        }
    }
    for (uint i = 0; i < n1; i++) {
        if (A[i] == n2) {
            int j = randMod(n2);
            while (G2AssignedNodes[j]) j = randMod(n2);
            A[i] = j;
            G2AssignedNodes[j] = true;
        }
    }
    Alignment alig(A);
    alig.printDefinitionErrors(G1, G2);
    assert(alig.isCorrectlyDefined(G1, G2));
    return alig;
}

Alignment Alignment::loadMapping(const string& fileName) {
    if (not fileExists(fileName)) {
        throw runtime_error("Starting alignment file "+fileName+" not found");
    }
    ifstream infile(fileName.c_str());
    string line;
    getline(infile, line); //reads only the first line, ignores the rest
    istringstream iss(line);
    vector<uint> A(0);
    int g2Ind;
    while (iss >> g2Ind) A.push_back(g2Ind);
    infile.close();
    return A;
}

Alignment Alignment::random(uint n1, uint n2) {
    //taken from: http://stackoverflow.com/questions/311703/algorithm-for-sampling-without-replacement
    vector<uint> alignment(n1);
    uint t = 0; // total input records dealt with
    uint m = 0; // number of items selected so far
    while (m < n1) {
        double u = randDouble();
        if ((n2 - t)*u >= n1 - m) {
            t++;
        }
        else {
            alignment[m] = t;
            t++;
            m++;
        }
    }
    randomShuffle(alignment);
    return alignment;
}

Alignment Alignment::empty() {
    vector<uint> emptyMapping(0);
    return Alignment(emptyMapping);
}

Alignment Alignment::identity(uint n) {
    vector<uint> A(n);
    for (uint i = 0; i < n; i++) {
        A[i] = i;
    }
    return Alignment(A);
}

Alignment Alignment::correctMapping(const Graph& G1, const Graph& G2) {
    if (not G1.hasSameNodeNamesAs(G2)) {
        throw runtime_error("cannot load correct mapping; nodes have different names");
    }
    vector<uint> A(G1.getNumNodes());
    for (uint i = 0; i < G1.getNumNodes(); i++) {
        A[i] = G2.getNameIndex(G1.getNodeName(i));
    }
    return Alignment(A);
}

Alignment &Alignment::operator=(Alignment other) {
    const uint n = max(A.size(), other.A.size());
    A.reserve(n);
    other.A.reserve(n);
    swap(A, other.A);
    A.shrink_to_fit();
    other.A.shrink_to_fit();
    return *this;
}

uint Alignment::numAlignedEdges(const Graph& G1, const Graph& G2) const {
    uint res = 0;
    for (const auto& edge: *(G1.getEdgeList()))
        if (G2.hasEdge(A[edge[0]], A[edge[1]])) res++;
    return res;
}

Alignment Alignment::reverse(uint n2) const {
    uint n1 = size();
    vector<uint> A(n2, n1); //n1 used for invalid mapping
    for (uint i = 0; i < n1; i++) A[A[i]] = i;
    return Alignment(A);
}

void Alignment::compose(const Alignment& other) {
    for (uint i = 0; i < size(); i++) A[i] = other.A[A[i]];
}

bool Alignment::isCorrectlyDefined(const Graph& G1, const Graph& G2) {
    uint n1 = G1.getNumNodes();
    uint n2 = G2.getNumNodes();
    vector<bool> G2AssignedNodes(n2, false);
    vector<uint> colorMap = G1.myColorIdsToOtherGraphColorIds(G2);

    if (A.size() != n1) return false;
    for (uint i = 0; i < n1; i++) {
        if (A[i] < 0 or A[i] >= n2 or G2AssignedNodes[A[i]]) return false;
        G2AssignedNodes[A[i]] = true;
        uint g1Color = G1.getNodeColor(i);
        uint g2Color = G2.getNodeColor(A[i]);
        if (colorMap.at(g1Color) != g2Color) return false;
    }
    return true;
}

void Alignment::printDefinitionErrors(const Graph& G1, const Graph& G2) {
    uint n1 = G1.getNumNodes(), n2 = G2.getNumNodes();
    vector<bool> G2AssignedNodes(n2, false);
    vector<uint> colorMap = G1.myColorIdsToOtherGraphColorIds(G2);
    int count = 0;
    if (A.size() != n1) 
        cerr << "Incorrect size: "<<A.size()<<", should be "<<n1<<endl;
    for (uint i = 0; i < n1; i++) {
        if (A[i] < 0 or A[i] >= n2) {
            cerr<<count<<": node "<<i<<" ("<<G1.getNodeName(i)<<") maps to ";
            cerr<<A[i]<<", which is not in range 0..n2 ("<<n2<<")"<<endl;
            count++;
        }
        if (G2AssignedNodes[A[i]]) {
            cerr<<count<<": node "<<i<<" ("<<G1.getNodeName(i)<<") maps to "<<A[i]<<" (";
            cerr<<G2.getNodeName(A[i])<<"), which is also mapped to by a previous node"<<endl;
            count++;
        }
        G2AssignedNodes[A[i]] = true;
        uint g1Color = G1.getNodeColor(i);
        uint g2Color = G2.getNodeColor(A[i]);
        if (colorMap.at(g1Color) != g2Color) {
            string g1ColorName = G1.getColorName(g1Color);
            string g2ColorName = G2.getColorName(g2Color);
            cerr<<count<<": node "<<i<<" ("<<G1.getNodeName(i)<<") of color ";
            cerr<<G1.getColorName(g1Color)<<" maps to "<<A[i]<<" (";
            cerr<<G2.getNodeName(A[i])<<") of color "<<G2.getColorName(g2Color)<<endl;
            count++;
        }
    }
}

//precondition: a valid color-restricted matching exists between G1 and G2
//that means: every color in G1 has at least as many nodes in G2
Alignment Alignment::randomColorRestrictedAlignment(const Graph& G1, const Graph& G2) {

    vector<uint> G2ColorIdToG1ColorId = G2.myColorIdsToOtherGraphColorIds(G1);
    vector<vector<uint>> G2NodesByG1ColorId(G1.numColors());
    for (uint node = 0; node < G2.getNumNodes(); node++) {
        uint g2ColorId = G2.getNodeColor(node);
        uint g1ColorId = G2ColorIdToG1ColorId[g2ColorId];
        if (g1ColorId == Graph::INVALID_COLOR_ID) continue;
        G2NodesByG1ColorId[g1ColorId].push_back(node);
    }
    for (uint id = 0; id < G1.numColors(); id++) {
        randomShuffle(G2NodesByG1ColorId[id]);
    }
    
    vector<uint> colorIndices(G1.numColors(), 0);
    vector<uint> A(0);
    A.reserve(G1.getNumNodes());
    for (uint node = 0; node < G1.getNumNodes(); node++) {
        uint colorId = G1.getNodeColor(node);
        if (colorIndices[colorId] >= G2NodesByG1ColorId[colorId].size()) {
            throw runtime_error("for some color, ran out of nodes in g2 to assign to nodes in g1");
        }
        A.push_back(G2NodesByG1ColorId[colorId][colorIndices[colorId]]);
        colorIndices[colorId]++;
    }

    Alignment alig(A);
    if (not alig.isCorrectlyDefined(G1, G2)) {
        alig.printDefinitionErrors(G1, G2);
        throw runtime_error("alignment not correctly defined");
    }
    return alig;   
}
