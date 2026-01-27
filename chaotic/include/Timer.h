#pragma once

/* TIMER CLASS HEADER FILE
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This class is a handy multiplatform timer to be used by applications 
that require some type of timing. It produces precise timing using QPC 
for Windows and the system clock for other operating systems.

It has an array of times stored so that averages can be computed and 
it circles through it with a user set max length.

It also includes some sleeping functions. In Windows it sets the sleeping timer
resolution to 1ms by default, modifiable with set_sleep_timer_resolution_1ms().
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

#define DEFAULT_TIMER_CAP_ 60u // Default max number of markers for Timer

// Definition of the class, everything in this header is contained inside the class Timer.

// A timer object can be generated anywhere in your code, and will run independently of 
// the other timers. Upon construction the timer is reset and a first push is made.
class Timer {
private:

	// Time stamps history ring

	unsigned int cap_ = DEFAULT_TIMER_CAP_;		// MaxMarkers
	unsigned int size_ = 0u;					// Number of valid entries
	unsigned int head_ = 0u;					// Index of most-recent entry (when size_>0)
	long long* stamps_ = nullptr;				// Pointer to the markers in ticks

	// Tick measuring variables

	long long    last_ = 0LL;					// Last timestamp
	static long long freq_;						// high-res frequency (ticks per second)

	// Pushes current 'last_' into ring (most recent).
	inline void push_last();

	// Converts tick delta to seconds by dividing by _freq.
	static inline float to_sec(long long dt);

#ifdef _WIN32
	// Reads QPC counter to obtain time.
	static inline long long qpc_now();

	// Reads frequency to be stored in _freq.
	static inline long long qpc_freq();
#endif

public:
	// Copy functions are deleted. Copies not allowed

	Timer(const Timer&)				= delete;
	Timer& operator=(const Timer&)	= delete;

	// Constructor / Destructor.

	Timer();
	~Timer();

	// Resets history and start new timing window.
	void reset();

	// Pushes current time, returns seconds since previous mark.
	float mark();

	// Returns seconds since previous mark without modifying history.
	float check();

	// Advances 'last' by elapsed time (shifts existing markers forward), 
	// practically skipping the time since last mark, returns elapsed seconds.
	float skip();

	// Returns the seconds since the oldest stored marker.
	// If size_ < cap_ this will be the time since last reset.
	float checkTotal();

	// Returns average seconds per interval over the stored markers. 
	// (0 if insufficient data, size_ < 2).
	float average();

	// Returns number of stored markers (size_).
	unsigned int getSize();

	// Sets the maximum size for the history ring.
	void setMax(unsigned int max);


#ifdef _WIN32
private:
	// Static helper class calls TimeBeginPeriod(1) and TimeEndPeriod(1).
	class precisionSleeper
	{
	public:
		static bool _precise_period;
	private:
		precisionSleeper();
		~precisionSleeper();
		static precisionSleeper helper;
	};
public:

	// Static Helper Functions

	// It is set to true by default by precisionSleeper but can be modified.
	static void set_sleep_timer_resolution_1ms(bool enable);
#endif

	// Waits for a precise amount of microseconds.
	static void sleep_for_us(unsigned long long us);

	// Waits for a certain amount of milliseconds, precise in Windows if
	// timer resolution is set to 1ms (default).
	static void sleep_for(unsigned long ms);

	// Returns system time in nanoseconds as a 64-bit tick (monotonic).
	// Perfect for precise time tracking and as seed for random variables.
	static unsigned long long get_system_time_ns();

};