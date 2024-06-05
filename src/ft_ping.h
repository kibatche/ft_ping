#ifndef FT_PING_H
# define FT_PING_H

# include "macros.h"

# include <netinet/in.h>
# include <netinet/ip_icmp.h>
# include <argp.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <stddef.h>
# include <errno.h>
# include <error.h>
# include <stdlib.h>
# include <netdb.h>
# include <string.h>

typedef struct ping_infos
{
    int                 ping_fd;/*fd attribué à l'opération*/
    struct icmphdr      ping_pckt;/*le packet icmp avec différentes valeur dedans (contient l'id, la séquence etc.)*/

    struct timeval      ping_start_time;/*pour calculer le temps mis pour recevoir un paquet*/
    size_t              ping_interval;/*interval en seconde à attendre avant chaque envoi*/
    size_t              ping_datalen;/*taille des données*/
    struct sockaddr_in  ping_address;/*adresse de ping*/
    struct sockaddr_in  destination_address;/*adresse de l'hôte*/
    size_t              packet_emitted;/*nombre de paquets émis*/
    size_t              packet_received;/*nombre de paquets reçus*/ 
    size_t              packet_duplicated;/*nombre de paquets dupliqués*/
}               ping_infos;

typedef struct ping_stats
{
    double min_round_trip;/*temps minimum de réception d'un paquet pour un hôte donné*/
    double max_round_trip;/*temps maximum de réception d'un paquet pour un hôte donné*/
    double sum_of_round_trip;/*somme de tous les temps de réception de paquets pour un hôte donné afin de calculer la moyenne*/
    double squared_of_round_trip;/*somme de tous les temps de réception au carré de paquets pour un hôte donné afin de calculer l'écart type*/
}               ping_stats;

char *dns_lookup(void);

#endif