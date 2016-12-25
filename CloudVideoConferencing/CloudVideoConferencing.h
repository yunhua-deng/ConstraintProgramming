#pragma once

#include "Base.h"

namespace CloudVideoConferencingProblem
{
	using ID = size_t;
	
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
		ID cheapest_dc;

		list<ID> dc_domain; // the list of possible assignments, using list (rather than vector) to support efficient removal of elements
	};

	bool operator==(const Client& c_x, const Client& c_y);
	bool operator!=(const Client& c_x, const Client& c_y);
	bool operator<(const Client& c_x, const Client& c_y);
	bool ClientComparator_ByDomainSize(const Client& c_x, const Client& c_y);

	struct Datacenter
	{
		ID id;
		string name;
		string provider;
		double server_rental_price;
		double external_bandwidth_price;
		double internal_bandwidth_price;
	};
	bool DatacenterComparator_ByExternalBandwidthPrice(const Datacenter& d_x, const Datacenter& d_y);

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
	bool operator<(const Path& p_x, const Path& p_y);
	bool operator==(const Path& p_x, const Path& p_y);
	void PrintOutPath(const Path& p);
	Path GetInversePath(const Path & p);

	struct Setting
	{		
		size_t session_size;
		size_t session_count;
		bool region_control;

		Setting(const size_t given_session_size, const size_t given_session_count, const bool given_region_control)
		{
			session_size = given_session_size;
			session_count = given_session_count;
			region_control = given_region_control;
		}

		Setting() {}
	};

	struct Global
	{
		string data_directory = ".\\Data\\";
		string output_directory = data_directory + "Output\\";	

		double dc_to_dc_latency_discount = 0;
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

	struct Solution
	{		
		Path longest_path;
		double latency; // the bound of the interaction latency of between any client pair
		double cost; // the total traffic cost
		int cardinality; // the number of selected datacenters
		vector<int> assignedDc_ranking;
	};

	struct Result
	{
		map<string, vector<double>> time_total_result;
		map<string, vector<double>> time_proportion_result; // the proportion of time spent on optimizing the latency
		map<string, vector<double>> latency_result;
		map<string, vector<double>> cost_result;
		map<string, vector<int>> cardinality_result;
		map<string, vector<int>> assignedDc_ranking_result;
		map<string, map<pair<string, string>, int>> farthestClientPair_dist_result;

		Result(const vector<string> algName_list)
		{
			for (string algName : algName_list)
			{
				time_total_result.insert({ algName, vector<double>() });
				time_proportion_result.insert({ algName, vector<double>() });
				latency_result.insert({ algName, vector<double>() });
				cost_result.insert({ algName, vector<double>() });
				cardinality_result.insert({ algName, vector<int>() });
				assignedDc_ranking_result.insert({ algName, vector<int>() });
				farthestClientPair_dist_result.insert({ algName, map<pair<string, string>, int>() });
			}
		}
	};

	struct Constraint
	{			
		size_t proximity_constraint; // if top_k is 0, then all dc's are put into each client's domain, otherwise, only consider top_k dc's (k-nearest dc's)
		size_t cardinality_constraint; // i.e., the allowed maximum number of datacenters appearing in the solution (if it is 0, this constraint is ignored)

		Constraint(const size_t proximity_constraint_in, const size_t cardinality_constraint_in)
		{
			proximity_constraint = proximity_constraint_in;
			cardinality_constraint = cardinality_constraint_in;
		}

		Constraint() {}
	};

	class SimulationBase
	{	
	protected:
		void Initialize();
		bool isInitialized = false;
		Global global;

		void RankDatacenters4Client(Client&, const vector<ID>&);
		int GetAssignedDcRanking(const Client&, const ID);		
						
		double GetPathLength(const Path&);
		Path GetShortestPathOfClientPair(const Client &, const Client &, const vector<ID> &);				

		double GetSessionLatencyLowerBound(const vector<Client> &);		
		double GetSessionCostLowerBound(const vector<Client> &);

		Path GetSessionLongestPathAfterAssignment(const vector<Client> &, const vector<ID> &);				
		double GetSessionCostAfterAssignment(const vector<Client> &, const vector<ID> &);
				
		Solution GetSessionSolutionInfoAfterAssignment(const vector<Client> &, const vector<ID> &);

		vector<ID> GenerateOneRandomSessionNoRegionControl(const size_t);
		vector<ID> GenerateOneRandomSessionWithRegionControl(const size_t);
		vector<ID> GenerateOneRandomSessionWithRegionControlTwoRegion(const size_t);
		vector<vector<Client>> GenerateRandomSessions(const Setting &);

		/*CP stuff*/		
		bool EnforceNodeConsistency(vector<Client> &);		
		bool EnforceArcConsistency(vector<Client> &, const size_t k = 0);
		bool ArcReduce(Client &, const Client &);
		bool EnforceLocalConsistency(vector<Client> &);	// including node (unary) and arc (binary)
		void InitializeDomains4Clients(vector<Client> &);
		void FindCheapestDcInDomain4Clients(vector<Client> &);		
		void AssignClient(const vector<Client> &, const size_t, vector<ID> &, const bool first_solution_only);
		void AssignClient_FC(vector<Client> &, const size_t, vector<ID> &);
		void AssignClient_LA(vector<Client> &, const size_t, vector<ID> &);
		bool BackwardChecking(const vector<Client> &, const size_t, const vector<ID> &);
		bool ForwardChecking(vector<Client> &, const size_t, const vector<ID> &);
		bool LookAhead(vector<Client> &, const size_t, const vector<ID> &);
		
		/*branch and bound*/
		bool ViolateCardinalityConstraint(const size_t k, const vector<ID>&);
		bool CannotImproveCost(const vector<Client> &, const size_t, const vector<ID> &);

		double end_to_end_delay_constraint; // i.e., the allowed maximum one-way latency between any client pair
		Constraint additional_contraints;
		size_t num_discovered_solutions = 0; // always remember to reset it to 0
		double optimal_cost;
		vector<ID> optimal_assignment;
		clock_t starting_time;
		double time_latency_stage = 0;
		double time_cost_stage = 0;
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
		void CP(vector<Client> &, const double);		
	};

	class OptimizingLatencyFirst : public SimulationBase
	{
	public:
		void Simulate(const Setting &);
		string local_output_directory = "OptimizingLatencyFirst\\";

	private:
		Solution CP(vector<Client>, const Constraint & input_additional_contraints = Constraint(0, 0));
	};

	void RunSimulation_OptimizingLatencyFirst(const Setting &);
}