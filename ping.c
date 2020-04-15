// Add all includes here

#include<stdio.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/ip_icmp.h>

// Global variables and defines here

#define pkt_size 64

int port_no = 0;  // default port number
int sleep = 1000000;
int timeout = 1;   // timeout to recieve reply in seconds
int pingloop = 1;

/*
 * ICMP packet structure.
 * Required since ICMP does not have a default packet.
 */

struct pkt {
  struct icmphdr hdr;   // ICMP header
  char msg[pkt_size - sizeof(struct icmphdr)];  // message for echo
};




int main() {

  return 0;
}
