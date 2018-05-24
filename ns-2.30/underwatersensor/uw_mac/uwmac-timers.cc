#include <delay.h>
#include <connector.h>
#include <packet.h>
#include <random.h>

// #define DEBUG
//#include <debug.h>
#include <arp.h>
#include <ll.h>
#include <mac.h>
#include "underwatersensor/uw_mac/uwmac-timers.h"
#include "underwatersensor/uw_mac/uw802_11.h"

/*
 * Force timers to expire on slottime boundries.
 */
// #define USE_SLOT_TIME

// change wrt Mike's code

 #ifdef USE_SLOT_TIME
 #error "Incorrect slot time implementation - don't use USE_SLOT_TIME..."
 #endif

#define ROUND_TIME()	\
    {								\
        assert(slottime);					\
        double rmd = remainder(s.clock() + rtime, slottime);	\
        if(rmd > 0.0)						\
            rtime += (slottime - rmd);			\
        else							\
            rtime += (-rmd);				\
    }


/* ======================================================================
   Timers
   ====================================================================== */

void
MacTimer::start(double time)
{
    Scheduler &s = Scheduler::instance();
    assert(busy_ == 0);

    busy_ = 1;
    paused_ = 0;
    stime = s.clock();
    rtime = time;
    assert(rtime >= 0.0);


    s.schedule(this, &intr, rtime);
}

void
MacTimer::stop(void)
{
    Scheduler &s = Scheduler::instance();

    assert(busy_);

    if(paused_ == 0)
        s.cancel(&intr);

    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;
}

/* ======================================================================
   Defer Timer
   ====================================================================== */
void
DeferTimer::start(double time)
{
    Scheduler &s = Scheduler::instance();

    assert(busy_ == 0);

    busy_ = 1;
    paused_ = 0;
    stime = s.clock();
    rtime = time;
#ifdef USE_SLOT_TIME
    ROUND_TIME();
#endif
    assert(rtime >= 0.0);

    s.schedule(this, &intr, rtime);
}


void
DeferTimer::handle(Event *)
{
    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;



    mac->deferHandler();
}


/* ======================================================================
   NAV Timer
   ====================================================================== */
void
NavTimer::handle(Event *)
{
    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;

    mac->navHandler();
}


/* ======================================================================
   Receive Timer
   ====================================================================== */
void
RxTimer::handle(Event *)
{
    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;

    mac->recvHandler();
}


/* ======================================================================
   Send Timer
   ====================================================================== */
void
TxTimer::handle(Event *)
{
    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;



    mac->sendHandler();
}


/* ======================================================================
   Interface Timer
   ====================================================================== */
void
IFTimer::handle(Event *)
{
    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;

    mac->txHandler();
}


/* ======================================================================
   Backoff Timer
   ====================================================================== */
void
BackoffTimer::handle(Event *)
{
    busy_ = 0;
    paused_ = 0;
    stime = 0.0;
    rtime = 0.0;
    difs_wait = 0.0;

    mac->backoffHandler();
}

void
BackoffTimer::start(int cw, int idle, double difs)
{
    Scheduler &s = Scheduler::instance();
    assert(busy_ == 0);
    busy_ = 1;
    paused_ = 0;
    stime = s.clock();
    rtime = (Random::random() % cw) * mac->phymib_.getSlotTime();


#ifdef USE_SLOT_TIME
    ROUND_TIME();
#endif
    difs_wait = difs;
    if(idle == 0)
        paused_ = 1;
    else {
        assert(rtime + difs_wait >= 0.0);
        s.schedule(this, &intr, rtime + difs_wait);
    }
}


void
BackoffTimer::pause()
{
    Scheduler &s = Scheduler::instance();

    //the caculation below make validation pass for linux though it
    // looks dummy

    double st = s.clock();
    double rt = stime + difs_wait;
    double sr = st - rt;
    double mst = (mac->phymib_.getSlotTime());
    int slots = int (sr/mst);
    if(slots < 0)
        slots = 0;
    assert(busy_ && ! paused_);

    paused_ = 1;
    rtime -= (slots * mac->phymib_.getSlotTime());


    assert(rtime >= 0.0);

    difs_wait = 0.0;

    s.cancel(&intr);
}


void
BackoffTimer::resume(double difs)
{
    Scheduler &s = Scheduler::instance();

    assert(busy_ && paused_);

    paused_ = 0;
    stime = s.clock();

    /*
     * The media should be idle for DIFS time before we start
     * decrementing the counter, so I add difs time in here.
     */
    difs_wait = difs;
    /*
#ifdef USE_SLOT_TIME
    ROUND_TIME();
#endif
    */
    assert(rtime + difs_wait >= 0.0);
        s.schedule(this, &intr, rtime + difs_wait);
}


