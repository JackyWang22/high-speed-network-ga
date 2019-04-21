#include <fstream>
#include "llshaper.h"
#include "scheduler.h"
#include "packet.h"
#include<iostream>

void LLShaperHandler::handle(Event *e)
{
	shaper_->resume(flow_id);
}

static class LLShaperTclClass : public TclClass {
public:
	LLShaperTclClass() : TclClass("LLShaper") {}
	TclObject* create(int, const char*const*) {
                return (new LLShaper);
        }
} class_llshaper ;

LLShaper::LLShaper() : Connector(),
			   received_packets(0),
			   sent_packets(0),
			   shaped_packets(0),
			   dropped_packets(0),
			   max_queue_length(10000),
			   logging(0)
{

}

void LLShaper::recv(Packet *p, Handler *h)
{
	struct hdr_ip *iph = HDR_IP(p) ;
	//Does the fid have a RL associated with it?  If not, just pass the packet on to the target without shaping.  If it does exist, proceed with shaping the packet.
	RL_itr RL_entry;
	int fid=iph->fid_;
	RL_entry=rate_limiters.find(fid);
	//No rate limiting if the rate limiter is inactive, or if there is no entry for the given FID
	if ((RL_entry==rate_limiters.end()) || (RL_entry->second.RL_ACTIVE==false))
	{
		//Just send packets which fid_ do not match
	        target_->recv(p,h);
	        return;
	}
	else
	{
		//The structure RL_entry->second contains the fid associated rate limiter specific data
		TB_flowctrl *RL;
		RL=&(RL_entry->second);
		//update global stats
		received_packets++;
		if (RL->shape_queue.length() == 0)
		{
			//There are no packets being shapped. Tests profile.
	    		if (in_profile(RL,p))
			{
			//no packets in queue, and the packetsize is less than the burst size... thats good :)
 	        		sent_packets++;
	        		target_->recv(p,h);
	    		}
			else
			{
	    			//else, we'll just have to shape the packet
	        		if (shape_packet(RL,p))
           	    			schedule_packet(RL,p);
	           		//else
	                  	//Shaper is being used as a dropper
            		}
  		}
		else
		{
	         	//There are packets being shaped. Shape this packet too.
            		shape_packet(RL,p);
		}
	}
}

bool LLShaper::shape_packet(TB_flowctrl *RL,Packet *p)
{
        if (RL->shape_queue.length() >= max_queue_length) {
	    drop (p);
	    dropped_packets++;
	    return (false);
        }
        RL->shape_queue.enque(p);
        shaped_packets++;
	return (true);
}

void LLShaper::schedule_packet(TB_flowctrl *RL,Packet *p)
{
	Scheduler& s = Scheduler::instance();
	//Was schedule_packet called by resume, when the Rate limiter was already de-activated?
	//If so, directly send the packet.  However, let RL->sh_ do this, since it has the code to pick the next packet and call shape_packet again.. bit roundabout, but simple, and consistent with existing code from original DSShaper implementation
	if (RL->RL_ACTIVE==false)
	{
		//s.schedule(RL->sh_, p, 0);
            	target_->recv(p,(Handler*) NULL);
	}
	else
	{
		//calculate time to wake up
		int packetsize = hdr_cmn::access(p)->size_ * 8 ;
		double delay = (packetsize - RL->curr_bucket_contents)/(double)RL->peak_;
		//Should not happen, but seems to occur when dynamically changing rate.. might look into why in the future (or not)!
		if (delay <= 0)
		//	delay=0;
            		target_->recv(p,(Handler*) NULL);
		else
			s.schedule(RL->sh_, p, delay);
	}
}


void LLShaper::resume(int flow_id)
{
	RL_itr RL_entry;
	RL_entry=rate_limiters.find(flow_id);
	if (RL_entry==rate_limiters.end())
	{
		cout << "Resume called for flow_id "<<flow_id<<", which does not have a rate limiter associated with it\n";
	        exit(-1);
	}
	else
	{
		//The structure RL_entry->second contains the fid associated rate limiter specific data
		TB_flowctrl *RL;
		RL=&(RL_entry->second);
		Packet *p = RL->shape_queue.deque();
		if (in_profile(RL,p))
		{
            		sent_packets++;
            		target_->recv(p,(Handler*) NULL);
		}
		else
		{
			//This is not an expected error for fixed rate settings, because the packet was scheduled to wake up
			//exactly when it could be sent
			//however, with variable rate control, maybe the rate was decreased even further after the packet was scheduled.  Just send the packet but make a remark if necessary
	    		//puts ("shaper/resume - Target rate reduced after packet scheduling!\n");
			//Just try rescheduling the packet for now!
			//cout <<"Rescheduling packet\n";
	   		//schedule_packet(RL,p);
			//cout << "done\n";
            		sent_packets++;
            		target_->recv(p,(Handler*) NULL);
			/*
	    		drop (p);
	    		dropped_packets++;
			*/
        	}

		if (RL->shape_queue.length() > 0)
		{
			//There are packets in the queue. Schedule the first one.
	           	Packet *first_p = RL->shape_queue.lookup(0);
			//Got the first packet in the queue.
	   		schedule_packet(RL,first_p);
        	}
	}
}

bool LLShaper::in_profile(TB_flowctrl *RL,Packet *p)
{
	update_bucket_contents(RL) ;
	int packetsize = hdr_cmn::access(p)->size_ * 8 ; // in bits
	if (packetsize > RL->curr_bucket_contents)
	{
		return false;
	}
	else {
		RL->curr_bucket_contents -= packetsize ;
		return true ;
	}
}

void LLShaper::update_bucket_contents(TB_flowctrl *RL)
{
//      I'm using the token bucket implemented by Sean Murphy

	double current_time = Scheduler::instance().clock() ;

	double added_bits = (current_time - RL->last_time) * RL->peak_ ;
	//if (added_bits > 3* RL->burst_size_)
		//cout << "Added bits: " << added_bits << "\ncurrent_time: " << current_time << "\tlast_time " << RL->last_time << "\nPeak: " << RL->peak_ <<"\tBurst: " << RL->burst_size_ << endl;
	(RL->curr_bucket_contents) = (long int) min((RL->curr_bucket_contents+added_bits + 0.5),RL->burst_size_);
	//if (RL->curr_bucket_contents > RL->burst_size_)
	//	RL->curr_bucket_contents=RL->burst_size_ ;
	RL->last_time = current_time ;


}

int LLShaper::command(int argc, const char* const*argv)
{
	if (argc==2) {
		if (strcmp(argv[1],"reset-counters")==0) {
			reset_counters() ;
			return TCL_OK ;
		}
		if (strcmp(argv[1],"get-received-packets")==0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d",received_packets);
			return TCL_OK ;
		}
		if (strcmp(argv[1],"get-sent-packets")==0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d",sent_packets);
			return TCL_OK ;
 		}
		if (strcmp(argv[1],"get-shaped-packets")==0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d",shaped_packets);
			return TCL_OK ;
		}
		if (strcmp(argv[1],"get-dropped-packets")==0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d",dropped_packets);
			return TCL_OK ;
		}
	}
	if (argc==3) {
		if (strcmp("deactivate-fid", argv[1])==0) {
		    RL_deactivate(atoi(argv[2]));
			return TCL_OK ;
		}
		if (strcmp("set-queue-length", argv[1])==0) {
		    max_queue_length = atoi(argv[2]);
			return TCL_OK ;
		}
		return Connector::command(argc,argv) ;
	}
	if (argc==5) {
		if (strcmp("activate-fid",argv[1])==0)
		{
			double fid=atoi(argv[2]);
			double peak_=atoi(argv[3]);
			int burst_size_=atoi(argv[4]);
			RL_activate(fid,peak_,burst_size_);
			return TCL_OK;
		}
		if (strcmp("modify-fid",argv[1])==0)
		{
			int fid=atoi(argv[2]);
			double peak_=atoi(argv[3]);
			double burst_size_=atoi(argv[4]);
			RL_modify(fid,peak_,burst_size_);
			return TCL_OK;
		}
	}
	return Connector::command(argc, argv) ;
}

void LLShaper::reset_counters()
{
	received_packets = sent_packets = shaped_packets = dropped_packets = 0 ;
}

void LLShaper::RL_activate(int fid,double peak_,double burst_size_)
{
	RL_itr RL_entry;
	TB_flowctrl *RL;
	RL_entry=rate_limiters.find(fid);
	//Nothing to do if no entry exists, else de-activate it
	if (RL_entry==rate_limiters.end())
	{
		cout << "Adding new rate limiter for fid "<<fid<<" with peak rate "<<peak_<<" and burst rate "<<burst_size_<<endl;
		RL=new(TB_flowctrl);
		//Add a handler for this rate limiter
		RL->sh_=new LLShaperHandler(this,fid);
		RL->flow_id_=fid;
		//RL->peak_=peak_;
		//RL->burst_size_=RL->curr_bucket_contents=burst_size_;
		RL->peak_=max(LLSHAPER_MIN_RATE,peak_);
		RL->burst_size_=RL->curr_bucket_contents=max(0.0,burst_size_);
		RL->last_time=0;
		RL->RL_ACTIVE=true;
		rate_limiters.insert(RL_val(fid,*RL));
		if (logging==1)
			logfile<<"FID(inserted):"<<fid<<"\tpeak:"<<RL->peak_<<"\tburst:"<<RL->burst_size_<<endl;
	}
	else
	{
		RL=&(RL_entry->second);
		if (RL->RL_ACTIVE==false)
		{
			//RL->flow_id_=fid;
			//RL->burst_size_=RL->curr_bucket_contents=burst_size_;
			RL->peak_=max(LLSHAPER_MIN_RATE,peak_);
			RL->burst_size_=RL->curr_bucket_contents=max(0.0,burst_size_);
			RL->peak_=peak_;
			RL->last_time=0;
			RL->RL_ACTIVE=true;
			if (logging==1)
				logfile<<"FID(re-activated):"<<fid<<"\tpeak:"<<RL->peak_<<"\tburst:"<<RL->burst_size_<<endl;
		}
		//else, do nothing
	}
}

void LLShaper::RL_modify(int fid,double peak_,double burst_size_)
{
	RL_itr RL_entry;
	RL_entry=rate_limiters.find(fid);
	//Nothing to do if no entry exists, else de-activate it
	if (RL_entry==rate_limiters.end())
		cout << "No Rate limiter associated with flow " << fid << ", cannot modify\n";
	else
	{
		TB_flowctrl *RL;
		RL=&(RL_entry->second);
		//Whatever tokens were achieved using the old rate, update those first!
		//update_bucket_contents(RL) ;
		RL->curr_bucket_contents=min(RL->curr_bucket_contents,(long int)burst_size_);
		RL->flow_id_=fid;
		RL->peak_=max(LLSHAPER_MIN_RATE,peak_);
		RL->burst_size_=max(0.0,burst_size_);
		if (logging==1)
			logfile<<"FID:"<<fid<<"\tpeak:"<<RL->peak_<<"\tburst:"<<RL->burst_size_<<"\ttokens:"<<RL->curr_bucket_contents<<endl;
	}
}

void LLShaper::RL_deactivate(int fid)
{
	RL_itr RL_entry;
	RL_entry=rate_limiters.find(fid);
	//Nothing to do if no entry exists, else de-activate it
	if (RL_entry!=rate_limiters.end())
	{
		RL_entry->second.RL_ACTIVE=false;
		if (logging==1)
			logfile<<"FID:"<<fid<<"\tRL deactivated\n";
	}
	return;
}

bool LLShaper::RL_isactive(int fid)
{
	RL_itr RL_entry;
	RL_entry=rate_limiters.find(fid);
	//No rate limiting if the rate limiter is inactive, or if there is no entry for the given FID
	if ((RL_entry==rate_limiters.end()) || (RL_entry->second.RL_ACTIVE==false))
		return false;
	else
		return true;
}

void LLShaper::logto(string logfilename)
{
	logging=1;
	logfile.open(logfilename.c_str());
}
