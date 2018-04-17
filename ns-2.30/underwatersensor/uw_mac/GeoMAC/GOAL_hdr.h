#ifndef __GOAL_hdr_h__
#define __GOAL_hdr_h__

#include <packet.h>
#include <random.h>
#include <timer-handler.h>

#include <list>
using namespace std;


#define NSADDR_T_SIZE 10
typedef double Time;
//using implicit ACK.
enum BackoffType{
	VBF,
	HH_VBF
};


struct NodePos{
	double X_;
	double Y_;
	double Z_;

	void setValue(double X, double Y, double Z) {
		X_ = X;
		Y_ = Y;
		Z_ = Z;
	}

	void setValue(NodePos& Pos) {
		X_ = Pos.X_;
		Y_ = Pos.Y_;
		Z_ = Pos.Z_;
	}
};



//PT_GOAL_REQ
//GOAL stands for Mac-Routing
struct hdr_GOAL_req{
	nsaddr_t	SA_;	//source address        //use 10 bits
	nsaddr_t	RA_;	//receiver address      //use 10 bits
	nsaddr_t	DA_;	//destination address   //use 10 bits  //not useful, so it is not counted in size
	Time		SendTime_;						//use 2 bytes
	Time		TxTime_;						//use 1 byte
	int			ReqID_;							//use 1 byte

	//following three are for VBF
	NodePos		SenderPos;						//3*2bytess
	//following two are for HH-VBF
	NodePos		SinkPos;						//3*2bytes
	NodePos		SourcePos;						//3*2bytes

	static int size(BackoffType type) {

		int hdr_size = 10*2 + 4*8;

		switch( type ) {
			case VBF:
				hdr_size += 96;
				break;
			case HH_VBF:
				hdr_size += 144;
				break;
			default:
				;
		}

		return hdr_size/8+1;
	}

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_GOAL_req* access(const Packet*  p) {
		return (hdr_GOAL_req*) p->access(offset_);
	}
};


//PT_GOAL_REP
struct hdr_GOAL_rep{
	nsaddr_t	SA_;	//source address	10 bit
	nsaddr_t	RA_;	//receiver address  10 bit
	Time		SendTime_;			//2 B
	Time		TxTime_;			//1 B
	int			ReqID_;				//1 B
	Time		BackoffTime;  /* used to decide which replyer is better
							   * This item can be calculated by ReplyerPos and
							   * sender's knowledge. To simplify the code, we include
							   * it in reply packet, so we do not count this item
							   * in the packet size.
							   */
									//2 B

	NodePos		ReplyerPos;			//6 B

	static int size(BackoffType type) {
		return 15; //bytes
	}

	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_GOAL_rep* access(const Packet*  p) {
		return (hdr_GOAL_rep*) p->access(offset_);
	}
};



//PT_GOAL_ACK
/*only the destination use this message*/
struct hdr_GOAL_ack{
	nsaddr_t	SA_;	//10 bits, source address
	nsaddr_t	RA_;	//10 bites, receiver address: BROADCAST because it is broadcast mac
	bool		PUSH;	/*1 bit. if PUSH is set, recver should check the DataSendTimerSet
						   there must be some packets have been sent before.
						*/
	int			ReqID_;

	static int size() {
			return 4;  //bytes
	}


	static int offset_;
  	inline static int& offset() { return offset_; }
  	inline static hdr_GOAL_ack* access(const Packet*  p) {
		return (hdr_GOAL_ack*) p->access(offset_);
	}
};



//treat default packet type as data packet.


struct SchedElem{
	Time BeginTime;
	Time EndTime;
	bool IsRecvSlot;
	SchedElem(Time BeginTime_, Time EndTime_, bool IsRecvSlot_=false);
	SchedElem(SchedElem& e);
};


class TimeSchedQueue{
private:
	list<SchedElem*> SchedQ_;
	Time MinInterval_;
	Time BigIntervalLen_;

public:
	TimeSchedQueue(Time MinInterval, Time BigIntervalLen);
	SchedElem* insert(Time BeginTime, Time EndTime, bool IsRecvSlot=false);
	SchedElem* insert(SchedElem* e);
	void remove(SchedElem* e);
	//return an available start time for sending packet
	Time getAvailableTime(Time EarliestTime, Time SlotLen, bool BigInterval = false);
	//true for no collision
	bool checkCollision(Time BeginTime, Time EndTime);
	void clearExpiredElems();
	//for test
	void print(char* filename);
};


#endif






