#pragma once

#include "Base.h"

namespace CloudVideoConferencingProblem
{
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
		ID nearest_dc;
		vector<ID> ranked_dc_list;
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
		string data_directory = ".\\Data\\";
		string output_directory = data_directory + "Output\\";

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
		vector<vector<ID>> all_dc_subsets;
		map<ID, Client> client;
		map<string, vector<ID>> client_cluster;
		vector<vector<Client>> all_sessions;
	};

	class SimulationBase
	{
	public:
		SimulationBase(Setting given_sim_setting) {	global.sim_setting = given_sim_setting;	}		
		void Initialize();
		void Check_InterDcNetwork_Advantage();
		void Get_DelayToNearestDc_CDF();

	protected:		
		Global global;		
		
		void RankDatacenters4Client(Client&, const vector<ID>&);

		void FindShortestPaths(vector<Client>&, const vector<ID>&);
		void FindAllPaths(vector<Client>&, const vector<ID>&);
		
		void GenerateOneSession(vector<ID>&);
		void GenerateOneSessionWithTwoRegion(vector<ID>&);
		
		double GetPathLength(const Path&);		

		double GetSessionDelayLowerBound(const vector<Client> &);
		double GetSessionDelayUpperBound(const vector<Client> &);
		double GetSessionCostLowerBound(const vector<Client> &);

		double GetSessionDelayAfterAssignment(const vector<Client>&, const vector<ID>&);
		double GetSessionCostAfterAssignment(const vector<Client>&, const vector<ID>&);
				
		void PrintGlobalInfo();
		void PrintClientDomain(const Client &);
				
		double path_length_constraint; // i.e., the maximum allowed one-way latency between any client pair		
		void InitializeDomains4Clients(vector<Client> &, size_t top_k = 0); // if top_k == 0, then all dc's are put into each client's domain, otherwise, only consider top_k dc's (k-nearest dc's)
		double num_discovered_solutions;

		/*constraint propagation*/
		bool EnforceLocalConsistency(vector<Client> &);
		bool EnforceNodeConsistency(vector<Client> &);
		bool EnforceArcConsistency(vector<Client> &);
		bool ArcReduce(Client &, const Client &);

		/*backtracking search (delay) and optimization (cost)*/
		bool IsValidPartialSolution(const vector<Client> &, const size_t, const vector<ID> &, const ID, const size_t);
		bool IsWorthyExtension(const vector<Client> &, const size_t, vector<ID> &, const ID, const double);
		double IsValidPartialSolution_false_counter;
		double IsWorthyExtension_false_counter;
		void AssignClient_ConstraintSatisfaction4Delay(vector<Client>&, const size_t, vector<ID>&, const size_t);
		void ConstraintSatisfaction4Delay(vector<Client>&, vector<ID>&, const size_t);
		void AssignClient_ConstrainedOptimization4Cost(vector<Client>&, const size_t, vector<ID>&, const size_t, double&, vector<ID>&);
		void ConstrainedOptimization4Cost(vector<Client>&, vector<ID>&, const size_t);
	};

	class OptimizationProblem : public SimulationBase
	{
	public:				
		bool output_assignment = false;		
		string alg_to_run;
		void Run();
		void Run_CostOptimization(const string);

	private:
				
		/*other algorithms*/		
		void NA_all(vector<Client>&, vector<ID>&, const size_t);
		void NA_all_CostOptimization(vector<Client>&, vector<ID>&, const size_t);
		void NA_sub(vector<Client>&, vector<ID>&);
		void NA_sub_CostOptimization(vector<Client>&, vector<ID>&);
		//void Random(vector<ID>&);
		
		/*some utility functions*/		
		void OutputPerformanceData();
		void OutputAssignmentOfOneSession(const int, const vector<Client>&);
		void ResetPerformanceDataStorage();

		/*the following are performance metrics*/		
		double traffic_cost;
		double interDC_cost_ratio;
		double num_of_chosen_DCs;
		double alg_running_time;
		vector<double> achieved_delay_bound_all_sessions;
		vector<double> traffic_cost_all_sessions;
		vector<double> interDC_cost_ratio_all_sessions;
		vector<double> num_of_chosen_DCs_all_sessions;
		vector<double> num_discovered_solutions_all_sessions;
		vector<double> alg_running_time_all_sessions;
	};
}