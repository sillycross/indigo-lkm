#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>

const int input_vec_len = 9;
const int max_time_series_len = 1000;

struct Record
{
	unsigned long long ts;
	int cwnd;
	int action;
	float ivs[input_vec_len];
};

std::map<int, std::vector<Record> > all;

int main()
{
	FILE* p = fopen("/proc/indigo_training_output", "r");
	if (p == nullptr)
	{
		assert(!"Cannot open file");
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
			r.ivs[num] = fv;
			num++;
			assert(num <= input_vec_len);
		}
		assert(num == input_vec_len);
		all[id].push_back(r);
	}
	for (auto it = all.begin(); it != all.end(); it++)
	{
		int len = (int)it->second.size();
		len = std::min(len, max_time_series_len);
		printf("%d\n", len);
		for (int k = 0; k < len; k++)
		{
			Record r = it->second[k];
			printf("%llu %d %d", r.ts, r.cwnd, r.action);
			for (int i = 0; i < input_vec_len; i++)
			{
				printf(" %.8e", r.ivs[i]);
			}
			printf("\n");
		}
	}
	return 0;
}

