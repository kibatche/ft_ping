#ifndef FT_PING_H
#define FT_PING_H

#include <netinet/in.h>
#include <icmp.h>
#include <argp.h>
#include <errno.h>
#include <error.h>
#include <sys/socket.h>

typedef struct ping_infos
{
    int                 ping_fd;/*fd attribué à l'opération*/
    uint16_t            ping_id;/*l'id attribué à un packet. Unsigned short.*/
    int                 ping_type;/*icmp*/
    size_t              ping_count;/*nombre de paquets à transmettre*/
    struct timeval      ping_start_time;/*pour calculer le temps mis pour recevoir un paquet*/
    size_t              ping_interval;/*interval en seconde à attendre avant chaque envoi*/
    size_t              ping_datalen;/*taille des données*/
    struct sockaddr_in  ping_address;/*adresse de ping*/

    struct sockaddr_in  host_address;/*adresse de l'hôte*/
    char                *hostname;/*nom d'hôte de la cible*/
    char                *buffer_for_rec;
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