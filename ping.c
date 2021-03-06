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
#include <errno.h>



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

    (*addr).sin_family = host->h_addrtype;
    (*addr).sin_port = htons (port_no);
    (*addr).sin_addr.s_addr  = *(long*)host->h_addr;
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
 * Helper function to calculate checksum
 */
unsigned short checksum( void* packet, int len ) {

  unsigned short *buf = packet;
  unsigned short result, sum = 0;

  for (sum = 0 ; len > 1 ; len -= 2 ) {
    sum += *buf++;
  }

  if ( len == 1 )
    sum += *(unsigned char*)buf;

  sum = ( sum >> 16 ) + (sum & 0xFFFF );
  sum += (sum >> 16);
  result = ~sum;
  return result;
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
  long double rtt_ms = 0;
  long double total_ms = 0;
  struct timeval tv_out;
  tv_out.tv_sec = 1;
  tv_out.tv_usec  = 0;

  clock_gettime(CLOCK_MONOTONIC, &tfs);

  int ttlval = ttl;

  // Set socket ttl
  int ret = setsockopt(sock, SOL_IP, IP_TTL, &ttlval, sizeof(ttlval));
  if ( ret != 0 ) {
    printf("Setting ttl failed\n");
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

    for ( i = 0 ; i < sizeof(packet.msg) - 1 ; i++ ) {
      packet.msg[i] = i+'0';  // Convert to ascii
    }
    packet.msg[i] = 0; //Null terminator

    packet.hdr.un.echo.sequence = msg_count++;
    packet.hdr.checksum = checksum(&packet, sizeof(packet));

    // Sleep before pinging
    usleep(1000000);
    //sleep(sleep_time);

    // Send packet
    clock_gettime( CLOCK_MONOTONIC, &start );
    ret = sendto ( sock, &packet, sizeof(packet), 0,(struct sockaddr*)addr,
      sizeof ( *addr ));
    if ( ret <= 0 ) {
      printf("Unable to send packet.\n");
    }

    // Receive packet
    addr_len = sizeof(r_addr);
    ret = recvfrom( sock, &packet, sizeof(packet), 0, (struct sockaddr*)&r_addr,
      &addr_len);
    if ( ret <= 0 && msg_count > 1 ) {
      printf("No packet received.\n");
    } else {
      clock_gettime(CLOCK_MONOTONIC, &end);
      double elapsed = ((double)(end.tv_nsec - start.tv_nsec))/1000000.0;
      rtt_ms = (end.tv_sec - start.tv_sec) * 1000.0 + elapsed;

      // if packet was not sent, don't receive.
      if(sent) {
        if( packet.hdr.type !=69 || packet.hdr.code!=0) {
          printf("Error..Packet received with ICMP type %d code %d\n",
                    packet.hdr.type, packet.hdr.code);
           } else {
             printf("%d bytes from %s: icmp_seq=%d ttl=%d rtt = %Lf ms.\n",
              pkt_size, ip, msg_count, ttl, rtt_ms);
            recv_count++;
          }
       }
    }

  } //while

  // Find total time
  clock_gettime(CLOCK_MONOTONIC, &tfe);
  double elapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec))/1000000.0;
  total_ms = (tfe.tv_sec-tfs.tv_sec) * 1000.0 + elapsed;



  // Ending output
  float packet_loss = (float) ((float)(msg_count - recv_count)/(float)msg_count)*100;

  printf("--- %s ping statistics ---\n",hostname);
  printf("%d packets transmitted, %d received, %.2f%% packet_loss, time %dms\n\n",msg_count,
    recv_count,packet_loss, total_ms);



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
