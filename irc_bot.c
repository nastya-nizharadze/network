#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <curl/curl.h>

void str_replace(char *target, const char *needle, const char *replacement)
{
    char buffer[1024] = { 0 };
    char *insert_point = &buffer[0];
    const char *tmp = target;
    size_t needle_len = strlen(needle);
    size_t repl_len = strlen(replacement);

    while (1) {
        const char *p = strstr(tmp, needle);
        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }
        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;
        memcpy(insert_point, replacement, repl_len);
        insert_point += repl_len;
        tmp = p + needle_len;
    }
    strcpy(target, buffer);
}

void log_with_date(char *line) {
    char date[50];
    struct tm *current_time;

    time_t now = time(0);
    current_time = gmtime(&now);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", current_time);

    printf("[%s] %s\n", date, line);
}

void log_to_file(char *line, FILE *logfile) {
    char date[50];
    struct tm *current_time;

    time_t now = time(0);
    current_time = gmtime(&now);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", current_time);

    fprintf(logfile, "[%s] %s\n", date, line);
    fflush(logfile);
}

void send_message(int sock, char * to, char * message){
    char message_packet[512];
    sprintf(message_packet, "PRIVMSG %s :%s\r\n", to, message);
    send(sock, message_packet, strlen(message_packet), 0);
}

void anekdot (int sock, char * to, char * channel, char * nick, FILE * logfile){

    char logline[512];
    system("curl -s https://bash.im/forweb/?u > bash.txt");
    FILE *fp = fopen("bash.txt", "rw");
    char s[1025];
    while(0 <= fscanf(fp, "%[^<]<%[^>]", s, s)){
        //printf("%s\n", s);
        if (!strcmp(s, "' + 'div id=\"b_q_t\" style=\"padding: 1em 0;\"")){
            break;
        }
    }

    while(0 <= fscanf(fp, ">%[^<]", s)){
        if (!strcmp(s , "' + 'br")|| !strcmp(s, "' + 'br /")){
            send_message(sock, to, " ");
            goto xyi;
        } 
        //printf("%s\n", s);
        str_replace(s, "&quot;", "\"");
        str_replace(s, "&lt;", "<");
        str_replace(s, "&gt;", ">");

        char * clone1 = (char*) malloc(sizeof(char)*2048); 
        char * clone2 = (char*) malloc(sizeof(char)*2048);
        memset (clone1, '\0', 2048); 
        memset (clone2, '\0', 2048);
        char * p1 = clone1;
        char * p2 = clone2; 
        strcpy(clone1, s);
        while(strlen(clone1)>200){
            clone2 = strtok(clone1 + 200, " ");
            clone2 = clone1 + strlen(clone1) + 1;
            //printf("clone1, clone2 %s, %s\n", clone1, clone2);

            send_message(sock, to, clone1);
            sprintf(logline, "%s/%s: %s", channel, nick, clone1);
            log_with_date(logline);
            log_to_file(logline, logfile);
            clone1 = clone2;
            //printf("clone1, clone2 %s, %s\n", clone1, clone2);
        }
        send_message(sock, to, clone1);
        sprintf(logline, "%s/%s: %s", channel, nick, clone1);
        log_with_date(logline);
        log_to_file(logline, logfile);
        clone1 = p1;
        clone2 = p2;
        free(clone1);
        free(clone2);
        xyi:
        fscanf(fp, "<%[^>]", s);
        if (strcmp(s, "' + 'br") && strcmp(s, "' + 'br /")){
            break;
        }
    }

        
    fclose(fp);

}

void parser (char * line, char * username, char * command, char * argument, char * last_argument){

    char * clone = (char*) malloc(sizeof(char)*513); 
    memset (clone, '\0', 513);     
    char prefix[512];
    char * splitted;
    char * clone_of_clone = clone; 

    strncpy(clone, line, strlen(line) + 1);                                   //username
    if (clone[0] == ':'){
        splitted = strtok(clone, " ");
        strcpy(prefix, clone);
        clone = clone + strlen(clone) + 1;
        splitted = strtok(prefix, "!");
        if (splitted != NULL){
            strncpy(username, prefix + 1, strlen(prefix));
        }
    }

    splitted = strtok(clone, " ");                                            //command
    if(splitted != NULL){
        strcpy(command, clone);
        clone = clone + strlen(clone) + 1;

    }

    if(clone[0] == ':'){                                                      //arguments
        strcpy(last_argument, clone + 1);
    }else{
        splitted = strtok(clone, ":"); 
        if(splitted != NULL){
            strncpy(argument, clone, strlen(clone));
            argument[strlen(clone)] = '\0';
            clone = clone + strlen(clone) + 1;
            strncpy(last_argument, clone, strlen(clone));
            last_argument[strlen(clone)] = '\0';
        }else{
            strcpy(argument, clone);
        }
    }

    clone = clone_of_clone;
    free(clone);

}

int read_line(int sock, char *buffer) {
    size_t length = 0;

    while (1) {
        char data;
        int result = recv(sock, &data, 1, 0);

        if ((result <= 0) || (data == EOF)){
            perror("Connection closed");
            exit(1);
        }

        buffer[length] = data;
        length++;
        
        if (length >= 2 && buffer[length-2] == '\r' && buffer[length-1] == '\n') {
            buffer[length-2] = '\0';
            return length;
        }
    }
}

int main() {

    FILE *logfile = fopen("bot.log.txt", "a+");
    char magic[] = "do some magic";

    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);                              //ipv4     //tcp     //ip
    if (socket_desc == -1){
       perror("Could not create socket");
       exit(1);
    }

    char ip[255];
    printf("server: ");
    scanf("%s", ip);

    int port;
    printf("port: ");
    scanf("%i", &port);

    char nick[255];
    printf("nick: ");
    scanf("%s", nick);

    char channel[255];
    printf("channel: ");
    scanf("%s", channel);

    struct sockaddr_in server;                      
    server.sin_addr.s_addr = inet_addr(ip);                                         //inet_addr : from ip to type for server.sin_addr.s_addr
    server.sin_family = AF_INET;                    
    server.sin_port = htons(port);                                                  //htons : from int to type for server.sin_port

    if (connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0){     //connect socket to the adress
            perror("Could not connect");
            exit(1);
    }

    char nick_packet[512];
    sprintf(nick_packet, "NICK %s\r\n", nick);
    send(socket_desc, nick_packet, strlen(nick_packet), 0);

    char user_packet[512];
    sprintf(user_packet, "USER %s 0 * :%s\r\n", nick, nick);
    send(socket_desc, user_packet, strlen(user_packet), 0);

    char join_packet[512];
    sprintf(join_packet, "JOIN %s\r\n", channel);
    send(socket_desc, join_packet, strlen(join_packet), 0);

    while (1){

        char * username = (char*) malloc(sizeof(char)*512); 
        char * command = (char*) malloc(sizeof(char)*512);
        char * argument = (char*) malloc(sizeof(char)*512);
        char * last_argument = (char*) malloc(sizeof(char)*512);

        username[0] = '\0';
        command[0] = '\0';
        argument[0] = '\0';
        last_argument[0] = '\0';

        char line[512];
        read_line(socket_desc, line);
        parser (line, username, command, argument, last_argument);

        if (strcmp(command, "PING") == 0){
            char pong_packet[512];
            sprintf(pong_packet, "PONG :%s\r\n", last_argument);
            send(socket_desc, pong_packet, strlen(pong_packet), 0);
            log_with_date("Got ping. Replying with pong...");

        }else if (strcmp(command, "PRIVMSG") == 0){
            char logline[512];
            char *channel = argument;
            sprintf(logline, "%s/%s: %s", channel, username, last_argument);
            log_with_date(logline);
            log_to_file(logline, logfile);
            if ((strstr(last_argument, magic) != NULL) && ((strstr(last_argument, nick) != NULL) || (strstr(argument, nick) != NULL))){
                anekdot(socket_desc, channel, channel, username, logfile);
                // char reply_packet[512];
                // sprintf(reply_packet, "PRIVMSG %s :%s\r\n", channel, "*some magic*");
                // send(socket_desc, reply_packet, strlen(reply_packet), 0);
                // sprintf(logline, "%s/%s: %s", channel, nick, "*some magic*");
                // log_with_date(logline);
                // log_to_file(logline, logfile);
            }

        }else if (strcmp(command, "JOIN") == 0){
            char logline[512];
            char *channel = argument;
            sprintf(logline, "%s joined %s.", username, channel);
            log_with_date(logline);
            log_to_file(logline, logfile);

        }else if (strcmp(command, "PART") == 0){
            char logline[512];
            char *channel = argument;
            sprintf(logline, "%s left %s.", username, channel);
            log_with_date(logline);
            log_to_file(logline, logfile);
        }

        free(username);
        free(command);
        free(argument);
        free(last_argument);
    }
    
    return 0;
}

//195.154.200.232