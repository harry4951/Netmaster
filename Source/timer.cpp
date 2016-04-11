#include "timer.h"

void TimerNet::threadStart()
{
    ScopedLock lock(sync);

    while (!done)
    {
        if (queue.empty())
        {
            // Wait (forever) for work
            wakeUp.wait(lock);
        }
        else
        {
            auto firstInstance = queue.begin();
            Instance& instance = *firstInstance;
            auto now = Clock::now();
            if (now >= instance.next)
            {
                queue.erase(firstInstance);

                // Mark it as running to handle racing destroy
                instance.running = true;

                // Call the handler
                lock.unlock();
                instance.handler();
                lock.lock();

                if (done)
                {
                    break;
                }
                else if (!instance.running)
                {
                    // Running was set to false, destroy was called
                    // for this Instance while the callback was in progress
                    // (this thread was not holding the lock during the callback)
                    active.erase(instance.id);
                }
                else
                {
                    instance.running = false;

                    // If it is periodic, schedule a new one
                    if (instance.period.count() > 0)
                    {
                        instance.next = instance.next + instance.period;
                        queue.insert(instance);
                    } else {
                        active.erase(instance.id);
                    }
                }
            } else {
                // Wait until the timer is ready or a timer creation notifies
                wakeUp.wait_until(lock, instance.next);
            }
        }
    }
}

TimerNet::TimerNet()
    : nextId(1)
    , queue(comparator)
    , done(false)
{
    ScopedLock lock(sync);
    worker = std::thread(std::bind(&TimerNet::threadStart, this));
}

TimerNet::~TimerNet()
{
    ScopedLock lock(sync);
    done = true;
    wakeUp.notify_all();
    lock.unlock();
    worker.join();
}

TimerNet::timer_id TimerNet::create(uint64_t msFromNow, uint64_t msPeriod,
        const std::function<void()> &handler)
{
    return createImpl(Instance(0,
            Clock::now() + Duration(msFromNow), Duration(msPeriod),
            handler));
}

TimerNet::timer_id TimerNet::create(uint64_t msFromNow, uint64_t msPeriod,
        std::function<void()>&& handler)
{
    return createImpl(Instance(0,
            Clock::now() + Duration(msFromNow), Duration(msPeriod),
            std::move(handler)));
}

TimerNet::timer_id TimerNet::createImpl(Instance&& item)
{
    ScopedLock lock(sync);
    item.id = nextId++;
    auto iter = active.emplace(item.id, std::move(item));
    queue.insert(iter.first->second);
    wakeUp.notify_all();
    return item.id;
}

bool TimerNet::destroy(timer_id id)
{
    ScopedLock lock(sync);
    auto i = active.find(id);
    if (i == active.end())
        return false;
    else if (i->second.running)
    {
        // A callback is in progress for this Instance,
        // so flag it for deletion in the worker
        i->second.running = false;
    }
    else
    {
        queue.erase(std::ref(i->second));
        active.erase(i);
    }

    wakeUp.notify_all();
    return true;
}

bool TimerNet::exists(timer_id id)
{
    ScopedLock lock(sync);
    return active.find(id) != active.end();
}
