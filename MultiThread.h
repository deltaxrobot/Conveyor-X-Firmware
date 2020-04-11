#ifndef MULTI_THREAD_H
#define MULTI_THREAD_H

#include "Arduino.h"

class MultiThread
{
  public:
    MultiThread();
    void Schedule(uint16_t milliTime, void (*func)(uint8_t), uint8_t arg);
    void Schedule(uint16_t milliTime, void (*func)());
    bool isSchedule(uint16_t milliTime);
    bool isCount;
    uint16_t countValue;
    unsigned long Counter;
  private:
  
};

#define RUN_EVERY(Schedule, Time) {if( !Schedule.isSchedule(Time))\
                                    return;}

#endif
