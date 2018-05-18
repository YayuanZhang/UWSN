#include "delay.h"
#include "connector.h"
#include "packet.h"
#include "random.h"
#include "mobilenode.h"

//#define DEBUG 99
//#define DEBUG_LUV
#include "arp.h"
#include "ll.h"
#include "mac.h"
#include "uwmac-timers.h"
#include "uw802_11.h"
#include "cmu-trace.h"
// Added by Sushmita to support event tracing
#include "agent.h"
#include "basetrace.h"

#include "underwatersensor/uw_common/underwatersensornode.h"
#include "underwaterphy.h"


/* our backoff timer doesn't count down in idle times during a
 * frame-exchange sequence as the mac tx state isn't idle; genreally
 * these idle times are less than DIFS and won't contribute to
 * counting down the backoff period, but this could be a real
 * problem if the frame exchange ends up in a timeout! in that case,
 * i.e. if a timeout happens we've not been counting down for the
 * duration of the timeout, and in fact begin counting down only
 * DIFS after the timeout!! we lose the timeout interval - which
 * will is not the REAL case! also, the backoff timer could be NULL
 * and we could have a pending transmission which we could have
 * sent! one could argue this is an implementation artifact which
 * doesn't violate the spec.. and the timeout interval is expected
 * to be less than DIFS .. which means its not a lot of time we
 * lose.. anyway if everyone hears everyone the only reason a ack will
 * be delayed will be due to a collision => the medium won't really be
 * idle for a DIFS for this to really matter!!
 */

inline void
Macuw802_11::checkBackoffTimer()
{
    if(is_idle() && mhBackoff_.paused())
      {
        mhBackoff_.resume(phymib_.getDIFS());
#ifdef DEBUG_LUV
        printf("node %d at %lf mhBackoff_.resume %d\n",index_,NOW,phymib_.getDIFS());
#endif
    }
    if(! is_idle() && mhBackoff_.busy() && ! mhBackoff_.paused())
      {
        mhBackoff_.pause();
        #ifdef DEBUG_LUV
        printf("node %d at %lf mhBackoff_.pause\n",index_,NOW);
        #endif
    }
}

inline void
Macuw802_11::transmit(Packet *p, double timeout)
{
    #ifdef DEBUG_LUV
    printf("node %d transmit \n",index_);
    #endif
    tx_active_ = 1;

    if (EOTtarget_) {
        assert (eotPacket_ == NULL);
        eotPacket_ = p->copy();
    }

    /*
     * If I'm transmitting without doing CS, such as when
     * sending an ACK, any incoming packet will be "missed"
     * and hence, must be discarded.
     */
    if(rx_state_ != MAC_IDLE) {
        //assert(dh->dh_fc.fc_type == MAC_Type_Control);
        //assert(dh->dh_fc.fc_subtype == MAC_Subtype_ACK);
        assert(pktRx_);
        struct hdr_cmn *ch = HDR_CMN(pktRx_);
        ch->error() = 1;        /* force packet discard */
        printf("rx_state_ != MAC_IDLE\n");
    }

    /*
     * pass the packet on the "interface" which will in turn
     * place the packet on the channel.
     *
     * NOTE: a handler is passed along so that the Network
     *       Interface can distinguish between incoming and
     *       outgoing packets.
     */
    struct hdr_macuw802_11* dh = HDR_MACUW802_11(p);
    u_int8_t  type = dh->dh_fc.fc_type;
    u_int8_t  subtype = dh->dh_fc.fc_subtype;
    switch(type) {
    case MAC_Type_Management:
        printf("node %d send Management to %d at %lf\n",index_,ETHER_ADDR(dh->dh_ra),NOW);
        break;
    case MAC_Type_Control:
        switch(subtype) {
        case MAC_Subtype_RTS:
            printf("node %d send RTS to %d at %lf\n",index_,ETHER_ADDR(dh->dh_ra),NOW);
            break;
        case MAC_Subtype_CTS:
           printf("node %d send CTS to %d at %lf\n",index_,ETHER_ADDR(dh->dh_ra),NOW);
            break;
        case MAC_Subtype_ACK:
            printf("node %d send ACK to %d at %lf\n",index_,ETHER_ADDR(dh->dh_ra),NOW);
            break;
        default:
           printf("666\n");
            exit(1);
        }
        break;
    case MAC_Type_Data:
        switch(subtype) {
        case MAC_Subtype_Data:
            printf("node %d send DATA to %d at %lf\n",index_,ETHER_ADDR(dh->dh_ra),NOW);
            break;
        default:
            printf("777\n");
            exit(1);
        }
    }
    downtarget_->recv(p->copy(), this);
    printf("timeout %lf txtime %lf \n",timeout,txtime(p));
    mhSend_.start(timeout);
    mhIF_.start(txtime(p));
    hdr_cmn *ch=HDR_CMN(p);
    if (ch->pkt_flag==1)
    sendDATA(stored2);
}

inline void
Macuw802_11::setRxState(MacState newState)
{
    #ifdef DEBUG_LUV
    printf("setRxState\n");
    #endif
    rx_state_ = newState;
    checkBackoffTimer();
}

inline void
Macuw802_11::setTxState(MacState newState)
{
    tx_state_ = newState;
    checkBackoffTimer();
}


/* ======================================================================
   TCL Hooks for the simulator
   ====================================================================== */
static class Macuw802_11Class : public TclClass {
public:
    Macuw802_11Class() : TclClass("Mac/UnderwaterMac/uw802_11") {}
    TclObject* create(int, const char*const*) {
    return (new Macuw802_11());
}
} class_Macuw802_11;


/* ======================================================================
   Mac  and Phy MIB Class Functions
   ====================================================================== */

PHY_MIB::PHY_MIB(Macuw802_11 *parent)
{
    /*
     * Bind the phy mib objects.  Note that these will be bound
     * to Mac/802_11 variables
     */

    parent->bind("CWMin_", &CWMin);
    parent->bind("CWMax_", &CWMax);
    parent->bind("SlotTime_", &SlotTime);
    parent->bind("SIFS_", &SIFSTime);
    parent->bind("PreambleLength_", &PreambleLength);
    parent->bind("PLCPHeaderLength_", &PLCPHeaderLength);
    parent->bind_bw("PLCPDataRate_", &PLCPDataRate);
    SlotTime=0.5;//0.0002
    SIFSTime=0.1;//0.0001
    //printf("bind PreambleLength %lf\n",PreambleLength);
    //printf("bind PLCPHeaderLength %lf\n",PLCPHeaderLength);
    //printf("bind PLCPDataRate %lf\n",PLCPDataRate);

    #ifdef DEBUG_LUV
    printf("bind SlotTime %lf\n",SlotTime);
    printf("bind SIFSTime %lf\n",SIFSTime);
    #endif
}


MAC_MIB::MAC_MIB(Macuw802_11 *parent)
{
    /*
     * Bind the phy mib objects.  Note that these will be bound
     * to Mac/802_11 variables
     */

    parent->bind("RTSThreshold_", &RTSThreshold);
    parent->bind("ShortRetryLimit_", &ShortRetryLimit);
    parent->bind("LongRetryLimit_", &LongRetryLimit);
}

/* ======================================================================
   Mac Class Functions
   ====================================================================== */
Macuw802_11::Macuw802_11() :
    UnderwaterMac(), phymib_(this), macmib_(this), mhIF_(this), mhNav_(this),
    mhRecv_(this), mhSend_(this),
    mhDefer_(this), mhBackoff_(this)
{

    nav_ = 0.0;
    tx_state_ = rx_state_ = MAC_IDLE;
    tx_active_ = 0;
    eotPacket_ = NULL;
    pktRTS_ = 0;
    pktCTRL_ = 0;
    cw_ = phymib_.getCWMin();
    ssrc_ = slrc_ = 0;
    // Added by Sushmita
        et_ = new EventTrace();

    sta_seqno_ = 1;
    cache_ = 0;
    cache_node_count_ = 0;

    // chk if basic/data rates are set
    // otherwise use bandwidth_ as default;

    Tcl& tcl = Tcl::instance();
    tcl.evalf("Mac/UnderwaterMac/uw802_11 set basicRate_");
    if (strcmp(tcl.result(), "0") != 0)
        bind_bw("basicRate_", &basicRate_);
    else
        basicRate_ = bandwidth_;
   #ifdef DEBUG_LUV
    printf("basicRate_ %lf\n",basicRate_);
   #endif
    tcl.evalf("Mac/UnderwaterMac/uw802_11 set dataRate_");
    if (strcmp(tcl.result(), "0") != 0)
        bind_bw("dataRate_", &dataRate_);
    else
        dataRate_ = bandwidth_;

    bind_bool("bugFix_timer_", &bugFix_timer_);

        EOTtarget_ = 0;
        bss_id_ = IBSS_ID;
    //printf("bssid in constructor %d\n",bss_id_);
}


int
Macuw802_11::command(int argc, const char*const* argv)
{
    if (argc == 3) {
        if (strcmp(argv[1], "eot-target") == 0) {
            EOTtarget_ = (NsObject*) TclObject::lookup(argv[2]);
            if (EOTtarget_ == 0)
                return TCL_ERROR;
            return TCL_OK;
        } else if (strcmp(argv[1], "bss_id") == 0) {
            bss_id_ = atoi(argv[2]);
            return TCL_OK;
        } else if (strcmp(argv[1], "log-target") == 0) {
            logtarget_ = (NsObject*) TclObject::lookup(argv[2]);
            if(logtarget_ == 0)
                return TCL_ERROR;
            return TCL_OK;
        } else if(strcmp(argv[1], "nodes") == 0) {
            if(cache_) return TCL_ERROR;
            cache_node_count_ = atoi(argv[2]);
            cache_ = new Host[cache_node_count_ + 1];
            assert(cache_);
            bzero(cache_, sizeof(Host) * (cache_node_count_+1 ));
            return TCL_OK;
        } else if(strcmp(argv[1], "eventtrace") == 0) {
            // command added to support event tracing by Sushmita
                        et_ = (EventTrace *)TclObject::lookup(argv[2]);
                        return (TCL_OK);
                }
    }
    return UnderwaterMac::command(argc, argv);
}


// Added by Sushmita to support event tracing
void Macuw802_11::trace_event(char *eventtype, Packet *p)
{
        if (et_ == NULL) return;
        char *wrk = et_->buffer();
        char *nwrk = et_->nbuffer();

        //char *src_nodeaddr =
    //       Address::instance().print_nodeaddr(iph->saddr());
        //char *dst_nodeaddr =
        //      Address::instance().print_nodeaddr(iph->daddr());

        struct hdr_macuw802_11* dh = HDR_MACUW802_11(p);

        //struct hdr_cmn *ch = HDR_CMN(p);

    if(wrk != 0) {
        sprintf(wrk, "E -t "TIME_FORMAT" %s %2x ",
            et_->round(Scheduler::instance().clock()),
                        eventtype,
                        //ETHER_ADDR(dh->dh_sa)
                        ETHER_ADDR(dh->dh_ta)
                        );
        }
        if(nwrk != 0) {
                sprintf(nwrk, "E -t "TIME_FORMAT" %s %2x ",
                        et_->round(Scheduler::instance().clock()),
                        eventtype,
                        //ETHER_ADDR(dh->dh_sa)
                        ETHER_ADDR(dh->dh_ta)
                        );
        }
        et_->dump();
}

/* ======================================================================
   Debugging Routines
   ====================================================================== */
void
Macuw802_11::trace_pkt(Packet *p)
{
    struct hdr_cmn *ch = HDR_CMN(p);
    struct hdr_macuw802_11* dh = HDR_MACUW802_11(p);
    u_int16_t *t = (u_int16_t*) &dh->dh_fc;

    fprintf(stderr, "\t[ %2x %2x %2x %2x ] %x %s %d\n",
        *t, dh->dh_duration,
         ETHER_ADDR(dh->dh_ra), ETHER_ADDR(dh->dh_ta),
        index_, packet_info.name(ch->ptype()), ch->size());
}

void
Macuw802_11::dump(char *fname)
{
    fprintf(stderr,
        "\n%s --- (INDEX: %d, time: %2.9f)\n",
        fname, index_, Scheduler::instance().clock());

    fprintf(stderr,
        "\ttx_state_: %x, rx_state_: %x, nav: %2.9f, idle: %d\n",
        tx_state_, rx_state_, nav_, is_idle());

    fprintf(stderr,
        "\tpktTx_: %lx, pktRx_: %lx, pktRTS_: %lx, pktCTRL_: %lx, callback: %lx\n",
        (long) pktTx_, (long) pktRx_, (long) pktRTS_,
        (long) pktCTRL_, (long) callback_);

    fprintf(stderr,
        "\tDefer: %d, Backoff: %d (%d), Recv: %d, Timer: %d Nav: %d\n",
        mhDefer_.busy(), mhBackoff_.busy(), mhBackoff_.paused(),
        mhRecv_.busy(), mhSend_.busy(), mhNav_.busy());
    fprintf(stderr,
        "\tBackoff Expire: %f\n",
        mhBackoff_.expire());
}


/* ======================================================================
   Packet Headers Routines
   ====================================================================== */
inline int
Macuw802_11::hdr_dst(char* hdr, int dst )
{
    struct hdr_macuw802_11 *dh = (struct hdr_macuw802_11*) hdr;

       if (dst > -2) {
               if ((bss_id() == ((int)IBSS_ID)) || (addr() == bss_id())) {
                       /* if I'm AP (2nd condition above!), the dh_3a
                        * is already set by the MAC whilst fwding; if
                        * locally originated pkt, it might make sense
                        * to set the dh_3a to myself here! don't know
                        * how to distinguish between the two here - and
                        * the info is not critical to the dst station
                        * anyway!
                        */
                       STORE4BYTE(&dst, (dh->dh_ra));
               } else {
                       /* in BSS mode, the AP forwards everything;
                        * therefore, the real dest goes in the 3rd
                        * address, and the AP address goes in the
                        * destination address
                        */
                       STORE4BYTE(&bss_id_, (dh->dh_ra));
                       STORE4BYTE(&dst, (dh->dh_3a));
               }
       }


       return (u_int32_t)ETHER_ADDR(dh->dh_ra);
}

inline int
Macuw802_11::hdr_src(char* hdr, int src )
{
    struct hdr_macuw802_11 *dh = (struct hdr_macuw802_11*) hdr;
        if(src > -2)
               STORE4BYTE(&src, (dh->dh_ta));
        return ETHER_ADDR(dh->dh_ta);
}

inline int
Macuw802_11::hdr_type(char* hdr, u_int16_t type)
{
    struct hdr_macuw802_11 *dh = (struct hdr_macuw802_11*) hdr;
    if(type)
        STORE2BYTE(&type,(dh->dh_body));
    return GET2BYTE(dh->dh_body);
}


/* ======================================================================
   Misc Routines
   ====================================================================== */
inline int
Macuw802_11::is_idle()
{
    if(rx_state_ != MAC_IDLE)
        return 0;
    if(tx_state_ != MAC_IDLE)
        return 0;
    if(nav_ > Scheduler::instance().clock())
        return 0;

    return 1;
}

void
Macuw802_11::discard(Packet *p, const char* why)
{
    #ifdef DEBUG_LUV
    printf("node %d discard \n",index_);
    #endif
    hdr_macuw802_11* mh = HDR_MACUW802_11(p);
    hdr_cmn *ch = HDR_CMN(p);

    /* if the rcvd pkt contains errors, a real MAC layer couldn't
       necessarily read any data from it, so we just toss it now */
    if(ch->error() != 0) {
        Packet::free(p);
        return;
    }

    switch(mh->dh_fc.fc_type) {
    case MAC_Type_Management:
        drop(p, why);
        return;
    case MAC_Type_Control:
        switch(mh->dh_fc.fc_subtype) {
        case MAC_Subtype_RTS:
             if((u_int32_t)ETHER_ADDR(mh->dh_ta) ==  (u_int32_t)index_) {
                drop(p, why);
                return;
            }
            /* fall through - if necessary */
        case MAC_Subtype_CTS:
        case MAC_Subtype_ACK:
            if((u_int32_t)ETHER_ADDR(mh->dh_ra) == (u_int32_t)index_) {
                drop(p, why);
                return;
            }
            break;
        default:
            fprintf(stderr, "invalid MAC Control subtype\n");
            exit(1);
        }
        break;
    case MAC_Type_Data:
        switch(mh->dh_fc.fc_subtype) {
        case MAC_Subtype_Data:
            if((u_int32_t)ETHER_ADDR(mh->dh_ra) == \
                           (u_int32_t)index_ ||
                          (u_int32_t)ETHER_ADDR(mh->dh_ta) == \
                           (u_int32_t)index_ ||
                          (u_int32_t)ETHER_ADDR(mh->dh_ra) == MAC_BROADCAST) {
                                drop(p,why);
                                return;
            }
            break;
        default:
            fprintf(stderr, "invalid MAC Data subtype\n");
            exit(1);
        }
        break;
    default:
        fprintf(stderr, "invalid MAC type (%x)\n", mh->dh_fc.fc_type);
        trace_pkt(p);
        exit(1);
    }
    Packet::free(p);
}

void
Macuw802_11::capture(Packet *p)
{
    #ifdef DEBUG_LUV
    printf("node %d capture \n",index_);
    #endif
    /*
     * Update the NAV so that this does not screw
     * up carrier sense.
     */
    set_nav(usec(phymib_.getEIFS() + txtime(p)));
    Packet::free(p);
}

void
Macuw802_11::collision(Packet *p)
{
    #ifdef DEBUG_LUV
    printf("node %d collision at %lf\n",index_,NOW);
    #endif
    switch(rx_state_) {
    case MAC_RECV:
        setRxState(MAC_COLL);
        /* fall through */
    case MAC_COLL:
        assert(pktRx_);
        assert(mhRecv_.busy());
        /*
         *  Since a collision has occurred, figure out
         *  which packet that caused the collision will
         *  "last" the longest.  Make this packet,
         *  pktRx_ and reset the Recv Timer if necessary.
         */
        if(txtime(p) > mhRecv_.expire()) {
            mhRecv_.stop();
            discard(pktRx_, DROP_MAC_COLLISION);
            pktRx_ = p;
            mhRecv_.start(txtime(pktRx_));
        }
        else {
            discard(p, DROP_MAC_COLLISION);
        }
        break;
    default:
        assert(0);
    }
}

void
Macuw802_11::tx_resume()
{

    printf("node %d tx_resume at %lf\n",index_,NOW);
    double rTime;
    assert(mhSend_.busy() == 0);
    assert(mhDefer_.busy() == 0);
    if(pktCTRL_) {
        /*
         *  Need to send a CTS or ACK.
         */
        printf("1114\n");
        mhDefer_.start(phymib_.getSIFS());
    } else if(pktRTS_) {
        if (mhBackoff_.busy() == 0) {
            if (bugFix_timer_) {
                mhBackoff_.start(cw_, is_idle(),
                         phymib_.getDIFS());
            }
            else {
                rTime = (Random::random() % cw_) *
                    phymib_.getSlotTime();
                printf("1115\n");
                mhDefer_.start( phymib_.getDIFS() + rTime);
            }
        }
    } else if(pktTx_) {
        if (mhBackoff_.busy() == 0) {
            hdr_cmn *ch = HDR_CMN(pktTx_);
            struct hdr_macuw802_11 *mh = HDR_MACUW802_11(pktTx_);

            if ((u_int32_t) ch->size() < macmib_.getRTSThreshold()
                || (u_int32_t) ETHER_ADDR(mh->dh_ra) == MAC_BROADCAST) {
                if (bugFix_timer_) {
                    mhBackoff_.start(cw_, is_idle(),
                             phymib_.getDIFS());
                }
                else {
                    rTime = (Random::random() % cw_)
                        * phymib_.getSlotTime();
                    printf("11116\n");
                    mhDefer_.start(phymib_.getDIFS() +
                               rTime);
                }
                        } else {

                mhDefer_.start(phymib_.getSIFS());

                        }
        }
    } else if(callback_) {
        printf("callback\n");
        Handler *h = callback_;
        callback_ = 0;
        h->handle((Event*) 0);
    }
    printf("node %d setTxState MAC_IDLE at %lf\n",index_,NOW);
    setTxState(MAC_IDLE);
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(IDLEE);
}

void
Macuw802_11::rx_resume()
{
    #ifdef DEBUG_LUV
    printf("node %d rx_resume at %lf\n",index_,NOW);
    #endif
    assert(pktRx_ == 0);
    assert(mhRecv_.busy() == 0);
    setRxState(MAC_IDLE);
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(IDLEE);
}


/* ======================================================================
   Timer Handler Routines
   ====================================================================== */
void
Macuw802_11::backoffHandler()
{
    if(pktCTRL_) {
        assert(mhSend_.busy() || mhDefer_.busy());
        return;
    }

    if(check_pktRTS() == 0)
        return;

    if(check_pktTx() == 0)
        return;
}

void
Macuw802_11::deferHandler()
{
    assert(pktCTRL_ || pktRTS_ || pktTx_);

    if(check_pktCTRL() == 0)
        return;
    assert(mhBackoff_.busy() == 0);
    if(check_pktRTS() == 0)
        return;
    if(check_pktTx() == 0)
        return;
}

void
Macuw802_11::navHandler()
{
   #ifdef DEBUG_LUV
    printf("node %d at %lf navHandler\n",index_,NOW);
    #endif
    if(is_idle() && mhBackoff_.paused())
        mhBackoff_.resume(phymib_.getDIFS());
}

void
Macuw802_11::recvHandler()
{
    printf("node %d at %lf recvHandler\n",index_,NOW);
    recv_timer();
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(IDLEE);
}

void
Macuw802_11::sendHandler()
{
    printf("node %d at %lf sendHandler\n",index_,NOW);
    send_timer();
}


void
Macuw802_11::txHandler()
{
    printf("node %d at %lf txHandler\n",index_,NOW);
    if (EOTtarget_) {
        printf("eottarget\n");
        assert(eotPacket_);
        EOTtarget_->recv(eotPacket_, (Handler *) 0);
        eotPacket_ = NULL;
    }
    tx_active_ = 0;
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(IDLEE);
}



/* ======================================================================
   The "real" Timer Handler Routines
   ====================================================================== */
void
Macuw802_11::send_timer()
{
    printf("send_timer\n");
    switch(tx_state_) {
    /*
     * Sent a RTS, but did not receive a CTS.
     */
    case MAC_RTS:
        RetransmitRTS();
        break;
    /*
     * Sent a CTS, but did not receive a DATA packet.
     */
    case MAC_CTS:
        assert(pktCTRL_);
        Packet::free(pktCTRL_);
        pktCTRL_ = 0;
        break;
    /*
     * Sent DATA, but did not receive an ACK packet.
     */
    case MAC_SEND:
        RetransmitDATA();
        break;
    /*
     * Sent an ACK, and now ready to resume transmission.
     */
    case MAC_ACK:
        assert(pktCTRL_);
        Packet::free(pktCTRL_);
        pktCTRL_ = 0;
        break;
    case MAC_IDLE:
        break;
    default:
        assert(0);
    }
    tx_resume();
}


/* ======================================================================
   Outgoing Packet Routines
   ====================================================================== */
int
Macuw802_11::check_pktCTRL()
{
    printf("node %d check_pktCTRL at %lf\n",index_,NOW);
    struct hdr_macuw802_11 *mh;
    double timeout;

    if(pktCTRL_ == 0)
      {  printf("pktCTRL_ == 0\n");
         return -1; }
    if(tx_state_ == MAC_CTS || tx_state_ == MAC_ACK)
       {
         printf("tx_state_ == MAC_CTS || tx_state_ == MAC_ACK \n");
         return -1; }

    mh = HDR_MACUW802_11(pktCTRL_);
    switch(mh->dh_fc.fc_subtype) {
    /*
     *  If the medium is not IDLE, don't send the CTS.
     */
    case MAC_Subtype_CTS:
        if(!is_idle()) {
            discard(pktCTRL_, DROP_MAC_BUSY); pktCTRL_ = 0;
            printf("discard CTS\n");
            return 0;
        }
        printf("node %d setTxState MAC_CTS at %lf\n",index_,NOW);
        setTxState(MAC_CTS);
        /*
         * timeout:  cts + data tx time calculated by
         *           adding cts tx time to the cts duration
         *           minus ack tx time -- this timeout is
         *           a guess since it is unspecified
         *           (note: mh->dh_duration == cf->cf_duration)
         */
         timeout = txtime(phymib_.getCTSlen(), basicRate_)
                        + DSSS_MaxPropagationDelay                      // XXX
                        + sec(mh->dh_duration)
                        + DSSS_MaxPropagationDelay                      // XXX
                        + txtime(400, basicRate_)
                        +9
                       - phymib_.getSIFS()
                       - txtime(phymib_.getACKlen(), basicRate_);
        break;
        /*
         * IEEE 802.11 specs, section 9.2.8
         * Acknowledments are sent after an SIFS, without regard to
         * the busy/idle state of the medium.
         */
    case MAC_Subtype_ACK:
        #ifdef DEBUG_LUV
        printf("node %d setTxState MAC_ACK at %lf\n",index_,NOW);
        #endif
        setTxState(MAC_ACK);
        timeout = txtime(phymib_.getACKlen(), basicRate_);
        break;
    default:
        fprintf(stderr, "check_pktCTRL:Invalid MAC Control subtype\n");
        exit(1);
    }
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(SENDE);
    transmit(pktCTRL_, timeout);
    return 0;
}

int
Macuw802_11::check_pktRTS()
{
    #ifdef DEBUG_LUV
    printf("node %d check_pktRTS at %lf\n",index_,NOW);
    #endif
    struct hdr_macuw802_11 *mh;
    double timeout;

    assert(mhBackoff_.busy() == 0);

    if(pktRTS_ == 0)
        return -1;
    mh = HDR_MACUW802_11(pktRTS_);

    switch(mh->dh_fc.fc_subtype) {
    case MAC_Subtype_RTS:
        if(! is_idle()) {
            inc_cw();
            mhBackoff_.start(cw_, is_idle());
            return 0;
        }
        #ifdef DEBUG_LUV
        printf("node %d setTxState MAC_RTS at %lf\n",index_,NOW);
        #endif
        //printf(" getRTSlen %lf\n",phymib_.getRTSlen());
       //printf(" basicRate_ %lf\n",basicRate_);
       // printf(" txtime_ %lf\n",txtime(phymib_.getRTSlen(), basicRate_));
        setTxState(MAC_RTS);
        timeout = txtime(phymib_.getRTSlen(), basicRate_)
            + DSSS_MaxPropagationDelay                       // XXX
            + phymib_.getSIFS()
            + txtime(phymib_.getCTSlen(), basicRate_)
            + DSSS_MaxPropagationDelay;
        #ifdef DEBUG_LUV
        printf("DSSS %d ",DSSS_MaxPropagationDelay);
        printf("timeout %lf\n",timeout);
        #endif
        break;
    default:
        fprintf(stderr, "check_pktRTS:Invalid MAC Control subtype\n");
        exit(1);
    }
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(SENDE);
    transmit(pktRTS_, timeout);
    return 0;
}

int
Macuw802_11::check_pktTx()
{
    #ifdef DEBUG_LUV
    printf("node %d check_pktTx at %lf\n",index_,NOW);
    #endif
    struct hdr_macuw802_11 *mh;
    struct hdr_cmn *cmh;
    double timeout;

    assert(mhBackoff_.busy() == 0);

    if(pktTx_ == 0)
        return -1;

    mh = HDR_MACUW802_11(pktTx_);
    cmh = HDR_CMN(pktTx_);
    switch(mh->dh_fc.fc_subtype) {
    case MAC_Subtype_Data:
        if(! is_idle()) {
            sendRTS(ETHER_ADDR(mh->dh_ra));
            inc_cw();
            mhBackoff_.start(cw_, is_idle());
            return 0;
        }
        #ifdef DEBUG_LUV
        printf("node %d setTxState MAC_send at %lf\n",index_,NOW);
        #endif
        setTxState(MAC_SEND);
        if((u_int32_t)ETHER_ADDR(mh->dh_ra) != MAC_BROADCAST)
        {
            if(cmh->pkt_flag ==1)
                timeout=txtime(pktTx_);
            else
                timeout = txtime(pktTx_)
                           + DSSS_MaxPropagationDelay              // XXX
                               + phymib_.getSIFS()
                               + txtime(phymib_.getACKlen(), basicRate_)
                               + DSSS_MaxPropagationDelay;             // XXX
        }
        else
            timeout = txtime(pktTx_);
        break;
    default:
        fprintf(stderr, "check_pktTx:Invalid MAC Control subtype\n");
        exit(1);
    }
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    n->SetTransmissionStatus(SENDE);
    transmit(pktTx_, timeout);
    printf("node %d pktTx_, timeout %lf txtime(pktTx_) %lf\n",index_,timeout,txtime(pktTx_));
    return 0;
}
/*
 * Low-level transmit functions that actually place the packet onto
 * the channel.
 */
void
Macuw802_11::sendRTS(int dst)
{
   #ifdef DEBUG_LUV
    printf("node %d sendRTS to %d at %lf\n",index_,dst,NOW);
   #endif
    Packet *p = Packet::alloc();
    hdr_cmn* ch = HDR_CMN(p);
    struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);

    assert(pktTx_);
    assert(pktRTS_ == 0);

    /*
     *  If the size of the packet is larger than the
     *  RTSThreshold, then perform the RTS/CTS exchange.
     */
    if( (u_int32_t) HDR_CMN(pktTx_)->size() < macmib_.getRTSThreshold() ||
            (u_int32_t) dst == MAC_BROADCAST) {
        Packet::free(p);
        return;
    }

    ch->uid() = 0;
    ch->ptype() = PT_MAC;
    ch->size() = phymib_.getRTSlen();
    ch->iface() = -2;
    ch->error() = 0;

    bzero(rf, MAC_HDR_LEN);

    rf->rf_fc.fc_protocol_version = MAC_ProtocolVersion;
    rf->rf_fc.fc_type	= MAC_Type_Control;
    rf->rf_fc.fc_subtype	= MAC_Subtype_RTS;
    rf->rf_fc.fc_to_ds	= 0;
    rf->rf_fc.fc_from_ds	= 0;
    rf->rf_fc.fc_more_frag	= 0;
    rf->rf_fc.fc_retry	= 0;
    rf->rf_fc.fc_pwr_mgt	= 0;
    rf->rf_fc.fc_more_data	= 0;
    rf->rf_fc.fc_wep	= 0;
    rf->rf_fc.fc_order	= 0;

    //rf->rf_duration = RTS_DURATION(pktTx_);
    STORE4BYTE(&dst, (rf->rf_ra));

    /* store rts tx time */
    ch->txtime() = txtime(ch->size(), basicRate_);
    STORE4BYTE(&index_, (rf->rf_ta));

    /* calculate rts duration field */
    rf->rf_duration = usec(phymib_.getSIFS()
                   + txtime(phymib_.getCTSlen(), basicRate_)
                   + phymib_.getSIFS()
                               + txtime(pktTx_)
                   + phymib_.getSIFS()
                               + txtime(pktTx_)
                   + phymib_.getSIFS()
                   +9
                   + txtime(phymib_.getACKlen(), basicRate_));
    pktRTS_ = p;
}

void
Macuw802_11::sendCTS(int dst, double rts_duration)
{
    printf("node %d prepare sendCTS to %d at %lf\n",index_,dst,NOW);
    Packet *p = Packet::alloc();
    hdr_cmn* ch = HDR_CMN(p);
    struct cts_frame *cf = (struct cts_frame*)p->access(hdr_mac::offset_);

    assert(pktCTRL_ == 0);

    ch->uid() = 0;
    ch->ptype() = PT_MAC;
    ch->size() = phymib_.getCTSlen();


    ch->iface() = -2;
    ch->error() = 0;
    //ch->direction() = hdr_cmn::DOWN;
    bzero(cf, MAC_HDR_LEN);

    cf->cf_fc.fc_protocol_version = MAC_ProtocolVersion;
    cf->cf_fc.fc_type	= MAC_Type_Control;
    cf->cf_fc.fc_subtype	= MAC_Subtype_CTS;
    cf->cf_fc.fc_to_ds	= 0;
    cf->cf_fc.fc_from_ds	= 0;
    cf->cf_fc.fc_more_frag	= 0;
    cf->cf_fc.fc_retry	= 0;
    cf->cf_fc.fc_pwr_mgt	= 0;
    cf->cf_fc.fc_more_data	= 0;
    cf->cf_fc.fc_wep	= 0;
    cf->cf_fc.fc_order	= 0;

    //cf->cf_duration = CTS_DURATION(rts_duration);
    STORE4BYTE(&dst, (cf->cf_ra));

    /* store cts tx time */
    ch->txtime() = txtime(ch->size(), basicRate_);

    /* calculate cts duration */
    cf->cf_duration = usec(sec(rts_duration)
                              - phymib_.getSIFS()
                              - txtime(phymib_.getCTSlen(), basicRate_));



    pktCTRL_ = p;

}

void
Macuw802_11::sendACK(int dst)
{
    printf("node %d sendACK at %lf\n",index_,NOW);
    Packet *p = Packet::alloc();
    hdr_cmn* ch = HDR_CMN(p);
    struct ack_frame *af = (struct ack_frame*)p->access(hdr_mac::offset_);

    assert(pktCTRL_ == 0);

    ch->uid() = 0;
    ch->ptype() = PT_MAC;
    // CHANGE WRT Mike's code
    ch->size() = phymib_.getACKlen();
    ch->iface() = -2;
    ch->error() = 0;

    bzero(af, MAC_HDR_LEN);

    af->af_fc.fc_protocol_version = MAC_ProtocolVersion;
    af->af_fc.fc_type	= MAC_Type_Control;
    af->af_fc.fc_subtype	= MAC_Subtype_ACK;
    af->af_fc.fc_to_ds	= 0;
    af->af_fc.fc_from_ds	= 0;
    af->af_fc.fc_more_frag	= 0;
    af->af_fc.fc_retry	= 0;
    af->af_fc.fc_pwr_mgt	= 0;
    af->af_fc.fc_more_data	= 0;
    af->af_fc.fc_wep	= 0;
    af->af_fc.fc_order	= 0;

    //af->af_duration = ACK_DURATION();
    STORE4BYTE(&dst, (af->af_ra));

    /* store ack tx time */
    ch->txtime() = txtime(ch->size(), basicRate_);

    /* calculate ack duration */
    af->af_duration = 0;

    pktCTRL_ = p;
}

void
Macuw802_11::sendDATA(Packet *p)
{
    #ifdef DEBUG_LUV
    printf("node %d sendDATA at %lf\n",index_,NOW);
    #endif
    hdr_cmn* ch = HDR_CMN(p);
    struct hdr_macuw802_11* dh = HDR_MACUW802_11(p);

    assert(pktTx_ == 0);

    /*
     * Update the MAC header
     */
    ch->size() += phymib_.getHdrLen11();

    dh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
    dh->dh_fc.fc_type       = MAC_Type_Data;
    dh->dh_fc.fc_subtype    = MAC_Subtype_Data;

    dh->dh_fc.fc_to_ds      = 0;
    dh->dh_fc.fc_from_ds    = 0;
    dh->dh_fc.fc_more_frag  = 0;
    dh->dh_fc.fc_retry      = 0;
    dh->dh_fc.fc_pwr_mgt    = 0;
    dh->dh_fc.fc_more_data  = 0;
    dh->dh_fc.fc_wep        = 0;
    dh->dh_fc.fc_order      = 0;
    // printf("node %d sendDATA to %d at %lf\n",index_,ETHER_ADDR(dh->dh_ra),NOW);
    /* store data tx time */
    ch->txtime() = txtime(ch->size(), dataRate_);
    if((u_int32_t)ETHER_ADDR(dh->dh_ra) != MAC_BROADCAST) {
        /* store data tx time for unicast packets */
        ch->txtime() = txtime(ch->size(), dataRate_);
        dh->dh_duration = usec(txtime(phymib_.getACKlen(), basicRate_)
                       + phymib_.getSIFS());



    } else {
        /* store data tx time for broadcast packets (see 9.6) */
        ch->txtime() = txtime(ch->size(), basicRate_);

        dh->dh_duration = 0;
    }
    pktTx_ = p;

}

/* ======================================================================
   Retransmission Routines
   ====================================================================== */
void
Macuw802_11::RetransmitRTS()
{
    printf("node %d RetransmitRTS at %lf\n",index_,NOW);
    assert(pktTx_);
    assert(pktRTS_);
    assert(mhBackoff_.busy() == 0);
    macmib_.RTSFailureCount++;

    ssrc_ += 1;			// STA Short Retry Count

    if(ssrc_ >= macmib_.getShortRetryLimit()) {
        discard(pktRTS_, DROP_MAC_RETRY_COUNT_EXCEEDED); pktRTS_ = 0;
        /* tell the callback the send operation failed
           before discarding the packet */
        hdr_cmn *ch = HDR_CMN(pktTx_);
        if (ch->xmit_failure_) {
                        /*
                         *  Need to remove the MAC header so that
                         *  re-cycled packets don't keep getting
                         *  bigger.
                         */
            ch->size() -= phymib_.getHdrLen11();
                        ch->xmit_reason_ = XMIT_REASON_RTS;
                        ch->xmit_failure_(pktTx_->copy(),
                                          ch->xmit_failure_data_);
                }
        discard(pktTx_, DROP_MAC_RETRY_COUNT_EXCEEDED);
        pktTx_ = 0;
        ssrc_ = 0;
        rst_cw();
    } else {
        struct rts_frame *rf;
        rf = (struct rts_frame*)pktRTS_->access(hdr_mac::offset_);
        rf->rf_fc.fc_retry = 1;

        inc_cw();
       #ifdef DEBUG_LUV
        printf("node %d RetransmitRTS at %lf : mhBackoff_\n",index_,NOW);
        #endif
        mhBackoff_.start(cw_, is_idle());
    }
}

void
Macuw802_11::RetransmitDATA()
{
    printf("node %d RetransmitDATA at %lf\n",index_,NOW);
    struct hdr_cmn *ch;
    struct hdr_macuw802_11 *mh;
    u_int32_t *rcount, thresh;
    assert(mhBackoff_.busy() == 0);

    assert(pktTx_);
    assert(pktRTS_ == 0);

    ch = HDR_CMN(pktTx_);
    mh = HDR_MACUW802_11(pktTx_);
    printf("retrans data no %d\n",ch->pkt_flag);
    /*
     *  Broadcast packets don't get ACKed and therefore
     *  are never retransmitted.
     */

    if((u_int32_t)ETHER_ADDR(mh->dh_ra) == MAC_BROADCAST) {
        Packet::free(pktTx_);
        pktTx_ = 0;

        /*
         * Backoff at end of TX.
         */
        rst_cw();
        mhBackoff_.start(cw_, is_idle());

        return;
    }

    macmib_.ACKFailureCount++;

    if((u_int32_t) ch->size() <= macmib_.getRTSThreshold()) {
                        rcount = &ssrc_;
               thresh = macmib_.getShortRetryLimit();
        } else {
                rcount = &slrc_;
               thresh = macmib_.getLongRetryLimit();
        }

    (*rcount)++;

    if(*rcount >= thresh) {
        /* IEEE Spec section 9.2.3.5 says this should be greater than
           or equal */
        macmib_.FailedCount++;
        /* tell the callback the send operation failed
           before discarding the packet */
        hdr_cmn *ch = HDR_CMN(pktTx_);
        if (ch->xmit_failure_) {
                        ch->size() -= phymib_.getHdrLen11();
            ch->xmit_reason_ = XMIT_REASON_ACK;
                        ch->xmit_failure_(pktTx_->copy(),
                                          ch->xmit_failure_data_);
                }

        discard(pktTx_, DROP_MAC_RETRY_COUNT_EXCEEDED);
        pktTx_ = 0;
        *rcount = 0;
        rst_cw();
    }
    else {
        struct hdr_macuw802_11 *dh;
        dh = HDR_MACUW802_11(pktTx_);
        dh->dh_fc.fc_retry = 1;
     /*   sendRTS(ETHER_ADDR(mh->dh_ra));
        inc_cw();
        printf("cw: %d\n",cw_);
        mhBackoff_.start(cw_, is_idle());*/
     //add by aloe
     //tx_resume();
    }
}

/* ======================================================================
   Incoming Packet Routines
   ====================================================================== */
void
Macuw802_11::send(Packet *p, Handler *h)
{
    struct hdr_cmn* cmh=HDR_CMN(p);
    struct hdr_uwvb* vbh = HDR_UWVB(p);
    printf("no %d",vbh->pk_num);
    callback_ = h;
   if (cmh->mobibct_flag_==0)
  {
        if (sendcount==1)
      {
        cmh->pkt_flag=2;
        sendcount=0;
        printf("send 2\n");
       }
       else if (sendcount==0)
        {
            stored=p->copy();
            struct hdr_cmn *h1=HDR_CMN(stored);
            h1->pkt_flag=1;
            Packet::free(p);
            sendcount=1;
            printf("stored 1 packet\n");
            tx_resume();
            return;
        }
    }
   else cmh->pkt_flag=3;
   // stored=p->copy();
    UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
    //n->SetTransmissionStatus(SENDE);
    printf("node %d UW802 packetsize_ %d\n",index_,cmh->size());
    double rTime;
    struct hdr_macuw802_11* dh;
    if(cmh->mobibct_flag_==0)
  {      dh = HDR_MACUW802_11(stored);
    if (n->setHopStatus){
     hdr_dst((char*)dh,n->next_hop);
     //vbh->target_id.addr_ = cmh->next_hop();
    }
    }
        dh = HDR_MACUW802_11(p);
  if (n->setHopStatus){
         hdr_dst((char*)dh,n->next_hop);
         //vbh->target_id.addr_ = cmh->next_hop();
        }

  /*  EnergyModel *em = netif_->node()->energy_model();
    if (em && em->sleep()) {
        em->set_node_sleep(0);
        em->set_node_state(EnergyModel::INROUTE);
    }*/
    if(cmh->mobibct_flag_==0)
      {
        sendDATA(stored);
        stored2=p->copy();
    }
    else
       sendDATA(p);
    if ((cmh->mobibct_flag_)==0)
    {
        sendRTS(ETHER_ADDR(dh->dh_ra));
    }


    /*
     * Assign the data packet a sequence number.
     */
    dh->dh_scontrol = sta_seqno_++;

    /*
     *  If the medium is IDLE, we must wait for a DIFS
     *  Space before transmitting.
     */
    if(mhBackoff_.busy() == 0) {
        if(is_idle()) {
            if (mhDefer_.busy() == 0) {
                /*
                 * If we are already deferring, there is no
                 * need to reset the Defer timer.
                 */
                if (bugFix_timer_) {
                     mhBackoff_.start(cw_, is_idle(),
                              phymib_.getDIFS());
                }
                else {
                    rTime = (Random::random() % cw_)
                        * (phymib_.getSlotTime());
                    mhDefer_.start(phymib_.getDIFS() +
                               rTime);

                }
            }
        } else {
            /*
             * If the medium is NOT IDLE, then we start
             * the backoff timer.
             */
            mhBackoff_.start(cw_, is_idle());
        }
    }

}

void
Macuw802_11::recv(Packet *p, Handler *h)
{

    printf("node %d 802_11 recv at %lf\n",index_,NOW);
    struct hdr_cmn *hdr = HDR_CMN(p);

    /*
     * Sanity Check
     */
    assert(initialized());

    /*
     *  Handle outgoing packets.
     */
    if(hdr->direction() == hdr_cmn::DOWN) {
        #ifdef DEBUG_LUV
        printf("node %d,direction down,send\n",index_);
        #endif
        send(p, h);
        return;
        }

   #ifdef DEBUG_LUV
    printf("direction up,not send\n");
    #endif
    /*
     *  Handle incoming packets.
     *
     *  We just received the 1st bit of a packet on the network
     *  interface.
     *
     */

    /*
     *  If the interface is currently in transmit mode, then
     *  it probably won't even see this packet.  However, the
     *  "air" around me is BUSY so I need to let the packet
     *  proceed.  Just set the error flag in the common header
     *  to that the packet gets thrown away.
     */
    if(tx_active_ && hdr->error() == 0) {
        hdr->error() = 1;
        printf("error\n");
    }

    if(rx_state_ == MAC_IDLE) {
        setRxState(MAC_RECV);
       UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
       n->SetTransmissionStatus(RECVE);
        pktRx_ = p;
        /*
         * Schedule the reception of this packet, in
         * txtime seconds.
         */
        mhRecv_.start(txtime(p));
    } else {
        /*
         *  If the power of the incoming packet is smaller than the
         *  power of the packet currently being received by at least
                 *  the capture threshold, then we ignore the new packet.
         */
        if(pktRx_->txinfo_.RxPr / p->txinfo_.RxPr >= p->txinfo_.CPThresh) {
            capture(p);
        } else {
            collision(p);
        }
    }
}

void
Macuw802_11::recv_timer()
{

    u_int32_t src;
    hdr_cmn *ch = HDR_CMN(pktRx_);
    hdr_macuw802_11 *mh = HDR_MACUW802_11(pktRx_);
    u_int32_t dst = ETHER_ADDR(mh->dh_ra);

    u_int8_t  type = mh->dh_fc.fc_type;
    u_int8_t  subtype = mh->dh_fc.fc_subtype;

    assert(pktRx_);
    assert(rx_state_ == MAC_RECV || rx_state_ == MAC_COLL);

        /*
         *  If the interface is in TRANSMIT mode when this packet
         *  "arrives", then I would never have seen it and should
         *  do a silent discard without adjusting the NAV.
         */
        if(tx_active_) {
                Packet::free(pktRx_);
                goto done;
        }

    /*
     * Handle collisions.
     */
    if(rx_state_ == MAC_COLL) {
        discard(pktRx_, DROP_MAC_COLLISION);
        set_nav(usec(phymib_.getEIFS()));
        goto done;
    }

    /*
     * Check to see if this packet was received with enough
     * bit errors that the current level of FEC still could not
     * fix all of the problems - ie; after FEC, the checksum still
     * failed.
     */
    if( ch->error() ) {
        Packet::free(pktRx_);
        set_nav(usec(phymib_.getEIFS()));
        goto done;
    }

    /*
     * IEEE 802.11 specs, section 9.2.5.6
     *	- update the NAV (Network Allocation Vector)
     */
    if(dst != (u_int32_t)index_) {
        set_nav(mh->dh_duration);
    }

        /* tap out - */
        if (tap_ && type == MAC_Type_Data &&
            MAC_Subtype_Data == subtype )
        tap_->tap(pktRx_);
    /*
     * Adaptive Fidelity Algorithm Support - neighborhood infomation
     * collection
     *
     * Hacking: Before filter the packet, log the neighbor node
     * I can hear the packet, the src is my neighbor
     */
  /*  if (netif_->node()->energy_model() &&
        netif_->node()->energy_model()->adaptivefidelity()) {
        src = ETHER_ADDR(mh->dh_ta);
        netif_->node()->energy_model()->add_neighbor(src);
    }*/
    /*
     * Address Filtering
     */
    if(dst != (u_int32_t)index_ && dst != MAC_BROADCAST) {
        /*
         *  We don't want to log this event, so we just free
         *  the packet instead of calling the drop routine.
         */
        discard(pktRx_, "---");
        goto done;
    }

    switch(type) {

    case MAC_Type_Management:
        discard(pktRx_, DROP_MAC_PACKET_ERROR);
        goto done;
    case MAC_Type_Control:
        switch(subtype) {
        case MAC_Subtype_RTS:
            recvRTS(pktRx_);
            break;
        case MAC_Subtype_CTS:
            recvCTS(pktRx_);
            break;
        case MAC_Subtype_ACK:
            recvACK(pktRx_);
            break;
        default:
            fprintf(stderr,"recvTimer1:Invalid MAC Control Subtype %x\n",
                subtype);
            exit(1);
        }
        break;
    case MAC_Type_Data:
        switch(subtype) {
        case MAC_Subtype_Data:
            recvDATA(pktRx_);
            break;
        default:
            fprintf(stderr, "recv_timer2:Invalid MAC Data Subtype %x\n",
                subtype);
            exit(1);
        }
        break;
    default:
        fprintf(stderr, "recv_timer3:Invalid MAC Type %x\n", subtype);
        exit(1);
    }
 done:
    pktRx_ = 0;
    rx_resume();
}


void
Macuw802_11::recvRTS(Packet *p)
{
    #ifdef DEBUG_LUV
    printf("node %d recvRTS\n",index_);
    #endif
    struct rts_frame *rf = (struct rts_frame*)p->access(hdr_mac::offset_);
    printf("node %d revRTS from %d at %lf\n",index_,ETHER_ADDR(rf->rf_ta),NOW);
    if(tx_state_ != MAC_IDLE) {
        printf("tx_state_ != MAC_IDLE\n");
        discard(p, DROP_MAC_BUSY);
        return;
    }

    /*
     *  If I'm responding to someone else, discard this RTS.
     */
    if(pktCTRL_) {
        printf("responding to someone else\n");
        discard(p, DROP_MAC_BUSY);
        return;
    }

    sendCTS(ETHER_ADDR(rf->rf_ta), rf->rf_duration);

    /*
     *  Stop deferring - will be reset in tx_resume().
     */
    if(mhDefer_.busy()) mhDefer_.stop();

    tx_resume();

    mac_log(p);
}

/*
 * txtime()	- pluck the precomputed tx time from the packet header
 */
double
Macuw802_11::txtime(Packet *p)
{
    struct hdr_cmn *ch = HDR_CMN(p);
    double t = ch->txtime();
    if (t < 0.0) {
        drop(p, "XXX");
        exit(1);
    }
    return t;
}


/*
 * txtime()	- calculate tx time for packet of size "psz" bytes
 *		  at rate "drt" bps
 */
double
Macuw802_11::txtime(double psz, double drt)
{
    double dsz = psz - phymib_.getPLCPhdrLen();
    //printf("PreambleLength: %d\n",phymib_.getPreambleLength());
    //printf("PLCPHeaderLength: %d\n",phymib_.getPLCPHeaderLength());
   // printf("PLCPhdrLen %d\n",phymib_.getPLCPhdrLen());
        int plcp_hdr = phymib_.getPLCPhdrLen() << 3;
    int datalen = (int)dsz << 3;
   // printf("datalen %d\n",datalen);
   // printf("getPLCPDataRate %lf\n",phymib_.getPLCPDataRate());
    double t = (((double)plcp_hdr)/phymib_.getPLCPDataRate())
                                       + (((double)datalen)/drt);
    return(t);
}



void
Macuw802_11::recvCTS(Packet *p)
{
    printf("node %d recvCTS at %lf\n",index_,NOW);
    if(tx_state_ != MAC_RTS) {
        #ifdef DEBUG_LUV
        printf("disard CTS\n");
       #endif
        discard(p, DROP_MAC_INVALID_STATE);
        return;
    }
    printf("acceptCTS\n");
    assert(pktRTS_);
    Packet::free(pktRTS_); pktRTS_ = 0;

    assert(pktTx_);
    mhSend_.stop();

    /*
     * The successful reception of this CTS packet implies
     * that our RTS was successful.
     * According to the IEEE spec 9.2.5.3, you must
     * reset the ssrc_, but not the congestion window.
     */
    ssrc_ = 0;
    printf("tx_resume before\n");
    tx_resume();

    mac_log(p);
}

void
Macuw802_11::recvDATA(Packet *p)
{

    struct hdr_macuw802_11 *dh = HDR_MACUW802_11(p);
    u_int32_t dst, src, size;
    struct hdr_cmn *ch = HDR_CMN(p);
    dst = ETHER_ADDR(dh->dh_ra);
    src = ETHER_ADDR(dh->dh_ta);
    size = ch->size();
    printf("node %d recvDATA from %d to %d at %lf txstate is %x \n",index_,src,dst,NOW,tx_state_);
    /*
     * Adjust the MAC packet size - ie; strip
     * off the mac header
     */


    if((ch->mobibct_flag_)==1)
    {
       #ifdef DEBUG_LUV
        printf("dst==broadcast %d",int(dst==MAC_BROADCAST));
        #endif
        //assert(pktCTRL_);
        //Packet::free(pktCTRL_); pktCTRL_ = 0;
        printf("acceptDATA from mobile\n");
        //mhSend_.stop();
        //ssrc_=0;
        //pktTx_=0;
        //tx_resume();
        uptarget_->recv(p, this);
        return;
    }
    else
    {

       ch->size() -= phymib_.getHdrLen11();
        ch->num_forwards() += 1;
         hdr_cmn *cmh=HDR_CMN(p);
         printf("recv no %d\n",cmh->pkt_flag);
    /*
     *  If we sent a CTS, clean up...
     */    if(dst != MAC_BROADCAST) {
        if(size >= macmib_.getRTSThreshold()) {
            if (tx_state_ == MAC_CTS) {
                if (cmh->pkt_flag == 2)
               { assert(pktCTRL_);
                Packet::free(pktCTRL_); pktCTRL_ = 0;
                mhSend_.stop();}
                /*
                 * Our CTS got through.
                 */
            } else {
                discard(p, DROP_MAC_BUSY);
                return;
            }

            printf("acceptDATA no %d\n", cmh->pkt_flag);
            if(cmh->pkt_flag == 2)
            {
                sendACK(src);
             }
            else if(cmh->pkt_flag == 1)
            { uptarget_->recv(p, (Handler*) 0);
                return;
            }
           tx_resume();

        } else {
            /*
             *  We did not send a CTS and there's no
             *  room to buffer an ACK.
             */
            if(pktCTRL_) {
                discard(p, DROP_MAC_BUSY);
                return;
            }
            sendACK(src);
            if(mhSend_.busy() == 0)
                tx_resume();
        }
    }
    }

    /* ============================================================
       Make/update an entry in our sequence number cache.
       ============================================================ */

    /* Changed by Debojyoti Dutta. This upper loop of if{}else was
       suggested by Joerg Diederich <dieder@ibr.cs.tu-bs.de>.
       Changed on 19th Oct'2000 */

        if(dst != MAC_BROADCAST) {
                if (src < (u_int32_t) cache_node_count_) {
                        Host *h = &cache_[src];

                        if(h->seqno && h->seqno == dh->dh_scontrol) {
                                discard(p, DROP_MAC_DUPLICATE);
                                return;
                        }
                        h->seqno = dh->dh_scontrol;
                } else {
            static int count = 0;
            if (++count <= 10) {
                printf ("MAC_802_11: accessing MAC cache_ array out of range (src %u, dst %u, size %d)!\n", src, dst, cache_node_count_);
                if (count == 10)
                    printf ("[suppressing additional MAC cache_ warnings]\n");
            };
        };
    }

    /*
     *  Pass the packet up to the link-layer.
     *  XXX - we could schedule an event to account
     *  for this processing delay.
     */

    /* in BSS mode, if a station receives a packet via
     * the AP, and higher layers are interested in looking
     * at the src address, we might need to put it at
     * the right place - lest the higher layers end up
     * believing the AP address to be the src addr! a quick
     * grep didn't turn up any higher layers interested in
     * the src addr though!
     * anyway, here if I'm the AP and the destination
     * address (in dh_3a) isn't me, then we have to fwd
     * the packet; we pick the real destination and set
     * set it up for the LL; we save the real src into
     * the dh_3a field for the 'interested in the info'
     * receiver; we finally push the packet towards the
     * LL to be added back to my queue - accomplish this
     * by reversing the direction!*/

   if ((bss_id() == addr()) && ((u_int32_t)ETHER_ADDR(dh->dh_ra)!= MAC_BROADCAST)&& ((u_int32_t)ETHER_ADDR(dh->dh_3a) != ((u_int32_t)addr()))) {
       printf("bss_id\n");
        struct hdr_cmn *ch = HDR_CMN(p);
        u_int32_t dst = ETHER_ADDR(dh->dh_3a);
        u_int32_t src = ETHER_ADDR(dh->dh_ta);
        //if it is a broadcast pkt then send a copy up
         //my stack also
        if (dst == MAC_BROADCAST) {
            uptarget_->recv(p->copy(), (Handler*) 0);
        }

        ch->next_hop() = dst;
        STORE4BYTE(&src, (dh->dh_3a));
        ch->addr_type() = NS_AF_ILINK;
        ch->direction() = hdr_cmn::DOWN;
    }
    uptarget_->recv(p, (Handler*) 0);
}


void
Macuw802_11::recvACK(Packet *p)
{
    #ifdef DEBUG_LUV
    printf("node %d recvACK at %lf\n",index_,NOW);
    #endif
    printf("node %d recvACK at %lf\n",index_,NOW);
    if(tx_state_ != MAC_SEND) {
        discard(p, DROP_MAC_INVALID_STATE);
        return;
    }
    assert(pktTx_);
    printf("acceptACK\n");
    mhSend_.stop();

    /*
     * The successful reception of this ACK packet implies
     * that our DATA transmission was successful.  Hence,
     * we can reset the Short/Long Retry Count and the CW.
     *
     * need to check the size of the packet we sent that's being
     * ACK'd, not the size of the ACK packet.
     */
    if((u_int32_t) HDR_CMN(pktTx_)->size() <= macmib_.getRTSThreshold())
        ssrc_ = 0;
    else
        slrc_ = 0;
    rst_cw();
    Packet::free(pktTx_);
    pktTx_ = 0;

    /*
     * Backoff before sending again.
     */
    assert(mhBackoff_.busy() == 0);
    mhBackoff_.start(cw_, is_idle());

    tx_resume();

    mac_log(p);

}
