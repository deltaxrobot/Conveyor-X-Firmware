#include "MultiThread.h"

MultiThread::MultiThread()
{
  isCount = false;
}

void MultiThread::Schedule(uint16_t milliTime, void (*func)(uint8_t), uint8_t arg)
{
  if (isCount == false)
  {
    Counter = millis();
    isCount = true;
  }
  countValue = millis() - Counter;
  if ( countValue > milliTime )
  {
    func(arg);
    isCount = false;
  }  
}

void MultiThread::Schedule(uint16_t milliTime, void (*func)())
{
  if (isCount == false)
  {
    Counter = millis();
    isCount = true;
  }
  countValue = millis() - Counter;
  if ( countValue > milliTime )
  {
    func();
    isCount = false;
  }  
}

bool MultiThread::isSchedule(uint16_t milliTime)
{
  if (isCount == false)
  {
    Counter = millis();
    isCount = true;
  }
  countValue = millis() - Counter;
  if ( countValue > milliTime )
  {
    isCount = false;
    return true;
  }
  return false;
}

