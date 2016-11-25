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
		double outgoing_data_amount = 0;
		double incoming_data_amount = 0;
		ID nearest_dc;
		vector<ID> ranked_dc_list;
		string region;
		string subregion;
		
		list<ID> dc_domain; // i.e., the list of assignment options, using list (rather than vector) for deletion of elements
		ID cheapest_dc;		

		//map<ID, vector<Path>> path_to_client; // memory-intensive (not applicable to large input size)
		//map<ID, Path> shortest_path_to_client;
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
		size_t session_size;
		size_t session_count;

		Setting(size_t given_session_size, size_t given_session_count)
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
	};

	class SimulationBase
	{	
	protected:
		void Initialize();
		bool isInitialized = false;
		Global global;

		void RankDatacenters4Client(Client&, const vector<ID>&);
		int GetAssignedDcRanking(const Client&, const ID);
				
		//void FindShortestPaths(vector<Client>&, const vector<ID>&); /*only applicable to small client_list input*/
		//void FindAllPaths(vector<Client>&, const vector<ID>&); /*only applicable to small client_list input*/
						
		double GetPathLength(const Path&);
		double GetShortestPathLengthOfClientPair(const Client &, const Client &, const vector<ID> &);

		double GetSessionLatencyLowerBound(const vector<Client> &);
		double GetSessionCostLowerBound(const vector<Client> &);

		double GetSessionLatencyAfterAssignment(const vector<Client>&, const vector<ID>&);
		double GetSessionCostAfterAssignment(const vector<Client>&, const vector<ID>&);
				
		vector<ID> GenerateOneSession(const size_t);
		vector<ID> GenerateOneSessionWithTwoRegion(const size_t);
		vector<vector<Client>> GenerateRandomSessions(const Setting &);

		/*constraint programming stuff*/		
		bool EnforceNodeConsistency(vector<Client> &);
		bool EnforceArcConsistency(vector<Client> &);
		bool ArcReduce(Client &, const Client &);
		bool EnforceLocalConsistency(vector<Client> &);	
		void AssignClient(const vector<Client> &, const size_t, vector<ID> &, const bool first_solution_only);

		/*utility stuff for backtracking search*/
		void InitializeDomains4Clients(vector<Client> &, const size_t top_k = 0); // if top_k is 0, then all dc's are put into each client's domain, otherwise, only consider top_k dc's (k-nearest dc's)
		void FindCheapestDcInDomain4Clients(vector<Client> &);
		bool IsValidPartialSolution(const vector<Client> &, const size_t, const vector<ID> &, const ID);
		bool IsWorthExtension(const vector<Client> &, const size_t, const vector<ID> &, const ID, const double);
		double path_length_constraint; // i.e., the allowed maximum one-way latency between any client pair (also it is the latency objective)
		size_t solution_cardinality_constraint = 0; // i.e., the allowed maximum number of datacenters appearing in the solution (if it is 0, this constraint is ignored)		
		size_t num_discovered_solutions = 0; // always remember to reset it to 0
		double optimal_cost;
		vector<ID> optimal_solution;
	};

	class DatasetAnalysis : public SimulationBase
	{
	public:
		void Get_ClientCluster_Info();
		void Check_InterDcNetwork_Advantage();
		void Get_DelayToNearestDc_CDF();
		void Get_ShortestPathLength_CDF();
	};

	class OptimizingCostByTradingOffLatency : public SimulationBase
	{
	public:		
		void Simulate(const Setting &);
		string local_output_directory = "OptimizingCostByTradingOffLatency\\";

	private:
		
		/*CP*/
		void CP(vector<Client> &, const double);		
	};

	class OptimizingLatencyFirst : public SimulationBase
	{
	public:
		void Simulate(const Setting &);
		string local_output_directory = "OptimizingLatencyFirst\\";

	private:

		/*CP*/
		void CP(vector<Client> &);

		/*NA*/
		vector<ID> NearestAssignment(vector<Client>);
	};
}