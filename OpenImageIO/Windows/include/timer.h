/*
  Copyright 2008 Larry Gritz and the other authors and contributors.
  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the software's owners nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  (This is the Modified BSD License)
*/


/// @file timer.h
/// @brief Simple timer class.


#ifndef OPENIMAGEIO_TIMER_H
#define OPENIMAGEIO_TIMER_H

#include "oiioversion.h"
#include "export.h"

#ifdef _WIN32
# include "osdep.h"
#elif defined(__APPLE__)
# include <mach/mach_time.h>
#else
#include <sys/time.h>
#include <cstdlib>    // Just for NULL definition
#endif
#include <iostream>


OIIO_NAMESPACE_ENTER
{

/// Simple timer class.
///
/// This class allows you to time things, for runtime statistics and the
/// like.  The simplest usage pattern is illustrated by the following
/// example:
///
/// \code
///    Timer mytimer;                // automatically starts upon construction
///    ...do stuff
///    float t = mytimer();          // seconds elapsed since start
///
///    Timer another (false);        // false means don't start ticking yet
///    another.start ();             // start ticking now
///    another.stop ();              // stop ticking
///    another.start ();             // start again where we left off
///    another.stop ();
///    another.reset ();             // reset to zero time again
/// \endcode
///
/// These are not very high-resolution timers.  A Timer begin/end pair
/// takes somewhere in the neighborhood of 0.1 - 0.3 us (microseconds),
/// and can vary by OS.  This means that (a) it's not useful for timing
/// individual events near or below that resolution (things that would
/// take only tens or hundreds of processor cycles, for example), and
/// (b) calling it millions of times could make your program appreciably
/// more expensive due to the timers themselves.
///
class OIIO_API Timer {
public:
    typedef long long ticks_t;
    enum StartNowVal { DontStartNow, StartNow };
    enum PrintDtrVal { DontPrintDtr, PrintDtr };

    /// Constructor -- reset at zero, and start timing unless optional
    /// 'startnow' argument is false.
    Timer (StartNowVal startnow=StartNow,
           PrintDtrVal printdtr=DontPrintDtr,
           const char *name=NULL)
        : m_ticking(false), m_printdtr(printdtr),
          m_starttime(0), m_elapsed_ticks(0),
          m_name(name)
    {
        if (startnow == StartNow)
            start();
    }

    /// DEPRECATED Constructor -- reset at zero, and start timing unless
    /// optional 'startnow' argument is false.
    Timer (bool startnow)
        : m_ticking(false), m_printdtr(DontPrintDtr),
          m_starttime(0), m_elapsed_ticks(0),
          m_name(NULL)
    {
        if (startnow)
            start();
    }

    /// Destructor.
    ~Timer () {
        if (m_printdtr == PrintDtr)
            std::cout << "Timer " << (m_name?m_name:"") << ": "
                      << seconds(ticks()) << "s\n";
    }

    /// Start (or restart) ticking, if we are not currently.
    void start () {
        if (! m_ticking) {
            m_starttime = now();
            m_ticking = true;
        }
    }

    /// Stop ticking, return the total amount of time that has ticked
    /// (both this round as well as previous laps).  Current ticks will
    /// be added to previous elapsed time.
    double stop () {
        if (m_ticking) {
            ticks_t n = now();
            m_elapsed_ticks += tickdiff (m_starttime, n);
            m_ticking = false;
        }
        return seconds(m_elapsed_ticks);
    }

    /// Reset at zero and stop ticking.
    ///
    void reset (void) {
        m_elapsed_ticks = 0;
        m_ticking = false;
    }

    /// Return just the ticks of the current lap (since the last call to
    /// start() or lap()), add that to the previous elapsed time, reset
    /// current start time to now, keep the timer going (if it was).
    ticks_t lap_ticks () {
        ticks_t n = now();
        ticks_t r = m_ticking ? tickdiff (m_starttime, n) : ticks_t(0);
        m_elapsed_ticks += r;
        m_starttime = n;
        m_ticking = true;
        return r;
    }

    /// Return just the time of the current lap (since the last call to
    /// start() or lap()), add that to the previous elapsed time, reset
    /// current start time to now, keep the timer going (if it was).
    double lap () { return seconds(lap_ticks()); }

    /// Total number of elapsed ticks so far, including both the currently-
    /// ticking clock as well as any previously elapsed time.
    ticks_t ticks () const { return ticks_since_start() + m_elapsed_ticks; }

    /// Operator () returns the elapsed time so far, in seconds, including
    /// both the currently-ticking clock as well as any previously elapsed
    /// time.
    double operator() (void) const { return seconds (ticks()); }

    /// Return just the ticks since we called start(), not any elapsed
    /// time in previous start-stop segments.
    ticks_t ticks_since_start (void) const {
        return m_ticking ? tickdiff (m_starttime, now()) : ticks_t(0);
    }

    /// Return just the time since we called start(), not any elapsed
    /// time in previous start-stop segments.
    double time_since_start (void) const { return seconds (ticks_since_start()); }

    /// Convert number of ticks to seconds.
    static double seconds (ticks_t ticks) { return ticks * seconds_per_tick; }

    /// Is the timer currently ticking?
    bool ticking () const { return m_ticking; }

private:
    bool m_ticking;       ///< Are we currently ticking?
    bool m_printdtr;      ///< Print upon destruction?
    ticks_t m_starttime;  ///< Time since last call to start()
    ticks_t m_elapsed_ticks; ///< Time elapsed BEFORE the current start().
    const char *m_name;      ///< Timer name

    /// Platform-dependent grab of current time, expressed as ticks_t.
    ///
    ticks_t now (void) const {
#ifdef _WIN32
        LARGE_INTEGER n;
        QueryPerformanceCounter (&n);   // From MSDN web site
        return n.QuadPart;
#elif defined(__APPLE__)
        return mach_absolute_time();
#else
        struct timeval t;
        gettimeofday (&t, NULL);
        return (long long) t.tv_sec*1000000ll + t.tv_usec;
#endif
    }

    /// Difference between two times, expressed in (platform-dependent)
    /// ticks.
    ticks_t tickdiff (ticks_t then, ticks_t now) const {
        return (now>then) ? now-then : then-now;
    }

    /// Difference between two times, expressed in seconds.
    double diff (ticks_t then, ticks_t now) const {
        return seconds (tickdiff (then, now));
    }

    static double seconds_per_tick;
    friend class TimerSetupOnce;
};



/// Helper class that starts and stops a timer when the ScopedTimer goes
/// in and out of scope.
class ScopedTimer {
public:
    /// Given a reference to a timer, start it when this constructor
    /// occurs.
    ScopedTimer (Timer &t) : m_timer(t) { start(); }

    /// Stop the timer from ticking when this object is destroyed (i.e.
    /// it leaves scope).
    ~ScopedTimer () { stop(); }

    /// Explicit start of the timer.
    ///
    void start () { m_timer.start(); }

    /// Explicit stop of the timer.
    ///
    void stop () { m_timer.stop(); }

    /// Explicit reset of the timer.
    ///
    void reset () { m_timer.reset(); }

private:
    Timer &m_timer;
};



/// Helper template that runs a function (or functor) n times, using a
/// Timer to benchmark the results, and returning the fastest trial.  If
/// 'range' is non-NULL, the range (max-min) of the various time trials
/// will be stored there.
template<class FUNC>
double
time_trial (FUNC func, int n=1, double *range=NULL)
{
    double mintime = 1.0e30, maxtime = 0.0;
    while (n-- > 0) {
        Timer timer;
        func ();
        double t = timer();
        if (t < mintime)
            mintime = t;
        if (t > maxtime)
            maxtime = t;
    }
    if (range)
        *range = maxtime-mintime;
    return mintime;
}



}
OIIO_NAMESPACE_EXIT

#endif // OPENIMAGEIO_TIMER_H
