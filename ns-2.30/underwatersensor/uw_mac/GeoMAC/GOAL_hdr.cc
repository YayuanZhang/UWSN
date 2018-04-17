#include "GOAL_hdr.h"

int hdr_GOAL_req::offset_;

static class GOAL_req_HeaderClass: public PacketHeaderClass{
public:
	GOAL_req_HeaderClass():
			PacketHeaderClass("PacketHeader/GOAL_REQ",sizeof(hdr_GOAL_req))
	{
		bind_offset(&hdr_GOAL_req::offset_);
	}
}class_GOAL_req_hdr;


//---------------------------------------------------------------------
int hdr_GOAL_rep::offset_;
static class GOAL_rep_HeaderClass: public PacketHeaderClass{
public:
	GOAL_rep_HeaderClass() :
			PacketHeaderClass("PacketHeader/GOAL_REP", sizeof(hdr_GOAL_rep))
	{
		bind_offset(&hdr_GOAL_rep::offset_);
	}

}class_GOAL_rep_hdr;


//---------------------------------------------------------------------
int hdr_GOAL_ack::offset_;
static class GOAL_ack_HeaderClass: public PacketHeaderClass{
public:
	GOAL_ack_HeaderClass() :
			PacketHeaderClass("PacketHeader/GOAL_ACK", sizeof(hdr_GOAL_ack))
	{
		bind_offset(&hdr_GOAL_ack::offset_);
	}

}class_GOAL_ack_hdr;


//---------------------------------------------------------------------
SchedElem::SchedElem(Time BeginTime_, Time EndTime_, bool IsRecvSlot_)
{
	BeginTime = BeginTime_;
	EndTime = EndTime_;
	IsRecvSlot = IsRecvSlot_;
}

//---------------------------------------------------------------------
SchedElem::SchedElem(SchedElem& e)
{
	BeginTime = e.BeginTime;
	EndTime = e.EndTime;
}


//---------------------------------------------------------------------
TimeSchedQueue::TimeSchedQueue(Time MinInterval, Time BigIntervalLen)
{
	MinInterval_ = MinInterval;
	BigIntervalLen_ = BigIntervalLen;
}

//---------------------------------------------------------------------
SchedElem* TimeSchedQueue::insert(SchedElem *e)
{
	list<SchedElem*>::iterator pos = SchedQ_.begin();
	while( pos != SchedQ_.end() ) {

		if( (*pos)->BeginTime < e->BeginTime )
			break;

		pos++;
	}

	if( pos == SchedQ_.begin() ) {
		SchedQ_.push_front(e);
	}
	else if( pos == SchedQ_.end() ) {
		SchedQ_.push_back(e);
	}
	else{
		SchedQ_.insert(pos, e);
	}

	return e;
}


//---------------------------------------------------------------------
SchedElem* TimeSchedQueue::insert(Time BeginTime, Time EndTime, bool IsRecvSlot)
{
	SchedElem* e = new SchedElem(BeginTime, EndTime, IsRecvSlot);
	return insert(e);
}


//---------------------------------------------------------------------
void TimeSchedQueue::remove(SchedElem *e)
{
	SchedQ_.remove(e);
	delete e;
}


//---------------------------------------------------------------------
Time TimeSchedQueue::getAvailableTime(Time EarliestTime, Time SlotLen, bool BigInterval)
{
	/*
	 * how to select the sending time is critical
	 * random in the availabe time interval
	 */
	clearExpiredElems();
	Time MinStartInterval = 0.1;
	Time Interval = 0.0;
	if( BigInterval ) {
		Interval = BigIntervalLen_;	//Big Interval is for DATA packet.
		MinStartInterval = 0.0;		//DATA packet does not need to Jitter the start time.
	}
	else {
		Interval = MinInterval_;
	}

	Time LowerBeginTime = EarliestTime;
	Time UpperBeginTime = -1.0;   //infinite;
	list<SchedElem*>::iterator pos = SchedQ_.begin();


	while( pos != SchedQ_.end() ) {
		while( pos!=SchedQ_.end() && (*pos)->IsRecvSlot ) {
			pos++;
		}

		if( pos == SchedQ_.end() ) {
			break;
		}

		if( (*pos)->BeginTime - LowerBeginTime > SlotLen+Interval+MinStartInterval ) {
			break;
		}
		else {
			LowerBeginTime = max((*pos)->EndTime, LowerBeginTime);
		}
		pos++;
	}

	UpperBeginTime = LowerBeginTime + MinStartInterval;


	return Random::uniform(LowerBeginTime, UpperBeginTime);
}


//---------------------------------------------------------------------
bool TimeSchedQueue::checkCollision(Time BeginTime, Time EndTime)
{
	clearExpiredElems();

	list<SchedElem*>::iterator pos = SchedQ_.begin();
	BeginTime -= MinInterval_;   //consider the guard time
	EndTime += MinInterval_;

	if( SchedQ_.empty() ) {
		return true;
	}
	else {
		while( pos != SchedQ_.end() ) {
			if( (BeginTime < (*pos)->BeginTime && EndTime > (*pos)->BeginTime)
				|| (BeginTime<(*pos)->EndTime && EndTime > (*pos)->EndTime ) ) {
				return false;
			}
			else {
				pos++;
			}
		}
		return true;
	}
}


//---------------------------------------------------------------------
void TimeSchedQueue::clearExpiredElems()
{
	SchedElem* e = NULL;
	while( !SchedQ_.empty() ) {
		e = SchedQ_.front();
		if( e->EndTime + MinInterval_ < NOW )
		{
			SchedQ_.pop_front();
			delete e;
			e = NULL;
		}
		else
			break;
	}
}


//---------------------------------------------------------------------
void TimeSchedQueue::print(char* filename)
{
	FILE* stream = fopen(filename, "a");
	list<SchedElem*>::iterator pos = SchedQ_.begin();
	while( pos != SchedQ_.end() ) {
		fprintf(stream, "(%f, %f)\t", (*pos)->BeginTime, (*pos)->EndTime);
		pos++;
	}
	fprintf(stream, "\n");
	fclose(stream);
}








