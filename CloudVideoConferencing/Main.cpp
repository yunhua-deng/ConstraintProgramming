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

/*multi-threading*/
//int main(int argc, char *argv[])
//{
//	const size_t nloop = 11;
//
//	// Serial version
//	{
//		// Pre loop
//		std::cout << "serial:" << std::endl;
//		// loop over all items
//		for (int i = 0; i<nloop; i++)
//		{
//			// inner loop
//			{
//				const int j = i*i;
//				std::cout << j << std::endl;
//			}
//		}
//		// Post loop
//		std::cout << std::endl;
//	}
//
//	// Parallel version
//	// number of threads
//	const size_t nthreads = std::thread::hardware_concurrency();
//	{
//		// Pre loop
//		std::cout << "parallel (" << nthreads << " threads):" << std::endl;
//		std::vector<std::thread> threads(nthreads);
//		std::mutex critical;
//		for (int t = 0; t<nthreads; t++)
//		{
//			threads[t] = std::thread(std::bind(
//				[&](const int bi, const int ei, const int t)
//			{
//				// loop over all items
//				for (int i = bi; i<ei; i++)
//				{
//					// inner loop
//					{
//						const int j = i*i;
//						// (optional) make output critical
//						std::lock_guard<std::mutex> lock(critical);
//						std::cout << j << std::endl;
//					}
//				}
//			}, t*nloop / nthreads, (t + 1) == nthreads ? nloop : (t + 1)*nloop / nthreads, t));
//		}
//		std::for_each(threads.begin(), threads.end(), [](std::thread& x) {x.join(); });
//		// Post loop
//		std::cout << std::endl;
//	}
//}

using namespace CloudVideoConferencingProblem;

/*argc: number of strings in array argv (at least 1);
argv[]: array of command-line argument strings (start from 1 because 0 is the function name)*/
int main(int argc, char *argv[])
{	
	/*vector<thread> workers;
	for (size_t session_size = 4; session_size <= 10; session_size += 2)
	{
		workers.push_back(thread(RunSimulation_OptimizingLatencyFirst, Setting(session_size, 300, false)));
		workers.push_back(thread(RunSimulation_OptimizingLatencyFirst, Setting(session_size, 300, true)));
	}
	std::for_each(workers.begin(), workers.end(), [](thread &t) { t.join(); });*/

	RunSimulation_OptimizingLatencyFirst(Setting(8, 10, false));

	return 0;
}