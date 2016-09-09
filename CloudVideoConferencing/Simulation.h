#ifndef SIMULATION_H_INCLUDED
#define SIMULATION_H_INCLUDED
#endif
#pragma once

#include "Base.h"

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
	bool cluster_by_subregion = false;
	bool output_assignment = false;
	void Initialize();
	string alg_to_run;
	void Run();
	void Run_MinCost(const string);

	void CheckInterDatacenterLink();

private:
	
	/*CP utility functions*/
	bool EnforceLocalConsistency(vector<Client>&);
	bool EnforceNodeConsistency(vector<Client>&);
	bool AC3Algorithm(vector<Client>&);
	bool ArcReduce(Client&, const Client&);
	bool IsConsistentWithPreviousAssignments(const vector<Client>&, const size_t, const vector<ID>&, const ID, const size_t);
	bool IsWorthy(const vector<Client>&, const size_t, vector<ID>&, const ID, const double); // bounding
	double IsConsistentWithPreviousAssignments_false_counter;
	double IsWorthy_false_counter;
	
	/*CP algorithms*/	
	void AssignClient(vector<Client>&, const size_t, vector<ID>&, const size_t);
	void CP(vector<Client>&, vector<ID>&, const size_t);	
	void AssignClient_MinCost(vector<Client>&, const size_t, vector<ID>&, const size_t, double&, vector<ID>&);
	void CP_MinCost(vector<Client>&, vector<ID>&, const size_t);	

	/*other algorithms*/	
	void NA_all(vector<Client>&, vector<ID>&, const size_t);
	void NA_all_MinCost(vector<Client>&, vector<ID>&, const size_t);
	void NA_sub(vector<Client>&, vector<ID>&);
	void NA_sub_MinCost(vector<Client>&, vector<ID>&);
	void Random(vector<ID>&);
	
	/*some utility functions*/	
	double GetAssignmentDelay(const vector<Client>&, const vector<ID>&);
	double CalculatePathLength(const Path&);
	void GenerateOneSession(vector<ID>&);
	void GenerateOneSessionWithTwoRegion(vector<ID>&);
	void PrintGlobalInfo();
	void PrintClientDomain(const Client&);
	void FindNearestDCs4Client(Client&, const vector<ID>&);
	void FindShortestPaths(vector<Client>&, const vector<ID>&);
	void FindAllAndShortestPaths(vector<Client>&, const vector<ID>&);
	double CalculateAssignmentCost(const vector<Client>&, const vector<ID>&);
	void OutputPerformanceData();
	void OutputAssignmentOfOneSession(const int, const vector<Client>&);
	void ResetPerformanceDataStorage();

	/*the following are global stuff*/
	Global global;
	vector<vector<ID>> all_sessions;
	vector<vector<ID>> all_dc_subsets;

	/*the following are performance metrics*/
	double achieved_delay_bound; // primary objective
	double data_transfer_cost; // secondary objectve
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