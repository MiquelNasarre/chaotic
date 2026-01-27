#include "Timer.h"

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#elif defined __APPLE__
#include <mach/mach_time.h>
#include <time.h>
#else
#include <time.h>
#endif

long long Timer::freq_ = 0LL;

#ifdef _WIN32
bool Timer::precisionSleeper::_precise_period = false;
Timer::precisionSleeper Timer::precisionSleeper::helper;
#endif

/*
-------------------------------------------------------------------------------------------------------
Internal helpers
-------------------------------------------------------------------------------------------------------
*/

// Pushes current 'last_' into ring (most recent).

inline void Timer::push_last()
{
    if (size_)
        head_ = (head_ + 1u) % cap_;

    stamps_[head_] = last_;
    if (size_ < cap_)
        size_++;
}

// Converts tick delta to seconds by dividing by _freq.

inline float Timer::to_sec(long long dt)
{
    return (float)((double)dt / (double)freq_);
}

#ifdef _WIN32
// Reads QPC counter to obtain time.

inline long long Timer::qpc_now() {
    LARGE_INTEGER v;
    QueryPerformanceCounter(&v);
    return (long long)v.QuadPart;
}

// Reads frequency to be stored in _freq.

inline long long Timer::qpc_freq() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return (long long)f.QuadPart;
}
#endif

/*
-------------------------------------------------------------------------------------------------------
Constructors and destructors
-------------------------------------------------------------------------------------------------------
*/

// Constructor: Allocates the stamps_ and pushes to the first one

Timer::Timer()
{
    {
#ifdef _WIN32
        freq_ = qpc_freq();
        stamps_ = (long long*)::calloc(cap_, sizeof(long long));
        last_ = qpc_now();
#else
        freq_ = 1000000000LL; // ns/sec
        stamps_ = (long long*)::calloc(cap_, sizeof(long long));
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        last_ = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
#endif
        // seed first mark
        push_last();
    }
}

// Destructor: Frees the stamps_

Timer::~Timer()
{
    if (stamps_) 
        ::free(stamps_); 
}

/*
-------------------------------------------------------------------------------------------------------
User end Timer class functions
-------------------------------------------------------------------------------------------------------
*/

// Resets history and start new timing window.

void Timer::reset()
{
#ifdef _WIN32
    last_ = qpc_now();
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    last_ = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
#endif
    size_ = 0u;
    head_ = 0u;

    push_last();
}

// Pushes current time, returns seconds since previous mark.

float Timer::mark()
{
#ifdef _WIN32
    const long long old = last_;
    last_ = qpc_now();
#else
    const long long old = last_;
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    last_ = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
#endif
    push_last();
    return to_sec(last_ - old);
}

// Returns seconds since previous mark without modifying history.

float Timer::check()
{
#ifdef _WIN32
    const long long now = qpc_now();
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const long long now = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
#endif
    return to_sec(now - last_);
}

// Advances 'last' by elapsed time (shifts existing markers forward), 
// practically skipping the time since last mark, returns elapsed seconds.

float Timer::skip()
{
    const long long old = last_;
#ifdef _WIN32
    last_ = qpc_now();
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    last_ = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
#endif
    const long long dt = last_ - old;

    // shift existing markers forward by dt (preserve window without inserting)

    if (size_ > 0) 
    {
        // walk stored entries and add dt
        unsigned idx = head_;
        for (unsigned i = 0; i < size_; ++i) {
            stamps_[idx] += dt;
            idx = (idx + cap_ - 1u) % cap_; // move toward older
        }
    }

    return to_sec(dt);
}

// Returns the seconds since the oldest stored marker.
// If size_ < cap_ this will be the time since last reset.

float Timer::checkTotal()
{
    if (size_ < 1) return 0.0f;
    const unsigned oldest_idx = (head_ + cap_ - (size_ - 1u)) % cap_;
    const long long oldest_stamp = stamps_[oldest_idx];
#ifdef _WIN32
    const long long now = qpc_now();
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const long long now = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
#endif

    return to_sec(now - oldest_stamp);
}

// Returns average seconds per interval over the stored markers. 
// (0 if insufficient data, size_ < 2).

float Timer::average()
{
    if (size_ < 2) return 0.0f;
    // newest = stamps_[head_], oldest = walk back size_-1 steps
    const unsigned oldest_idx = (head_ + cap_ - (size_ - 1u)) % cap_;
    const long long span = stamps_[head_] - stamps_[oldest_idx];
    return to_sec(span) / (float)(size_ - 1u);
}

// Returns number of stored markers (size_).

unsigned int Timer::getSize()
{
    return size_;
}

// Sets the maximum size for the history ring.

void Timer::setMax(unsigned int max)
{
    if (max == 0u) max = 1u;
    // allocate new ring and copy newest-first up to max
    long long* fresh = (long long*)calloc(max, sizeof(long long));

    unsigned new_size = (size_ < max) ? size_ : max;
    unsigned idx = head_;
    for (unsigned i = 0; i < new_size; ++i)
    {
        fresh[new_size - 1 - i] = stamps_[idx];
        idx = (idx + cap_ - 1u) % cap_;
    }
    // replace
    if (stamps_) 
        free(stamps_);

    stamps_ = fresh;
    cap_ = max;
    size_ = new_size;
    head_ = new_size ? (new_size - 1u) : 0u;
}

/*
-------------------------------------------------------------------------------------------------------
Static sleep helpers
-------------------------------------------------------------------------------------------------------
*/

#ifdef _WIN32
// Calls TimeBeginPeriod(1) at the start of execution.

Timer::precisionSleeper::precisionSleeper()
{
    _precise_period = true;
    timeBeginPeriod(1);
}

// If _precise_period calls TimeEndPeriod(1) at the end of execution.

Timer::precisionSleeper::~precisionSleeper()
{
    if(_precise_period)
        timeEndPeriod(1);
}

// Allows for modification of sleep timer resolution during runtime.

void Timer::set_sleep_timer_resolution_1ms(bool enable)
{
    if (!enable && precisionSleeper::_precise_period)
    {
        timeEndPeriod(1);
        precisionSleeper::_precise_period = false;
    }
    else if (enable && !precisionSleeper::_precise_period)
    {
        timeBeginPeriod(1);
        precisionSleeper::_precise_period = true;
    }
}
#endif

// For Windows uses CreateWaitableTimer to create an object with specific
// lifetime and wait for it to end. For other OSes uses nanosleep.

void Timer::sleep_for_us(unsigned long long us)
{
#ifdef _WIN32
    HANDLE h = CreateWaitableTimerExW(nullptr, nullptr,
        CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

    if (!h) 
        return;

    LARGE_INTEGER due;  // relative, 100 ns units
    due.QuadPart = -(LONGLONG)(us * 10ULL);

    BOOL ok = SetWaitableTimer(h, &due, 0, nullptr, nullptr, FALSE);

    if (ok) 
        WaitForSingleObject(h, INFINITE);

    CloseHandle(h);
    return;
#else
    timespec req;
    req.tv_sec = (time_t)(us / 1000000ULL);
    req.tv_nsec = (long)((us % 1000000ULL) * 1000ULL);
    nanosleep(&req, nullptr);
#endif
}

// Waits for a given amount of milliseconds.

void Timer::sleep_for(unsigned long ms)
{
#ifdef _WIN32
    ::Sleep(ms); // coarse (~1ms guarded by precisionSleeper)
#else
    timespec req;
    req.tv_sec = (time_t)(ms / 1000UL);
    req.tv_nsec = (long)((ms % 1000UL) * 1000000L);
    nanosleep(&req, nullptr);
#endif
}

// Returns system time in nanoseconds as a 64-bit tick (monotonic).
// Perfect for precise time tracking and as seed for random variables.

unsigned long long Timer::get_system_time_ns()
{
#if defined(_WIN32)
    LARGE_INTEGER freq, ctr;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ctr);
    return (unsigned long long)((ctr.QuadPart * 1000000000ULL) / freq.QuadPart);
#elif defined(__APPLE__)
    // macOS mach_absolute_time
    mach_timebase_info_data_t tb;
    mach_timebase_info(&tb);
    unsigned long long t = mach_absolute_time();
    return (t * tb.numer) / tb.denom;
#else
    // POSIX
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ull + (unsigned long long)ts.tv_nsec;
#endif
}
