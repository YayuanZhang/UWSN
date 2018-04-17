#include "GOAL.h"
#include "underwatersensor/uw_routing/vectorbasedforward.h"


int GOAL::ReqPktSeq_ = 0;

static class GOAL_Class : public TclClass {
public:
	GOAL_Class():TclClass("Mac/UnderwaterMac/GOAL") {}
	TclObject* create(int, const char*const*) {
		return (new GOAL());
	}
}class_GOAL;



//---------------------------------------------------------------------
void  GOAL_CallbackHandler::handle(Event *e)
{
	mac_->CallbackProcess(e);
}

void GOAL_StatusHandler::handle(Event* e)
{
	mac_->StatusProcess(e);
}

//---------------------------------------------------------------------
void GOAL_BackoffTimer::expire(Event *e)
{
	mac_->processBackoffTimeOut(this);
}


//---------------------------------------------------------------------
void GOAL_PreSendTimer::expire(Event *e)
{
	mac_->processPreSendTimeout(this);
}


////---------------------------------------------------------------------
//void GOAL_RepTimeoutTimer::expire(Event *e)
//{
//	mac_->processRepTimeout(this);
//}


//---------------------------------------------------------------------
void GOAL_AckTimeoutTimer::expire(Event* e)
{
	mac_->processAckTimeout(this);
}


//---------------------------------------------------------------------
void GOAL_DataSendTimer::expire(Event* e)
{
	mac_->processDataSendTimer(this);
}


//---------------------------------------------------------------------
void GOAL_SinkAccumAckTimer::expire(Event *e)
{
	mac_->processSinkAccumAckTimeout();
}


void GOAL_NxtRoundTimer::expire(Event *e)
{
	mac_->processNxtRoundTimeout();
}

//---------------------------------------------------------------------
GOAL::GOAL():UnderwaterMac(), MaxBurst(1), DataPktInterval(0.0001), GuardTime(0.05),
	EstimateError(0.005), TSQ_(0.01, 1), MaxRetransTimes(6), SinkAccumAckTimer(this), SinkSeq(0),
	QsPktNum(0), RecvedListAliveTime(100.0), NxtRoundTimer(this), NxtRoundMaxWaitTime(1.0),
	CallbackHandler(this), StatusHandler(this)
{
	PropSpeed = 1500.0;
	IsForwarding = false;
	TxRadius = UnderwaterChannel::Transmit_distance();
	MaxDelay = TxRadius/PropSpeed;
	PipeWidth = 100.0;
	//DataPktSize = 300;   //Byte
	backoff_type = VBF;

	bind("MaxBurst", &MaxBurst);
	bind("VBF_MaxDelay", &VBF_MaxDelay);
	//bind("DataPktSize", &DataPktSize);
	bind("MaxRetxTimes", &MaxRetransTimes);
	MaxBackoffTime = 4*MaxDelay+VBF_MaxDelay*1.5+2;
	Random::seed_heuristically();
}


//---------------------------------------------------------------------
int GOAL::command(int argc, const char *const *argv)
{
	if(argc == 3) {
		if (strcmp(argv[1], "node_on") == 0) {
			Node* n1=(Node*) TclObject::lookup(argv[2]);
			if (!n1) return TCL_ERROR;
			node_ =n1;
			return TCL_OK;
		}
	}

	return UnderwaterMac::command(argc, argv);
}



//---------------------------------------------------------------------
void GOAL::CallbackProcess(Event* e)
{
	callback_->handle(e);
}


//---------------------------------------------------------------------
void GOAL::StatusProcess(Event *e)
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	if( SEND == n->TransmissionStatus() ){
        n->SetTransmissionStatus(IDLEE);
	}
}

//---------------------------------------------------------------------
void GOAL::RecvProcess(Packet *pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);
	hdr_mac* mach = hdr_mac::access(pkt);
	nsaddr_t dst = mach->macDA();


    if( cmh->error() )
    {
     	//printf("broadcast:node %d  gets a corrupted packet at  %f\n",index_,NOW);
     	if(drop_)
			drop_->recv(pkt,"Error/Collision");
     	else
			Packet::free(pkt);

     	return;
    }


	if( dst == index_ || dst == nsaddr_t(MAC_BROADCAST) ) {

		switch(cmh->ptype() ) {
			case PT_GOAL_REQ:			//req is broadcasted, it is also data-ack
				processReqPkt(pkt);  //both for me and overhear
				break;
			case PT_GOAL_REP:			//unicast, but other's should overhear
				processRepPkt(pkt);
				break;
			case PT_GOAL_ACK:
				if( hdr_GOAL_ack::access(pkt)->PUSH )
					processPSHAckPkt(pkt);
				else
					processAckPkt(pkt);
				break;
			default:
				processDataPkt(pkt);
				//free data if it's not for this node
				//
				return;
				;
		}

	}
	/*packet to other nodes*/
	else if( cmh->ptype() == PT_GOAL_REP ) {
		/*based on other's ack, reserve
		  timeslot to avoid receive-receive collision*/
		processOverhearedRepPkt(pkt);
	}

	Packet::free(pkt);

}


//---------------------------------------------------------------------
void GOAL::TxProcess(Packet *pkt)
{
	//this node must be the origin

	hdr_cmn* cmh = hdr_cmn::access(pkt);
	hdr_ip* iph = hdr_ip::access(pkt);
	//schedule immediately after receiving. Or cache first then schedule periodically

	UnderwaterSensorNode* n = (UnderwaterSensorNode*)node_;
	hdr_uwvb* vbh = hdr_uwvb::access(pkt);
	n->update_position();
	vbh->info.ox = n->X();
	vbh->info.oy = n->Y();
	vbh->info.oz = n->Z();

	cmh->size() += sizeof(nsaddr_t)*2;  //plus the size of MAC header: Source address and destination address
	cmh->txtime() = getTxtimebyPktSize(cmh->size());
	cmh->num_forwards() = 0;	//use this area to store number of retrans

	iph->daddr() = vbh->target_id.addr_;
	iph->saddr() = index_;
	//suppose Sink has broadcast its position before. To simplify the simulation, we
	//directly get the position of the target node (Sink)
	UnderwaterSensorNode* tn = (UnderwaterSensorNode*)(Node::get_node_by_address(vbh->target_id.addr_));
	tn->update_position();
	vbh->info.tx = tn->X();
	vbh->info.ty = tn->Y();
	vbh->info.tz = tn->Z();

	OriginPktSet[cmh->uid()] = NOW;
	insert2PktQs(pkt);
	Scheduler::instance().schedule(&CallbackHandler,
						&CallbackEvent, GOAL_CALLBACK_DELAY);
}

//---------------------------------------------------------------------
//packet will be sent DataSendTime seconds later
Packet* GOAL::makeReqPkt(set<Packet*>& DataPktSet, Time DataSendTime, Time TxTime)
{
	UnderwaterSensorNode* n = (UnderwaterSensorNode*)node_;
	n->update_position();
	Packet* pkt = Packet::alloc();
	hdr_mac* mach = hdr_mac::access(pkt);
	hdr_GOAL_req* vmq = hdr_GOAL_req::access(pkt);
	hdr_cmn* cmh = hdr_cmn::access(pkt);
	Packet* DataPkt = *(DataPktSet.begin());
	hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);

	vmq->RA_ = MAC_BROADCAST;
	vmq->SA_ = index_;
	vmq->DA_ = vbh->target_id.addr_;   //sink address
	ReqPktSeq_++;
	vmq->ReqID_ = ReqPktSeq_;
	vmq->SenderPos.setValue(n->X(), n->Y(), n->Z());
	vmq->SendTime_ = DataSendTime-NOW;
	vmq->TxTime_ = TxTime;
	vmq->SinkPos.setValue(vbh->info.tx, vbh->info.ty, vbh->info.tz);
	vmq->SourcePos.setValue(vbh->info.ox, vbh->info.oy, vbh->info.oz);

	cmh->ptype() = PT_GOAL_REQ;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->error() = 0;
	cmh->next_hop() = MAC_BROADCAST;
	cmh->size() = vmq->size(backoff_type)+(NSADDR_T_SIZE*DataPktSet.size())/8+1;
	cmh->txtime() = getTxtimebyPktSize(cmh->size());
	cmh->addr_type() = NS_AF_ILINK;
	cmh->timestamp() = NOW;

	mach->macDA() = vmq->RA_;
	mach->macSA() = index_;

	pkt->allocdata(sizeof(uint)+sizeof(int)*DataPktSet.size());

	unsigned char* walk = (unsigned char*)pkt->accessdata();
	*((uint*)walk) = DataPktSet.size();
	walk += sizeof(uint);

	for( set<Packet*>::iterator pos = DataPktSet.begin();
		pos != DataPktSet.end(); pos++) {

		*((int*)walk) = HDR_CMN(*pos)->uid();
		walk += sizeof(int);
	}

	return pkt;
}


//---------------------------------------------------------------------
void GOAL::processReqPkt(Packet *ReqPkt)
{

	hdr_GOAL_req* vmq = hdr_GOAL_req::access(ReqPkt);
	int		PktID;
	bool	IsLoop = false;
	GOAL_AckTimeoutTimer* AckTimeoutTimer = NULL;
	set<GOAL_AckTimeoutTimer*>::iterator pos;
	Packet* pkt = NULL;
	unsigned char* walk = NULL;
	set<int> DuplicatedPktSet;
	set<int> AvailablePktSet;

	//check duplicated packet in the request.
	walk = (unsigned char*)ReqPkt->accessdata();
	uint PktNum = *((uint*)walk);
	walk += sizeof(uint);
	for( uint i=0; i<PktNum; i++) {
		PktID = *((int*)walk);
		if( RecvedList.count(PktID) != 0 ) {
			if( RecvedList[PktID].Sender == vmq->SA_ )
				DuplicatedPktSet.insert(PktID);
		}

		AvailablePktSet.insert(PktID);

		walk += sizeof(int);
	}

	if( !DuplicatedPktSet.empty() ) {
		//make Ack packet with PUSH flag and send immediately
		Packet* AckPkt = makeAckPkt(DuplicatedPktSet, true, vmq->ReqID_);
		Time Txtime = getTxtimebyPktSize(HDR_CMN(AckPkt)->size());
		Time SendTime = TSQ_.getAvailableTime(NOW, Txtime);
		TSQ_.insert(SendTime, SendTime+Txtime);
		PreSendPkt(AckPkt, SendTime-NOW);
	}

	if( DuplicatedPktSet.size()==PktNum ) {
		//all packets are duplicated, don't need to do further process
		return;  //need to reserve slot for receiving???
	}


	//check if it is implicit ack
	//check the DataPktID carried by the request packet
	set<int>::iterator pointer = AvailablePktSet.begin();
	while( pointer != AvailablePktSet.end() ) {
		PktID = *pointer;

		pos = AckTimeoutTimerSet_.begin();
		while( pos!=AckTimeoutTimerSet_.end() ) {
			AckTimeoutTimer = *pos;
			if( AckTimeoutTimer->PktSet().count(PktID) != 0 ) {
				pkt = AckTimeoutTimer->PktSet().operator[](PktID);
				Packet::free(pkt);
				AckTimeoutTimer->PktSet().erase(PktID);

				IsLoop = true;
			}
			pos++;
		}

		//check if the packets tranmitted later is originated from this node
		//In other words, is there a loop?
		if( OriginPktSet.count(PktID) != 0 ) {
			IsLoop = true;
		}

		pointer++;
	}


	//clear the empty entries
	pos = AckTimeoutTimerSet_.begin();
	while( pos!=AckTimeoutTimerSet_.end() ) {
		AckTimeoutTimer = *pos;
		if( AckTimeoutTimer->PktSet().empty() ) {
			if( AckTimeoutTimer->status() == TIMER_PENDING ) {
				AckTimeoutTimer->cancel();
			}

			AckTimeoutTimerSet_.erase(pos);
			pos = AckTimeoutTimerSet_.begin();

			delete AckTimeoutTimer;
			IsForwarding = false;
		}
		else {
			pos++;
		}
	}


	gotoNxtRound();  //try to send data. prepareDataPkts can check if it should send data pkts

	//start to backoff
	if( !IsLoop && TSQ_.checkCollision(NOW+vmq->SendTime_, NOW+vmq->SendTime_+vmq->TxTime_) ) {

		Time BackoffTimeLen = getBackoffTime(ReqPkt);
		//this node is in the forwarding area.
		if( BackoffTimeLen > 0.0 ) {
			GOAL_BackoffTimer* backofftimer = new GOAL_BackoffTimer(this);
			Time RepPktTxtime = getTxtimebyPktSize(hdr_GOAL_rep::size(backoff_type));
			Time RepSendTime = TSQ_.getAvailableTime(BackoffTimeLen+NOW+JitterStartTime(RepPktTxtime),
				RepPktTxtime );
			SchedElem* SE = new SchedElem(RepSendTime, RepSendTime+RepPktTxtime);
			//reserve the sending interval for Rep packet
			TSQ_.insert(SE);
			backofftimer->ReqPkt() = ReqPkt->copy();
			backofftimer->setSE(SE);
			backofftimer->BackoffTime() = BackoffTimeLen;
			backofftimer->resched(RepSendTime-NOW);
			BackoffTimerSet_.insert(backofftimer);
			//avoid send-recv collision or recv-recv collision at this node
			TSQ_.insert(NOW+vmq->SendTime_, NOW+vmq->SendTime_+vmq->TxTime_);
			return;
		}
	}

	//reserve time slot for receiving data packet
	//avoid recv-recv collision at this node
	TSQ_.insert(NOW+vmq->SendTime_, NOW+vmq->SendTime_+vmq->TxTime_, true);
}


//---------------------------------------------------------------------
Packet* GOAL::makeRepPkt(Packet *ReqPkt, Time BackoffTime)
{
	UnderwaterSensorNode* n = (UnderwaterSensorNode*)node_;
	n->update_position();
	hdr_GOAL_req* vmq = hdr_GOAL_req::access(ReqPkt);
	Packet* pkt = Packet::alloc();
	hdr_mac* mach = hdr_mac::access(pkt);
	hdr_GOAL_rep* vmp = hdr_GOAL_rep::access(pkt);
	hdr_cmn* cmh = hdr_cmn::access(pkt);

	vmp->SA_ = index_;
	vmp->RA_ = vmq->SA_;
	vmp->ReqID_ = vmq->ReqID_;
	vmp->ReplyerPos.setValue(n->X(), n->Y(), n->Z());
	vmp->SendTime_ = vmq->SendTime_;	//update this item when being sent out
	vmp->BackoffTime = BackoffTime;

	cmh->direction() = hdr_cmn::DOWN;
	cmh->ptype() = PT_GOAL_REP;
	cmh->error() = 0;
	cmh->next_hop() = vmp->RA_;			//reply the sender of request pkt
	cmh->size() = hdr_GOAL_rep::size(backoff_type);
	cmh->addr_type() = NS_AF_ILINK;
	cmh->timestamp() = NOW;

	mach->macDA() = vmp->RA_;
	mach->macSA() = vmp->SA_;

	return pkt;
}


//---------------------------------------------------------------------
void GOAL::processRepPkt(Packet *RepPkt)
{
	//here only process the RepPkt for this node( the request sender)
	hdr_GOAL_rep* vmp = hdr_GOAL_rep::access(RepPkt);
	set<GOAL_DataSendTimer*>::iterator pos = DataSendTimerSet_.begin();

	while( pos != DataSendTimerSet_.end() ) {

		if( (*pos)->ReqID() == vmp->ReqID_ ) {

			if( vmp->BackoffTime < (*pos)->MinBackoffTime() ) {
				(*pos)->NxtHop() = vmp->SA_;
				(*pos)->MinBackoffTime() = vmp->BackoffTime;
				(*pos)->GotRep() = true;
			}
			break;
		}

		pos++;
	}
}


//---------------------------------------------------------------------
void GOAL::processOverhearedRepPkt(Packet* RepPkt)
{
	//process the overheared reply packet
	//if this node received the corresponding req before,
	//cancel the timer
	hdr_GOAL_rep* vmp = hdr_GOAL_rep::access(RepPkt);
	set<GOAL_BackoffTimer*>::iterator pos = BackoffTimerSet_.begin();
	while( pos != BackoffTimerSet_.end() ) {
		if( hdr_GOAL_rep::access((*pos)->ReqPkt())->ReqID_ == vmp->ReqID_ ) {

			/*if( vmp->BackoffTime < (*pos)->BackoffTime() ) {
				if( (*pos)->status() == TIMER_PENDING ) {
					(*pos)->cancel();
				}
				TSQ_.remove((*pos)->SE());
				delete (*pos)->SE();
				Packet::free( (*pos)->ReqPkt() );
				}*/
			if( (*pos)->status() == TIMER_PENDING ) {
				(*pos)->cancel();
			}
			//change the type of reserved slot to avoid recv-recv collision at this node
			(*pos)->SE()->IsRecvSlot = true;
			/*TSQ_.remove((*pos)->SE());
			delete (*pos)->SE();*/
			Packet::free( (*pos)->ReqPkt() );
		}
		pos++;
	}

	//reserve time slot for the corresponding Data Sending event.
	//avoid recv-recv collision at neighbors
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	NodePos ThisNode;
	n->update_position();
	ThisNode.setValue(n->X(), n->Y(), n->Z());
	Time PropDelay = dist(ThisNode, vmp->ReplyerPos)/PropSpeed;
	Time BeginTime = NOW + (vmp->SendTime_ - 2*PropDelay-HDR_CMN(RepPkt)->txtime() );
	TSQ_.insert(BeginTime-GuardTime, BeginTime+vmp->TxTime_+GuardTime);

}


//---------------------------------------------------------------------
void GOAL::prepareDataPkts()
{
	/*if( IsForwarding || (QsPktNum == 0) )
		return;*/

	int		SinkAddr = -1;
	Time DataTxTime = 0.0;
	Time DataSendTime = 0.0;
	Time ReqSendTime = 0.0;
	Time ReqPktTxTime = getTxtimebyPktSize(hdr_GOAL_req::size(backoff_type));
	Packet* pkt = NULL;
	Packet* ReqPkt = NULL;
	GOAL_DataSendTimer* DataSendTimer = new GOAL_DataSendTimer(this);
	//GOAL_RepTimeoutTimer* RepTimeoutTimer = new GOAL_RepTimeoutTimer(this);

	if( PktQs.size() == 0 ) {
		printf("size of PktQs should not be 0, something must be wrong\n");
	}

	map<nsaddr_t, GOAL_PktQ>::iterator pos = PktQs.begin();
	for(int i=0; i<SinkSeq; i++) {
		pos++;
	}
	SinkSeq = (SinkSeq+1)%PktQs.size();
	SinkAddr = pos->first;

	for( int i=0; i<MaxBurst && (!PktQs[SinkAddr].Q_.empty()); i++) {
		pkt = PktQs[SinkAddr].Q_.front();
		DataSendTimer->DataPktSet().insert(pkt);
		PktQs[SinkAddr].Q_.pop_front();
		--QsPktNum;
		DataTxTime += HDR_CMN(pkt)->txtime() + DataPktInterval;
	}
	//additional DataPktInterval is considered, subtract it.
	DataTxTime -= DataPktInterval;

	//erase empty entry
	if( PktQs[SinkAddr].Q_.empty() )
		PktQs.erase(SinkAddr);



	ReqPkt = makeReqPkt(DataSendTimer->DataPktSet(), DataSendTime, DataTxTime);
	ReqPktTxTime = getTxtimebyPktSize(HDR_CMN(ReqPkt)->size());

	ReqSendTime = TSQ_.getAvailableTime(NOW+JitterStartTime(ReqPktTxTime), ReqPktTxTime);
	TSQ_.insert(ReqSendTime, ReqSendTime+ReqPktTxTime); //reserve time slot for sending REQ

	//reserve time slot for sending DATA
	DataSendTime = TSQ_.getAvailableTime(NOW+MaxBackoffTime+2*MaxDelay+EstimateError, DataTxTime, true);
	DataSendTimer->SetSE( TSQ_.insert(DataSendTime, DataSendTime+DataTxTime) );

	DataSendTimer->ReqID() = hdr_GOAL_req::access(ReqPkt)->ReqID_;
	DataSendTimer->TxTime() = DataTxTime;
	DataSendTimer->MinBackoffTime() = 100000.0;
	DataSendTimer->GotRep() = false;

	//send REQ
	PreSendPkt(ReqPkt, ReqSendTime-NOW);


	DataSendTimer->resched(DataSendTime - NOW);
	DataSendTimerSet_.insert(DataSendTimer);
}

//---------------------------------------------------------------------
void GOAL::sendDataPkts(set<Packet*> &DataPktSet, nsaddr_t NxtHop, Time TxTime)
{
	set<Packet*>::iterator pos = DataPktSet.begin();
	Time DelayTime = 0.00001;  //the delay of sending data packet
	hdr_mac* mach = NULL;
	hdr_cmn* cmh = NULL;
	GOAL_AckTimeoutTimer* AckTimeoutTimer = new GOAL_AckTimeoutTimer(this);

	while( pos != DataPktSet.end() ) {

		cmh = hdr_cmn::access(*pos);
		cmh->next_hop() = NxtHop;

		mach = hdr_mac::access(*pos);
		mach->macDA() = NxtHop;
		mach->macSA() = index_;

		PreSendPkt((*pos)->copy(), DelayTime);

		AckTimeoutTimer->PktSet().operator[](cmh->uid()) = (*pos);

		DelayTime += DataPktInterval + cmh->txtime();
		pos++;
	}

	AckTimeoutTimer->resched(2*MaxDelay+TxTime+this->NxtRoundMaxWaitTime+EstimateError+0.5);
	AckTimeoutTimerSet_.insert(AckTimeoutTimer);
}


//---------------------------------------------------------------------
void GOAL::processDataPkt(Packet *DataPkt)
{
	hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);
	hdr_cmn* cmh = hdr_cmn::access(DataPkt);

	if( RecvedList.count(cmh->uid()) != 0 ) {
		//duplicated packet, free it.
		Packet::free(DataPkt);
		return;
	}

	purifyRecvedList();
	RecvedList[cmh->uid()].RecvTime = NOW;
	RecvedList[cmh->uid()].Sender = hdr_mac::access(DataPkt)->macSA();

	if( vbh->target_id.addr_ == index_ ) {
		//ack this data pkt if this node is the destination
		//ack!!! insert the uid into AckSet.
		if( SinkAccumAckTimer.status() == TIMER_PENDING ) {
			SinkAccumAckTimer.cancel();
		}

		SinkAccumAckTimer.resched( HDR_CMN(DataPkt)->txtime()+ DataPktInterval*2);
		SinkAccumAckTimer.AckSet().insert( HDR_CMN(DataPkt)->uid() );

		if( SinkRecvedList.count(cmh->uid()) != 0 )
			Packet::free(DataPkt);
		else {
			SinkRecvedList.insert(cmh->uid());
			cmh->size() -= sizeof(nsaddr_t)*2;
			sendUp(DataPkt);
		}

	}
	else {
		//forward this packet later
		cmh->num_forwards() = 0;
		insert2PktQs(DataPkt);
	}
}

//---------------------------------------------------------------------
Packet* GOAL::makeAckPkt(set<int>& AckSet, bool PSH,  int ReqID)
{
	Packet* pkt = Packet::alloc();
	hdr_cmn* cmh = hdr_cmn::access(pkt);
	hdr_mac* mach = hdr_mac::access(pkt);
	hdr_GOAL_ack* ackh = hdr_GOAL_ack::access(pkt);
	pkt->allocdata(sizeof(int)*AckSet.size()+sizeof(uint));
	unsigned char* walk = (unsigned char*)pkt->accessdata();

	ackh->SA_ = index_;
	ackh->RA_ = MAC_BROADCAST;
	ackh->PUSH = PSH;
	if( PSH ) {
		ackh->ReqID_ = ReqID;
	}

	cmh->ptype() = PT_GOAL_ACK;
	cmh->direction() = hdr_cmn::DOWN;
	cmh->error() = 0;
	cmh->next_hop() = ackh->RA_;   //reply the sender of request pkt
	cmh->size() = hdr_GOAL_ack::size() + (NSADDR_T_SIZE*AckSet.size())/8+1;
	cmh->addr_type() = NS_AF_ILINK;

	mach->macDA() = ackh->RA_;
	mach->macSA() = ackh->SA_;

	*((uint*)walk) = AckSet.size();
	walk += sizeof(uint);
	for( set<int>::iterator pos=AckSet.begin();
		pos != AckSet.end(); pos++)   {
		*((int*)walk) = *pos;
		walk += sizeof(int);
	}

	return pkt;
}

//---------------------------------------------------------------------
void GOAL::processAckPkt(Packet* AckPkt)
{
	//cancel the Ack timeout timer and release data packet.
	int PktID;
	GOAL_AckTimeoutTimer* AckTimeoutTimer;
	Packet* pkt = NULL;
	set<GOAL_AckTimeoutTimer*>::iterator pos;

	//check the DataPktID carried by the ack packet
	unsigned char* walk = (unsigned char*)AckPkt->accessdata();
	uint PktNum = *((uint*)walk);
	walk += sizeof(uint);
	for( uint i=0; i < PktNum; i++) {
		PktID = *((int*)walk);

		pos = AckTimeoutTimerSet_.begin();

		while( pos != AckTimeoutTimerSet_.end() ) {
			AckTimeoutTimer = *pos;
			if( AckTimeoutTimer->PktSet().count(PktID) != 0 ) {
				pkt = AckTimeoutTimer->PktSet().operator[](PktID);
				Packet::free(pkt);
				AckTimeoutTimer->PktSet().erase(PktID);
			}
			pos++;
		}

		walk += sizeof(int);
	}

	//clear the empty entries
	pos = AckTimeoutTimerSet_.begin();
	while( pos != AckTimeoutTimerSet_.end() ) {
		AckTimeoutTimer = *pos;

		if( AckTimeoutTimer->PktSet().empty() ) {
			//all packet are acked
			if( AckTimeoutTimer->status() == TIMER_PENDING ) {
				AckTimeoutTimer->cancel();
			}

			AckTimeoutTimerSet_.erase(pos);
			pos = AckTimeoutTimerSet_.begin();
			delete AckTimeoutTimer;

			IsForwarding = false;
		}
		else {
			pos++;
		}
	}

	gotoNxtRound();

}


//---------------------------------------------------------------------
void GOAL::processPSHAckPkt(Packet* AckPkt)
{
	int ReqID = hdr_GOAL_ack::access(AckPkt)->ReqID_;
	GOAL_DataSendTimer* DataSendTimer=NULL;
	set<GOAL_DataSendTimer*>::iterator pos = DataSendTimerSet_.begin();

	while( pos != DataSendTimerSet_.end() ) {
		if( (*pos)->ReqID() == ReqID ) {
			DataSendTimer = *pos;
			break;
		}
		pos++;
	}

	if( DataSendTimer != NULL ) {
		//the reply is from this node??
		//check the duplicated DataPktID carried by the ack packet
		unsigned char* walk = (unsigned char*)AckPkt->accessdata();
		uint PktNum = *((uint*)walk);
		walk += sizeof(uint);

		for(uint i=0; i<PktNum; i++) {
			set<Packet*>::iterator pointer = DataSendTimer->DataPktSet().begin();
			while( pointer != DataSendTimer->DataPktSet().end() ) {
				if( HDR_CMN(*pointer)->uid() == *((int*)walk) ) {
					DataSendTimer->DataPktSet().erase(*pointer);
					break;
				}
				pointer++;
			}

			walk+= sizeof(int);
		}

		if( DataSendTimer->DataPktSet().empty() ) {
			if( DataSendTimer->status() == TIMER_PENDING ) {
				DataSendTimer->cancel();
			}
			TSQ_.remove(DataSendTimer->SE());

			DataSendTimerSet_.erase(DataSendTimer);
			delete DataSendTimer;

			IsForwarding = false;
			gotoNxtRound();
		}

		return;
	}

}



//---------------------------------------------------------------------
void GOAL::insert2PktQs(Packet* DataPkt, bool FrontPush)
{
	hdr_uwvb* vbh = hdr_uwvb::access(DataPkt);
	hdr_cmn* cmh = hdr_cmn::access(DataPkt);


	if( cmh->num_forwards() > MaxRetransTimes ) {
		Packet::free(DataPkt);
		return;
	}
	else {
		cmh->num_forwards_++;
	}

	if( FrontPush )
		PktQs[vbh->target_id.addr_].Q_.push_front(DataPkt);
	else
		PktQs[vbh->target_id.addr_].Q_.push_back(DataPkt);

	QsPktNum++;

	gotoNxtRound();
	////how to control??????
	//if( QsPktNum==1 ) {  /*trigger forwarding if Queue is empty*/
	//	prepareDataPkts(vbh->target_id.addr_);
	//}
	//else if( SendOnebyOne  ) {
	//	if( !IsForwarding )
	//		prepareDataPkts(vbh->target_id.addr_);
	//}
	//else{
	//	prepareDataPkts(vbh->target_id.addr_);
	//}
}

//---------------------------------------------------------------------
void GOAL::processBackoffTimeOut(GOAL_BackoffTimer *backoff_timer)
{
	Packet* RepPkt = makeRepPkt(backoff_timer->ReqPkt(),
								backoff_timer->BackoffTime());

	/* The time slot for sending reply packet is
	 * already reserved when processing request packet,
	 * so RepPkt is directly sent out.
	 */
	sendoutPkt(RepPkt);
	Packet::free(backoff_timer->ReqPkt());
	BackoffTimerSet_.erase(backoff_timer);
	delete backoff_timer;
}

//---------------------------------------------------------------------
void GOAL::processDataSendTimer(GOAL_DataSendTimer *DataSendTimer)
{
	if( !DataSendTimer->GotRep() ) {
		//if havenot gotten reply, resend the packet
		set<Packet*>::iterator pos = DataSendTimer->DataPktSet().begin();
		while( pos != DataSendTimer->DataPktSet().end() ) {
			insert2PktQs(*pos, true); //although gotoNxtRound() is called, it does not send ReqPkt
			pos++;
		}

		DataSendTimer->DataPktSet().clear();

		IsForwarding = false;
		gotoNxtRound();   //This call is successful, so all packets will be sent together.
	}
	else {
		sendDataPkts(DataSendTimer->DataPktSet(),
				DataSendTimer->NxtHop(), DataSendTimer->TxTime());
		DataSendTimer->DataPktSet().clear();
	}

	DataSendTimerSet_.erase(DataSendTimer);
	delete DataSendTimer;
}

//---------------------------------------------------------------------
void GOAL::processSinkAccumAckTimeout()
{
	Packet* AckPkt = makeAckPkt( SinkAccumAckTimer.AckSet() );
	SinkAccumAckTimer.AckSet().clear();
	Time AckTxtime = getTxtimebyPktSize(HDR_CMN(AckPkt)->size());
	Time AckSendTime = TSQ_.getAvailableTime(NOW+JitterStartTime(AckTxtime), AckTxtime);
	TSQ_.insert(AckSendTime, AckSendTime+AckTxtime);
	PreSendPkt(AckPkt, AckSendTime-NOW);
}

//---------------------------------------------------------------------
void GOAL::processPreSendTimeout(GOAL_PreSendTimer* PreSendTimer)
{
	sendoutPkt(PreSendTimer->pkt());
	PreSendTimer->pkt() = NULL;
	PreSendTimerSet_.erase(PreSendTimer);
	delete PreSendTimer;
}

//---------------------------------------------------------------------
void GOAL::processAckTimeout(GOAL_AckTimeoutTimer *AckTimeoutTimer)
{
	//SentPktSet.erase( HDR_CMN(AckTimeoutTimer->pkt())->uid() );
	//insert2PktQs(AckTimeoutTimer->pkt(), true);
	//AckTimeoutTimer->pkt() = NULL;
	//delete AckTimeoutTimer;
	//IsForwarding = false;
	//prepareDataPkts();  //???? how to do togther???
	//set<Packet*>::iterator pos = AckTimeoutTimer->DataPktSet().begin();
	//while( pos != AckTimeoutTimer->DataPktSet().end() ) {
	//	insert2PktQs(*pos, true);  //right??
	//	pos++;
	//}
	//AckTimeoutTimerSet_.erase(AckTimeoutTimer);
	//delete AckTimeoutTimer;

	map<int, Packet*>::iterator pos = AckTimeoutTimer->PktSet().begin();
	while( pos != AckTimeoutTimer->PktSet().end() ) {
		insert2PktQs(pos->second, true);
		pos++;
	}

	AckTimeoutTimer->PktSet().clear();
	AckTimeoutTimerSet_.erase(AckTimeoutTimer);
	delete AckTimeoutTimer;

	IsForwarding = false;
	gotoNxtRound();
}


////---------------------------------------------------------------------
//void GOAL::processRepTimeout(GOAL_RepTimeoutTimer *RepTimeoutTimer)
//{
//	GOAL_DataSendTimer* DataSendTimer = RepTimeoutTimer->DataSendTimer();
//
//
//	if( !RepTimeoutTimer->GotRep() ) {
//		//if havenot gotten reply, stop datasend timer
//		if( DataSendTimer->status() == TIMER_PENDING ) {
//			DataSendTimer->cancel();
//		}
//
//		set<Packet*>::iterator pos = DataSendTimer->DataPktSet().begin();
//		while( pos != DataSendTimer->DataPktSet().end() ) {
//			insert2PktQs(*pos, true); //right??
//			pos++;
//		}
//		DataSendTimerSet_.erase(DataSendTimer);
//	}
//	RepTimeoutTimerSet_.erase(RepTimeoutTimer);
//	delete RepTimeoutTimer;
//	IsForwarding = false;
//}


//---------------------------------------------------------------------
void GOAL::PreSendPkt(Packet *pkt, Time delay)
{
	GOAL_PreSendTimer* PreSendTimer = new GOAL_PreSendTimer(this);
	PreSendTimer->pkt() = pkt;
	PreSendTimer->resched(delay);
	PreSendTimerSet_.insert(PreSendTimer);
}


//---------------------------------------------------------------------
void GOAL::sendoutPkt(Packet *pkt)
{
	hdr_cmn* cmh=HDR_CMN(pkt);
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;

	cmh->txtime() = getTxtimebyPktSize(cmh->size());

	//update the the Time for sending out the data packets.

	double txtime=cmh->txtime();
	Scheduler& s=Scheduler::instance();

	switch( n->TransmissionStatus() ) {
		case SLEEP:
			Poweron();

        case RECVE:
			//interrupt reception
			InterruptRecv(txtime);
        case IDLEE:
			n->SetTransmissionStatus(SEND);
			switch( cmh->ptype() ) {
				case PT_GOAL_REQ:
					{
						hdr_GOAL_req* vmq = hdr_GOAL_req::access(pkt);
						vmq->SendTime_ -= NOW - cmh->timestamp();
					}
					break;
				case PT_GOAL_REP:
					{
						hdr_GOAL_rep* vmp = hdr_GOAL_rep::access(pkt);
						vmp->SendTime_ -= NOW - cmh->timestamp();
					}
					break;
				default:
					;
			}

			cmh->timestamp() = NOW;
			cmh->direction() = hdr_cmn::DOWN;
			sendDown(pkt);
			s.schedule(&StatusHandler,&StatusEvent,txtime);
			break;

		//case RECV:
		//	//backoff???
		//	Packet::free(pkt);
		//	break;
		//
		default:
			//status is SEND
			printf("node%d send data too fast\n",index_);
			Packet::free(pkt);

	}

	return;
}


//---------------------------------------------------------------------
Time GOAL::getTxtimebyPktSize(int PktSize)
{
	return (PktSize*8)*encoding_efficiency_/bit_rate_;
}

//---------------------------------------------------------------------
double GOAL::dist(NodePos& Pos1, NodePos& Pos2)
{
	double delta_x = Pos1.X_ - Pos2.X_;
	double delta_y = Pos1.Y_ - Pos2.Y_;
	double delta_z = Pos1.Z_ - Pos2.Z_;

	return sqrt(delta_x*delta_x + delta_y*delta_y + delta_z*delta_z);
}


//---------------------------------------------------------------------
Time GOAL::getBackoffTime(Packet *ReqPkt)
{
	hdr_GOAL_req* vmq = hdr_GOAL_req::access(ReqPkt);
	Time BackoffTime;

	switch( backoff_type ) {
		case VBF:
			BackoffTime = getVBFbackoffTime(vmq->SourcePos, vmq->SenderPos, vmq->SinkPos);
			break;
		case HH_VBF:
			BackoffTime = getHH_VBFbackoffTime(vmq->SenderPos, vmq->SinkPos);
			break;
		default:
			printf("No such backoff type\n");
			exit(0);
	}

	return BackoffTime;
}

//---------------------------------------------------------------------
//Backoff Functions
Time GOAL::getVBFbackoffTime(NodePos& Source, NodePos& Sender, NodePos& Sink)
{
	double DTimesCosTheta =0.0;
	double p = 0.0;;
	double alpha = 0.0;
	NodePos ThisNode;
	UnderwaterSensorNode* n = (UnderwaterSensorNode*)node_;
	n->update_position();
	ThisNode.setValue(n->X(), n->Y(), n->Z());


	if( dist(Sender, Sink) < dist(ThisNode, Sink) )
		return -1.0;

	DTimesCosTheta = distToLine(Sender, Sink);
	p = distToLine(Source, Sink);

	if( p > PipeWidth )
		return -1.0;

	alpha = p/PipeWidth + (TxRadius-DTimesCosTheta)/TxRadius;

	return VBF_MaxDelay*sqrt(alpha)+2*(TxRadius-dist(Sender, ThisNode))/PropSpeed;
}

//---------------------------------------------------------------------
double GOAL::getHH_VBFbackoffTime(NodePos &Sender, NodePos &Sink)
{
	return getVBFbackoffTime(Sender, Sender, Sink);
}


//---------------------------------------------------------------------
//Line Point1 and Line Point2 is two points on the line
double GOAL::distToLine(NodePos& LinePoint1, NodePos& LinePoint2)
{
	UnderwaterSensorNode* n=(UnderwaterSensorNode*) node_;
	NodePos ThisNode;
	n->update_position();
	ThisNode.setValue(n->X(), n->Y(), n->Z());
	double P1ThisNodeDist = dist(LinePoint1, ThisNode);
	double P1P2Dist = dist(LinePoint1, LinePoint2);
	double ThisNodeP2Dist = dist(ThisNode, LinePoint2);
	double CosTheta = (P1ThisNodeDist*P1ThisNodeDist +
				P1P2Dist*P1P2Dist -
				ThisNodeP2Dist*ThisNodeP2Dist)/(2*P1ThisNodeDist*P1P2Dist);
	//the distance from this node to the pipe central axis
	return P1ThisNodeDist*sqrt(1-CosTheta*CosTheta);
}


//---------------------------------------------------------------------
void GOAL::purifyRecvedList()
{
	map<int, RecvedInfo> tmp(RecvedList.begin(), RecvedList.end());
	map<int, RecvedInfo>::iterator pos;
	RecvedList.clear();

	for( pos=tmp.begin(); pos!=tmp.end(); pos++) {
		if( pos->second.RecvTime > NOW-RecvedListAliveTime )
			RecvedList.insert(*pos);
	}
}


//---------------------------------------------------------------------
void GOAL::processNxtRoundTimeout()
{
	prepareDataPkts();
}

//---------------------------------------------------------------------
void GOAL::gotoNxtRound()
{
	if( IsForwarding || (NxtRoundTimer.status()==TIMER_PENDING) || (QsPktNum==0) )
		return;

	IsForwarding = true;

	NxtRoundTimer.resched( Random::uniform(NxtRoundMaxWaitTime) );
}

//---------------------------------------------------------------------


Time GOAL::JitterStartTime(Time Txtime)
{
	Time BeginTime = 5*Txtime*Random::uniform();
	return BeginTime;
}








