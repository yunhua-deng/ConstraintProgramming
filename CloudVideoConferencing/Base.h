#pragma once

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <math.h>
#include <direct.h>

using namespace std;

double GetSumValue(const vector<double>&);
double GetMeanValue(const vector<double>&);
double GetStdValue(const vector<double>&);
double GetMaxValue(const vector<double> &);
double GetMinValue(const vector<double> &);
double GetPercentile(vector<double>, const double);
double GetRatioOfGreaterThan(const vector<double>&, const double);
void GenerateAllSubsets(const vector<size_t>&, vector<bool>&, size_t, vector<vector<size_t>>&);
bool SubsetComparatorBySize(const vector<size_t>&, const vector<size_t>&);
vector<vector<string>> ReadDelimitedTextFileIntoVector(const string, const char, const bool);
set<size_t> GenerateRandomIndexes(const size_t, const size_t, const size_t);
void DumpLabeledMatrixToDisk(const map<pair<string, string>, int> &, const vector<string> &, const string &);