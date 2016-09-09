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

using ID = size_t;

struct Path
{
	ID sender;
	ID dc_sender;
	ID dc_receiver;
	ID receiver;

	Path(ID s, ID d_s, ID d_r, ID r)
	{
		sender = s;
		dc_sender = d_s;
		dc_receiver = d_r;
		receiver = r;
	}

	Path() {}
};
bool operator<(const Path&, const Path&);
bool operator==(const Path&, const Path&);
void PrintOutPath(const Path&);

struct Client
{
	ID id;
	string name;
	double outgoing_data_amount;
	double incoming_data_amount;
	map<ID, double> delay_to_dc;
	ID nearest_dc;
	vector<ID> nearest_dc_list;
	string region;
	string subregion;

	map<ID, vector<Path>> path_to_client; // memory-intensive (not applicable to large input size)
	map<ID, Path> shortest_path_to_client;

	list<ID> dc_domain; // using list (rather than vector) for efficient deletion of elements in the middle in constraint propogation
	ID cheapest_dc;

	ID assigned_dc;
};
bool operator==(const Client&, const Client&);
bool operator!=(const Client&, const Client&);
bool operator<(const Client&, const Client&);
bool ClientComparator_ByDomainSize(const Client&, const Client&);

struct Datacenter
{
	ID id;
	string name;
	string provider;
	double server_rental_price;
	double external_bandwidth_price;
	double internal_bandwidth_price;
};
bool DatacenterComparator_ByExternalBandwidthPrice(const Datacenter&, const Datacenter&);

struct Setting
{
	/*double recommended_delay_bound;
	double maximum_allowed_delay_bound;*/
	double session_size;
	double session_count;

	Setting(double given_session_size, double given_session_count)
	{
		session_size = given_session_size;
		session_count = given_session_count;
	}

	Setting() {}
};

struct Global
{
	Setting sim_setting;

	vector<vector<double>> client_to_dc_delay_table;
	vector<vector<double>> dc_to_dc_delay_table;
	vector<string> client_name_list;
	vector<string> dc_name_list;
	vector<double> dc_server_rental_price_list;
	vector<double> dc_external_bandwidth_price_list;
	vector<double> dc_internal_bandwidth_price_list;

	vector<ID> dc_id_list;
	vector<ID> client_id_list;

	map<ID, Datacenter> datacenter;
	map<ID, Client> client;
	map<string, vector<ID>> client_cluster;
	//map<Path, double> path_length; // memory-intensive: not applicable for large input sizes (e.g., > 1000 candidate clients)
};


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