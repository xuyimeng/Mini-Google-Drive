#ifndef TIMER_H
#define TIMER_H
#include <time.h>

class Timer {
	private:
		unsigned long begTime;
	public:
		void start();
		unsigned long elapsedTime();
		bool isTimeout(unsigned long seconds);
};

#endif // TIMER_H



