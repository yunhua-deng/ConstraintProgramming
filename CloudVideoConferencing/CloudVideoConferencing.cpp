#include "CloudVideoConferencing.h"

namespace CloudVideoConferencingProblem
{
	bool operator<(const Path& p_x, const Path& p_y)
	{
		if (p_x.sender < p_y.sender) return true;
		else if (p_x.sender == p_y.sender)
		{
			if (p_x.receiver < p_y.receiver) return true;
			else if (p_x.receiver == p_y.receiver)
			{
				if (p_x.dc_sender < p_y.dc_sender) return true;
				else if (p_x.dc_sender == p_y.dc_sender) return p_x.dc_receiver < p_y.dc_receiver;
				else return false;
			}
			else return false;
		}
		else return false;
	};

	bool operator==(const Path& p_x, const Path& p_y)
	{
		return (p_x.sender == p_y.sender
			&& p_x.dc_sender == p_y.dc_sender
			&& p_x.dc_receiver == p_y.dc_receiver
			&& p_x.receiver == p_y.receiver);
	};

	void PrintOutPath(const Path& p)
	{
		std::cout << p.sender << "->" << p.dc_sender << "->" << p.dc_receiver << "->" << p.receiver << "\n";
	}

	bool operator==(const Client& c_x, const Client& c_y)
	{
		return (c_x.id == c_y.id);
	};

	bool operator!=(const Client& c_x, const Client& c_y)
	{
		return (c_x.id != c_y.id);
	}

	bool operator<(const Client& c_x, const Client& c_y)
	{
		return (c_x.id < c_y.id);
	}

	bool ClientComparator_ByDomainSize(const Client& c_x, const Client& c_y)
	{
		return (c_x.dc_domain.size() < c_y.dc_domain.size());
	}

	bool DatacenterComparator_ByExternalBandwidthPrice(const Datacenter& d_x, const Datacenter& d_y)
	{
		return (d_x.external_bandwidth_price < d_y.external_bandwidth_price);
	}

	/*reading data from files, initializing global stuff*/
	void SimulationBase::Initialize()
	{
		/*make it empty*/		
		global = Global();
		
		/*client_to_dc_delay_table, client_name_list*/
		auto strings_read = ReadDelimitedTextFileIntoVector(global.data_directory + "ping_to_prefix_median_matrix.csv", ',', true);
		for (auto row : strings_read)
		{
			global.client_name_list.push_back(row.at(0));
			vector<double> client_to_dc_delay_table_row;
			for (auto col = 1; col < row.size(); col++)
			{
				client_to_dc_delay_table_row.push_back(stod(row.at(col)) / 2); // one-way delay = rtt / 2
			}
			global.client_to_dc_delay_table.push_back(client_to_dc_delay_table_row);
		}

		/*dc_to_dc_delay_table, dc_name_list*/
		strings_read = ReadDelimitedTextFileIntoVector(global.data_directory + "ping_to_dc_median_matrix.csv", ',', true);
		for (auto row : strings_read)
		{
			global.dc_name_list.push_back(row.at(0));
			vector<double> dc_to_dc_delay_table_row;
			for (auto col = 1; col < row.size(); col++)
			{
				dc_to_dc_delay_table_row.push_back(stod(row.at(col)) / 2); // one-way delay = rtt / 2
			}
			global.dc_to_dc_delay_table.push_back(dc_to_dc_delay_table_row);
		}

		/*dc_internal_bandwidth_price_list, dc_external_bandwidth_price_list, dc_server_rental_price_list*/
		strings_read = ReadDelimitedTextFileIntoVector(global.data_directory + "pricing_bandwidth_server.csv", ',', true);
		for (auto row : strings_read)
		{
			global.dc_internal_bandwidth_price_list.push_back(stod(row.at(1)));
			global.dc_external_bandwidth_price_list.push_back(stod(row.at(2)));
			global.dc_server_rental_price_list.push_back(stod(row.at(3)));
		}

		/*create datacenter*/
		const auto total_dc_count = global.dc_name_list.size();
		for (size_t i = 0; i < total_dc_count; i++)
		{
			Datacenter d;
			d.id = ID(i);
			global.dc_id_list.push_back(d.id);
			d.name = global.dc_name_list.at(i);
			d.provider = d.name.substr(0, d.name.find_first_of("-")); // e.g. "ec2"
			d.server_rental_price = global.dc_server_rental_price_list.at(i);
			d.external_bandwidth_price = global.dc_external_bandwidth_price_list.at(i);
			d.internal_bandwidth_price = global.dc_internal_bandwidth_price_list.at(i);

			global.datacenter[d.id] = d;
		}

		/*create client and client clusters*/
		const auto total_client_count = global.client_name_list.size();
		for (size_t i = 0; i < total_client_count; i++)
		{
			Client c;
			c.id = (ID)i;
			global.client_id_list.push_back(c.id);
			c.name = global.client_name_list.at(i); // client domain name			
						
			/*important*/
			RankDatacenters4Client(c, global.dc_id_list);

			/*get subregion (e.g. "ec2-ap-northeast-1")*/
			c.subregion = global.datacenter.at(c.nearest_dc).name;

			/*get region (e.g. "ap")*/
			auto pos = global.datacenter.at(c.nearest_dc).name.find_first_of("-");
			c.region = global.datacenter.at(c.nearest_dc).name.substr(pos + 1, 2); // e.g. extract "ap" from "ec2-ap-northeast-1"
			
			/*record this client*/
			global.client[c.id] = c;

			/*cluster this client subject to the clustering criterion to avoid including some isolated remote clients into any session
			(i.e., distance to its nearest_dc must be < 50, because over 90% clients are within 50ms to their nearest datacenters)*/
			if (global.client_to_dc_delay_table.at(c.id).at(c.nearest_dc) <= 50)
			{
				global.client_cluster[c.region].push_back(c.id);
			}
		}		

		/*generate all non-empty dc combinations (i.e., the collection of subsets of the full dc set)*/
		vector<bool> x;
		x.assign(global.dc_id_list.size(), false);
		GenerateAllSubsets(global.dc_id_list, x, 0, global.all_dc_subsets);
		std::sort(global.all_dc_subsets.begin(), global.all_dc_subsets.end(), SubsetComparatorBySize);	

		isInitialized = true;
	}

	void DatasetAnalysis::Get_ClientCluster_Info()
	{
		if (!isInitialized) { Initialize(); }
		
		// dump client cluster to disk	
		_mkdir(global.output_directory.c_str()); // because _mkdir() only accepts const char*		
		ofstream cluster_file(global.output_directory + "Client_Clustering.txt", ios::binary);
		string buffer = "";
		for (auto & it : global.client_cluster)
		{
			buffer += it.first + ",";
			for (auto & c : it.second)
			{
				buffer += global.client.at(c).name + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		cluster_file << buffer;
		cluster_file.close();
	}

	/*check whether inter-dc links have shorter latencies than other links*/
	void DatasetAnalysis::Check_InterDcNetwork_Advantage()
	{	
		if (!isInitialized) { Initialize(); }
		
		map<string, vector<ID>> dc_group;
		for (auto& it : global.datacenter)
		{
			dc_group["all"].push_back(it.first); // all
			dc_group[it.second.provider].push_back(it.first); // azure or ec2
		}

		vector<Client> all_clients;
		for (auto& it : global.client)
		{
			all_clients.push_back(it.second);
		}
		double num_all_client_pairs = (double)all_clients.size() * (all_clients.size() - 1);

		std::cout << "\ncheck the ratio of client pairs that favor inter-dc links\n";

		/*based on nearest datacenter*/
		std::cout << "| checking based on nearest datacenter\n";
		for (auto& it : dc_group)
		{
			/*find nearest datacenters for clients of this dc_group*/
			for (auto& c : all_clients)	RankDatacenters4Client(c, it.second);

			/*examine*/
			double num_distinct_client_pairs = 0;
			double num_pairs_favor_interDC = 0;
			for (auto& c_x : all_clients)
			{
				for (auto& c_y : all_clients)
				{
					if (c_x != c_y)
					{
						if (c_x.nearest_dc != c_y.nearest_dc)
						{
							num_distinct_client_pairs++;
							double len_a = GetPathLength(Path(c_x.id, c_x.nearest_dc, c_y.nearest_dc, c_y.id));
							double len_b = GetPathLength(Path(c_x.id, c_x.nearest_dc, c_x.nearest_dc, c_y.id));
							double len_c = GetPathLength(Path(c_x.id, c_y.nearest_dc, c_y.nearest_dc, c_y.id));
							if ((len_a < len_b) && (len_a < len_c))
								num_pairs_favor_interDC++;
						}
					}
				}
			}
			std::cout << "| | " << it.first << "\n";
			std::cout << "| | | " << "ratio_distinct_pairs:\t" << num_distinct_client_pairs << "/" << num_all_client_pairs <<
				" = " << num_distinct_client_pairs / num_all_client_pairs << "\n";
			std::cout << "| | | | " << "ratio_prefer_interDC:\t" << num_pairs_favor_interDC << "/" << num_distinct_client_pairs <<
				" = " << num_pairs_favor_interDC / num_distinct_client_pairs << "\n";
		}		
	}

	void DatasetAnalysis::Get_DelayToNearestDc_CDF()
	{		
		if (!isInitialized) { Initialize(); }
		
		string buffer = "";
		for (const auto & it : global.client)
		{
			buffer += std::to_string((int)global.client_to_dc_delay_table.at(it.second.id).at(it.second.nearest_dc));
			buffer += "\n";
		}
		ofstream data_file(global.output_directory + "CDF_DelayToNearestDC.txt");
		data_file << buffer;
		data_file.close();
	}

	void DatasetAnalysis::Get_ShortestPathLength_CDF()
	{
		if (!isInitialized) { Initialize(); }

		/*entire dataset*/
		string buffer = "";
		for (auto i : global.client_id_list)
		{
			for (auto j : global.client_id_list)
			{
				if (j != i)
				{
					buffer += std::to_string((int)GetShortestPathLengthOfClientPair(global.client.at(i), global.client.at(j), global.dc_id_list));
					buffer += "\n";
				}
			}
		}
		ofstream data_file(global.output_directory + "CDF_ShortestPathLength.txt");
		data_file << buffer;
		data_file.close();

		/*select partial data (smaller file)*/
		/*auto copy_of_client_id_list = global.client_id_list;
		random_shuffle(copy_of_client_id_list.begin(), copy_of_client_id_list.end());
		vector<ID> partial_client_id_list;
		partial_client_id_list.assign(copy_of_client_id_list.begin(), copy_of_client_id_list.begin() + 1000);
		string buffer = "";
		for (auto i : partial_client_id_list)
		{
			for (auto j : partial_client_id_list)
			{
				if (j != i)
				{
					buffer += std::to_string((int)GetShortestPathLengthOfClientPair(global.client.at(i), global.client.at(j), global.dc_id_list));
					buffer += "\n";
				}
			}			
		}
		ofstream data_file(global.output_directory + "CDF_ShortestPathLength.txt");
		data_file << buffer;
		data_file.close();*/		
	}

	/*rank datacenters in ascending order of delay (also find the nearest_dc for each client)*/
	void SimulationBase::RankDatacenters4Client(Client& the_client, const vector<ID>& dc_id_list)
	{
		list<ID> dc_id_list_copy(dc_id_list.begin(), dc_id_list.end());
		the_client.ranked_dc_list.clear();
		while (!dc_id_list_copy.empty())
		{
			auto the_nearest_dc = dc_id_list_copy.front();
			for (const auto & it : dc_id_list_copy)
			{
				if (global.client_to_dc_delay_table.at(the_client.id).at(it) < global.client_to_dc_delay_table.at(the_client.id).at(the_nearest_dc))
				{
					the_nearest_dc = it;
				}
			}
			the_client.ranked_dc_list.push_back(the_nearest_dc);
			dc_id_list_copy.remove(the_nearest_dc);
		}
		the_client.nearest_dc = the_client.ranked_dc_list.front();
	}

	/* reference: http://stackoverflow.com/questions/15099707/how-to-get-position-of-a-certain-element-in-strings-vector-to-use-it-as-an-inde */
	int SimulationBase::GetAssignedDcRanking(const Client & the_client, const ID assignedDc)
	{
		int result = int(std::find(the_client.ranked_dc_list.begin(), the_client.ranked_dc_list.end(), assignedDc) - the_client.ranked_dc_list.begin());
		if (result >= the_client.ranked_dc_list.size()) // not found
		{			
			cout << "sth wrong happens in GetAssignedDcRanking()!\n";
			cin.get();
			return -1;
		}
		else 
		{ 
			return (result + 1); // we count 1 as the No.1 ranking, not 0
		}
	}

	//void SimulationBase::FindShortestPaths(vector<Client>& client_list, const vector<ID>& dc_id_list)
	//{
	//	for (auto& c_x : client_list)
	//	{			
	//		for (auto& c_y : client_list)
	//		{
	//			if (c_x != c_y)
	//			{
	//				c_x.shortest_path_to_client[c_y.id] = Path(c_x.id, dc_id_list.front(), dc_id_list.front(), c_y.id);
	//				for (auto& d_i : dc_id_list)
	//				{
	//					for (auto& d_j : dc_id_list)
	//					{
	//						auto one_path = Path(c_x.id, d_i, d_j, c_y.id);
	//						if (GetPathLength(one_path) < GetPathLength(c_x.shortest_path_to_client.at(c_y.id)))
	//						{
	//							c_x.shortest_path_to_client[c_y.id] = one_path;
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}
		
	//void SimulationBase::FindAllPaths(vector<Client>& client_list, const vector<ID>& dc_id_list)
	//{
	//	for (auto& c_x : client_list)
	//	{
	//		/*reset in case of non-empty*/
	//		c_x.path_to_client.clear();

	//		/*find every possible path and the shortest path to c_y*/
	//		for (auto& c_y : client_list)
	//		{
	//			if (c_x != c_y)
	//			{
	//				for (auto& d_i : dc_id_list)
	//				{
	//					for (auto& d_j : dc_id_list)
	//					{
	//						c_x.path_to_client[c_y.id].push_back(Path(c_x.id, d_i, d_j, c_y.id)); // find every possible path to c_y
	//					}
	//				}

	//				c_x.shortest_path_to_client[c_y.id] = c_x.path_to_client.at(c_y.id).front(); // initialize for finding the shorest path to c_y
	//				for (auto& p : c_x.path_to_client.at(c_y.id))
	//				{						
	//					if (GetPathLength(p) < GetPathLength(c_x.shortest_path_to_client.at(c_y.id)))
	//					{
	//						c_x.shortest_path_to_client.at(c_y.id) = p;
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	double SimulationBase::GetShortestPathLengthOfClientPair(const Client & c_x, const Client & c_y, const vector<ID> & dc_id_list)
	{
		double result;
		
		if (c_x == c_y) { result = 0; }
		else
		{
			result = GetPathLength(Path(c_x.id, dc_id_list.front(), dc_id_list.front(), c_y.id));
			for (auto d_i : dc_id_list)
			{
				for (auto d_j : dc_id_list)
				{
					auto result_temp = GetPathLength(Path(c_x.id, d_i, d_j, c_y.id));
					if (result_temp < result) {	result = result_temp; }
				}
			}
		}
		
		return result;
	}

	/*ensure the generated session contains clients from all regions*/
	vector<ID> SimulationBase::GenerateOneSession(const size_t session_size)
	{
		/*shuffle for creating randomness of each session*/
		for (auto& cluster : global.client_cluster)
		{
			random_shuffle(cluster.second.begin(), cluster.second.end());
		}

		/*making one random session with the number of clients from each cluster being as close as possible*/
		vector<ID> one_session;
		size_t remaining = session_size;
		size_t next_pos = 0;
		vector<vector<ID>> client_clusters; // because global.client_cluster cannot be shuffled, hence we need a vector to shuffle
		for (auto& cluster : global.client_cluster)	{ client_clusters.push_back(cluster.second); }
		std::random_shuffle(client_clusters.begin(), client_clusters.end()); // shuffle for each session
		while (remaining > 0)
		{
			for (auto& cluster : client_clusters)
			{
				if (next_pos <= (cluster.size() - 1))
				{
					one_session.push_back(cluster.at(next_pos));
					remaining--;
					if (remaining <= 0) break;
				}
				else
				{
					std::cout << "\nsome unhandled exception(s) when generating sessions!\n";
					cin.get();
				}
			}
			next_pos++;
		}

		/*sort the client id's in ascending order*/
		std::sort(one_session.begin(), one_session.end());

		return one_session;
	}

	/*to generate session that contains clients from two regions*/
	vector<ID> SimulationBase::GenerateOneSessionWithTwoRegion(const size_t session_size)
	{
		/*shuffle for creating randomness of each session*/
		vector<string> cluster_names;
		for (auto& cluster : global.client_cluster)
		{
			random_shuffle(cluster.second.begin(), cluster.second.end());
			cluster_names.push_back(cluster.first);
		}
		random_shuffle(cluster_names.begin(), cluster_names.end());
		if (cluster_names.size() < 2)
		{
			std::cout << "\ninsufficient number of client clusters!\n";
			cin.get();
		}
		vector<string> two_cluster_names(cluster_names.begin(), cluster_names.begin() + 2);

		/*making one random session with equal number of clients from each cluster*/
		vector<ID> one_session;
		size_t remaining = session_size;
		size_t next_pos = 0;
		while (remaining > 0)
		{
			for (auto& cluster : global.client_cluster)
			{
				/*filter out the cluster whose name is not included in the two_cluster_names*/
				if (std::find(two_cluster_names.begin(), two_cluster_names.end(), cluster.first) != two_cluster_names.end())
				{
					if (next_pos <= (cluster.second.size() - 1))
					{
						one_session.push_back(cluster.second.at(next_pos));
						remaining--;
						if (remaining <= 0) break;
					}
					else
					{
						std::cout << "\na unhandled exception when generating sessions!\n";
						cin.get();						
					}
				}
			}
			next_pos++;
		}

		/*align the client id's in ascending order*/
		std::sort(one_session.begin(), one_session.end());

		return one_session;
	}

	vector<vector<Client>> SimulationBase::GenerateRandomSessions(const Setting & sim_setting)
	{		
		/*set clients' bitrates (incoming and outgoing data amounts) according to sim_setting*/
		for (auto & i : global.client_id_list)
		{
			global.client.at(i).outgoing_data_amount = 1;
			global.client.at(i).incoming_data_amount = global.client.at(i).outgoing_data_amount * (sim_setting.session_size - 1);
		}

		/*generate random sessions according to sim_setting*/
		vector<vector<Client>> random_sessions;		
		srand(997);
		for (size_t i = 0; i < sim_setting.session_count; i++)
		{
			auto one_session = GenerateOneSession(sim_setting.session_size);
			vector<Client> session_clients;
			for (auto & i : one_session)
			{
				session_clients.push_back(global.client.at(i));
			}			
			random_sessions.push_back(session_clients);
		}		
		return random_sessions;
	}

	double SimulationBase::GetPathLength(const Path & path)
	{
		return (global.client_to_dc_delay_table.at(path.sender).at(path.dc_sender) 
			+ global.dc_to_dc_delay_table.at(path.dc_sender).at(path.dc_receiver) 
			+ global.client_to_dc_delay_table.at(path.receiver).at(path.dc_receiver));
	}

	/*by assuming that every client pair can use the shortest path between them*/
	double SimulationBase::GetSessionLatencyLowerBound(const vector<Client> & session_clients)
	{	
		double result = 0;
		for (auto & c_i : session_clients)
		{
			for (auto & c_j : session_clients)
			{
				if (c_j.id != c_i.id)
				{
					auto result_temp = GetShortestPathLengthOfClientPair(c_i, c_j, global.dc_id_list);
					if (result_temp > result)
					{
						result = result_temp;
					}
				}
			}
		}
		return result;
	}	

	double SimulationBase::GetSessionCostLowerBound(const vector<Client> & session_clients)
	{		
		// find cheapest_dc
		auto cheapest_dc = global.dc_id_list.front();
		for (auto it : global.dc_id_list)
		{
			if (global.datacenter.at(it).external_bandwidth_price < global.datacenter.at(cheapest_dc).external_bandwidth_price)
			{
				cheapest_dc = it;
			}
		}		
		
		// compute costLowerBound by assuming that every client is assigned to the cheapest_dc (no inter-dc traffic)
		double result = 0;
		for (auto & c : session_clients)
		{
			result += c.incoming_data_amount * global.datacenter.at(cheapest_dc).external_bandwidth_price;
		}
		return result;
	}

	double SimulationBase::GetSessionLatencyAfterAssignment(const vector<Client> & session_clients, const vector<ID> & session_assignment)
	{
		double result = 0;
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			for (size_t j = 0; j < session_clients.size(); j++)
			{
				if (j != i)
				{
					auto result_temp = GetPathLength(Path(session_clients.at(i).id, session_assignment.at(i), session_assignment.at(j), session_clients.at(j).id));
					if (result_temp > result) { result = result_temp; }
				}
			}
		}
		return result;
	}	

	double SimulationBase::GetSessionCostAfterAssignment(const vector<Client> & session_clients, const vector<ID> & session_assignment)
	{
		/*create some stuff to facilitate the cost calculation*/
		map<ID, vector<ID>> assigned_clients_to_dc;
		for (size_t i = 0; i < session_assignment.size(); i++)
		{
			assigned_clients_to_dc[session_assignment.at(i)].push_back(session_clients.at(i).id);
		}
		set<ID> chosen_dc_set(session_assignment.begin(), session_assignment.end());		

		/*data delivery cost*/
		double data_delivery_cost = 0;
		for (auto d : chosen_dc_set)
		{
			for (auto c : assigned_clients_to_dc.at(d))
			{
				data_delivery_cost += global.client.at(c).incoming_data_amount * global.datacenter.at(d).external_bandwidth_price;
			}
		}

		/*data relay cost (only if more than 1 dc's are chosen)*/
		double data_relay_cost = 0;
		if (chosen_dc_set.size() > 1)
		{
			for (auto sending_dc : chosen_dc_set)
			{
				for (auto receiving_dc : chosen_dc_set)
				{
					if (receiving_dc != sending_dc)
					{
						if (global.datacenter.at(receiving_dc).provider != global.datacenter.at(sending_dc).provider)
						{
							for (auto c : assigned_clients_to_dc.at(sending_dc))
							{
								data_relay_cost += global.client.at(c).outgoing_data_amount * global.datacenter.at(sending_dc).external_bandwidth_price;
							}
						}
						else
						{
							for (auto c : assigned_clients_to_dc.at(sending_dc))
							{
								data_relay_cost += global.client.at(c).outgoing_data_amount * global.datacenter.at(sending_dc).internal_bandwidth_price;
							}
						}
					}
				}
			}
		}

		/*return the sum*/
		return (data_relay_cost + data_delivery_cost);
	}	

	Solution SimulationBase::GetSolutionInfoAfterAssignment(const vector<Client> & session_clients, const vector<ID> & session_assignment)
	{
		auto solution = Solution();
		solution.latency = GetSessionLatencyAfterAssignment(session_clients, session_assignment);
		solution.cost = GetSessionCostAfterAssignment(session_clients, session_assignment);
		set<ID> assignedDc_set(session_assignment.begin(), session_assignment.end());
		solution.cardinality = (int)assignedDc_set.size();
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			solution.assignedDc_ranking.push_back(GetAssignedDcRanking(session_clients.at(i), session_assignment.at(i)));
		}
		return solution;
	}

	void SimulationBase::InitializeDomains4Clients(vector<Client> & session_clients)
	{
		if (0 == extraConstraintSet.client_domain_constraint)
		{
			for (auto & c : session_clients)
			{
				c.dc_domain.clear();
				for (auto d : c.ranked_dc_list)
				{
					c.dc_domain.push_back(d);
				}
			}
		}
		else
		{
			for (auto & c : session_clients)
			{				
				if (extraConstraintSet.client_domain_constraint > c.ranked_dc_list.size()) /* exception handling */
				{
					cout << "sth wrong in InitializeDomains4Clients()\n";
					cin.get();
					return;
				}
				c.dc_domain.clear();
				for (size_t i = 0; i < extraConstraintSet.client_domain_constraint; i++)
				{
					c.dc_domain.push_back(c.ranked_dc_list.at(i));
				}
			}
		}
	}

	void SimulationBase::FindCheapestDcInDomain4Clients(vector<Client> & session_clients)
	{
		for (auto & c : session_clients)
		{
			if (c.dc_domain.empty())
			{
				std::cout << "\nsome client has empty dc_domain when calling FindCheapestDcInDomain4Clients()\n";
				cin.get();
				return;
			}

			c.cheapest_dc = c.dc_domain.front();
			for (auto it : c.dc_domain)
			{
				if (global.datacenter.at(it).external_bandwidth_price < global.datacenter.at(c.cheapest_dc).external_bandwidth_price)
				{
					c.cheapest_dc = it;
				}
			}
		}
	}

	/*return true iff every client's dc_domain is non-empty after removing inconsistent values based on unary constraints*/
	bool SimulationBase::EnforceNodeConsistency(vector<Client> & session_clients)
	{
		for (auto & c : session_clients)
		{
			/*find bad values and remove them from the domain*/
			for (auto it = c.dc_domain.begin(); it != c.dc_domain.end();)
			{
				if (global.client_to_dc_delay_table.at(c.id).at(*it) > path_length_constraint) // unary constraint
				{
					it = c.dc_domain.erase(it); // remove this value ('it' will point to the value following the removed one)
				}
				else it++; // manually make 'it' point to the next value
			}

			/*return false if a client has an empty domain*/
			if (c.dc_domain.empty())
			{
				//std::cout << "EnforceNodeConsistency() returns false\n";
				return false;
			}
		}

		/*it is node-consistent*/
		//std::cout << "EnforceNodeConsistency() returns true\n";
		return true;
	}

	/*a untility function used by EnforceArcConsistency()*/
	bool SimulationBase::ArcReduce(Client& c_i, const Client& c_j)
	{
		bool domain_reduced = false; // check if domain(c_i) is reduced at the end

		/*find and remove bad values from domain(c_i)*/
		for (auto d_i = c_i.dc_domain.begin(); d_i != c_i.dc_domain.end();)
		{
			bool to_be_removed = true; // check if d_i should be removed		
			for (auto& d_j : c_j.dc_domain)
			{				
				if (GetPathLength(Path(c_i.id, *d_i, d_j, c_j.id)) <= path_length_constraint) // binary constraint
				{
					to_be_removed = false; // no need to remove d_i in c_i's domain because we can find a consistent value d_j in c_j's domain
					break;
				}
			}

			if (true == to_be_removed)
			{
				d_i = c_i.dc_domain.erase(d_i); // remove this value ('d_i' will point to the value following the removed one)
				domain_reduced = true;
			}
			else d_i++; // manually make d_i point to next value
		}

		return domain_reduced;
	}

	/*enforce arc consistency
	return true iff every client's dc_domain is non-empty after removing inconsistent values based on binary constraints
	a classic (mostly used) arc consistency enforcing algorithm: https://en.wikipedia.org/wiki/AC-3_algorithm */
	bool SimulationBase::EnforceArcConsistency(vector<Client>& session_clients)
	{
		/*'worklist' contains all arcs we wish to prove consistent or not.*/
		vector<pair<reference_wrapper<Client>, reference_wrapper<Client>>> worklist;
		for (auto& c_i : session_clients)
		{
			for (auto& c_j : session_clients)
			{
				if (c_j.id != c_i.id)
				{
					/*using reference_wrapper such that the item being pushed to the vector is not a copy but a reference*/
					worklist.push_back(pair<reference_wrapper<Client>, reference_wrapper<Client>>(c_i, c_j));
				}
			}
		}

		/*reduce domain values by checking the consistency of one arc at a time*/
		while (false == worklist.empty())
		{
			/*select one arc from worklist and remove it from the list*/
			auto arc = worklist.back();
			worklist.pop_back();

			/*work on this arc by applying ArcReduce()*/
			auto c_i = arc.first;
			auto c_j = arc.second;
			if (ArcReduce(c_i, c_j))
			{
				/*if domain(c_i) is empty, return false*/
				if (c_i.get().dc_domain.empty())
				{
					//std::cout << "EnforceArcConsistency() returns false\n";
					return false;
				}

				/*update worklist by adding every arc (c_k, c_i) (k != j and k != i) which is pointing to c_i*/
				for (auto& c_k : session_clients)
				{
					if ((c_k.id != c_j.get().id) && (c_k.id != c_i.get().id))
					{
						worklist.push_back(pair<reference_wrapper<Client>, reference_wrapper<Client>>(c_k, c_i));
					}
				}
			}
		}

		/*it can be arc-consistent*/
		//std::cout << "EnforceArcConsistency() returns true\n";
		return true;
	}

	/*a function that integrates EnforceNodeConsistency() and EnforceArcConsistency()*/
	bool SimulationBase::EnforceLocalConsistency(vector<Client>& session_clients)
	{
		if (EnforceNodeConsistency(session_clients))
		{
			if (EnforceArcConsistency(session_clients))
			{
				//std::cout << "EnforceLocalConsistency() returns true\n";
				return true;
			}
		}

		//std::cout << "EnforceLocalConsistency() returns false\n";
		return false;
	}

	/*a utility function to check if the assignment to client k is consistent with the previous assignments*/
	bool SimulationBase::IsValidPartialSolution(const vector<Client>& session_clients, const size_t k, const vector<ID>& session_assignment, const ID d_k)
	{
		/*check whether this assignment for client k will violate a constraint between k'th and one of the previous 0'th to (k - 1)'th clients*/
		for (size_t i = 0; i < k; i++)
		{			
			if (GetPathLength(Path(session_clients.at(k).id, d_k, session_assignment.at(i), session_clients.at(i).id)) > path_length_constraint ||
				GetPathLength(Path(session_clients.at(i).id, session_assignment.at(i), d_k, session_clients.at(k).id)) > path_length_constraint)
			{
				return false;
			}
		}

		/*consider the solution_cardinality_constraint only if it is != 0*/
		if (extraConstraintSet.solution_cardinality_constraint != 0)
		{
			set<ID> chosen_dc_subset(session_assignment.begin(), session_assignment.begin() + k); // k'th not included
			chosen_dc_subset.insert(d_k);
			if (chosen_dc_subset.size() > extraConstraintSet.solution_cardinality_constraint)
			{ 
				return false; 
			}
		}

		return true;
	}

	/*a utility employed by AssignClient_ConstrainedOptimization4Cost() for bounding*/
	bool SimulationBase::IsWorthExtension(const vector<Client>& session_clients, const size_t k, const vector<ID>& session_assignment, const ID d_k, const double optimal_cost)
	{
		/*of course this extension is worth because none has been found yet*/
		if (0 == num_discovered_solutions) { return true; }

		/*compute the cost lower bound by ignoring the relay traffic cost*/
		double cost_lower_bound = 0;
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			if (i < k) // previous 0'th to (k - 1)'th assigned clients
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(session_assignment.at(i)).external_bandwidth_price; // use session_assignment.at(i)
			}
			else if (k == i) // the k'th client
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(d_k).external_bandwidth_price; // use d_k
			}
			else // unassigned clients
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(session_clients.at(i).cheapest_dc).external_bandwidth_price; // use cheapest_dc
			}
		}
		return (cost_lower_bound < optimal_cost);
	}

	Solution SimulationBase::NearestAssignment(vector<Client> session_clients)
	{	
		vector<ID> session_assignment(session_clients.size());
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			session_assignment.at(i) = session_clients.at(i).nearest_dc;
		}
		return GetSolutionInfoAfterAssignment(session_clients, session_assignment);
	}

	Solution SimulationBase::SingleDatacenter(vector<Client> session_clients)
	{
		vector<ID> session_assignment(session_clients.size(), global.dc_id_list.front());
		for (const auto d : global.dc_id_list)
		{
			if (GetSessionLatencyAfterAssignment(session_clients, vector<ID>(session_clients.size(), d)) < GetSessionLatencyAfterAssignment(session_clients, session_assignment))
			{
				session_assignment = vector<ID>(session_clients.size(), d);
			}
		}		
		return GetSolutionInfoAfterAssignment(session_clients, session_assignment);
	}

	/*use first_solution_only to control whether it will terminate after finding the first solution*/
	void SimulationBase::AssignClient(const vector<Client> & session_clients, const size_t k, vector<ID> & session_assignment, const bool first_solution_only)
	{
		/*go through every value (i.e., dc) in this variable's (i.e., client's) domain (i.e., the set of assignment options)*/
		for (auto dc : session_clients.at(k).dc_domain)
		{
			if ((1 == num_discovered_solutions) && first_solution_only) { return; }
			
			if (IsValidPartialSolution(session_clients, k, session_assignment, dc))
			{
				if (IsWorthExtension(session_clients, k, session_assignment, dc, optimal_cost))
				{
					// assignment for k'th variable
					session_assignment.at(k) = dc;

					// a complete solution is discovered
					if (k == (session_clients.size() - 1))
					{
						// count this solution
						num_discovered_solutions++;

						// get the cost of this assignment solution
						double this_cost = GetSessionCostAfterAssignment(session_clients, session_assignment);

						// firstly check if this is the first solution before comparison
						if ((1 == num_discovered_solutions) || (this_cost < optimal_cost))
						{
							optimal_cost = this_cost;
							optimal_assignment = session_assignment;
						}						
					}
					else // continue on extending the partial solution
					{
						AssignClient(session_clients, k + 1, session_assignment, first_solution_only);
					}
				}
			}
		}
	}
	
	void OptimizingCostByTradingOffLatency::Simulate(const Setting & sim_setting)
	{		
		/*initialize global stuff*/
		if (!isInitialized) { Initialize(); }
		
		/*create folder and files for output*/
		auto this_output_directory = global.output_directory + local_output_directory;
		_mkdir(this_output_directory.c_str());		
		ofstream computational_time_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "computationalTime.csv");
		ofstream latency_measure_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "latencyMeasure.csv");
		ofstream cost_measure_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "costMeasure.csv");
		ofstream solution_cardinality_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "solutionCardinality.csv");
		ofstream solution_cardinality_CDF_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "solutionCardinality_CDF.csv");

		/*generate random_sessions*/
		auto random_sessions = GenerateRandomSessions(sim_setting);

		/*perform experiments*/
		for (const double latency_tradeoff_rate : { 0.0, 0.1, 0.2 })
		{
			cout << "latency_tradeoff_rate: " << latency_tradeoff_rate << "\n";
			vector<double> latency_measure_CP;
			vector<double> cost_measure_CP;
			vector<double> solution_cardinality_CP;
			auto startTime_CP = clock();
			for (auto session_clients : random_sessions)
			{
				CP(session_clients, latency_tradeoff_rate);
				latency_measure_CP.push_back(path_length_constraint);
				cost_measure_CP.push_back(optimal_cost);
				set<ID> solution_dc_set_CP(optimal_assignment.begin(), optimal_assignment.end());
				solution_cardinality_CP.push_back((double)solution_dc_set_CP.size());
			}

			/*write data into files*/
			computational_time_file << (difftime(clock(), startTime_CP) / random_sessions.size()) << ",";
			computational_time_file << (difftime(clock(), startTime_CP) / random_sessions.size());
			latency_measure_file << GetMeanValue(latency_measure_CP) << ",";
			cost_measure_file << GetMeanValue(cost_measure_CP) << ",";
			solution_cardinality_file << GetMeanValue(solution_cardinality_CP) << ",";
			for (size_t i = 0; i < random_sessions.size(); i++)
			{
				solution_cardinality_CDF_file << solution_cardinality_CP.at(i) << ",";
			}
			solution_cardinality_CDF_file << "\n";
		}

		/*close files*/
		computational_time_file.close();
		latency_measure_file.close();
		cost_measure_file.close();
		solution_cardinality_file.close();
	}

	void OptimizingCostByTradingOffLatency::CP(vector<Client> & session_clients, const double latency_tradeoff_rate)
	{
		/*find the optimal latency*/
		path_length_constraint = GetSessionLatencyLowerBound(session_clients); // initialize by the lower bound
		while (true)
		{									
			InitializeDomains4Clients(session_clients); // initialize every client's dc_domain	
			num_discovered_solutions = 0;			
			if (EnforceLocalConsistency(session_clients))
			{
				FindCheapestDcInDomain4Clients(session_clients); //required by IsWorthExtension()
				std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially
				vector<ID> session_assignment(session_clients.size());
				AssignClient(session_clients, 0, session_assignment, true); // begin with the first client (i.e., the 0'th client)
			}

			if (1 == num_discovered_solutions) break;
			else path_length_constraint++; // increment
		}

		/*find the optimal cost with latency tradoff*/
		path_length_constraint *= (1 + latency_tradeoff_rate);
		InitializeDomains4Clients(session_clients);
		num_discovered_solutions = 0;		
		if (EnforceLocalConsistency(session_clients))
		{			
			FindCheapestDcInDomain4Clients(session_clients); //required by IsWorthExtension()
			std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially			
			vector<ID> session_assignment(session_clients.size());
			AssignClient(session_clients, 0, session_assignment, false); // begin with the first client (i.e., the 0'th client)
		}
	}

	void OptimizingLatencyFirst::Simulate(const Setting & sim_setting)
	{		
		/*initialize global stuff*/
		if (!isInitialized) { Initialize(); }
		
		/*generate random_sessions*/
		auto random_sessions = GenerateRandomSessions(sim_setting);		

		/*stuff to store results*/
		vector<string> algList = { "SD", "NA", "CP", "CP-L", "CP-G" };
		auto resultContainer = Result(algList);

		/*perform experiments*/
		for (auto session_clients : random_sessions) // SD
		{			
			auto solution = SingleDatacenter(session_clients);
			
			string algName = "SD";
			resultContainer.latency_result.at(algName).push_back(solution.latency);
			resultContainer.cost_result.at(algName).push_back(solution.cost);
			for (auto it : solution.assignedDc_ranking) { resultContainer.assignedDc_ranking_result.at(algName).push_back(it); }
		}
		
		for (auto session_clients : random_sessions) // NA
		{			
			auto solution = NearestAssignment(session_clients);
			
			string algName = "NA";
			resultContainer.latency_result.at(algName).push_back(solution.latency);
			resultContainer.cost_result.at(algName).push_back(solution.cost);
			resultContainer.cardinality_result.at(algName).push_back(solution.cardinality);
		}
			
		vector<double> time_CP;
		for (auto session_clients : random_sessions) // CP
		{
			auto timePoint = clock();
			auto solution = CP(session_clients);	
			time_CP.push_back(difftime(clock(), timePoint));

			string algName = "CP";
			resultContainer.latency_result.at(algName).push_back(solution.latency);
			resultContainer.cost_result.at(algName).push_back(solution.cost);
			resultContainer.cardinality_result.at(algName).push_back(solution.cardinality);
			for (auto it : solution.assignedDc_ranking) { resultContainer.assignedDc_ranking_result.at(algName).push_back(it); }
		}
		
		vector<double> time_CP_L;
		for (auto session_clients : random_sessions) // CP-L
		{
			auto timePoint = clock();
			auto solution = CP(session_clients, Constraint(5, 0));
			time_CP_L.push_back(difftime(clock(), timePoint));
			
			string algName = "CP-L";
			resultContainer.latency_result.at(algName).push_back(solution.latency);
			resultContainer.cost_result.at(algName).push_back(solution.cost);
			resultContainer.cardinality_result.at(algName).push_back(solution.cardinality);
			for (auto it : solution.assignedDc_ranking) { resultContainer.assignedDc_ranking_result.at(algName).push_back(it); }
		}

		vector<double> time_CP_G;
		for (auto session_clients : random_sessions) // CP-G
		{
			auto timePoint = clock();
			auto solution = CP(session_clients, Constraint(0, 5));
			time_CP_G.push_back(difftime(clock(), timePoint));
			
			string algName = "CP-G";
			resultContainer.latency_result.at(algName).push_back(solution.latency);
			resultContainer.cost_result.at(algName).push_back(solution.cost);
			resultContainer.cardinality_result.at(algName).push_back(solution.cardinality);
			for (auto it : solution.assignedDc_ranking) { resultContainer.assignedDc_ranking_result.at(algName).push_back(it); }
		}
				
		/*create folder and files and write data*/
		auto this_output_directory = global.output_directory + local_output_directory;
		_mkdir(this_output_directory.c_str());
		ofstream time_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "time.csv");
		ofstream latency_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "latency.csv");
		ofstream cost_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "cost.csv");		
		ofstream cardinality_CDF_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "cardinality_CDF.csv");
		ofstream ranking_CDF_file(this_output_directory + "sessionSize[" + std::to_string(sim_setting.session_size) + "]_" + "ranking_CDF.csv");

		time_file << GetMaxValue(time_CP) << "," << GetMaxValue(time_CP_L) << "," << GetMaxValue(time_CP_G);
		
		string buffer = "";
		for (auto algName : algList)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.latency_result.at(algName))) + ",";
		}
		buffer.pop_back();
		latency_file << buffer;

		buffer = "";
		for (auto algName : algList)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.cost_result.at(algName))) + ",";
		}
		buffer.pop_back();
		cost_file << buffer;
		
		buffer = "";
		for (auto algName : { "NA", "CP", "CP-L", "CP-G" })
		{
			for (auto it : resultContainer.cardinality_result.at(algName))
			{
				buffer += std::to_string(it) + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		cardinality_CDF_file << buffer;
		
		buffer = "";
		for (auto algName : { "SD", "CP", "CP-L", "CP-G" })
		{
			for (auto it : resultContainer.assignedDc_ranking_result.at(algName))
			{
				buffer += std::to_string(it) + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		ranking_CDF_file << buffer;		
		
		/*close files*/
		time_file.close();
		latency_file.close();
		cost_file.close();
		cardinality_CDF_file.close();
		ranking_CDF_file.close();
	}

	Solution OptimizingLatencyFirst::CP(vector<Client> session_clients, const Constraint & input_extraConstraintSet)
	{		
		extraConstraintSet = input_extraConstraintSet;		
		path_length_constraint = GetSessionLatencyLowerBound(session_clients);
		while (true)
		{
			InitializeDomains4Clients(session_clients);	
			num_discovered_solutions = 0;
			vector<ID> session_assignment(session_clients.size());
			if (EnforceLocalConsistency(session_clients)) /* if EnforceLocalConsistency() returns false, no need to run AssignClient() */
			{				
				std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially				
				FindCheapestDcInDomain4Clients(session_clients); //required by IsWorthExtension() for searching cost-optimal solution
				AssignClient(session_clients, 0, session_assignment, false); // begin with the first client (i.e., the 0'th client)				
			}

			if (num_discovered_solutions > 0) break; /* break the loop as we have found at least one feasible solution */
			else path_length_constraint++; /* loosen the constraint and continue to loop */
		}
		return GetSolutionInfoAfterAssignment(session_clients, optimal_assignment);
	}
}