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
		bool region_control;
		size_t session_count;

		Setting(const size_t given_session_size, const bool given_region_control = false, const size_t given_session_count = 1000)
		{
			session_size = given_session_size;			
			region_control = given_region_control;
			session_count = given_session_count;
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
		vector<int> proximity; // record all of the session		
		double ratioNearest; // the ratio of clients that are assigned to their (gloablly) nearest datacenters
		vector<int> proximityLocal; // with respect to the selected datacenters only
		double ratioNearestLocal; // the ratio of clients that are assigned to their locally nearest datacenters
	};

	struct Result
	{
		map<string, vector<double>> time_result;
		map<string, vector<double>> latency_result;
		map<string, vector<double>> cost_result;
		map<string, vector<double>> cardinality_result;		
		map<string, vector<double>> proximity_result;
		map<string, vector<double>> ratioNearest_result;
		map<string, vector<double>> proximityLocal_result;
		map<string, vector<double>> ratioNearestLocal_result;
		map<string, map<pair<string, string>, int>> farthestClientPair_dist_result;

		Result(const vector<string> algName_list)
		{
			for (string algName : algName_list)
			{
				time_result.insert({ algName, vector<double>() });
				latency_result.insert({ algName, vector<double>() });
				cost_result.insert({ algName, vector<double>() });
				cardinality_result.insert({ algName, vector<double>() });
				proximity_result.insert({ algName, vector<double>() });
				ratioNearest_result.insert({ algName, vector<double>() });
				proximityLocal_result.insert({ algName, vector<double>() });
				ratioNearestLocal_result.insert({ algName, vector<double>() });
				farthestClientPair_dist_result.insert({ algName, map<pair<string, string>, int>() });
			}
		}
	};

	class SimulationBase
	{	
	protected:
		void Initialize();
		bool isInitialized = false;
		Global global;		
		void RankDcByProximity4Client(Client&, const vector<ID>&);
		int GetAssignedDcProximity(const Client&, const ID);
		int GetAssignedDcProximityLocal(const Client&, const ID, const set<ID> &);
						
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
		void FindCheapestDcInDomain4Clients(vector<Client> &); // used together with AssignClient_FindAllSolutions()		
		void AssignClient(const vector<Client> &, const size_t, vector<ID> &);
		void AssignClient_FindAllSolutions(const vector<Client> &, const size_t, vector<ID> &);
		void AssignClient_FC(vector<Client> &, const size_t, vector<ID> &);
		void AssignClient_LA(vector<Client> &, const size_t, vector<ID> &);
		bool BackwardChecking(const vector<Client> &, const size_t, const vector<ID> &);
		bool ForwardChecking(vector<Client> &, const size_t, const vector<ID> &);
		bool LookAhead(vector<Client> &, const size_t, const vector<ID> &);
		
		/*branch and bound*/
		bool ViolateCardinalityConstraint(const size_t k, const vector<ID>&);
		bool CannotImproveCost(const vector<Client> &, const size_t, const vector<ID> &); // require FindCheapestDcInDomain4Clients()

		double latency_constraint = 0; // the allowed maximum one-way latency between any client pair
		size_t proximity_constraint = 0; // if top_k is 0, then all dc's are put into each client's domain, otherwise, only consider top_k dc's (k-nearest dc's)
		size_t cardinality_constraint = 0; // the allowed maximum number of datacenters appearing in the solution (if it is 0, this constraint is ignored)

		const double latency_constraint_delta = 1;

		size_t num_discovered_solutions = 0; // always remember to reset it to 0
		double optimal_cost;
		vector<ID> optimal_assignment;
	};

	class DatasetAnalysis : public SimulationBase
	{
	public:
		void Get_ClientCluster_Info();
		void Check_InterDcNetwork_Advantage();
		void Get_DelayToNearestDc_CDF();
		void Get_ShortestPathLength_CDF();
	};

	class MultilevelOptimization : public SimulationBase
	{
	public:
		void Simulate(const Setting &);
		string local_output_directory = "MultilevelOptimization\\";

	private:
		Solution CP(vector<Client>);
		Solution CP_Card(vector<Client>, const size_t given_cardinality_constraint = 0);
		Solution CP_Prox(vector<Client>, const size_t given_proximity_constraint = 0); // if using the default parameter, it is equivalent to CP() because InitializeDomains4Clients() orders the values for each variable according to proximity
	};

	void RunSimulation_MultilevelOptimization(const Setting &);
}