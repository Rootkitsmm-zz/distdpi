#ifndef TIMER_HEADER
#define TIMER_HEADER

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <ctime>
#include <time.h>

#define NANO_SECOND_MULTIPLIER  1000000000

using namespace std;

class Timer {
public:
    Timer(float time) {
        t = time;
    };

    //~Timer();
    /* task to call execute call back
       when timer expires
     */
    void addTask(Timer *timer) {
        while(1) {
            const uint64_t INTERVAL_MS = t * NANO_SECOND_MULTIPLIER;
            timespec sleepValue = {0};
            sleepValue.tv_nsec = INTERVAL_MS;
	    if (INTERVAL_MS >= 1000000000ul) {
                sleepValue.tv_sec += INTERVAL_MS / (1000000000ul);
                sleepValue.tv_nsec = INTERVAL_MS%(1000000000ul);
            }

	    nanosleep(&sleepValue, NULL);
            timer->executeCb();
        }
    };
    /* update timer value */
    void updateTimer(float time) {
        t = time;
    }

protected:
    /* execute call back when timer expires */
    virtual void executeCb() = 0;

private:
    float t;

};
#endif
