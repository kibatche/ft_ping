#ifndef FT_PING_H
# define FT_PING_H

# include "macros.h"
# include "utils.h"

# include <argp.h>
# include <arpa/inet.h>
# include <errno.h>
# include <error.h>
# include <float.h>
# include <netinet/in.h>
# include <netinet/ip_icmp.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netdb.h>
# include <stddef.h>
# include <stdlib.h>
# include <string.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <unistd.h>
# include <signal.h>

typedef struct ping_infos
{
    int                 ping_fd;/*fd attribué à l'opération*/
    struct icmphdr      ping_pckt;
    size_t              ping_datalen;/*taille des données*/
    struct sockaddr_in  ping_address;/*adresse de ping*/
    struct sockaddr_in  destination_address;/*adresse de l'hôte*/
    char                *destination_host_name;
    char                destination_ip_addr[INET_ADDRSTRLEN];
    size_t              packet_transmitted;/*nombre de paquets émis*/
    size_t              packet_received;/*nombre de paquets reçus*/ 
}               ping_infos;

typedef struct ping_stats
{
    double min_round_trip;/*temps minimum de réception d'un paquet pour un hôte donné*/
    double max_round_trip;/*temps maximum de réception d'un paquet pour un hôte donné*/
    double sum_of_round_trip;/*somme de tous les temps de réception de paquets pour un hôte donné afin de calculer la moyenne*/
    double squared_of_round_trip;/*somme de tous les temps de réception au carré de paquets pour un hôte donné afin de calculer l'écart type*/
}               ping_stats;

void free_arg(void *arg);
void errors(const char *error, int err);
void sig_handler(int signal);

void init_args();
void init_ping(ping_infos *ping);
void check_host(ping_infos *ping);
void init_stats();
void send_ping();
void receive_ping();
void read_recv_buffer(char *recv_buf, int len, double time_spent);
void print_ip_icmp_headers(struct iphdr *ip, struct icmphdr *icmp);
void print_icmp_control_message(int type, int code, int len, char *ip_addr);
void print_intro();
void print_outro();
void print_stats();

#endif