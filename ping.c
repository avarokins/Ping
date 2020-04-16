// Add all includes here

#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/ip_icmp.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>



// Global variables and defines here

#define pkt_size 64

int port_no = 0;  // default port number
int sleep = 1000000;  // Time before pinging again (1s)
int timeout = 1;   // timeout to recieve reply in seconds
bool keepalive = true;  // Keeps the ping loop running
int ttl = 64; // Default ttl

/*
 * ICMP packet structure.
 * Required since ICMP does not have a default packet.
 */
struct pkt {
  struct icmphdr hdr;   // ICMP header
  char msg[pkt_size - sizeof(struct icmphdr)];  // message for echo
};

// Handler for SIGINT (ctrl-C)
void handler( int sig ) {
  keepalive = false;  // End program when ctrl-C
}

char* dns_lookup () {



}








int main(int argc, char * argv[]) {

  // Socket for ICMP
  int sock;
  char* ip;
  char* rev_hostname;

  struct sockaddr_in addr;  // Struct for internet addresses
  int addr_len = sizeof(addr_con);
  char net_but[NI_MAXHOST];

  // Parse arguments
  char* hostname;

  if ( argc < 2 ) {
    printf("Format:\nsudo ./myPing [-t <ttl>] <hostname>\n");
    return 1;
  }

  for ( int i = 1 ; i < argc ; i++ ) {
    if ( strcmp(argv[i],"-t") == 0) {
      i++;
      if ( i == argc-1 ) {
        printf("Format:\nsudo ./myPing [-t <ttl>] <hostname>\n");
        return 1;
      }
      sscanf(argv[i], "%d", &ttl);
    } else {
      hostname = argv[i];
    }
  }

  // Perform DNS lookup
  ip = dns_lookup( hostname, &addr)






  signal( SIGINT, handler );

  return 0;
}
