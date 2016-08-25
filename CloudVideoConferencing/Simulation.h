#ifndef SIMULATION_H_INCLUDED
#define SIMULATION_H_INCLUDED
#endif
#pragma once

#include "Base.h"

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

	Path(){}
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
	string region;
	string subregion;

	map<ID, vector<Path>> path_to_client; // memory-intensive (not applicable to large input size)
	map<ID, Path> shortest_path_to_client;

	list<ID> dc_domain; // using list (rather than vector) for efficient deletion of elements in the middle
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
	double recommended_delay_bound;
	double maximum_allowed_delay_bound;
	double session_size;	
	double bound_increment_stepsize;
	double session_count;

	Setting(double given_recommended_delay_bound, double given_maximum_allowed_delay_bound, double given_session_size, double given_bound_increment_stepsize, double given_session_count)
	{
		recommended_delay_bound = given_recommended_delay_bound;
		maximum_allowed_delay_bound = given_maximum_allowed_delay_bound;
		session_size = given_session_size;		
		bound_increment_stepsize = given_bound_increment_stepsize;
		session_count = given_session_count;
	}

	Setting(){}
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

class Simulation
{
public:
	Simulation(Setting given_sim_setting)
	{
		global.sim_setting = given_sim_setting;
	}	
	
	string data_directory;	
	string client_dc_latency_file;	
	string output_directory;
	bool cluster_by_subregion;
	void Initialize();

	string algorithm_to_run; // specify the algorithm to evaluate before calling Run()
	bool output_assignment; // whether to output assignment details for each session	
	void Run();

	void CheckInterDatacenterLink();

private:
	void Alg_CP(const size_t, const bool);
	void Alg_NA_all();
	void Alg_NA_sub();
	
	/*CP utility functions*/
	bool EnforceLocalConsistency(vector<Client>&);
	bool EnforceNodeConsistency(vector<Client>&);
	bool AC3Algorithm(vector<Client>&);
	bool ArcReduce(Client&, const Client&);
	
	/*CP algorithms*/
	bool IsAllowedByBackwardChecking(const vector<Client>&, const size_t, const vector<ID>&, const ID, const size_t);
	bool IsAllowedByForwardChecking(const vector<Client>&, const size_t, const vector<ID>&, const ID, const size_t);
	void AssignClient(vector<Client>&, const size_t, vector<ID>&, const size_t);
	void CP(vector<Client>&, vector<ID>&, const size_t);
	bool IsWorthy(const vector<Client>&, const size_t, vector<ID>&, const ID, const double);
	void AssignClient_optimal(vector<Client>&, const size_t, vector<ID>&, const size_t, double&, vector<ID>&);
	void CP_optimal(vector<Client>&, vector<ID>&, const size_t);
	double IsAllowed_false_counter;
	double IsWorthy_false_counter;

	/*alternative algorithms*/
	bool IsValidAssignment(const vector<Client>&, const vector<ID>&);
	void NA_all(vector<Client>&, vector<ID>&);
	void NA_sub(vector<Client>&, vector<ID>&);
	void NA_sub_optimal(vector<Client>&, vector<ID>&);

	/*some utility functions*/
	double CalculatePathLength(Path&);
	void GenerateOneSession(vector<ID>&);
	void GenerateOneSessionWithTwoRegion(vector<ID>&);
	void PrintGlobalInfo();
	void PrintClientDomain(const Client&);	
	void FindNearestDC4Client(Client&, const vector<ID>&);	
	void FindShortestPaths(vector<Client>&, const vector<ID>&);
	void FindAllAndShortestPaths(vector<Client>&, const vector<ID>&);
	double CalculateSessionCost(const vector<Client>&, const vector<ID>&);
	void OutputPerformanceData();
	void OutputAssignmentOfOneSession(const int, const vector<Client>&);
	void ResetPerformanceDataStorage();

	/*the following are global stuff that will not be modified once intitialized*/
	Global global;
	vector<vector<ID>> all_sessions;
	vector<vector<ID>> all_dc_subsets;

	/*the following are performance metrics*/
	double achieved_delay_bound;
	double data_transfer_cost;	
	double interDC_cost_ratio;
	double num_of_chosen_DCs;
	double num_evaluated_solutions;
	double alg_running_time;
	vector<double> achieved_delay_bound_all_sessions;
	vector<double> data_transfer_cost_all_sessions;
	vector<double> interDC_cost_ratio_all_sessions;
	vector<double> num_of_chosen_DCs_all_sessions;
	vector<double> num_evaluated_solutions_all_sessions;
	vector<double> alg_running_time_all_sessions;	
};