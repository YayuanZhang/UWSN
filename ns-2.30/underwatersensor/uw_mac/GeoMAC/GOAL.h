#ifndef __GOAL_H__
#define	__GOAL_H__

/***********************************************************
 * This is a geo-routing MAC designed in cross-layer approach
 * It smoonthly integrates VBF and handshake to avoid collisions
 * and guarantee a better reliability
 * The corresponding conference paper is
 @inproceedings{Zhu2010co:geomac,
	author={Yibo Zhu and Zhong Zhou and Peng Zheng and Jun-Hong Cui},
	title={{An Efficient Geo-Routing Aware MAC Protocol for Underwater Acoustic Networks (Invited Paper)}},
	year= {2010},
	booktitle= {Proc. ADHOCNETS},
	pages= {185--200},
	address= {Victoria, BC, Canada}
 }

 The Journal verion is
 @article{Zhu2011at:effic,
	author={Yibo Zhu and Zhong Zhou and Zheng Peng and Jun-Hong Cui},
	title={{An Efficient Geo-routing Aware MAC Protocol for Underwater Acoustic Networks}},
	year= {2011},
	journal= {ICST Transactions on Mobile Communications and Applications},
	pages= {1--14},
	number= {7--9},
	volume= {11}
 }
 
 */

#include "GOAL_hdr.h"
#include <random.h>
#include <timer-handler.h>

#include <mac.h>
#include "underwatersensor/uw_mac/underwatermac.h"
#include "underwatersensor/uw_mac/underwaterchannel.h"
#include "underwatersensor/uw_mac/underwaterpropagation.h"

#include <deque>
#include <set>
#include <map>

using namespace std;


#define GOAL_CALLBACK_DELAY	0.001

class GOAL;
class GOAL_DataSendTimer;

struct GOAL_PktQ{
	int BurstNum;
	deque<Packet*>  Q_;

	GOAL_PktQ(): BurstNum(0) {

	}
};

//---------------------------------------------------------------------
class GOAL_PreSendTimer: public TimerHandler{
public:
	GOAL_PreSendTimer(GOAL* mac): TimerHandler(), mac_(mac) {
	}

	Packet*&	pkt() {
		return pkt_;
	}

protected:
	GOAL*		mac_;
	Packet*		pkt_;
	virtual void expire(Event* e);
};

//---------------------------------------------------------------------
class GOAL_CallbackHandler: public Handler{
public:
	GOAL_CallbackHandler(GOAL* mac): Handler(), mac_(mac) {
	}

protected:
	GOAL*		mac_;
	virtual void handle(Event* e);
};

//---------------------------------------------------------------------
class GOAL_StatusHandler: public Handler{
public:
	GOAL_StatusHandler(GOAL* mac): Handler(), mac_(mac) {
	}

protected:
	GOAL*		mac_;
	virtual void handle(Event* e);
};


//---------------------------------------------------------------------
class GOAL_BackoffTimer: public TimerHandler{
public:
	GOAL_BackoffTimer(GOAL* mac): TimerHandler(), mac_(mac) {
	}

	Packet*&	ReqPkt() {
		return ReqPkt_;
	}
	Time& BackoffTime() {
		return BackoffTime_;
	}
	SchedElem* SE() {
		return SE_;
	}

	void	setSE(SchedElem* SE) {
		SE_ = SE;
	}
protected:
	GOAL*		mac_;
	Packet*		ReqPkt_;
	SchedElem*	SE_;
	Time		BackoffTime_;
	virtual void expire(Event* e);
};


//---------------------------------------------------------------------
class GOAL_AckTimeoutTimer: public TimerHandler{
public:
	GOAL_AckTimeoutTimer(GOAL* mac): TimerHandler(), mac_(mac) {
	}

	/*Packet*& pkt() {
		return pkt_;
	}*/
	/*Time& SendTime() {
		return SendTime_;
	}*/
	map<int, Packet*>& PktSet() {
		return PktSet_;
	}

protected:
	GOAL*		mac_;
	map<int, Packet*> PktSet_; //map uid to packet
	//Packet*		pkt_;
	//Time		SendTime_;  //the time when this packet will be sent out
	virtual void expire(Event* e);
};

//---------------------------------------------------------------------
class GOAL_NxtRoundTimer: public TimerHandler{
public:
	GOAL_NxtRoundTimer(GOAL* mac): TimerHandler(), mac_(mac) {
	}

protected:
	GOAL*		mac_;
	virtual void expire(Event* e);
};

//---------------------------------------------------------------------
class GOAL_DataSendTimer: public TimerHandler{
public:
	GOAL_DataSendTimer(GOAL* mac): TimerHandler(), mac_(mac) {
		MinBackoffTime_ = 100000000.0;
		NxtHop_ = -1;
		GotRep_ = false;
	}

	set<Packet*>& DataPktSet() {
		return DataPktSet_;
	}

	nsaddr_t& NxtHop() {
		return NxtHop_;
	}

	Time& MinBackoffTime() {
		return MinBackoffTime_;
	}

	Time& TxTime() {
		return TxTime_;
	}

	int& ReqID() {
		return ReqID_;
	}

	bool& GotRep() {
		return GotRep_;
	}

	SchedElem* SE() {
		return SE_;
	}

	void SetSE(SchedElem* SE) {
		SE_ = SE;
	}

protected:
	GOAL*	mac_;
	set<Packet*> DataPktSet_;
	nsaddr_t	NxtHop_;
	Time		MinBackoffTime_;
	Time		TxTime_;

	SchedElem*	SE_;

	int			ReqID_;
	bool		GotRep_;

	virtual void expire(Event* e);
};


//---------------------------------------------------------------------
//used for accumulative Ack
class GOAL_SinkAccumAckTimer: public TimerHandler{
public:
	GOAL_SinkAccumAckTimer(GOAL* mac): TimerHandler(), mac_(mac) {

	}

	set<int>& AckSet() {
		return AckSet_;
	}


protected:
	GOAL*		mac_;
	set<int>	AckSet_;
	virtual void expire(Event* e);
};

struct RecvedInfo{
	nsaddr_t	Sender;
	Time		RecvTime;
};

//---------------------------------------------------------------------
//check if the destination is same. If same, schedule the data together!
class GOAL: public UnderwaterMac{
public:
	GOAL();
	int  command(int argc, const char*const* argv);
	// to process the incomming packet
	virtual  void RecvProcess(Packet*);
	// to process the outgoing packet
	virtual  void TxProcess(Packet*);

private:
	int		MaxBurst;			//maximum number of packets sent in one burst
	Time	DataPktInterval;
	Time	GuardTime;			//to tolerate the mobility and inaccuracy of position information
	Time	EstimateError;


	bool	IsForwarding;		//true for initializing a session of forwarding packet
	double	PropSpeed;		//the speed of propagation
	double	TxRadius;
	Time	MaxDelay;		//the maximum propagation delay between two one-hop neighbor
	double	PipeWidth;		//for VBF and HH-VBF
	TimeSchedQueue TSQ_;	//Time schedule queue.
	static	int ReqPktSeq_;
	int		DataPktSize;	//the size of data packet, in byte
	int		MaxRetransTimes;

	/*
	 * which kind of backoff function of existing routing protocol is used, such as HH-VBF
	 */
	BackoffType					backoff_type;
	GOAL_SinkAccumAckTimer		SinkAccumAckTimer;
	Time						MaxBackoffTime;		//the max time for waiting for the reply packet

	Time						VBF_MaxDelay;		//predefined max delay for vbf


	set<GOAL_PreSendTimer*>		PreSendTimerSet_;
	set<GOAL_BackoffTimer*>		BackoffTimerSet_;
	//data packet is stored here. It will be inserted into PktSendTimerSet_ after receiving AcK
	set<GOAL_AckTimeoutTimer*>	AckTimeoutTimerSet_;
	//set<GOAL_RepTimeoutTimer*>	RepTimeoutTimerSet_;
	set<GOAL_DataSendTimer*>	DataSendTimerSet_;

	map<nsaddr_t, GOAL_PktQ>  PktQs;
	int				SinkSeq;     //the packet to which destination should be sent.
	int				QsPktNum;    //the number of packets in PktQs.
	//map<int, GOAL_AckTimeoutTimer*>	SentPktSet;

	map<int, Time> OriginPktSet;

	map<int, RecvedInfo> RecvedList;	/*the list of recved packet. map uid to Recv info( recv time+sender)
								 *used for preventing from retransmission
								 */
	set<int>	SinkRecvedList;
	Time	RecvedListAliveTime;
	GOAL_NxtRoundTimer		NxtRoundTimer;
	Time	NxtRoundMaxWaitTime;

	void purifyRecvedList();


	void PreSendPkt(Packet* pkt, Time delay=0.000001);  //default delay should be 0, but I am afraid it is stored as a minus value
	void sendoutPkt(Packet* pkt);

	void prepareDataPkts();
	void sendDataPkts(set<Packet*>& DataPktSet, nsaddr_t NxtHop, Time TxTime);


	Packet* makeReqPkt(set<Packet*>& DataPktset, Time DataSendTime, Time TxTime);	//broadcast. the datapkt is the one which will be sent
	Packet* makeRepPkt(Packet* ReqPkt, Time BackoffTime);		//unicast. Make sure that make ack pkt first, then backoff
	Packet* makeAckPkt(set<int>& AckSet, bool PSH=false, int ReqID=-1);	//only sink uses this ack
	void	processReqPkt(Packet* ReqPkt);
	void	processRepPkt(Packet* RepPkt);
	void	processDataPkt(Packet* DataPkt);
	void	processAckPkt(Packet* AckPkt);
	void	processPSHAckPkt(Packet* AckPkt);

	void	processPreSendTimeout(GOAL_PreSendTimer* PreSendTimer);
	//void	processRepTimeout(GOAL_RepTimeoutTimer* RepTimeoutTimer);
	void	processBackoffTimeOut(GOAL_BackoffTimer* backoff_timer);
	void	processAckTimeout(GOAL_AckTimeoutTimer* AckTimeoutTimer);
	void	processDataSendTimer(GOAL_DataSendTimer* DataSendTimer);
	void	processSinkAccumAckTimeout();
	void	processOverhearedRepPkt(Packet* RepPkt);
	void	processNxtRoundTimeout();

	void	gotoNxtRound();

	void	insert2PktQs(Packet* DataPkt, bool FrontPush=false);


	GOAL_CallbackHandler	CallbackHandler;
	Event					CallbackEvent;
	GOAL_StatusHandler		StatusHandler;
	Event					StatusEvent;

	//calculate the distance between two nodes.
	double	dist(NodePos& Pos1, NodePos& Pos2);

	//backoff time calculation method
	Time	getBackoffTime(Packet* ReqPkt);
	//VBF backoff time
	//minus value will be returned if this node is out of forward area.
	Time	getVBFbackoffTime(NodePos& Source, NodePos& Sender, NodePos& Sink);
	Time	getHH_VBFbackoffTime(NodePos& Sender, NodePos& Sink);

	//distance from this node to line
	double  distToLine(NodePos& LinePoint1, NodePos& LinePoint2);
	Time	getTxtimebyPktSize(int PktSize);
	Time	JitterStartTime(Time Txtime); //Jitter the start time to avoid collision

protected:
	void	CallbackProcess(Event* e);
	void	StatusProcess(Event* e);


	friend class GOAL_CallbackHandler;
	friend class GOAL_BackoffTimer;
	friend class GOAL_PreSendTimer;
	//friend class GOAL_RepTimeoutTimer;
	friend class GOAL_AckTimeoutTimer;
	friend class GOAL_DataSendTimer;
	friend class GOAL_SinkAccumAckTimer;
	friend class GOAL_StatusHandler;
	friend class GOAL_NxtRoundTimer;
};


#endif
