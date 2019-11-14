#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>

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
		
		int offset;
		unsigned long long ts;
		int cwnd;
		int action;
		int items = sscanf(buf, "%llu%d%d%n", &ts, &cwnd, &action, &offset);
		assert(items == 3);
		char* ptr = buf + offset;
		unsigned int value;
		printf("%llu %d %d", ts, cwnd, action);
		while (sscanf(ptr, "%u%n", &value, &offset) == 1)
		{
			ptr += offset;
			float fv = *((float*)&value);
			printf(" %.8e", fv);
		}
		printf("\n");
	}
	return 0;
}

