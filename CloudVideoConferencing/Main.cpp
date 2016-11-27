#include "CloudVideoConferencing.h"

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
//
//	/*vector<int> v(3, 1);
//	list<int> l;
//	cout << v.size() << "\n";
//	l.assign(v.begin(), v.begin());
//	cout << l.size() << "\n";*/
//
//	srand(time(NULL));
//	vector<int> l({1, 2, 3, 4, 5});	
//	random_shuffle(l.begin(), l.end());
//}

using namespace CloudVideoConferencingProblem;
/*argc: number of strings in array argv (at least 1);
argv[]: array of command-line argument strings (start from 1 because 0 is the function name)*/
int main(int argc, char *argv[])
{	
	/*auto obj = DatasetAnalysis();	
	obj.Get_DelayToNearestDc_CDF();
	obj.Get_ShortestPathLength_CDF();*/
	
	auto instance_1 = OptimizingLatencyFirst();
	std::thread th_1 = std::thread(&OptimizingLatencyFirst::Simulate, &instance_1, Setting(4, 1000));
	auto instance_2 = OptimizingLatencyFirst();
	std::thread th_2 = std::thread(&OptimizingLatencyFirst::Simulate, &instance_2, Setting(8, 1000));
	auto instance_3 = OptimizingLatencyFirst();
	std::thread th_3 = std::thread(&OptimizingLatencyFirst::Simulate, &instance_3, Setting(12, 1000));
	auto instance_4 = OptimizingLatencyFirst();
	std::thread th_4 = std::thread(&OptimizingLatencyFirst::Simulate, &instance_4, Setting(16, 100));
	th_1.join();
	th_2.join();
	th_3.join();
	th_4.join();

	/*auto OC1 = OptimizingCostByTradingOffLatency();
	std::thread th_OC1 = std::thread(&OptimizingCostByTradingOffLatency::Simulate, &OC1, Setting(8, 500));
	auto OC2 = OptimizingCostByTradingOffLatency();
	std::thread th_OC2 = std::thread(&OptimizingCostByTradingOffLatency::Simulate, &OC2, Setting(12, 500));
	auto OC3 = OptimizingCostByTradingOffLatency();
	std::thread th_OC3 = std::thread(&OptimizingCostByTradingOffLatency::Simulate, &OC3, Setting(16, 500));
	th_1.join();
	th_2.join();
	th_3.join();*/

	return 0;
}