#ifndef UWMACTIMERS
#define UWMACTIMERS

/* ======================================================================
   Timers
   ====================================================================== */
class Macuw802_11;

class MacTimer : public Handler {
public:
    MacTimer(Macuw802_11* m) : mac(m) {
        busy_ = paused_ = 0; stime = rtime = 0.0;
    }

    virtual void handle(Event *e) = 0;

    virtual void start(double time);
    virtual void stop(void);
    virtual void pause(void) { assert(0); }
    virtual void resume(void) { assert(0); }

    inline int busy(void) { return busy_; }
    inline int paused(void) { return paused_; }
    inline double expire(void) {
        return ((stime + rtime) - Scheduler::instance().clock());
    }

protected:
    Macuw802_11	*mac;
    int		busy_;
    int		paused_;
    Event		intr;
    double		stime;	// start time
    double		rtime;	// remaining time
};


class BackoffTimer : public MacTimer {
public:
    BackoffTimer(Macuw802_11 *m) : MacTimer(m), difs_wait(0.0) {}



    void	start(int cw, int idle, double difs = 0.0);
    void	handle(Event *e);
    void	pause(void);
    void	resume(double difs);
private:
    double	difs_wait;
};

class DeferTimer : public MacTimer {
public:
    DeferTimer(Macuw802_11 *m) : MacTimer(m) {}

    void	start(double);
    void	handle(Event *e);
};



class IFTimer : public MacTimer {
public:
    IFTimer(Macuw802_11 *m) : MacTimer(m) {}

    void	handle(Event *e);
};

class NavTimer : public MacTimer {
public:
    NavTimer(Macuw802_11 *m) : MacTimer(m) {}

    void	handle(Event *e);
};

class RxTimer : public MacTimer {
public:
    RxTimer(Macuw802_11 *m) : MacTimer(m) {}

    void	handle(Event *e);
};

class TxTimer : public MacTimer {
public:
    TxTimer(Macuw802_11 *m) : MacTimer(m) {}

    void	handle(Event *e);
};



#endif // UWMACTIMERS

