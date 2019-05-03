#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>

#define TIME_DELTA 2208988800

typedef struct
{
  uint8_t li_vn_mode;      

  uint8_t stratum;        
  uint8_t poll;            
  uint8_t precision;       

  uint32_t rootDelay;     
  uint32_t rootDispersion;
  uint32_t refId;         

  uint32_t refTm_s; 
  uint32_t refTm_f; 

  uint32_t origTm_s;
  uint32_t origTm_f; 

  uint32_t recTm_s;
  uint32_t recTm_f;

  uint32_t trTm_s; 
  uint32_t trTm_f; 

} sntp_packet;  

int main(int argc, char** argv) {

    int socket_desc = socket(AF_INET, SOCK_DGRAM, 0); 
    if (socket_desc == -1){
       perror("Could not create socket");
       exit(1);
    }

    struct sockaddr_in server;                      
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;                    
    server.sin_port = htons(atoi(argv[2])); 

    if (connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0){ 
        perror("Could not connect");
        exit(1);
    }

    uint32_t roundtrip = 0;
    int n;
    sntp_packet packet;

 //                                                                                               printf("qqq\n");
    for (int i = 0; i < 3; i++){

        memset( &packet, 0, sizeof(sntp_packet));
        *((char*) &packet + 0) = 0x23;

        time_t seconds_before = time(NULL);

        n = write(socket_desc, (char*) &packet, sizeof(sntp_packet));
        if ( n < 0 ) {
            perror("ERROR writing to socket");
            exit(1);
        }
        n = read(socket_desc, (char*) &packet, sizeof(sntp_packet));
        if ( n < 0 ) {
            perror("ERROR reading from socket");
            exit(1);
        }
//                                                                                                printf("ppp\n");
        time_t seconds_after = time(NULL);

        packet.trTm_s = ntohl(packet.trTm_s) - TIME_DELTA;
        packet.recTm_s = ntohl(packet.recTm_s) - TIME_DELTA;

        roundtrip += (((uint32_t)seconds_before - (uint32_t)seconds_after) - (packet.recTm_s - packet.trTm_s)) / 2;
//                                                                                                printf("%i\n", roundtrip);
    }

    roundtrip /= 3;
    uint32_t real_time = packet.trTm_s + roundtrip;

    printf( "roundtrip: %i\n", roundtrip);
    printf( "Time: %s", ctime((const time_t*) &real_time));

    return 0;
}