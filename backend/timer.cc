#include "timer.h"

void Timer::start() {
	begTime = clock();
}

unsigned long Timer::elapsedTime() {
	return ((unsigned long) clock() - begTime) / CLOCKS_PER_SEC;
}

bool Timer::isTimeout(unsigned long seconds) {
	return seconds >= elapsedTime();
}
