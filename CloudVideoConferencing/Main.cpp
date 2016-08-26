#include "Simulation.h"

/*test code*/
//void main()
//{
//	/*vector<size_t> s = { 1, 2, 3, 4, 5 };
//	vector<bool> x;
//	x.assign(s.size(), false);
//	vector<vector<size_t>> all_dc_subsets;
//	GenerateAllSubsets(s, x, 0, all_dc_subsets);
//	std::sort(all_dc_subsets.begin(), all_dc_subsets.end(), SubsetComparatorBySize);
//	std::cout << all_dc_subsets.size() << " subsets in total\n";
//	for (auto one_subset : all_dc_subsets)
//	{
//		cout << "{ ";
//		for (auto one_dc : one_subset)
//		{			
//			cout << one_dc << " ";
//		}
//		cout << "}\n";
//	}*/
//
//	/*vector<int> l = { 3, 3, 1, 1, 2};
//	unordered_set<int> s(l.begin(), l.begin() + 4);
//	for (auto i : s) cout << i << " ";*/
//
//	/*vector<int> l = { 3, 3, 1, 1, 2 };
//	static int n;
//	bool n_initialized = false;
//	for (auto i : l)
//	{
//		if (!n_initialized)
//		{
//			n = i;
//			n_initialized = true;
//		}
//
//		if (i < n) n = i;
//	}
//	cout << n;*/
//
//	/*vector<int> v1 = { 0, 1, 2, 3 };
//	vector<int> v2(v1.begin(), v1.begin() + 3);
//	cout << (std::find(v2.begin(), v2.end(), 4) != v2.end());*/
//
//	//string s = "CP_1";
//	//cout << (s.find("0") == std::string::npos);
//}

/*check whether inter-dc links have shorter latencies than other links*/
//int main()
//{
//	auto t_start = clock();
//
//	auto sim = Simulation(Setting(0, 0, 0, 0, 0), ".\\Data\\");
//	sim.Initialize();
//	sim.CheckInterDatacenterLink();
//
//	std::cout << "\nrunning time: " << difftime(clock(), t_start) / 1000 << " seconds\n";
//	return 0;
//}

/*argc: number of strings in array argv (at least 1); 
argv[]: array of command-line argument strings (start from 1 because 0 is the function itself)*/
int main(int argc, char *argv[])
{	
	/*double recommended_delay_bound = 150;
	double maximum_allowed_delay_bound = 300;*/
	double bound_increment_stepsize = 1;
	double session_count = 1000;
	std::cout << "common_settings\n";
	/*std::cout << " | recommended_delay_bound: " << recommended_delay_bound << "\n";
	std::cout << " | max_allowed_delay_bound: " << maximum_allowed_delay_bound << "\n";*/
	std::cout << " | bound_increment_stepsize: " << bound_increment_stepsize << "\n";
	std::cout << " | session_count: " << session_count << "\n";
	
	for (auto session_size : { 12 })
	{
		std::cout << " | session_size: " << session_size << "\n";
		try
		{
			if (/*recommended_delay_bound < 1 || */session_size < 2 || session_count < 1)
			{
				throw "bad simulation parameters\n";
			}
						
			Simulation sim = Simulation(Setting(session_size, bound_increment_stepsize, session_count));
			sim.data_directory = ".\\Data\\";
			sim.client_dc_latency_file = "ping_to_prefix_median_matrix.csv";
			sim.output_directory = sim.data_directory + "Output\\";
			sim.cluster_by_subregion = true;
			sim.output_assignment = false;

			sim.Initialize();			
			for (auto alg_name : { "CP-1", "CP-2", "CP-3", "CP-4", "CP-5", "NA-all", "NA-sub"})
			{
				sim.alg_to_run = alg_name;
				std::cout << " | | alg_name: " << sim.alg_to_run << "\n";
				sim.Run();
			}
		}
		catch (exception& e) // standard exceptions
		{
			std::cerr << e.what() << "\n";
		}
		catch (const char* msg) // customized exceptions
		{
			std::cerr << msg;
		}
	}

	return 0;
}