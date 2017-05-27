#include "PerfTimer.h"

PerfTimer::PerfTimer() {
	QueryPerformanceFrequency(&frequency);
}

PerfTimer::~PerfTimer() {
}

void PerfTimer::start() {
	QueryPerformanceCounter(&startingTime);
}

double PerfTimer::stop() {
	LARGE_INTEGER endingTime;
	QueryPerformanceCounter(&endingTime);
	LARGE_INTEGER time;
	time.QuadPart = endingTime.QuadPart - startingTime.QuadPart;
	return ((double)time.QuadPart * 1000.0 * 1000.0 / (double)frequency.QuadPart);
}
