#include <iostream>
#include <functional>
#include <queue>
#include <chrono>
#include <sys/time.h>   // for 'time_t' and 'struct timeval'
#include <unistd.h>     // for 'usleep'

/////////////////////////////////////////////////////////////////////////////////////////
// Callback один на все события, и для различия разных циклов в нем указан код цикла
/////////////////////////////////////////////////////////////////////////////////////////

namespace cycles
{
    struct event
    {
        typedef std::function<void(size_t,size_t)> callback_type;
        typedef std::chrono::time_point<std::chrono::system_clock> time_type;

        event(const callback_type &cb, const time_type &when, size_t cid, size_t sid)
            : callback_(cb), when_(when), cycle_id_(cid), sa_id_(sid)
            { }

        void operator()() const
            { callback_(cycle_id_, sa_id_); }

        callback_type callback_;
        time_type     when_;
        size_t        cycle_id_;    // Номер Цикла
        size_t        sa_id_;       // Номер СС
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

        event_queue.emplace(cb, real_when, 0, 0);
    }

    void add(const event::callback_type &cb, const timeval &when)
    {
        auto real_when = std::chrono::system_clock::from_time_t(when.tv_sec) +
                         std::chrono::microseconds(when.tv_usec);

        event_queue.emplace(cb, real_when, 0, 0);
    }

    void add(const event::callback_type &cb,
             const std::chrono::time_point<std::chrono::system_clock> &when)
    {
        event_queue.emplace(cb, when, 0, 0);
    }

    void clear()
    {
      while (!event_queue.empty())
      {
        event_queue.pop();
      }
    }

/*
    bool remove(const event& ev) {
      auto it = std::find(event_queue.begin(), event_queue.end(), ev);
      if (it != event_queue.end()) {
            event_queue.erase(it);
            std::make_heap(event_queue.begin(), event_queue.end(), event_less);
            return true;
      }
      else {
        return false;
      }
    }
*/

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
} // namespace events
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

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

  cycles::add(foo, now + std::chrono::seconds(2));

  cycles::add(std::bind(&bar::hello, b), now + std::chrono::seconds(4));

  cycles::add(done, now + std::chrono::seconds(6));

  while (true)
  {
    usleep(10000);
    cycles::timer();
  }

  return 0;
}
*/

