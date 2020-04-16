// Add all includes here

#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <netinet/ip_icmp.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>




// Global variables and defines here

#define pkt_size 64

int port_no = 0;  // default port number
int sleep_time = 1;  // Time before pinging again (1s)
int timeout = 1;   // timeout to recieve reply in seconds
bool keepalive = true;  // Keeps the ping loop running
int ttl = 64; // Default ttl

/*
 * ICMP packet structure.
 * Required since ICMP does not have a default packet.
 */
typedef struct pkt {
  struct icmphdr hdr;   // ICMP header
  char msg[pkt_size - sizeof(struct icmphdr)];  // message for echo
} pkt_t;

/*
 * Handler for SIGINT (ctrl-C)
 */
void handler( int sig ) {
  keepalive = false;  // End program when ctrl-C
}


/*
 * Helper function to find ip address from given hostname.
 */
char* dns_lookup (char* hostname, struct sockaddr_in* addr) {

  struct hostent* host;
  char* ip = malloc(NI_MAXHOST * sizeof(char));

  host = gethostbyname(hostname);
  if ( host == NULL ) {
    return NULL;
  } else {

    strcpy(ip, inet_ntoa( *(struct in_addr*)host->h_addr ));
    addr->sin_port = htons(port_no);
    addr->sin_family = host->h_addrtype;
    addr->sin_addr.s_addr = *(long*) host->h_addr;
  }
  return ip;
}

/*
 * Helper function to perform reverse dns lookup
 */
char* rev_lookup ( char* ip ) {

  struct sockaddr_in temp;
  socklen_t length = sizeof(struct sockaddr_in);
  char buffer[NI_MAXHOST];

  temp.sin_family = AF_INET;
  temp.sin_addr.s_addr = inet_addr(ip);

  int ret = getnameinfo((struct sockaddr *) &temp, length, buffer,
    sizeof(buffer), NULL, 0, NI_NAMEREQD);
  if ( ret != 0 ) {
    return NULL;
  }

  char *rev = malloc (( sizeof (buffer) + 1 ) * sizeof(char));
  strcpy(rev, buffer);  // Add null terminator
  return rev;
}

/*
 * Helper function to run ping loop.
 */
void ping( int sock, struct sockaddr_in *addr,
  char* rev, char* ip, char *hostname) {

  int i;
  int addr_len;
  int msg_count = 0;  // Echo messages sent
  int recv_count = 0; // replies received

  // Set up packet to be sent
  pkt_t packet;
  struct sockaddr_in r_addr;

  // set up timing
  struct timespec start, end, tfs, tfe; // Hold all time info
  long double rtt_msec = 0;
  long double total_msec = 0;
  struct timeval tv_out;
  tv_out.tv_sec = timeout;
  tv_out.tv_usec  = 0;

  clock_gettime(CLOCK_MONOTONIC, &tfs);

  // Set socket ttl
  int ret = setsockopt(sock, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
  if ( ret ) {
    printf("Setting ttl failed1\n");
  }

  // Set timeout for recv
  setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));


  bool sent;
  // Loop to send socket
  while ( keepalive ) {

    sent = true;

    // Fill packet
    bzero( &packet, sizeof( packet ));
    packet.hdr.type = ICMP_ECHO;
    packet.hdr.un.echo.id = getpid();

    for ( i = 0 ; i < sizeof ( packet.msg) -1 ; i++ ) {
      packet.msg[i] = i+'0';  // Convert to ascii
    }
    packet.msg[i] = '\0'; //Null terminator

    packet.hdr.un.echo.sequence = msg_count++;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));

    // Sleep before recalculating time
    sleep(sleep_time);




  } //while



}


/*
 * main method
 */
int main(int argc, char * argv[]) {

  // Socket for ICMP
  int sock;
  char* ip;
  char* rev_hostname;

  struct sockaddr_in addr;  // Struct for internet addresses
  int addr_len = sizeof(addr);
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
  ip = dns_lookup( hostname, &addr);
  if ( ip == NULL )  {
    printf("Cannot resolve hostname: %s\n", hostname);
    return 1;
  }

  // Perform reverse hostname search
  rev_hostname = rev_lookup( ip );
  if ( rev_hostname == NULL ) {
    printf("Can not perform reverse lookup of hostname!\n");
  }

  // Header output
  printf("PING %s (%s) - %d bytes of data.\n", hostname, ip, pkt_size);

  // Open an ICMP socket
  sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if ( sock < 0 ) {
    printf("Unable to open socket\n");
    return 1;
  }

  // Handle ctrl-C
  signal( SIGINT, handler );

  // Ping loop
  ping( sock, &addr, rev_hostname, ip, hostname );

  return 0;
}
