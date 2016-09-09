#include "Base.h"

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

vector<vector<string>> ReadDelimitedTextFileIntoVector(const string input_file_name, const char delimiter, const bool skip_first_row)
{
	vector<vector<string>> strings_read;

	fstream data_file(input_file_name);
	if (data_file.is_open())
	{
		string one_line;
		int row_counter = 0;
		while (std::getline(data_file, one_line))
		{
			row_counter++;
			if ((1 == row_counter) && skip_first_row) continue;
			stringstream one_line_stream(one_line);
			string one_string;
			vector<string> string_list;
			while (std::getline(one_line_stream, one_string, delimiter))
			{
				string_list.push_back(one_string);				
			}
			strings_read.push_back(string_list);
		}
		data_file.close();

		if (strings_read.empty() || strings_read.front().empty())
		{
			std::cout << "ERROR: " << input_file_name << " is empty!\n";
			cin.get();
		}
	}
	else
	{
		std::cout << "ERROR: cannot open " << input_file_name << "!\n";
		cin.get();
	}	

	return strings_read;
}

double GetSumValue(const vector<double> &v)
{
	if (v.empty()) return 0;

	double sum = 0;
	for (auto it : v)
	{
		sum += it;
	}

	return sum;
}

double GetMeanValue(const vector<double> &v)
{
	if (v.empty()) return 0;
	else return (GetSumValue(v) / v.size());
}

double GetStdValue(const vector<double> &v)
{
	if (v.empty()) return 0;

	double mean = GetMeanValue(v);
	vector<double> temp;
	for (auto it : v)
	{
		temp.push_back((it - mean) * (it - mean));
	}

	return sqrt(GetMeanValue(temp));
}

double GetMaxValue(const vector<double> &v)
{
	if (v.empty()) return 0;

	return *max_element(v.begin(), v.end());
}

double GetMinValue(const vector<double> &v)
{
	if (v.empty()) return 0;

	return *min_element(v.begin(), v.end());
}

double GetPercentile(vector<double> v, const double p)
{
	std::sort(v.begin(), v.end());
	int rank = (int)ceil((p / 100) * v.size()) - 1;
	return v.at(rank);
}

double GetRatioOfGreaterThan(const vector<double>& v, const double x)
{
	double counter = 0;
	for (auto i : v)
	{
		if (i > x) counter++;
	}
	return counter / v.size();
}

/*usage: GenerateAllSubsets(universe, x, 0, all_subsets)
sort all_subsets if necessary*/
void GenerateAllSubsets(const vector<size_t>& universe, vector<bool>& x, size_t k, vector<vector<size_t>>& all_subsets)
{
	const auto the_last = x.size() - 1;
	for (bool x_value : { false, true })
	{
		x.at(k) = x_value;
		if (the_last == k)
		{
			vector<size_t> this_subset;
			for (size_t i = 0; i <= the_last; i++)
			{
				if (x.at(i))
				{
					this_subset.push_back(universe.at(i));
				}
			}
			if (!this_subset.empty()) 
			{
				all_subsets.push_back(this_subset); // the empty subset is excluded
			}
		}
		else
		{
			GenerateAllSubsets(universe, x, k + 1, all_subsets);
		}
	}
}

bool SubsetComparatorBySize(const vector<size_t>& x, const vector<size_t>& y)
{
	return (x.size() < y.size());
}