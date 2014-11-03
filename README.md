group members:
===============
Yansong Wang
Bohua Pan


1: Modifications to ensure unicast addresses are bound:
No modifications are needed here to ensure only unicast addresses are bound.
We only do not refer to binding broadcast address part in the book.


2: Array of structures for sockets:
struct socket_configuration
{
   int sockfd;    /* socket file descriptor */
   struct in_addr ip;       /* primary address */
   struct in_addr mask;   /* network mask address */
   struct in_addr subnet;   /* subnet mask address */
};

3: Modifications of code of section 22.5
We have two modifications listed below.
  A.  int    rtt_timeout(struct rtt_info *ptr)
{
	ptr->rtt_rto *= 2; /* next RTO */
	ptr->rtt_rto=rtt_minmax(ptr->rtt_rto);        // We add this sentence here, pass its value through the function rtt_minmax
	if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
	return(-1); /* time to give up for this packet */
	return(0);
}

  B.   void   rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
	uint32_t	 delta;                  // we uses integer arithmetic rather than floating point
	ptr->rtt_rtt = ms / 1000.0; /* measured RTT in seconds */
	/*
	* Update our estimators of RTT and mean deviation of RTT.
	* See Jacobson's SIGCOMM '88 paper, Appendix A, for the details.
	* We use floating point here for simplicity.
	*/
	delta = ptr->rtt_rtt - ptr->rtt_srtt;
	ptr->rtt_srtt += delta / 8; /* g = 1/8 */
	if (delta < 0.0)
	delta = -delta; /* |delta| */
	ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4; /* h = 1/4 */
	ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}

4: TCP Mechanisms:

A: Reliable data transmission:
We implemented reliable data transmission using ARQ sliding-windows. 

-Timeouts:
Our timeout mechanism is realized with reference to Figure 22.7, 22.8 and 22.9. We used the timeout mechanism three times. 
i. The client sends a datagram to the server giving the filename. This send needs to be back up by a timeout (client side)
ii. Timeout mechanism for server sending ephemeral port number to client (server side)
iii. Timeout mechanism for server sending data to client (server side)

-Processing ACKs:
The sequence numbers for client / server are cumulative. On receiving an ACK, server fills it into the ACK array, and if
the left of the sending window is filled with an ACK, the window slides to right, otherwise it waits and maybe resends
data to get the other vacancies to be filled. In other words, we impelemnted with Selective Repeat technique.


B: Flow Control:

Our implemention of flow control uses the receiving window which is sent by the client to the server
with the initial connection setup (file request) message, and with every ACK.

-Receive Window Flow Control:
This window value is updated and checked upon each message send to ensure that the server will
not overflow the clients receive window. IF the window is full, a duplicate ACK is sent by client as soon as window opens again.

C: Congestion Control:

-Slow Start
When a new connection is established, cwnd (congestion window) is assigned to 1, sender starts sending data according to 
the size of cwnd, whenever a datagram is acknowledged, the size of cwnd increases exponentially with the RTT.(1, 2, 4, 8, ......).
If the bandwidth is w, it will take RTT*logW to occupy the full bandwidth.

-Congestion Avoidance('AIMD')
 From slow start, cwnd will increase very quickly, TCP uses ssthresh as the upper limit of cwnd. When it exceeds ssthresh, slow start ends, 
 then it comes to congestion avoidance. We use Additive-Increase now, meaning that cwnd increases by one each time an ACK is received. 
 When a datagram is lost, ssthresh is decreased to half of cwnd, cwnd is resetted to 1 and we re-enter slow start.

 -Fast Recovery
  1. When the third duplicate ACK is received, set ssthresh to one-half the cwnd. Retransmit the missing segment and set cwnd to ssthresh plus 3 times segment size. 
  2. Whenver another duplicate ACK arrives, increment cwnd by the segment size and transmit a packet. 
  3. When next ACK arrives that acknowledges new data, set cwnd to ssthresh(in step 1). Then we go back to congestion avoidance. 



 5 : dealing with certain issues

As shown in common.h, each of the datagram (struct Payload) includes a header and a data field. In the header there are
    unsigned long int seqNum
    unsigned long int timestamp
    unsigned long int ackNum
    unsigned short int winSize
    unsigned char flag
Most of them are self-descripted, while the flag contains 8 bits - if the leftmost bit is set to 1, it means the datagram carries an
ACK, whose ackNum equals to the sequence number of incoming datagram; if the second leftmost bit is set to 1, it means the datagram
carries a FIN (finsish notification).

-Sender notifying receiver of the last datagram:
Before sending the last datagram, the flag is turned on with FIN in the last datagram. When receiving (client) read the FIN, it begins
to clean up.

-Clean Closeing / Time Wait:
When receving an FIN, the client sends back an ACK and an alarm signal is set to wait for MSL (defined in constants.h), and then client
enters a loop to read any possible incoming datagram in case the ACK sent by client getting lost. If no incoming datagrams arrive, the
alarm times out and make the data-reading loop terminates. Additionally, pthread_join() is called to wait for the print thread to
terminate by itself.
