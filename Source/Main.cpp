/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"


/**
 * Test asynchronous timer class from implementation found in:
 *  http://codereview.stackexchange.com/questions/40473/portable-periodic-one-shot-timer-implementation
 *
 *  BUILD instructions with the provided CMakeLists.txt:
 *      mkdir build
 *      cd build
 *      cmake ..
 *      make
 *
 */
#include <cstdio>
#include <chrono>
#include <thread>

#include "timer.h"

using namespace std::chrono_literals;

class TimerTest
{
public:
    TimerTest() :
        _count(0),
        _is_timer_running(false),
        _last_timer_started(0)
    {
        _callback_handle = std::bind(&TimerTest::timer_callback, this);
    }

    ~TimerTest()
    {
    }

    void main_loop()
    {
        while (true)
        {
            // Start a one-shot timer that will trigger one second from now
            // if it is not already started
            if (! _is_timer_running)
            {
                _count++;
                _last_timer_started = _timer.create(1000, 0, _callback_handle);
                _is_timer_running = true;
            }
            // Stop one timer every two to test that callback are not fired in that case
            if ((_count % 2) == 0)
            {
                _timer.destroy(_last_timer_started);
                _is_timer_running = false;
            }
            // Sleep a little to avoid 100% CPU usage from this thread
            std::this_thread::sleep_for(10ms);
        }

    }

    void timer_callback()
    {
        printf("Timer Fired for the %d time\n", _count);
        _is_timer_running = false;
    }

private:
    int _count;
    bool _is_timer_running;

    TimerNet _timer;
    TimerNet::timer_id _last_timer_started;
    std::function<void()> _callback_handle;

};

int main(int argc, char *argv[])
{
    TimerTest timer_test;
    timer_test.main_loop();

    return 0;
}

