#ifndef ns_llshapper_h
#define ns_llshapper_h
#include <map>

#include "packet.h"
#include "connector.h"
#include "queue.h"

//Maximum number of Rate limiters allowed per class
//#define MAX_RL 3
//cur_bucket_contents should be at least equal to size of 1 packet (1500*8=12000, though ns default size might be more around 1024 bytes)
#define LLSHAPER_MIN_RATE 12000.00

using namespace std;

class LLShaper;
class LLShaperHandler;

//Information for controlling single fid with given rate
struct TB_flowctrl
{
	//Is the RL active?  If it isn't, the next time the packet queue gets empty, we're going to delete the packet queue as well as the RL entry as well
	bool RL_ACTIVE;
	//queue
	PacketQueue shape_queue;
	//Control Parameters-- the actual Token Bucket interface that the user has control over
	//we are dealing with really large numbers, so I prefer to keep everything double: int gave me a lot of trouble!
	double peak_;
	double burst_size_ ;
	//Initially set to burst_size_
	long int curr_bucket_contents ;
	int flow_id_;
	//last time update_bucket_contents() was called (Scheduler::instance().clock() at that time
	double last_time ;
	//ShaperHandler object that calls the resume function of LLSHaper when resume is called
	LLShaperHandler *sh_;
};
//Using std::map to simplify access
typedef std::map<int,TB_flowctrl> RL_map;
typedef RL_map::iterator RL_itr;
typedef RL_map::value_type RL_val;

class LLShaperHandler : public Handler {
public:
	LLShaperHandler(LLShaper *s,int fid) : shaper_(s), flow_id(fid) {}
	//Calls the resume function of the shaper (shaper_->resume())
	void handle(Event *e);
private:
	LLShaper *shaper_;
	int flow_id;
};


class LLShaper: public Connector {
private:
	//A map containing flowids and their corresponding rate limiters (if any)
	//In real life settings, you would most likely use basic techniques such as arrays
	//However, for simplicity, and high level abstraction (easier for me to focus on the logic when I don't bother about the low level implementation), I chose maps.  It should be fairly simple to replace the maps with arrays if needed
	RL_map rate_limiters;
	//We are only keeping global statistics in this implementation
	//packets received that actually MATCH the fid specified
	long int received_packets;
	long int sent_packets;
	long int shaped_packets;
	long int dropped_packets;
	//queue
	int max_queue_length;
	int logging;
	ofstream logfile;
	//Drop the packet if our shaper queue length is greater than the max queue length allowed for the shaper, otherwise schedule the packet, and enqueue it in the shaping queue.
        bool shape_packet(TB_flowctrl *RL,Packet *p) ;
	//Calculate delay for packet, and schedule it to be sent after that time
        void        schedule_packet(TB_flowctrl *RL,Packet *p) ;
	//True if packetsize is less than curr_bucket_contents, False otherwise
        bool in_profile(TB_flowctrl *RL,Packet *p) ;
	//Add tokens to the bucket (i.e. add bits equivalent to last update to bucket).  Updates curr_bucket_contents and last_time
	void update_bucket_contents(TB_flowctrl *RL) ;
	//The name tells it all: reset the counters (received_packets, sent_packets,shaped_packets,dropped_packets
	void reset_counters();
public:
	LLShaper();
	//send the packet that was scheduled, and if the queue is not empty, schedule the next packet
	void resume(int flow_id);
	/*Send immediately if:
		i. fid does not match our fid
		ii. no packets are in the queue, and the packet size is less than the burst size
	else:
		schedule the packet
	*/
	void recv(Packet *p, Handler *h) ;
	void RL_activate(int fid,double peak_,double burst_size_);
	void RL_modify(int fid,double peak_,double burst_size_);
	void RL_deactivate(int fid);
	bool RL_isactive(int fid);
	void logto(string logfile);
	int command(int argc, const char*const* argv) ;
} ;

#endif
