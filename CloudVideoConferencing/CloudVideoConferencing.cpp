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

		/*create client and generate client_cluster*/
		const auto total_client_count = global.client_name_list.size();
		for (size_t i = 0; i < total_client_count; i++)
		{
			Client c;
			c.id = (ID)i;
			global.client_id_list.push_back(c.id);
			c.name = global.client_name_list.at(i); // client domain name

			/* https://trueconf.com/support/communication-channels.html */			
			c.outgoing_data_amount = 0.5; // 1MBit/s for HD, 1 hour session length (1MBit/s * 3600s / 8Bit/Byte ~= 0.5GB)
			c.incoming_data_amount = c.incoming_data_amount * (global.sim_setting.session_size - 1);
						
			/*important*/
			RankDatacenters4Client(c, global.dc_id_list);

			/*get subregion (e.g. "ec2-ap-northeast-1")*/
			c.subregion = global.datacenter.at(c.nearest_dc).name;

			/*get region (e.g. "ap")*/
			auto pos = global.datacenter.at(c.nearest_dc).name.find_first_of("-");
			c.region = global.datacenter.at(c.nearest_dc).name.substr(pos + 1, 2); // e.g. extract "ap" from "ec2-ap-northeast-1"

			/*create clusters*/
			global.client_cluster[c.region].push_back(c.id);

			/*record this client*/
			global.client[c.id] = c;
		}

		// dump client cluster to disk	
		_mkdir(global.output_directory.c_str()); // because _mkdir() only accepts const char*
		string cluster_file_name = global.output_directory + "client_cluster.txt";
		ofstream cluster_file(cluster_file_name);
		if (cluster_file.is_open())
		{
			string one_line;
			for (auto& it : global.client_cluster)
			{
				one_line = it.first + ",";
				for (auto& c : it.second)
				{
					one_line += global.client.at(c).name + ",";
				}
				auto pos = one_line.find_last_of(",");
				cluster_file << one_line.substr(0, pos) << "\n";
			}
			cluster_file.close();
		}
		else
		{
			std::cout << "\nfailed to create " << cluster_file_name << "\n";
			std::cin.get();
		}

		/*make all_sessions according to client clusters*/
		//srand(time(NULL));
		srand(12345);
		for (size_t i = 0; i < global.sim_setting.session_count; i++)
		{
			vector<ID> one_session_id_list;
			GenerateOneSession(one_session_id_list);
			
			vector<Client> one_session_clients;
			for (auto & i : one_session_id_list)
			{
				one_session_clients.push_back(global.client.at(i));
			}

			global.all_sessions.push_back(one_session_clients);
		}

		/*generate all non-empty dc combinations (i.e., the collection of subsets of the full dc set)*/
		vector<bool> x;
		x.assign(global.dc_id_list.size(), false);
		GenerateAllSubsets(global.dc_id_list, x, 0, global.all_dc_subsets);
		std::sort(global.all_dc_subsets.begin(), global.all_dc_subsets.end(), SubsetComparatorBySize);

		/*pre-compute path_length, it is memory-intensive: not applicable for large input sizes (e.g., > 1000 candidate clients)*/
		/*Path one_path;
		for (auto s : global.client_id_list)
		{
		one_path.sender = s;

		for (auto r : global.client_id_list)
		{
		if (r != s)
		{
		one_path.receiver = r;

		for (auto d_s : global.dc_id_list)
		{
		one_path.dc_sender = d_s;

		for (auto d_r : global.dc_id_list)
		{
		one_path.dc_receiver = d_r;

		double length = global.client_to_dc_delay_table.at(s).at(d_s)
		+ global.dc_to_dc_delay_table.at(d_s).at(d_r)
		+ global.client_to_dc_delay_table.at(r).at(d_r);

		global.path_length[one_path] = length;
		}
		}
		}
		}
		}*/

		/*see if everything alright*/
		//PrintGlobalInfo();
	}

	/*check whether inter-datacentre links have shorter latencies than other links*/
	void SimulationBase::Check_InterDcNetwork_Advantage()
	{
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

							/*double len_a = global.path_length.at(Path(c_x.id, c_x.nearest_dc, c_y.nearest_dc, c_y.id));
							double len_b = global.path_length.at(Path(c_x.id, c_x.nearest_dc, c_x.nearest_dc, c_y.id));
							double len_c = global.path_length.at(Path(c_x.id, c_y.nearest_dc, c_y.nearest_dc, c_y.id));	*/
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

		/*based on shortest path*/
		//std::cout << "| checking based on shortest path\n";
		//for (auto& it : dc_group)
		//{
		//	/*find all paths and shortest paths between client pairs of this dc_group*/
		//	FindShortestPaths(all_clients, it.second);

		//	/*find nearest datacenters for clients of this dc_group*/
		//	for (auto& c : all_clients)	RankDatacenters4Client(c, it.second);
		//	
		//	/*examine*/
		//	double num_client_pairs = 0;
		//	double num_pairs_nearest_shortest = 0;
		//	double num_pairs_favor_interDC = 0;
		//	for (auto& c_x : all_clients)
		//	{
		//		for (auto& c_y : all_clients)
		//		{
		//			if (c_x != c_y)
		//			{
		//				num_client_pairs++;
		//				
		//				if (c_x.shortest_path_to_client.at(c_y.id).dc_sender != c_x.shortest_path_to_client.at(c_y.id).dc_receiver)
		//				{
		//					num_pairs_favor_interDC++;

		//					if (c_x.shortest_path_to_client.at(c_y.id).dc_sender == c_x.nearest_dc && c_x.shortest_path_to_client.at(c_y.id).dc_receiver == c_y.nearest_dc)
		//						num_pairs_nearest_shortest++;
		//				}
		//			}
		//		}
		//	}
		//	std::cout << "| | " << it.first << "\n";		
		//	std::cout << "| | | " << "ratio_prefer_interDC:\t" << num_pairs_favor_interDC << "/" << num_client_pairs <<
		//		" = " << num_pairs_favor_interDC / num_client_pairs << "\n";
		//	std::cout << "| | | | " << "ratio_prefer_nearest:\t" << num_pairs_nearest_shortest << "/" << num_pairs_favor_interDC <<
		//		" = " << num_pairs_nearest_shortest / num_pairs_favor_interDC << "\n";
		//}
	}

	void SimulationBase::Get_DelayToNearestDc_CDF()
	{
		ofstream data_file(global.output_directory + "CDF_DelayToNearestDC.csv");
		if (data_file.is_open())
		{
			for (const auto & it : global.client)
			{				
				data_file << global.client_to_dc_delay_table.at(it.second.id).at(it.second.nearest_dc) << ",";
			}
			data_file.close();
		}
		else
		{
			std::cout << "\nfailed to create " << "CDF_DelayToNearestDC.txt" << "\n";
			std::cin.get();
		}
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

	void SimulationBase::FindShortestPaths(vector<Client>& client_list, const vector<ID>& dc_id_list)
	{
		for (auto& c_x : client_list)
		{
			/*find the shortest path to c_y*/
			for (auto& c_y : client_list)
			{
				if (c_x != c_y)
				{
					c_x.shortest_path_to_client[c_y.id] = Path(c_x.id, dc_id_list.front(), dc_id_list.front(), c_y.id);
					for (auto& d_i : dc_id_list)
					{
						for (auto& d_j : dc_id_list)
						{
							auto one_path = Path(c_x.id, d_i, d_j, c_y.id);
							if (GetPathLength(one_path) < GetPathLength(c_x.shortest_path_to_client.at(c_y.id)))
							{
								c_x.shortest_path_to_client[c_y.id] = one_path;
							}
						}
					}
				}
			}
		}
	}

	/*including shortest pathes*/
	void SimulationBase::FindAllPaths(vector<Client>& client_list, const vector<ID>& dc_id_list)
	{
		for (auto& c_x : client_list)
		{
			/*reset in case of non-empty*/
			c_x.path_to_client.clear();

			/*find every possible path and the shortest path to c_y*/
			for (auto& c_y : client_list)
			{
				if (c_x != c_y)
				{
					for (auto& d_i : dc_id_list)
					{
						for (auto& d_j : dc_id_list)
						{
							c_x.path_to_client[c_y.id].push_back(Path(c_x.id, d_i, d_j, c_y.id)); // find every possible path to c_y
						}
					}

					c_x.shortest_path_to_client[c_y.id] = c_x.path_to_client.at(c_y.id).front(); // initialize for finding the shorest path to c_y
					for (auto& p : c_x.path_to_client.at(c_y.id))
					{						
						if (GetPathLength(p) < GetPathLength(c_x.shortest_path_to_client.at(c_y.id)))
						{
							c_x.shortest_path_to_client.at(c_y.id) = p;
						}
					}
				}
			}
		}
	}

	/*ensure the generated session contains clients from all regions*/
	void SimulationBase::GenerateOneSession(vector<ID>& one_session)
	{
		/*shuffle for creating randomness of each session*/
		for (auto& cluster : global.client_cluster)
		{
			random_shuffle(cluster.second.begin(), cluster.second.end());
		}

		/*making one random session with equal number of clients from each cluster*/
		one_session.clear(); // reset
		int remaining = (int)global.sim_setting.session_size;
		int next_pos = 0;

		vector<vector<ID>> client_clusters; // because global.client_cluster cannot be shuffled, hence we need a vector to shuffle
		for (auto& cluster : global.client_cluster)
		{
			client_clusters.push_back(cluster.second);
		}
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
					return;
				}
			}
			next_pos++;
		}

		/*sort the client id's in ascending order*/
		std::sort(one_session.begin(), one_session.end());
	}

	/*to generate session that contains clients from two regions*/
	void SimulationBase::GenerateOneSessionWithTwoRegion(vector<ID>& one_session)
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
			return;
		}
		vector<string> two_cluster_names(cluster_names.begin(), cluster_names.begin() + 2);

		/*making one random session with equal number of clients from each cluster*/
		one_session.clear(); // reset
		int remaining = (int)global.sim_setting.session_size;
		int next_pos = 0;
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
						return;
					}
				}
			}
			next_pos++;
		}

		/*align the client id's in ascending order*/
		std::sort(one_session.begin(), one_session.end());
	}

	double SimulationBase::GetPathLength(const Path& path)
	{
		return global.client_to_dc_delay_table.at(path.sender).at(path.dc_sender)
			+ global.dc_to_dc_delay_table.at(path.dc_sender).at(path.dc_receiver)
			+ global.client_to_dc_delay_table.at(path.receiver).at(path.dc_receiver);
	}

	/*assume that every client pair can use the shortest path (each client can has multiple assigned dc's)*/
	double SimulationBase::GetSessionDelayLowerBound(const vector<Client> & session_clients)
	{
		double delayLowerBound = GetPathLength(session_clients.at(0).shortest_path_to_client.at(session_clients.at(1).id));		
		for (auto & c_i : session_clients)
		{
			for (auto & c_j : session_clients)
			{
				if (c_j.id != c_i.id)
				{
					auto current_length = GetPathLength(c_i.shortest_path_to_client.at(c_j.id));
					if (current_length > delayLowerBound)
					{
						delayLowerBound = current_length;
					}
				}
			}
		}
		return delayLowerBound;
	}

	/*assume that each client is assigned to its nearest_dc*/
	double SimulationBase::GetSessionDelayUpperBound(const vector<Client> & session_clients)
	{
		double delayUpperBound = GetPathLength(Path(session_clients.at(0).id, session_clients.at(0).nearest_dc, session_clients.at(1).nearest_dc, session_clients.at(1).id));
		for (auto & c_i : session_clients)
		{
			for (auto & c_j : session_clients)
			{
				if (c_j.id != c_i.id)
				{
					auto current_length = GetPathLength(Path(c_i.id, c_i.nearest_dc, c_j.nearest_dc, c_j.id));
					if (current_length > delayUpperBound)
					{
						delayUpperBound = current_length;
					}
				}
			}
		}
		return delayUpperBound;
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
		double costLowerBound = 0;
		for (auto & c : session_clients)
		{
			costLowerBound += c.incoming_data_amount * global.datacenter.at(cheapest_dc).external_bandwidth_price;
		}
		return costLowerBound;
	}	

	double SimulationBase::GetSessionDelayAfterAssignment(const vector<Client>& session_clients, const vector<ID>& client_assignment)
	{
		double assignment_delay = 0;
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			for (size_t j = 0; j < session_clients.size(); j++)
			{
				if (j != i)
				{
					auto current_delay = GetPathLength(Path(session_clients.at(i).id, client_assignment.at(i), client_assignment.at(j), session_clients.at(j).id));
					if (current_delay > assignment_delay)
						assignment_delay = current_delay;
				}
			}
		}
		return assignment_delay;
	}

	double SimulationBase::GetSessionCostAfterAssignment(const vector<Client>& session_clients, const vector<ID>& client_assignment)
	{
		/*create some stuff to facilitate the cost calculation*/
		unordered_map<ID, vector<ID>> assigned_clients_to_dc;
		for (size_t i = 0; i < client_assignment.size(); i++)
		{
			assigned_clients_to_dc[client_assignment.at(i)].push_back(session_clients.at(i).id);
		}
		unordered_set<ID> chosen_dc_subset(client_assignment.begin(), client_assignment.end());
		//num_of_chosen_DCs = (double)chosen_dc_subset.size();

		/*data delivery cost*/
		double data_delivery_cost = 0;
		for (auto d : chosen_dc_subset)
		{
			for (auto c : assigned_clients_to_dc.at(d))
				data_delivery_cost += global.client.at(c).incoming_data_amount * global.datacenter.at(d).external_bandwidth_price;
		}

		/*data relay cost (only if more than 1 dc's are chosen)*/
		double data_relay_cost = 0;
		if (chosen_dc_subset.size() > 1)
		{
			for (auto sending_dc : chosen_dc_subset)
			{
				for (auto receiving_dc : chosen_dc_subset)
				{
					if (receiving_dc != sending_dc)
					{
						if (global.datacenter.at(receiving_dc).provider != global.datacenter.at(sending_dc).provider)
						{
							for (auto c : assigned_clients_to_dc.at(sending_dc))
								data_relay_cost += global.client.at(c).outgoing_data_amount * global.datacenter.at(sending_dc).external_bandwidth_price;
						}
						else
						{
							for (auto c : assigned_clients_to_dc.at(sending_dc))
								data_relay_cost += global.client.at(c).outgoing_data_amount * global.datacenter.at(sending_dc).internal_bandwidth_price;
						}
					}
				}
			}
		}

		/*compute the interDC_cost_ratio*/
		//interDC_cost_ratio = ((data_relay_cost + data_delivery_cost) > 0) ? (data_relay_cost / (data_relay_cost + data_delivery_cost)) : 0;

		/*the total traffic cost for this session*/
		double total_cost = data_relay_cost + data_delivery_cost;

		/*return total_cost*/
		return total_cost;
	}	

	void SimulationBase::PrintClientDomain(const Client& c)
	{
		std::cout << "domain of client " << c.id << " contains " << c.dc_domain.size() << " datacenters\n";
	}
	
	void SimulationBase::PrintGlobalInfo()
	{
		std::cout << "Simulation setup information:\n";
		std::cout << " | number of datacenters: " << global.datacenter.size() << "\n";
		std::cout << " | number of clients: " << global.client.size() << "\n";

		std::cout << " | number of client clusters: " << global.client_cluster.size() << "\n";
		for (auto& i : global.client_cluster)
			std::cout << " | | " << i.first << "\n";

		//std::cout << " | number of possible paths of all client pairs: " << global.path_length.size() << "\n";
		//std::cout << " | number of sessions: " << all_sessions.size() << "\n";

		/*for (auto& session : all_sessions)
		{
		for (auto& c : session) std::cout << c << " ";
		std::cout << "\n";

		for (auto& c : session) std::cout << global.client[c].name << " ";
		std::cout << "\n";
		}*/
	}

	void SimulationBase::InitializeDomains4Clients(vector<Client> & session_clients, size_t top_k)
	{
		if (0 == top_k)
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
				c.dc_domain.clear();
				for (size_t i = 0; i < top_k; i++)
				{
					c.dc_domain.push_back(c.ranked_dc_list.at(i));
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
	bool SimulationBase::IsValidPartialSolution(const vector<Client>& session_clients, const size_t k, const vector<ID>& client_assignment, const ID d_k, const size_t solution_cardinality_constraint)
	{
		/*check whether this assignment for client k will violate a constraint between k'th and one of the previous 0'th to (k - 1)'th clients*/
		for (size_t i = 0; i < k; i++)
		{			
			if (GetPathLength(Path(session_clients.at(k).id, d_k, client_assignment.at(i), session_clients.at(i).id)) > path_length_constraint ||
				GetPathLength(Path(session_clients.at(i).id, client_assignment.at(i), d_k, session_clients.at(k).id)) > path_length_constraint)
			{
				return false;
			}
		}

		/*consider the solution_cardinality_constraint only if it is != 0*/
		if (solution_cardinality_constraint != 0)
		{
			set<ID> chosen_dc_subset(client_assignment.begin(), client_assignment.begin() + k); // k'th not included
			chosen_dc_subset.insert(d_k);
			if (chosen_dc_subset.size() > solution_cardinality_constraint)
			{
				return false;
			}
		}

		/*consistent*/
		return true;
	}

	/*a utility employed by AssignClient_ConstrainedOptimization4Cost() for bounding*/
	bool SimulationBase::IsWorthyExtension(const vector<Client>& session_clients, const size_t k, vector<ID>& client_assignment, const ID d_k, double traffic_cost)
	{
		/*no need to care about the cost if no solutions have been found (because traffic_cost has not been assigned yet!)*/
		if (0 == num_discovered_solutions)
			return true;

		/*compute the cost lower bound by ignoring the relay traffic cost*/
		double cost_lower_bound = 0;
		for (size_t i = 0; i < session_clients.size(); i++)
		{
			if (i < k) // previous 0'th to (k - 1)'th assigned clients
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(client_assignment.at(i)).external_bandwidth_price; // use client_assignment.at(i)
			}
			else if (k == i) // the k'th client
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(d_k).external_bandwidth_price; // use d_k
			}
			else // future unassigned clients
			{
				cost_lower_bound += session_clients.at(i).incoming_data_amount * global.datacenter.at(session_clients.at(i).cheapest_dc).external_bandwidth_price; // use cheapest_dc
			}
		}
		return (cost_lower_bound < traffic_cost);
	}

	/*a recursive function, assign one client in each call (for constraint satisfaction problem)*/
	void SimulationBase::AssignClient_ConstraintSatisfaction4Delay(vector<Client>& session_clients, const size_t k, vector<ID>& client_assignment, const size_t solution_cardinality_constraint)
	{
		/*go through every value in this variable's domain*/
		for (auto dc : session_clients.at(k).dc_domain)
		{
			if (num_discovered_solutions > 0)
			{
				return; // return once finding a complete solution -> feasible
			}

			if (IsValidPartialSolution(session_clients, k, client_assignment, dc, solution_cardinality_constraint))
			{
				client_assignment.at(k) = dc; // assign dc to k'th client

				if (k == (session_clients.size() - 1))
				{
					num_discovered_solutions++; // found a complete solution and count it				
				}
				else
				{
					AssignClient_ConstraintSatisfaction4Delay(session_clients, k + 1, client_assignment, solution_cardinality_constraint); // continue on extending the partial solution
				}
			}
		}
	}

	/*CP (constraint satisfaction problem)*/
	void SimulationBase::ConstraintSatisfaction4Delay(vector<Client>& session_clients, vector<ID>& client_assignment, const size_t solution_cardinality_constraint)
	{
		/*initialize every client's dc_domain*/
		InitializeDomains4Clients(session_clients);

		/*check if achieved_delay_bound is valid*/
		if (EnforceLocalConsistency(session_clients)) // each client's dc_domain will be modified (reduced)
		{
			std::sort(session_clients.begin(), session_clients.end(), ClientComparator_ByDomainSize); // sorting can improve the efficiency substantially
			AssignClient_ConstraintSatisfaction4Delay(session_clients, 0, client_assignment, solution_cardinality_constraint); // begin with the first client (i.e., the 0'th client)
		}
	}

	/*must run after running ConstraintSatisfaction4Delay()*/
	void SimulationBase::ConstrainedOptimization4Cost(vector<Client>& session_clients, vector<ID>& client_assignment, const size_t solution_cardinality_constraint)
	{
		/*this is required by IsWorthyExtension()*/
		for (auto& c : session_clients)
		{
			c.cheapest_dc = c.dc_domain.front();
			for (auto it : c.dc_domain)
			{
				if (global.datacenter.at(it).external_bandwidth_price < global.datacenter.at(c.cheapest_dc).external_bandwidth_price)
				{
					c.cheapest_dc = it;
				}
			}
		}

		/*find the optimal assignment*/
		double traffic_cost;
		vector<ID> optimal_client_assignment;
		IsWorthyExtension_false_counter = 0;
		IsValidPartialSolution_false_counter = 0;
		num_discovered_solutions = 0; // important
		AssignClient_ConstrainedOptimization4Cost(session_clients, 0, client_assignment, solution_cardinality_constraint, traffic_cost, optimal_client_assignment);

		/*store the result for external use*/
		client_assignment = optimal_client_assignment;
	}

	/*a recursive function, work on each client at each call (for constraint optimization problem)
	must run after AssignClient_ConstraintSatisfaction4Delay()*/
	void SimulationBase::AssignClient_ConstrainedOptimization4Cost(vector<Client>& session_clients, const size_t k, vector<ID>& client_assignment, const size_t solution_cardinality_constraint, double& traffic_cost, vector<ID>& optimal_client_assignment)
	{
		for (auto dc : session_clients.at(k).dc_domain)
		{
			if (IsValidPartialSolution(session_clients, k, client_assignment, dc, solution_cardinality_constraint))
			{
				if (IsWorthyExtension(session_clients, k, client_assignment, dc, traffic_cost))
				{
					client_assignment.at(k) = dc; // assignment for k'th variable

					if (k == (session_clients.size() - 1)) // a complete solution
					{
						num_discovered_solutions++; // count this solution

						/*make sure they will only be initialized by the first solution*/
						if (1 == num_discovered_solutions)
						{
							traffic_cost = GetSessionCostAfterAssignment(session_clients, client_assignment);
							optimal_client_assignment = client_assignment;
						}

						/*choose the assignment with lower cost*/
						double this_cost = GetSessionCostAfterAssignment(session_clients, client_assignment);
						if (this_cost < traffic_cost)
						{
							traffic_cost = this_cost;
							optimal_client_assignment = client_assignment;
						}
					}
					else // continue on extending the partial solution
					{
						AssignClient_ConstrainedOptimization4Cost(session_clients, k + 1, client_assignment, solution_cardinality_constraint, traffic_cost, optimal_client_assignment);
					}
				}
				else
				{
					IsWorthyExtension_false_counter++;
				}
			}
			else
			{
				IsValidPartialSolution_false_counter++;
			}
		}
	}	

	void OptimizationProblem::OutputAssignmentOfOneSession(const int session_id, const vector<Client>& session_clients)
	{
		if (!output_assignment) return;

		auto assignment_directory = global.output_directory + "Assignment\\";
		_mkdir(assignment_directory.c_str());
		string data_file_name = assignment_directory + "SessionSize-" + std::to_string((int)global.sim_setting.session_size) + "_SessionID-" + std::to_string(session_id) + "_" + alg_to_run + ".csv";
		ofstream data_file(data_file_name);
		if (data_file.is_open())
		{
			data_file << "client_id,client_name,assigned_dc_id,assigned_dc_name";
			for (const auto& c : session_clients)
			{
				data_file << "\n";
				data_file << c.id << "," << c.name << "," << c.assigned_dc << "," << global.datacenter.at(c.assigned_dc).name;
			}

			data_file.close();
		}
		else
		{
			std::cout << "\nfailed to create " << data_file_name << "\n";
			std::cin.get();
		}
	}

	void OptimizationProblem::OutputPerformanceData()
	{
		/*display all statistics to console*/
		/*std::cout << " | | | session_satisfaction_rate:\t" << (1.0 - GetRatioOfGreaterThan(achieved_delay_bound_all_sessions, global.sim_setting.maximum_allowed_delay_bound)) << "\n";*/
		std::cout << " | | | achieved_delay_bound_avg:\t" << GetMeanValue(achieved_delay_bound_all_sessions) << "\n";
		std::cout << " | | | achieved_delay_bound_99th:\t" << GetPercentile(achieved_delay_bound_all_sessions, 99) << "\n";
		std::cout << " | | | traffic_cost_avg:\t" << GetMeanValue(traffic_cost_all_sessions) << "\n";
		std::cout << " | | | traffic_cost_99th:\t" << GetPercentile(traffic_cost_all_sessions, 99) << "\n";
		std::cout << " | | | interDC_cost_ratio_avg:\t" << GetMeanValue(interDC_cost_ratio_all_sessions) << "\n";
		std::cout << " | | | interDC_cost_ratio_99th:\t" << GetPercentile(interDC_cost_ratio_all_sessions, 99) << "\n";
		std::cout << " | | | num_chosen_DCs_avg:\t" << GetMeanValue(num_of_chosen_DCs_all_sessions) << "\n";
		std::cout << " | | | num_chosen_DCs_99th:\t" << GetPercentile(num_of_chosen_DCs_all_sessions, 99) << "\n";
		std::cout << " | | | num_discovered_solutions_avg: \t" << GetMeanValue(num_discovered_solutions_all_sessions) << "\n";
		std::cout << " | | | num_discovered_solutions_99th:\t" << GetPercentile(num_discovered_solutions_all_sessions, 99) << "\n";
		std::cout << " | | | alg_running_time_avg:\t" << GetMeanValue(alg_running_time_all_sessions) << "\n";

		/*dump all statistics to disk*/
		auto data_file_name = global.output_directory + "Statistics_" + "SessionSize-" + std::to_string((int)global.sim_setting.session_size) + "_" + alg_to_run + ".csv";
		ofstream data_file(data_file_name);
		if (data_file.is_open())
		{
			/*data_file << "session_satisfaction_rate," << (1.0 - GetRatioOfGreaterThan(achieved_delay_bound_all_sessions, global.sim_setting.maximum_allowed_delay_bound)) << "\n";*/
			data_file << "achieved_delay_bound_avg," << GetMeanValue(achieved_delay_bound_all_sessions) << "\n";
			data_file << "achieved_delay_bound_99th," << GetPercentile(achieved_delay_bound_all_sessions, 99) << "\n";
			data_file << "traffic_cost_avg," << GetMeanValue(traffic_cost_all_sessions) << "\n";
			data_file << "traffic_cost_99th," << GetPercentile(traffic_cost_all_sessions, 99) << "\n";
			data_file << "interDC_cost_ratio_avg," << GetMeanValue(interDC_cost_ratio_all_sessions) << "\n";
			data_file << "interDC_cost_ratio_99th," << GetPercentile(interDC_cost_ratio_all_sessions, 99) << "\n";
			data_file << "num_chosen_DCs_avg," << GetMeanValue(num_of_chosen_DCs_all_sessions) << "\n";
			data_file << "num_chosen_DCs_99th," << GetPercentile(num_of_chosen_DCs_all_sessions, 99) << "\n";
			data_file << "num_discovered_solutions_avg," << GetMeanValue(num_discovered_solutions_all_sessions) << "\n";
			data_file << "num_discovered_solutions_99th," << GetPercentile(num_discovered_solutions_all_sessions, 99) << "\n";
			data_file << "alg_running_time_avg," << GetMeanValue(alg_running_time_all_sessions) << "\n";

			data_file.close();
		}
		else
		{
			std::cout << "\nfailed to create " << data_file_name << "\n";
			std::cin.get();
		}

		/*dump all details to disk*/
		data_file_name = global.output_directory + "Details_" + "SessionSize-" + std::to_string((int)global.sim_setting.session_size) + "_" + alg_to_run + ".csv";
		data_file.open(data_file_name);
		if (data_file.is_open())
		{
			data_file << "session_id,normalized_delay_bound,traffic_cost,interDC_cost_ratio,num_chosen_DCs,alg_running_time";
			data_file << "\n";
			for (int i = 0; i < global.sim_setting.session_count; i++)
			{
				data_file << (i + 1) << ",";
				data_file << achieved_delay_bound_all_sessions.at(i) << ",";
				data_file << traffic_cost_all_sessions.at(i) << ",";
				data_file << interDC_cost_ratio_all_sessions.at(i) << ",";
				data_file << num_of_chosen_DCs_all_sessions.at(i) << ",";
				data_file << alg_running_time_all_sessions.at(i);
				data_file << "\n";
			}
			data_file.close();
		}
		else
		{
			std::cout << "\nfailed to create " << data_file_name << "\n";
			std::cin.get();
		}
	}

	/*minimize the delay (primary) and then cost (secondary)*/
	void OptimizationProblem::Run()
	{
		/*reset those performance stuff for this algorithm*/
		ResetPerformanceDataStorage();

		/*iterating through sessions one by one*/
		int session_id = 0;
		unordered_set<int> sessions_to_check = {};
		for (auto session_clients : global.all_sessions)
		{
			session_id++;

			/*****************************************************************************/
			/******** setup local stuff for current session based on global stuff ********/
			
			/*setup this session's assignment solution*/
			vector<ID> client_assignment; // each element is the id of the assigned dc of the client with the same position in session_clients
			client_assignment.assign(session_clients.size(), 0); // initialization

			/*setup paths between clients and find shortest paths for this session*/
			FindShortestPaths(session_clients, global.dc_id_list);			

			auto theoretical_lower_bound = GetSessionDelayLowerBound(session_clients);

			/*****************************************************************************/
			/*************** determine the minimum achievable delay bound ****************/

			auto algorithm_start_time = clock();
						
			path_length_constraint = theoretical_lower_bound;

			if (alg_to_run.find("CP") != string::npos) // CP
			{
				size_t solution_cardinality_constraint;
				if ("CP" == alg_to_run)	solution_cardinality_constraint = 0;
				else if ("CP-1" == alg_to_run) solution_cardinality_constraint = 1;
				else if ("CP-2" == alg_to_run) solution_cardinality_constraint = 2;
				else if ("CP-3" == alg_to_run) solution_cardinality_constraint = 3;
				else if ("CP-4" == alg_to_run) solution_cardinality_constraint = 4;
				else if ("CP-5" == alg_to_run) solution_cardinality_constraint = 5;
				else if ("CP-6" == alg_to_run) solution_cardinality_constraint = 6;

				num_discovered_solutions = 0; // critical
				while (true)
				{
					ConstraintSatisfaction4Delay(session_clients, client_assignment, solution_cardinality_constraint);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
				ConstrainedOptimization4Cost(session_clients, client_assignment, solution_cardinality_constraint);
			}
			else if (alg_to_run.find("NA-all") != string::npos) // NA-all
			{
				size_t k;
				if ("NA-all-1" == alg_to_run) k = 1;
				else if ("NA-all-2" == alg_to_run) k = 2;
				else if ("NA-all-3" == alg_to_run) k = 3;
				else if ("NA-all-4" == alg_to_run) k = 4;
				else if ("NA-all-5" == alg_to_run) k = 5;

				num_discovered_solutions = 0; // critical
				while (true)
				{
					NA_all(session_clients, client_assignment, k);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
				NA_all_CostOptimization(session_clients, client_assignment, k);
			}
			else if ("NA-sub" == alg_to_run) // NA-sub
			{
				num_discovered_solutions = 0; // critical
				while (true)
				{
					NA_sub(session_clients, client_assignment);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
				NA_sub_CostOptimization(session_clients, client_assignment);
			}
			else // wrong alg name
			{
				std::cout << "\nincorrect algorithm name\n";
				std::cin.get();
			}

			alg_running_time = difftime(clock(), algorithm_start_time);

			/*record performance metrics for this session*/
			//achieved_delay_bound_all_sessions.push_back(achieved_delay_bound);
			achieved_delay_bound_all_sessions.push_back(path_length_constraint / theoretical_lower_bound);
			traffic_cost_all_sessions.push_back(GetSessionCostAfterAssignment(session_clients, client_assignment));
			interDC_cost_ratio_all_sessions.push_back(interDC_cost_ratio);
			num_of_chosen_DCs_all_sessions.push_back(num_of_chosen_DCs);
			num_discovered_solutions_all_sessions.push_back(num_discovered_solutions);
			alg_running_time_all_sessions.push_back(alg_running_time);

			/*output assignment details for some selected sessions*/
			if (!sessions_to_check.empty() && sessions_to_check.find(session_id) != sessions_to_check.end())
			{
				for (int i = 0; i < session_clients.size(); i++)
				{
					session_clients.at(i).assigned_dc = client_assignment.at(i);
				}
				sort(session_clients.begin(), session_clients.end());
				OutputAssignmentOfOneSession(session_id, session_clients);
			}
			else if (sessions_to_check.empty())
			{
				for (int i = 0; i < session_clients.size(); i++)
				{
					session_clients.at(i).assigned_dc = client_assignment.at(i);
				}
				sort(session_clients.begin(), session_clients.end());
				OutputAssignmentOfOneSession(session_id, session_clients);
			}
		}

		/*output aggregated performance metrics*/
		OutputPerformanceData();
	}

	/*minimize cost subject to delay bound*/
	void OptimizationProblem::Run_CostOptimization(const string alg_to_get_delay_bound)
	{
		/*reset those performance stuff for this algorithm*/
		ResetPerformanceDataStorage();

		/*iterating through sessions one by one*/
		int session_id = 0;
		unordered_set<int> sessions_to_check = {};
		for (auto session_clients : global.all_sessions)
		{
			session_id++;

			/*****************************************************************************/
			/******** setup local stuff for current session based on global stuff ********/
			
			/*setup this session's assignment solution*/
			vector<ID> client_assignment; // each element is the id of the assigned dc of the client with the same position in session_clients
			client_assignment.assign(session_clients.size(), 0); // initialization

			/*setup paths between clients and find shortest paths for this session*/
			FindShortestPaths(session_clients, global.dc_id_list);

			auto theoretical_lower_bound = GetSessionDelayLowerBound(session_clients);

			/*****************************************************************************/

			auto algorithm_start_time = clock();

			path_length_constraint = theoretical_lower_bound;

			if (alg_to_get_delay_bound.find("CP") != string::npos) // CP
			{
				size_t solution_cardinality_constraint;
				if ("CP" == alg_to_get_delay_bound)	solution_cardinality_constraint = global.datacenter.size();
				else if ("CP-1" == alg_to_get_delay_bound) solution_cardinality_constraint = 1;
				else if ("CP-2" == alg_to_get_delay_bound) solution_cardinality_constraint = 2;
				else if ("CP-3" == alg_to_get_delay_bound) solution_cardinality_constraint = 3;
				else if ("CP-4" == alg_to_get_delay_bound) solution_cardinality_constraint = 4;
				else if ("CP-5" == alg_to_get_delay_bound) solution_cardinality_constraint = 5;
				else if ("CP-6" == alg_to_get_delay_bound) solution_cardinality_constraint = 6;

				num_discovered_solutions = 0; // critical
				while (true)
				{
					ConstraintSatisfaction4Delay(session_clients, client_assignment, solution_cardinality_constraint);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
			}
			else if (alg_to_get_delay_bound.find("NA-all") != string::npos)
			{
				size_t k;
				if ("NA-all-1" == alg_to_get_delay_bound) k = 1;
				else if ("NA-all-2" == alg_to_get_delay_bound) k = 2;
				else if ("NA-all-3" == alg_to_get_delay_bound) k = 3;
				else if ("NA-all-4" == alg_to_get_delay_bound) k = 4;
				else if ("NA-all-5" == alg_to_get_delay_bound) k = 5;

				num_discovered_solutions = 0; // critical
				while (true)
				{
					NA_all(session_clients, client_assignment, k);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
			}
			else if ("NA-sub" == alg_to_get_delay_bound)
			{
				num_discovered_solutions = 0; // critical
				while (true)
				{
					NA_sub(session_clients, client_assignment);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
			}
			else
			{
				std::cout << "\nincorrect alg_to_get_delay_bound\n";
				std::cin.get();
			}

			if (alg_to_run.find("CP") != string::npos) // CP
			{
				size_t solution_cardinality_constraint;
				if ("CP" == alg_to_run)	solution_cardinality_constraint = 0;
				else if ("CP-1" == alg_to_run) solution_cardinality_constraint = 1;
				else if ("CP-2" == alg_to_run) solution_cardinality_constraint = 2;
				else if ("CP-3" == alg_to_run) solution_cardinality_constraint = 3;
				else if ("CP-4" == alg_to_run) solution_cardinality_constraint = 4;
				else if ("CP-5" == alg_to_run) solution_cardinality_constraint = 5;
				else if ("CP-6" == alg_to_run) solution_cardinality_constraint = 6;

				num_discovered_solutions = 0; // critical
				while (true)
				{
					ConstraintSatisfaction4Delay(session_clients, client_assignment, solution_cardinality_constraint);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
				ConstrainedOptimization4Cost(session_clients, client_assignment, solution_cardinality_constraint);
			}
			else if (alg_to_run.find("NA-all") != string::npos) // NA-all
			{
				size_t k;
				if ("NA-all-1" == alg_to_run) k = 1;
				else if ("NA-all-2" == alg_to_run) k = 2;
				else if ("NA-all-3" == alg_to_run) k = 3;
				else if ("NA-all-4" == alg_to_run) k = 4;
				else if ("NA-all-5" == alg_to_run) k = 5;

				num_discovered_solutions = 0; // critical
				while (true)
				{
					NA_all(session_clients, client_assignment, k);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
				NA_all_CostOptimization(session_clients, client_assignment, k);
			}
			else if ("NA-sub" == alg_to_run) // NA-sub
			{
				num_discovered_solutions = 0; // critical
				while (true)
				{
					NA_sub(session_clients, client_assignment);
					if (num_discovered_solutions > 0) { num_discovered_solutions = 0; break; }
					path_length_constraint++;
				}
				NA_sub_CostOptimization(session_clients, client_assignment);
			}
			else // wrong alg name
			{
				std::cout << "\nincorrect algorithm name\n";
				std::cin.get();
			}

			alg_running_time = difftime(clock(), algorithm_start_time);

			/*****************************************************************************/

			/*record performance metrics for this session*/
			//achieved_delay_bound_all_sessions.push_back(achieved_delay_bound);
			achieved_delay_bound_all_sessions.push_back(path_length_constraint / theoretical_lower_bound);
			traffic_cost_all_sessions.push_back(GetSessionCostAfterAssignment(session_clients, client_assignment));
			interDC_cost_ratio_all_sessions.push_back(interDC_cost_ratio);
			num_of_chosen_DCs_all_sessions.push_back(num_of_chosen_DCs);
			num_discovered_solutions_all_sessions.push_back(num_discovered_solutions);
			alg_running_time_all_sessions.push_back(alg_running_time);

			/*output assignment details for some selected sessions*/
			if (!sessions_to_check.empty() && sessions_to_check.find(session_id) != sessions_to_check.end())
			{
				for (int i = 0; i < session_clients.size(); i++)
				{
					session_clients.at(i).assigned_dc = client_assignment.at(i);
				}
				sort(session_clients.begin(), session_clients.end());
				OutputAssignmentOfOneSession(session_id, session_clients);
			}
			else if (sessions_to_check.empty())
			{
				for (int i = 0; i < session_clients.size(); i++)
				{
					session_clients.at(i).assigned_dc = client_assignment.at(i);
				}
				sort(session_clients.begin(), session_clients.end());
				OutputAssignmentOfOneSession(session_id, session_clients);
			}
		}

		/*output aggregated performance metrics*/
		OutputPerformanceData();
	}

	//void Simulation::Random(vector<ID>& client_assignment)
	//{
	//	auto shuffled_dc_list = global.dc_id_list; // avoid shuffling the global.dc_id_list
	//	for (size_t i = 0; i < client_assignment.size(); i++)
	//	{
	//		random_shuffle(shuffled_dc_list.begin(), shuffled_dc_list.end());
	//		client_assignment.at(i) = shuffled_dc_list.front();
	//	}
	//}

	void OptimizationProblem::NA_all(vector<Client>& session_clients, vector<ID>& client_assignment, const size_t k)
	{
		if (k < 1 || k > global.datacenter.size())
		{
			cout << "error in NA_all(): invalid k\n";
			cin.get();
		}
		else
		{
			/*initialize the domain of each client to its k nearest dc's*/
			for (auto& c : session_clients)
			{
				RankDatacenters4Client(c, global.dc_id_list);
				c.dc_domain.assign(c.ranked_dc_list.begin(), c.ranked_dc_list.begin() + k);
			}

			/*apply constraint propogation and backtrack search*/
			if (EnforceLocalConsistency(session_clients))
			{
				AssignClient_ConstraintSatisfaction4Delay(session_clients, 0, client_assignment, global.datacenter.size());
			}
		}
	}

	/*must run after NA_all()*/
	void OptimizationProblem::NA_all_CostOptimization(vector<Client>& session_clients, vector<ID>& client_assignment, const size_t k)
	{
		if (k < 1 || k > global.datacenter.size())
		{
			cout << "error in NA_all(): invalid k\n";
			cin.get();
		}
		else
		{
			/*this is required by IsWorthyExtension()*/
			for (auto& c : session_clients)
			{
				c.cheapest_dc = c.dc_domain.front();
				for (auto it : c.dc_domain)
				{
					if (global.datacenter.at(it).external_bandwidth_price < global.datacenter.at(c.cheapest_dc).external_bandwidth_price)
					{
						c.cheapest_dc = it;
					}
				}
			}

			/*find the optimal assignment*/
			double traffic_cost;
			vector<ID> optimal_client_assignment;
			IsWorthyExtension_false_counter = 0;
			IsValidPartialSolution_false_counter = 0;
			num_discovered_solutions = 0; // important
			AssignClient_ConstrainedOptimization4Cost(session_clients, 0, client_assignment, global.datacenter.size(), traffic_cost, optimal_client_assignment);

			/*store the result for external use*/
			client_assignment = optimal_client_assignment;
		}
	}

	void OptimizationProblem::NA_sub(vector<Client>& session_clients, vector<ID>& client_assignment)
	{
		/*iterating through subsets (break once found)*/
		for (const auto& dc_subset : global.all_dc_subsets)
		{
			/*assign each client to its nearest dc in the subset*/
			for (size_t i = 0; i < session_clients.size(); i++)
			{
				RankDatacenters4Client(session_clients.at(i), dc_subset);
				client_assignment.at(i) = session_clients.at(i).nearest_dc;
			}

			/*check if achieved_delay_bound is valid*/
			if (GetSessionDelayAfterAssignment(session_clients, client_assignment) <= path_length_constraint)
			{
				num_discovered_solutions++;
				break;
			}
		}
	}

	void OptimizationProblem::NA_sub_CostOptimization(vector<Client>& session_clients, vector<ID>& client_assignment)
	{
		num_discovered_solutions = 0; // important

									 /*stuff to store temporary optimal assignment and the corresponding cost*/
		vector<ID> optimal_client_assignment;
		double traffic_cost;

		/*iterating through all subsets (no break)*/
		for (const auto& dc_subset : global.all_dc_subsets)
		{
			/*construct an assignment*/
			for (size_t i = 0; i < session_clients.size(); i++)
			{
				RankDatacenters4Client(session_clients.at(i), dc_subset);
				client_assignment.at(i) = session_clients.at(i).nearest_dc;
			}

			/*only consider valid assignment*/
			if (GetSessionDelayAfterAssignment(session_clients, client_assignment) <= path_length_constraint)
			{
				num_discovered_solutions++; // count it

										   /*ensure it will only be initialized by the first valid solution*/
				if (1 == num_discovered_solutions)
				{
					traffic_cost = GetSessionCostAfterAssignment(session_clients, client_assignment);
					optimal_client_assignment = client_assignment;
				}

				/*choose the assignment with lower cost*/
				double this_cost = GetSessionCostAfterAssignment(session_clients, client_assignment);
				if (this_cost < traffic_cost)
				{
					optimal_client_assignment = client_assignment;
					traffic_cost = this_cost;
				}
			}
		}

		/*store the result for external use*/
		client_assignment = optimal_client_assignment;
	}
	
	void OptimizationProblem::ResetPerformanceDataStorage()
	{
		achieved_delay_bound_all_sessions.clear();
		traffic_cost_all_sessions.clear();
		interDC_cost_ratio_all_sessions.clear();
		num_of_chosen_DCs_all_sessions.clear();
		num_discovered_solutions_all_sessions.clear();
		alg_running_time_all_sessions.clear();
	}	
}