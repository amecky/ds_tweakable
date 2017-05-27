#pragma once
#include <Windows.h>

class PerfTimer {

public:
	PerfTimer();
	~PerfTimer();
	void start();
	double stop();
private:
	LARGE_INTEGER frequency;
	LARGE_INTEGER startingTime;
};

