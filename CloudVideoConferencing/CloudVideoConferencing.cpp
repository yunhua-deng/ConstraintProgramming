#include "CloudVideoConferencing.h"

namespace CloudVideoConferencingProblem
{
	bool operator==(const Client& c_x, const Client& c_y) { return (c_x.id == c_y.id); }
	bool operator!=(const Client& c_x, const Client& c_y) { return (c_x.id != c_y.id); }
	bool operator<(const Client& c_x, const Client& c_y) { return (c_x.id < c_y.id); }
	bool ClientComparator_ByDomainSize(const Client& c_x, const Client& c_y) { return (c_x.dc_domain.size() < c_y.dc_domain.size()); }

	bool DatacenterComparator_ByExternalBandwidthPrice(const Datacenter& d_x, const Datacenter& d_y) { return (d_x.external_bandwidth_price < d_y.external_bandwidth_price); }

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
		return (p_x.sender == p_y.sender && p_x.dc_sender == p_y.dc_sender && p_x.dc_receiver == p_y.dc_receiver && p_x.receiver == p_y.receiver);
	};
	void PrintOutPath(const Path& p)
	{
		std::cout << p.sender << "->" << p.dc_sender << "->" << p.dc_receiver << "->" << p.receiver << "\n";
	};
	Path GetInversePath(const Path & p)
	{
		return Path(p.receiver, p.dc_receiver, p.dc_sender, p.sender);
	};

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
				dc_to_dc_delay_table_row.push_back((1 - global.dc_to_dc_latency_discount) * stod(row.at(col)) / 2); // one-way delay = rtt / 2, and considering discount
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
			d.internal_bandwidth_price = global.dc_internal_bandwidth_price_list.at(i);

			global.datacenter[d.id] = d;
		}

		/*create global.client*/
		const auto total_client_count = global.client_name_list.size();
		for (size_t i = 0; i < total_client_count; i++)
		{
			Client c;
			c.id = (ID)i;			
			
			/*discard those very remote clients which have a delay to any dc of higher than 50ms*/
			bool discarded = true;
			for (const auto & d : global.dc_id_list)
			{
				if (global.client_to_dc_delay_table.at(c.id).at(d) <= 50) { discarded = false; }
			}
			if (discarded) { continue; }

			/*its name in string*/
			c.name = global.client_name_list.at(i);
						
			/*important for the following*/
			RankDcByProximity4Client(c, global.dc_id_list);

			/*get subregion (e.g. "ec2-ap-northeast-1")*/
			c.subregion = global.datacenter.at(c.nearest_dc).name;

			/*get region (e.g. "ap")*/
			auto pos = global.datacenter.at(c.nearest_dc).name.find_first_of("-");
			c.region = global.datacenter.at(c.nearest_dc).name.substr(pos + 1, 2); // e.g. extract "ap" from "ec2-ap-northeast-1"
			
			/*record this client after finishing updating the client's properties*/
			global.client[c.id] = c; // client's id as the key for the client
			global.client_id_list.push_back(c.id);
		}
		
		/*create global.client_cluster*/
		for (const auto & c : global.client_id_list)
		{
			global.client_cluster[global.client.at(c).region].push_back(c);
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
			for (auto& c : all_clients)	RankDcByProximity4Client(c, it.second);

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
					buffer += std::to_string((int)GetPathLength(GetShortestPathOfClientPair(global.client.at(i), global.client.at(j), global.dc_id_list)));
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
	void SimulationBase::RankDcByProximity4Client(Client& the_client, const vector<ID>& dc_id_list)
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
	int SimulationBase::GetAssignedDcProximity(const Client & the_client, const ID assignedDc)
	{
		int result = int(std::find(the_client.ranked_dc_list.begin(), the_client.ranked_dc_list.end(), assignedDc) - the_client.ranked_dc_list.begin());
		if (result >= the_client.ranked_dc_list.size()) // not found
		{			
			cout << "sth wrong happens in GetAssignedDcProximity()!\n";
			cin.get();
			return -1;
		}
		else 
		{ 
			return (result + 1); // we count 1 as the No.1 ranking, not 0
		}
	}

	int SimulationBase::GetAssignedDcProximityLocal(const Client & the_client, const ID assigned_dc, const set<ID> & selected_dc_subset)
	{
		int result = 1;
		for (const auto dc : selected_dc_subset)
		{			
			if (global.client_to_dc_delay_table.at(the_client.id).at(dc) < global.client_to_dc_delay_table.at(the_client.id).at(assigned_dc)) result++;
		}
		return result;
	}
	
	/* to generate a session in a purely random manner */
	vector<ID> SimulationBase::GenerateOneRandomSessionNoRegionControl(const size_t session_size)
	{
		vector<ID> one_session;
		set<size_t> random_indexes = GenerateRandomIndexes(0, global.client_id_list.size() - 1, session_size);
		for (auto it : random_indexes) { one_session.push_back(global.client_id_list.at(it)); }

		/*sort the client id's in ascending order*/
		std::sort(one_session.begin(), one_session.end());

		return one_session;
	}

	/*to generate a session that contains clients from all regions*/
	vector<ID> SimulationBase::GenerateOneRandomSessionWithRegionControl(const size_t session_size)
	{
		/*shuffle each cluster of clients to introduce randomness*/
		for (auto & cluster : global.client_cluster)
		{
			random_shuffle(cluster.second.begin(), cluster.second.end());
		}

		/*making one random session with the number of clients from each cluster being as equal as possible*/
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

	/*to generate a session that contains clients from two regions*/
	vector<ID> SimulationBase::GenerateOneRandomSessionWithRegionControlTwoRegion(const size_t session_size)
	{
		/*shuffle each cluster of clients to introduce randomness*/
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
			vector<ID> one_session;
			if (sim_setting.region_control) one_session = GenerateOneRandomSessionWithRegionControl(sim_setting.session_size);
			else one_session = GenerateOneRandomSessionNoRegionControl(sim_setting.session_size);
			
			vector<Client> session_clients;
			for (auto & i : one_session)
			{
				session_clients.push_back(global.client.at(i));
			}
			
			if (session_clients.empty()) 
			{
				cout << "an unhandled exception occurs in GenerateRandomSessions()\n";
				cin.get();
				return vector<vector<Client>>();
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

	Path SimulationBase::GetShortestPathOfClientPair(const Client & c_x, const Client & c_y, const vector<ID> & dc_id_list)
	{
		Path result = Path(c_x.id, dc_id_list.front(), dc_id_list.front(), c_y.id);
		for (auto d_i : dc_id_list)
		{
			for (auto d_j : dc_id_list)
			{
				auto result_temp = Path(c_x.id, d_i, d_j, c_y.id);
				if (GetPathLength(result_temp) < GetPathLength(result)) 
				{ 
					result = result_temp; 
				}
			}
		}
		return result;
	}
		
	/* the longest of all shortest paths*/
	double SimulationBase::GetSessionLatencyLowerBound(const vector<Client> & session_clients)
	{	
		double result = 0;
		for (auto & c_i : session_clients)
		{
			for (auto & c_j : session_clients)
			{
				if (c_j.id != c_i.id)
				{
					auto result_temp = GetPathLength(GetShortestPathOfClientPair(c_i, c_j, global.dc_id_list));
					if (result_temp > result) // find the longest of all shortest paths
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

	Path SimulationBase::GetSessionLongestPathAfterAssignment(const vector<Client> & session_clients, const vector<ID> & session_assignment)
	{
		Path result = Path(session_clients.front().id, session_assignment.front(), session_assignment.back(), session_clients.back().id);		
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			for (size_t j = 0; j < session_clients.size(); j++)
			{
				if (j != i)
				{
					auto result_temp = Path(session_clients.at(i).id, session_assignment.at(i), session_assignment.at(j), session_clients.at(j).id);
					if (GetPathLength(result_temp) > GetPathLength(result))
					{ 
						result = result_temp; 
					}
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

		/*data relay cost*/
		double data_relay_cost = 0;
		if (chosen_dc_set.size() > 1)
		{
			for (auto sending_dc : chosen_dc_set)
			{
				for (auto receiving_dc : chosen_dc_set)
				{
					if (receiving_dc != sending_dc)
					{
						for (auto c : assigned_clients_to_dc.at(sending_dc))
						{
							if (global.datacenter.at(receiving_dc).provider != global.datacenter.at(sending_dc).provider)
								data_relay_cost += global.client.at(c).outgoing_data_amount * global.datacenter.at(sending_dc).external_bandwidth_price;
							else
								data_relay_cost += global.client.at(c).outgoing_data_amount * global.datacenter.at(sending_dc).internal_bandwidth_price;
						}
					}
				}
			}
		}

		/*return the sum*/
		return (data_relay_cost + data_delivery_cost);
	}

	Solution SimulationBase::GetSessionSolutionInfoAfterAssignment(const vector<Client> & session_clients, const vector<ID> & session_assignment)
	{
		Solution solution;

		solution.longest_path = GetSessionLongestPathAfterAssignment(session_clients, session_assignment);
		solution.latency = GetPathLength(solution.longest_path);
		solution.cost = GetSessionCostAfterAssignment(session_clients, session_assignment);
		set<ID> assignedDc_set(session_assignment.begin(), session_assignment.end());
		solution.cardinality = (int)assignedDc_set.size();
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			solution.proximity.push_back(GetAssignedDcProximity(session_clients.at(i), session_assignment.at(i)));
			solution.proximityLocal.push_back(GetAssignedDcProximityLocal(session_clients.at(i), session_assignment.at(i), assignedDc_set));
		}		

		return solution;
	}

	void SimulationBase::InitializeDomains4Clients(vector<Client> & session_clients)
	{			
		for (auto & c : session_clients)
		{
			c.dc_domain.clear();
			for (auto d : c.ranked_dc_list)
			{
				if (proximity_constraint < 1 || (c.dc_domain.size() < proximity_constraint && c.dc_domain.size() < c.ranked_dc_list.size()))
				{
					c.dc_domain.push_back(d);
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
				if (global.client_to_dc_delay_table.at(c.id).at(*it) > latency_constraint) // unary constraint
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

	/*a untility function used by EnforceArcConsistency()
	c_i's domain may be revised according to c_j*/
	bool SimulationBase::ArcReduce(Client& c_i, const Client& c_j)
	{
		if (c_j == c_i) return false;
		
		bool domain_reduced = false; // check if domain(c_i) is reduced at the end

		/*find and remove bad values from domain(c_i)*/
		for (auto d_i = c_i.dc_domain.begin(); d_i != c_i.dc_domain.end();)
		{
			bool to_be_removed = true; // check if d_i should be removed		
			for (auto& d_j : c_j.dc_domain)
			{				
				if (GetPathLength(Path(c_i.id, *d_i, d_j, c_j.id)) <= latency_constraint) // binary constraint
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
	bool SimulationBase::EnforceArcConsistency(vector<Client>& session_clients, const size_t k)
	{	
		//"worklist" contains the arcs (directed client pair) to be enforced for consistency (including clients from k to end)
		vector<pair<Client*, Client*>> worklist;
		for (size_t i = k; i < session_clients.size(); i++)
		{
			for (size_t j = k; j < session_clients.size(); j++)
			{
				if (session_clients.at(j) != session_clients.at(i))
				{
					worklist.push_back(pair<Client*, Client*>(&session_clients.at(i), &session_clients.at(j)));
				}
			}
		}
		
		/*reduce domain values by checking the consistency of one arc at a time*/
		while (!worklist.empty())
		{
			/*select one arc from worklist and remove it from the list*/
			auto arc = worklist.back();
			worklist.pop_back();

			/*work on this arc by applying ArcReduce()*/
			auto c_i = arc.first;
			auto c_j = arc.second;
			if (ArcReduce(*c_i, *c_j))
			{
				/*if domain(c_i) is empty, return false*/				
				if (c_i->dc_domain.empty())
				{
					//std::cout << "EnforceArcConsistency() returns false\n";
					return false;
				}

				/*update worklist by adding every arc (c_h, c_i) (h >= k && h != j && h != i) which is pointing to c_i as c_i's domain may be revised subject to c_j*/
				for (size_t h = k; h < session_clients.size(); h++)
				{
					if (session_clients.at(h) != *c_j && session_clients.at(h) != *c_i)
					{
						worklist.push_back(pair<Client*, Client*>(&session_clients.at(h), c_i));
					}
				}
			}
		}
		
		////"worklist" contains all arcs we wish to prove consistent or not
		//vector<pair<reference_wrapper<Client>, reference_wrapper<Client>>> worklist; //using reference_wrapper such that the item being pushed to the vector is not a copy but a reference
		//for (size_t i = k; i < session_clients.size(); i++)
		//{
		//	for (size_t j = k; j < session_clients.size(); j++)
		//	{
		//		if (session_clients.at(j) != session_clients.at(i))
		//		{					
		//			worklist.push_back(pair<reference_wrapper<Client>, reference_wrapper<Client>>(session_clients.at(i), session_clients.at(j)));
		//		}
		//	}
		//}
		////reduce domain values by checking the consistency of one arc at a time
		//while (!worklist.empty())
		//{
		//	/*select one arc from worklist and remove it from the list*/
		//	auto arc = worklist.back();
		//	worklist.pop_back();
		//	/*work on this arc by applying ArcReduce()*/
		//	auto c_i = arc.first;
		//	auto c_j = arc.second;
		//	if (ArcReduce(c_i, c_j))
		//	{
		//		/*if domain(c_i) is empty, return false*/
		//		if (c_i.get().dc_domain.empty())
		//		{
		//			//std::cout << "EnforceArcConsistency() returns false\n";
		//			return false;
		//		}
		//		/*update worklist by adding every arc (c_k, c_i) (k != j and k != i) which is pointing to c_i*/
		//		for (size_t h = k; h < session_clients.size(); h++)
		//		{
		//			if ((session_clients.at(h).id != c_j.get().id) && (session_clients.at(h).id != c_i.get().id))
		//			{
		//				worklist.push_back(pair<reference_wrapper<Client>, reference_wrapper<Client>>(session_clients.at(h), c_i));
		//			}
		//		}
		//	}
		//}

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
				return true;
			}
		}

		return false;
	}

	/*a utility function to check if the assignment to client k is consistent with any of the previous assignments*/
	bool SimulationBase::BackwardChecking(const vector<Client>& session_clients, const size_t k, const vector<ID>& session_assignment)
	{	
		// BackwardChecking: return false (i.e., inconsistent) if it will violate the constraint between k'th and any of the previous 0'th to (k - 1)'th clients
		for (size_t i = 0; i < k; i++)
		{			
			if (GetPathLength(Path(session_clients.at(k).id, session_assignment.at(k), session_assignment.at(i), session_clients.at(i).id)) > latency_constraint ||
				GetPathLength(Path(session_clients.at(i).id, session_assignment.at(i), session_assignment.at(k), session_clients.at(k).id)) > latency_constraint)
			{				
				return false; // backtrack (impossible to find any solution along this way)
			}
		}

		return true; // go deeper (still possible to find a solution along this way)
	}

	/*enforce partial arc consistency between current client and every future client*/
	bool SimulationBase::ForwardChecking(vector<Client>& session_clients, const size_t k, const vector<ID>& session_assignment)
	{
		//ForwardChecking: enforce partial arc consistency between current client and every future client
		if (k < (session_clients.size() - 1))
		{
			auto current_client = session_clients.at(k); // work on its copy to avoid modifying the domain of original client k
			current_client.dc_domain.assign(1, session_assignment.at(k)); // modify its copy's domain
			
			map<size_t, list<ID>> local_domain_backup; // for the future clients					
			for (size_t i = k + 1; i < session_clients.size(); i++)
			{
				local_domain_backup[i] = session_clients.at(i).dc_domain; // for restoring
				ArcReduce(session_clients.at(i), current_client); // revise the domain of session_clients.at(i) s.t. to current client
				if (session_clients.at(i).dc_domain.empty())
				{					
					for (size_t j = k + 1; j <= i; j++) { session_clients.at(j).dc_domain = local_domain_backup.at(j); } // restore before backtracking
										
					return false; // backtrack (impossible to find any solution along this way)
				}
			}
		}
		
		return true; // go deeper (still possible to find a solution along this way)
	}

	/*enforce full arc consistency among current and future clients*/
	bool SimulationBase::LookAhead(vector<Client>& session_clients, const size_t k, const vector<ID>& session_assignment)
	{
		/*LookAhead: enforce full arc consistency among current and future clients*/
		if (k < (session_clients.size() - 1))
		{
			map<size_t, list<ID>> local_domain_backup; // for avoiding using each client's dc_domain_backup
			for (size_t i = k; i < session_clients.size(); i++)
			{
				local_domain_backup[i] = session_clients.at(i).dc_domain;
			}
			bool is_arc_consistent = EnforceArcConsistency(session_clients, k);
			session_clients.at(k).dc_domain = local_domain_backup.at(k); // restore client k's domain
			if (!is_arc_consistent)
			{ 
				// restore before backtracking
				for (size_t i = k + 1; i < session_clients.size(); i++) 
				{ 
					session_clients.at(i).dc_domain = local_domain_backup.at(i); 
				}
								
				return false; // backtrack (impossible to find any solution along this way)
			}
		}

		return true; // go deeper (still possible to find a solution along this way)
	}

	/*a utility for bounding*/
	bool SimulationBase::ViolateCardinalityConstraint(const size_t k, const vector<ID>& session_assignment)
	{		
		if (cardinality_constraint >= 1)
		{
			set<ID> chosen_dc_set(session_assignment.begin(), session_assignment.begin() + k + 1);
			
			if (chosen_dc_set.size() > cardinality_constraint) 
				return true;
		}

		return false;
	}
	
	/*a utility for bounding used by AssignClient_FindAllSolutions()*/
	bool SimulationBase::CannotImproveCost(const vector<Client>& session_clients, const size_t k, const vector<ID>& session_assignment)
	{
		/*not sure as we haven't found any solution yet*/
		if (num_discovered_solutions < 1) 
			return false;

		/*compute the cost lower bound by ignoring the forwarding traffic cost*/
		double cost_lower_bound = 0;
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			if (i <= k) // assigned clients
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(session_assignment.at(i)).external_bandwidth_price;
			}
			else // unassigned clients
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(session_clients.at(i).cheapest_dc).external_bandwidth_price; // use cheapest_dc
			}
		}

		// return false only if we are sure that the lower bound is not smaller than the current optimal
		return (cost_lower_bound >= optimal_cost);
	}
		
	/*backtracking using backward checking*/
	void SimulationBase::AssignClient(const vector<Client> & session_clients, const size_t k, vector<ID> & session_assignment)
	{	
		/*go through every value (i.e., dc) in this variable's (i.e., client's) domain (i.e., the set of assignment options)*/
		const auto current_client_domain = session_clients.at(k).dc_domain;
		for (const auto dc : current_client_domain)
		{	
			// early stop once we have found 1 solution
			if (1 == num_discovered_solutions) return;				
			
			// assignment for k'th variable
			session_assignment.at(k) = dc;

			// check bounding conditions
			if (ViolateCardinalityConstraint(k, session_assignment)) continue;

			// backward checking
			if (BackwardChecking(session_clients, k, session_assignment))
			{
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
				else // continue to extend the partial solution towards a complete solution
				{
					AssignClient(session_clients, k + 1, session_assignment);
				}
			}
		}
	}

	void SimulationBase::AssignClient_FindAllSolutions(const vector<Client> & session_clients, const size_t k, vector<ID> & session_assignment)
	{
		/*go through every value (i.e., dc) in this variable's (i.e., client's) domain (i.e., the set of assignment options)*/
		const auto current_client_domain = session_clients.at(k).dc_domain;
		for (const auto dc : current_client_domain)
		{
			// assignment for k'th variable
			session_assignment.at(k) = dc;

			// check bounding conditions
			if (ViolateCardinalityConstraint(k, session_assignment) || CannotImproveCost(session_clients, k, session_assignment)) continue;

			// backward checking
			if (BackwardChecking(session_clients, k, session_assignment))
			{
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
				else // continue to extend the partial solution towards a complete solution
				{
					AssignClient(session_clients, k + 1, session_assignment);
				}
			}
		}
	}

	/*backtracking with forward checking*/
	void SimulationBase::AssignClient_FC(vector<Client> & session_clients, const size_t k, vector<ID> & session_assignment)
	{		
		/*go through every value (i.e., dc) in this variable's (i.e., client's) domain (i.e., the set of assignment options)*/
		const auto current_client_domain = session_clients.at(k).dc_domain;
		for (const auto dc : current_client_domain)
		{
			// FC only works for CSP (constraint satisfaction, i.e., one solution) but not COP (constraint optimization, i.e., best solution)
			if (1 == num_discovered_solutions)
				return;			

			// assignment for k'th variable
			session_assignment.at(k) = dc;

			// check bounding conditions
			if (ViolateCardinalityConstraint(k, session_assignment) || CannotImproveCost(session_clients, k, session_assignment))
				continue;

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
			else // continue to extend the partial solution towards a complete solution if ForwardChecking() suggests
			{			
				if (ForwardChecking(session_clients, k, session_assignment))
				{
					AssignClient_FC(session_clients, k + 1, session_assignment);
				}				
			}		
		}
	}

	/*backtracking with look ahead*/
	void SimulationBase::AssignClient_LA(vector<Client> & session_clients, const size_t k, vector<ID> & session_assignment)
	{		
		/*go through every value (i.e., dc) in this variable's (i.e., client's) domain (i.e., the set of assignment options)*/
		const auto current_client_domain = session_clients.at(k).dc_domain; 
		for (const auto dc : current_client_domain)
		{			
			// LA only works for CSP (constraint satisfaction, i.e., one solution) but not COP (constraint optimization, i.e., best solution)
			if (1 == num_discovered_solutions)
				return;

			// assignment for client k
			session_assignment.at(k) = dc;

			// check bounding conditions
			if (ViolateCardinalityConstraint(k, session_assignment) || CannotImproveCost(session_clients, k, session_assignment)) 
				continue;
									
			if (k == (session_clients.size() - 1)) // a complete solution is discovered
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
			else // continue to extend the partial solution towards a complete solution if LookAhead() suggests
			{				
				if (LookAhead(session_clients, k, session_assignment))
				{
					AssignClient_LA(session_clients, k + 1, session_assignment); 
				}				
			}		
		}
	}

	void MultilevelOptimization::Simulate(const Setting & sim_setting)
	{		
		/*initialize global stuff*/
		if (!isInitialized) { Initialize(); }
		
		/*generate random_sessions*/
		auto random_sessions = GenerateRandomSessions(sim_setting);

		/*location is client's subregion (i.e., dc'sname)*/
		map<pair<string, string>, int> allClientPairs_location_dist;
		for (const auto & dc_i : global.dc_name_list)
		{
			for (const auto & dc_j : global.dc_name_list)
			{
				allClientPairs_location_dist.insert({ { dc_i, dc_j }, 0 }); // initialize
			}
		}
		for (const auto & session_clients : random_sessions)
		{
			for (const auto & c_i : session_clients)
			{
				for (const auto & c_j : session_clients)
				{
					allClientPairs_location_dist.at({ c_i.subregion, c_j.subregion })++;
					allClientPairs_location_dist.at({ c_j.subregion, c_i.subregion })++;
				}
			}
		}
				
		/*perform experiments*/		
		vector<string> alg_name_list = { "CP", "CP-C", "Single-DC", "Nearest-DC" };
		auto resultContainer = Result(alg_name_list);
		for (const auto alg_name : alg_name_list)
		{
			for (auto session_clients : random_sessions)
			{
				Solution solution;

				auto startTime = clock();

				if ("CP" == alg_name) solution = CP(session_clients);
				else if ("CP-C" == alg_name) solution = CP_Card(session_clients);
				else if ("Single-DC" == alg_name) solution = CP_Card(session_clients, 1);
				else if ("Nearest-DC" == alg_name) solution = CP_Prox(session_clients, 1);

				resultContainer.time_result.at(alg_name).push_back(difftime(clock(), startTime));
				resultContainer.latency_result.at(alg_name).push_back(latency_constraint);
				resultContainer.cardinality_result.at(alg_name).push_back(solution.cardinality);
				for (auto it : solution.proximity) { resultContainer.proximity_result.at(alg_name).push_back(it); }
				for (auto it : solution.proximityLocal) { resultContainer.proximityLocal_result.at(alg_name).push_back(it); }
				/*resultContainer.proximity_result.at(alg_name).push_back(GetMaxValue(solution.proximity));
				resultContainer.proximityLocal_result.at(alg_name).push_back(GetMaxValue(solution.proximityLocal));*/
				/*farthestClientPair_location_dist_CP.at({ global.client.at(solution.longest_path.sender).subregion, global.client.at(solution.longest_path.receiver).subregion })++;
				farthestClientPair_location_dist_CP.at({ global.client.at(solution.longest_path.receiver).subregion, global.client.at(solution.longest_path.sender).subregion })++;*/
			}
		}

		//auto algName = alg_name_list.begin();
		//for (auto session_clients : random_sessions) 
		//{				
		//	auto solution = CP(session_clients, Constraint(0, 1));
		//	resultContainer.latency_result.at(*algName).push_back(solution.latency);
		//	resultContainer.cost_result.at(*algName).push_back(solution.cost);
		//	for (auto it : solution.proximity) { resultContainer.proximity_result.at(*algName).push_back(it); }
		//}
		//algName++; // next algorithm
		//		
		////map<pair<string, string>, int> farthestClientPair_location_dist_NA;
		////for (const auto & dc_i : global.dc_name_list)
		////{
		////	for (const auto & dc_j : global.dc_name_list)
		////	{
		////		farthestClientPair_location_dist_NA.insert({ { dc_i, dc_j }, 0 }); // initialize
		////	}
		////}
		//for (auto session_clients : random_sessions)
		//{
		//	auto solution = CP(session_clients, Constraint(1, 0));	
		//	resultContainer.latency_result.at(*algName).push_back(solution.latency);
		//	resultContainer.cost_result.at(*algName).push_back(solution.cost);
		//	resultContainer.cardinality_result.at(*algName).push_back(solution.cardinality);
		//	/*farthestClientPair_location_dist_NA.at({ global.client.at(solution.longest_path.sender).subregion, global.client.at(solution.longest_path.receiver).subregion })++;
		//	farthestClientPair_location_dist_NA.at({ global.client.at(solution.longest_path.receiver).subregion, global.client.at(solution.longest_path.sender).subregion })++;*/
		//}
		//algName++; // next algorithm
		//	
		////map<pair<string, string>, int> farthestClientPair_location_dist_CP;
		////for (const auto & dc_i : global.dc_name_list)
		////{
		////	for (const auto & dc_j : global.dc_name_list)
		////	{
		////		farthestClientPair_location_dist_CP.insert({ { dc_i, dc_j }, 0 }); // initialize
		////	}
		////}		
		//for (auto session_clients : random_sessions)
		//{				
		//	auto solution = CP(session_clients, Constraint(0, 2));			
		//	resultContainer.time_total_result.at(*algName).push_back(time_latency_stage + time_cost_stage);
		//	if ((time_latency_stage + time_cost_stage) > 0)
		//		resultContainer.time_proportion_result.at(*algName).push_back(time_latency_stage / (time_latency_stage + time_cost_stage));
		//	else 
		//		resultContainer.time_proportion_result.at(*algName).push_back(0);
		//	resultContainer.latency_result.at(*algName).push_back(solution.latency);
		//	resultContainer.cost_result.at(*algName).push_back(solution.cost);
		//	resultContainer.cardinality_result.at(*algName).push_back(solution.cardinality);
		//	for (auto it : solution.proximity) { resultContainer.proximity_result.at(*algName).push_back(it); }
		//	/*farthestClientPair_location_dist_CP.at({ global.client.at(solution.longest_path.sender).subregion, global.client.at(solution.longest_path.receiver).subregion })++;
		//	farthestClientPair_location_dist_CP.at({ global.client.at(solution.longest_path.receiver).subregion, global.client.at(solution.longest_path.sender).subregion })++;*/
		//}
		//algName++; // next algorithm
		//		
		//for (auto session_clients : random_sessions)
		//{	
		//	auto solution = CP(session_clients, Constraint(0, 3)); 
		//	if ((time_latency_stage + time_cost_stage) > 0)
		//		resultContainer.time_proportion_result.at(*algName).push_back(time_latency_stage / (time_latency_stage + time_cost_stage));
		//	else
		//		resultContainer.time_proportion_result.at(*algName).push_back(0);
		//	resultContainer.time_total_result.at(*algName).push_back(time_latency_stage + time_cost_stage);
		//	resultContainer.latency_result.at(*algName).push_back(solution.latency);
		//	resultContainer.cost_result.at(*algName).push_back(solution.cost);
		//	resultContainer.cardinality_result.at(*algName).push_back(solution.cardinality);
		//	for (auto it : solution.proximity) { resultContainer.proximity_result.at(*algName).push_back(it); }
		//}
		//algName++; // next algorithm
		//
		//for (auto session_clients : random_sessions)
		//{
		//	auto solution = CP(session_clients, Constraint(0, 4));						
		//	if ((time_latency_stage + time_cost_stage) > 0)
		//		resultContainer.time_proportion_result.at(*algName).push_back(time_latency_stage / (time_latency_stage + time_cost_stage));
		//	else
		//		resultContainer.time_proportion_result.at(*algName).push_back(0);
		//	resultContainer.time_total_result.at(*algName).push_back(time_latency_stage + time_cost_stage);
		//	resultContainer.latency_result.at(*algName).push_back(solution.latency);
		//	resultContainer.cost_result.at(*algName).push_back(solution.cost);
		//	resultContainer.cardinality_result.at(*algName).push_back(solution.cardinality);
		//	for (auto it : solution.proximity) { resultContainer.proximity_result.at(*algName).push_back(it); }
		//}
				
		/*create folder and files and write data*/
		auto this_output_directory = global.output_directory + local_output_directory;
		_mkdir(this_output_directory.c_str()); //_mkdir() cannot work if the parent directory does not exist (therefore, need to create one one-level directory at a time)
		this_output_directory += sim_setting.region_control ? "RegionControl\\" : "";
		_mkdir(this_output_directory.c_str());
		ofstream time_avg_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "time_avg.csv");
		ofstream time_max_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "time_max.csv");
		ofstream latency_avg_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "latency_avg.csv");
		ofstream latency_max_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "latency_max.csv");
		ofstream latency_CDF_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "latency_CDF.csv");
		ofstream cardinality_avg_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "cardinality_avg.csv");
		ofstream cardinality_CDF_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "cardinality_CDF.csv");
		ofstream proximity_avg_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "proximity_avg.csv");		
		ofstream proximity_CDF_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "proximity_CDF.csv");
		ofstream proximityLocal_avg_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "proximityLocal_avg.csv");
		ofstream proximityLocal_CDF_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "proximityLocal_CDF.csv");
		
		// time_avg
		string buffer = "";
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.time_result.at(algName))) + ",";
		}
		buffer.pop_back();
		time_avg_file << buffer;
		
		// time_99th
		buffer = "";
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetPercentile(resultContainer.time_result.at(algName), 99)) + ",";
		}
		buffer.pop_back();
		time_max_file << buffer;
			
		// latency_avg
		buffer = ""; 
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.latency_result.at(algName))) + ",";
		}
		buffer.pop_back();
		latency_avg_file << buffer;
		
		// latency_max
		buffer = "";
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetMaxValue(resultContainer.latency_result.at(algName))) + ",";
		}
		buffer.pop_back();
		latency_max_file << buffer;

		// latency_CDF
		buffer = "";
		for (auto algName : alg_name_list)
		{
			for (auto it : resultContainer.latency_result.at(algName))
			{
				buffer += std::to_string(it) + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		buffer.pop_back();
		latency_CDF_file << buffer;

		// cardinality_avg
		buffer = "";
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.cardinality_result.at(algName))) + ",";			
		}
		buffer.pop_back();
		cardinality_avg_file << buffer;

		// proximity_avg
		buffer = "";
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.proximity_result.at(algName))) + ",";
		}
		buffer.pop_back();
		proximity_avg_file << buffer;

		// proximityLocal_avg
		buffer = "";
		for (auto algName : alg_name_list)
		{
			buffer += std::to_string(GetMeanValue(resultContainer.proximityLocal_result.at(algName))) + ",";
		}
		buffer.pop_back();
		proximityLocal_avg_file << buffer;

		// cardinality_CDF
		buffer = "";
		for (auto algName : alg_name_list)
		{
			for (auto it : resultContainer.cardinality_result.at(algName))
			{
				buffer += std::to_string((int)it) + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		cardinality_CDF_file << buffer;
		
		// proximity_CDF
		buffer = "";
		for (auto algName : alg_name_list)
		{
			for (auto it : resultContainer.proximity_result.at(algName))
			{
				buffer += std::to_string((int)it) + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		proximity_CDF_file << buffer;

		// proximityLocal_CDF
		buffer = "";
		for (auto algName : alg_name_list)
		{
			for (auto it : resultContainer.proximityLocal_result.at(algName))
			{
				buffer += std::to_string((int)it) + ",";
			}
			buffer.pop_back();
			buffer.push_back('\n');
		}
		proximityLocal_CDF_file << buffer;

		/*ofstream allClientPair_location_dist_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "allClientPair_location_dist.csv");
		DumpLabeledMatrixToDisk(allClientPairs_location_dist, global.dc_name_list, this_output_directory + std::to_string(sim_setting.session_size) + "_" + "allClientPair_location_dist.csv");*/
		/*ofstream farthestClientPair_location_dist_NA_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "farthestClientPair_location_dist_NA.csv");			
		DumpLabeledMatrixToDisk(farthestClientPair_location_dist_NA, global.dc_name_list, this_output_directory + std::to_string(sim_setting.session_size) + "_" + "farthestClientPair_location_dist_NA.csv");
		ofstream farthestClientPair_location_dist_CP_file(this_output_directory + std::to_string(sim_setting.session_size) + "_" + "farthestClientPair_location_dist_CP.csv");
		DumpLabeledMatrixToDisk(farthestClientPair_location_dist_CP, global.dc_name_list, this_output_directory + std::to_string(sim_setting.session_size) + "_" + "farthestClientPair_location_dist_CP.csv");*/
	}
	
	Solution MultilevelOptimization::CP(vector<Client> session_clients)
	{		
		proximity_constraint = 0;
		cardinality_constraint = 0;
		latency_constraint = GetSessionLatencyLowerBound(session_clients); // start from lower bound

		while (true)
		{
			InitializeDomains4Clients(session_clients);
			num_discovered_solutions = 0;
			vector<ID> session_assignment(session_clients.size());
			
			if (EnforceLocalConsistency(session_clients)) /* if EnforceLocalConsistency() returns false, no need to run AssignClient_BC() */
			{
				std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially								
				AssignClient(session_clients, 0, session_assignment); // begin with the first client (i.e., the 0'th client)
			}
			
			if (num_discovered_solutions > 0) break; /* break the loop as we have found at least one feasible solution */
			else latency_constraint += latency_constraint_delta; /* loosen the constraint and continue to loop */
		}

		return GetSessionSolutionInfoAfterAssignment(session_clients, optimal_assignment);
	}

	Solution MultilevelOptimization::CP_Card(vector<Client> session_clients, const size_t given_cardinality_constraint)
	{			
		// exception reporting
		if (given_cardinality_constraint > global.dc_id_list.size() || given_cardinality_constraint > session_clients.size())
		{
			cout << "given_cardinality_constraint > global.dc_id_list.size() || given_cardinality_constraint > session_clients.size() in CP_Card()!\n";
			cin.get();
			return Solution();
		}
		
		proximity_constraint = 0; // reset to default

		if (given_cardinality_constraint >= 1) // trading off latency_constraint to cardinality_constraint
		{			
			latency_constraint = GetSessionLatencyLowerBound(session_clients);
			cardinality_constraint = 1; // begin with 1

			while (true)
			{
				InitializeDomains4Clients(session_clients);
				num_discovered_solutions = 0;
				vector<ID> session_assignment(session_clients.size());

				if (EnforceLocalConsistency(session_clients)) /* if EnforceLocalConsistency() returns false, no need to run AssignClient_BC() */
				{
					std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially							
					AssignClient(session_clients, 0, session_assignment); // begin with the first client (i.e., the 0'th client)
				}

				if (num_discovered_solutions > 0) break; /* break the loop as we have found at least one feasible solution */
				else
				{
					if (cardinality_constraint < given_cardinality_constraint) 
						cardinality_constraint++; // loosen the cardinality_constraint and continue to loop with this latency_constraint
					else
					{
						latency_constraint += latency_constraint_delta; // loosen the latency_constraint and continue to loop
						cardinality_constraint = 1; // reset to 1 to start from scrath with the new latency_constraint
					}
				}
			}
		}
		else // optimize latency_constraint first and then cardinality_constraint
		{
			// determine the optimal latency_constraint
			CP(session_clients);

			// optimize the cardinality_constraint subject to the optimal latency_constraint		
			for (cardinality_constraint = 1; cardinality_constraint <= global.dc_id_list.size() && cardinality_constraint <= session_clients.size(); cardinality_constraint++)
			{
				InitializeDomains4Clients(session_clients);
				num_discovered_solutions = 0;
				vector<ID> session_assignment(session_clients.size());

				if (EnforceLocalConsistency(session_clients)) /* if EnforceLocalConsistency() returns false, no need to run AssignClient_BC() */
				{
					std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially								
					AssignClient(session_clients, 0, session_assignment); // begin with the first client (i.e., the 0'th client)
				}

				if (num_discovered_solutions > 0) break;
			}
		}

		return GetSessionSolutionInfoAfterAssignment(session_clients, optimal_assignment);
	}

	Solution MultilevelOptimization::CP_Prox(vector<Client> session_clients, const size_t given_proximity_constraint)
	{
		// exception reporting
		if (given_proximity_constraint > global.dc_id_list.size())
		{
			cout << "given_proximity_constraint > global.dc_id_list.size() in CP_Card()!\n";
			cin.get();
			return Solution();
		}
		
		cardinality_constraint = 0; // set to 0 to ignore cardinality_constraint

		if (given_proximity_constraint >= 1) // trading off latency_constraint to proximity_constraint
		{			
			latency_constraint = GetSessionLatencyLowerBound(session_clients);
			proximity_constraint = 1; // begin with 1

			while (true)
			{
				InitializeDomains4Clients(session_clients);
				num_discovered_solutions = 0;
				vector<ID> session_assignment(session_clients.size());

				if (EnforceLocalConsistency(session_clients)) /* if EnforceLocalConsistency() returns false, no need to run AssignClient_BC() */
				{
					std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially							
					AssignClient(session_clients, 0, session_assignment); // begin with the first client (i.e., the 0'th client)
				}

				if (num_discovered_solutions > 0) break; /* break the loop as we have found at least one feasible solution */
				else
				{
					if (proximity_constraint < given_proximity_constraint) 
						proximity_constraint++; // loosen the proximity_constraint and continue to loop with this latency_constraint
					else
					{
						latency_constraint += latency_constraint_delta; // loosen the latency_constraint and continue to loop
						proximity_constraint = 1; // reset to 1 to start from scrath with the new latency_constraint
					}
				}
			}
		}
		else // optimize latency_constraint first and then proximity_constraint
		{
			// determine the optimal latency_constraint
			CP(session_clients);

			// optimize the proximity_constraint subject to the optimal latency_constraint			
			for (proximity_constraint = 1; proximity_constraint <= global.dc_id_list.size(); proximity_constraint++)
			{
				InitializeDomains4Clients(session_clients);
				num_discovered_solutions = 0;
				vector<ID> session_assignment(session_clients.size());

				if (EnforceLocalConsistency(session_clients)) /* if EnforceLocalConsistency() returns false, no need to run AssignClient_BC() */
				{
					std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially							
					AssignClient(session_clients, 0, session_assignment); // begin with the first client (i.e., the 0'th client)
				}

				if (num_discovered_solutions > 0) break;
			}
		}

		return GetSessionSolutionInfoAfterAssignment(session_clients, optimal_assignment);
	}

	void RunSimulation_MultilevelOptimization(const Setting & sim_setting)
	{
		auto sim = MultilevelOptimization();
		sim.Simulate(sim_setting);
	}
}