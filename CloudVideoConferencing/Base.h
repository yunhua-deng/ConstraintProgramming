#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED
#endif
#pragma once

#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <functional>
#include <exception>
#include <utility>
#include <thread>
#include <chrono>
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