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
We implemented reliable data transmission using ARQ sliding-windows, with Fast Retransmit. 

-Timeouts:
Our timeout mechanism is realized with reference to Figure 22.7, 22.8 and 22.9. We used the timeout mechanism
three times. 
i. The client sends a datagram to the server giving the filename. This send needs to be back up by a timeout (client side)
ii. Timeout mechanism for server sending ephemeral port number to client (server side)
iii. Timeout mechanism for server sending data to client (server side)

-Processing ACKs:
Upon receiving an ACK for a packet, which may be cumulative, the server will remove all packets from
the sending window up to, but not including, the sequence number ACKed back, as the client is still
waiting for that number.

-Fast Retransmit:
Furthermore, when an ACK is received, the packet for whom the pack was sent receives and updated ACK
counter. This counter is what is used in determining the fast retransmit. If a particular ack comes
back for a packet 4 times (which is three duplicate ACKs), then the serverwill execute a fast
retransmit for that packet.


B: Flow Control:
Our implemention of flow control uses the receive_window that is sent by the client to the server
with the initial connection setup (file request) message, and with every ACK.

-Receive Window Flow Control:
This window value is updated and checked upon each message send to ensure that the server will
not overflow the clients receive window.

C: Congestion Control:
The congestion control mechanism is implemented as Additive Increase Multiplicative Decrease with
Slow Start. ADIM is implemented by manipulating the congestion window, which is used as the limit for
the number of packets that can be in flight at any given time.

-Additive Increase:
Upon a packet being acknowledged the congestion window is increased by 1 if under the
Slow Start Threshold, or by 1/n (where n is the current value of congestion window) if at or over
the Slow Start Threshold.

-Multiplicative Decrease:
There are two occurances where the congestion window will decrease. This occurs when there is a
timeout and when there are three duplicate acks, known as fast retransmit.
1: In the case of a timeout, the congestion window drops to 1, and the threshold is set to half
of the congestion window's previous value.
2: In the case of fast retransmit, the congestion window drops to half the current value,
and the slow start threshold is also set to this value.


 5 : dealing with certain issues

-Sender notifying receiver of the last datagram:


-Clean Closeing / Time Wait:

