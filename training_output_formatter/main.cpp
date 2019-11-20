#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>

#include "take_action.h"
#include "../nn_params.h"
#include "cnpy/cnpy.h"

struct Record
{
	unsigned long long ts;
	int cwnd;
	int action;
	float ivs[NUM_FEATURES];
};

std::map<int, std::vector<Record> > all;

int get_expert_action(int cwnd, int expert_cwnd)
{
	int min_diff = 0x7fffffff;
	int min_diff_i = -1;
	for (int i = 0; i < NUM_ACTIONS; i++)
	{
		int new_cwnd = take_action(cwnd, i);
		if (abs(new_cwnd - expert_cwnd) < min_diff)
		{
			assert(new_cwnd > 0);
			min_diff = abs(new_cwnd - expert_cwnd);
			min_diff_i = i;
		}
	}
	assert(min_diff_i != -1);
	return min_diff_i;
}

int main(int argc, char** argv)
{
	if (argc != 4)
	{
		printf("Usage: ./format_output [time_series_len] [expert_cwnd] [path_to_store_formatted_output]\n");
		return 1;
	}
	size_t time_series_len = stoi(std::string(argv[1]));
	int expert_cwnd = stoi(std::string(argv[2]));
	std::string output_file = argv[3];	
	
	//FILE* p = fopen("b.txt", "r");
	FILE* p = fopen("/proc/indigo_training_output", "r");
	if (p == nullptr)
	{
		assert(!"Cannot open input file");
	}
	const size_t _n = 100000;
	size_t n = _n;
	char* buf = (char*)malloc(n);
	if (buf == nullptr)
	{
		assert(!"malloc OOM");
	}
	while (true)
	{
		ssize_t len = getline(&buf, &n, p);
		if (len == -1) break;
		assert(n == _n);
		assert(len < _n);
		
		int id;
		int offset;
		Record r;
		int items = sscanf(buf, "%d%llu%d%d%n", &id, &r.ts, &r.cwnd, &r.action, &offset);
		int num = 0;
		assert(items == 4);
		char* ptr = buf + offset;
		unsigned int value;
		// printf("%llu %d %d", ts, cwnd, action);
		while (sscanf(ptr, "%u%n", &value, &offset) == 1)
		{
			ptr += offset;
			float fv = *((float*)&value);
			assert(num < NUM_FEATURES);
			r.ivs[num] = fv;
			num++;
		}
		assert(num == NUM_FEATURES);
		all[id].push_back(r);
	}
	
	std::vector<float> all_input_vectors;
	std::vector<int> all_expert_actions;
	
	size_t num_records = 0;
	for (auto it = all.begin(); it != all.end(); it++)
	{
		{
			int len = (int)it->second.size();
			if (len < time_series_len)
			{
				printf("Warning: a trace has insufficient length %d, discarded (expert_cwnd = %d, output_file = %s)\n",
					   len, expert_cwnd, output_file.c_str());
				continue;
			}
		}
		
		num_records++;
		for (int k = 0; k < time_series_len; k++)
		{
			Record r = it->second[k];
			int expert_action = get_expert_action(r.cwnd, expert_cwnd);
			
			all_expert_actions.push_back(expert_action);
			for (int i = 0; i < NUM_FEATURES; i++)
			{
				all_input_vectors.push_back(r.ivs[i]);
			}
		}
	}
	
	assert(all_expert_actions.size() == num_records * time_series_len);
	assert(all_input_vectors.size() == num_records * time_series_len * NUM_FEATURES);
	
	printf("Processed %d time series. Writing output file..\n", (int)num_records);
	
	cnpy::npz_save(output_file.c_str(),
	               "expert_actions",
	               &all_expert_actions[0],
	               {num_records, time_series_len} /*shape*/,
	               "w" /*overwrite*/);
	               
	cnpy::npz_save(output_file.c_str(),
	               "input_vectors",
	               &all_input_vectors[0],
	               {num_records, time_series_len, NUM_FEATURES} /*shape*/,
	               "a" /*append*/);      
	               
	printf("Done.\n");
	          
	return 0;
}

