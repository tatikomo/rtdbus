#include <iostream>
#include <functional>
#include <queue>
#include <chrono>
#include <sys/time.h>   // for 'time_t' and 'struct timeval'
#include <unistd.h>     // for 'usleep'

/////////////////////////////////////////////////////////////////////////////////////////
// Callback один на все события, и для различия разных циклов в нем указан код цикла
/////////////////////////////////////////////////////////////////////////////////////////

namespace events
{
    struct event
    {
        typedef std::function<void(int,int)> callback_type;
        typedef std::chrono::time_point<std::chrono::system_clock> time_type;

        event(const callback_type &cb, const time_type &when, const int cid, const int sid)
            : callback_(cb), when_(when), cycle_id_(cid), sa_id_(sid)
            { }

        void operator()() const
            { callback_(cycle_id_, sa_id_); }

        callback_type callback_;
        time_type     when_;
        int           cycle_id_;    // Номер Цикла
        int           sa_id_;       // Номер СС
    };

    struct event_less : public std::less<event>
    {
            bool operator()(const event &e1, const event &e2) const
                {
                    return (e2.when_ < e1.when_);
                }
    };

    std::priority_queue<event, std::vector<event>, event_less> event_queue;

    void add(const event::callback_type &cb, const time_t &when)
    {
        auto real_when = std::chrono::system_clock::from_time_t(when);

        event_queue.emplace(cb, real_when, -1, -1);
    }

    void add(const event::callback_type &cb, const timeval &when)
    {
        auto real_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                         std::chrono::microseconds(when.tv_usec);

        event_queue.emplace(cb, real_when, -1, -1);
    }

    void add(const event::callback_type &cb,
             const std::chrono::time_point<std::chrono::system_clock> &when)
    {
        event_queue.emplace(cb, when, -1, -1);
    }

    void timer()
    {
        event::time_type now = std::chrono::system_clock::now();

        while (!event_queue.empty() &&
               (event_queue.top().when_ < now))
        {
            event_queue.top()();
            event_queue.pop();
        }
    }
}

/*
void foo()
{
    std::cout << "hello from foo\n";
}

void done()
{
    std::cout << "Done!\n";
}

struct bar
{
    void hello()
        { std::cout << "Hello from bar::hello\n"; }
};

int main()
{
  auto now = std::chrono::system_clock::now();
  bar b;

  events::add(foo, now + std::chrono::seconds(2));

  events::add(std::bind(&bar::hello, b), now + std::chrono::seconds(4));

  events::add(done, now + std::chrono::seconds(6));

  while (true)
  {
    usleep(10000);
    events::timer();
  }

  return 0;
}
*/

