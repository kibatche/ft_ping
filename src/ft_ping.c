# include "ft_ping.h"

/*volatile empêche l'opti du compilo, ref : https://www.gnu.org/software/c-intro-and-ref/manual/html_node/volatile.html */
volatile sig_atomic_t cont;/*pour arrêter la boucle*/

int sleep_sec;

ping_infos ping;
ping_stats *stats;
char *hostname;
char buffer_to_send[4096];
char buffer_to_receive[4096];
int sequence;
struct timespec tm_send;
struct timespec tm_recv;

/*opt*/
int verbose_opt;
int ttl_opt;

const char *argp_program_version = "ft_ping 1.0";
static char doc[] = "Send ICMP ECHO_REQUEST packets to network hosts.";
static char args_doc[] = "HOST";

static struct argp_option options[] = {
  {"verbose",  'v', 0, 0, "verbose output", 0},
  {"ttl", TTL_ARG, "N", 0, "specify N as time-to-live", 0},
  {0}
};


static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    switch(key)
    {
        case 'v':
            verbose_opt = 1;
            break;
        case TTL_ARG:
            ttl_opt = check_ttl_value(arg);
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
                argp_usage(state);
            hostname = strdup(arg);
            if (hostname == NULL)
                errors("strdup", errno);
           break;     
        case ARGP_KEY_END:
            if (state->arg_num < 1)
                argp_usage(state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

void errors(const char *errstr, int err)
{
    free_arg(hostname);
    free_arg(stats);
    if (errstr !=  NULL && err != 0)
        error(EXIT_FAILURE, err, "%s\n", errstr);
    exit(EXIT_FAILURE);
}

void sig_handler(int sig)
{
    (void)sig;
    cont = 0;
}

void check_host(ping_infos *ping)
{
    struct hostent *host_entity = NULL;
    char *ip_temp;
    int res;
    size_t len;

    res = inet_pton(AF_INET, hostname, &ping->destination_address.sin_addr); 
    if (!res)
    {
        host_entity = gethostbyname(hostname);    
        if (host_entity == NULL)
        {
            fprintf(stderr, "ft_ping: unknown host\n");
            errors(NULL, 0);
        }
        ip_temp = inet_ntoa(*((struct in_addr*) host_entity->h_addr_list[0]));
        len = strlen(ip_temp);
        if (len >= INET_ADDRSTRLEN)
        {
            fprintf(stderr, "ft_ping: bad ip len.\n");
            errors(NULL, 0);
        }
        strcpy(ping->destination_ip_addr, ip_temp);
        ping->destination_ip_addr[len] = 0;
        if (inet_aton(ip_temp, &ping->destination_address.sin_addr) == 0)
        {
            fprintf(stderr, "ft_ping: inet_aton failed. Bad ip notation ?\n");
            errors(NULL, 0);
        }
    }
    else
        strcpy(ping->destination_ip_addr, hostname);
}

void init_ping(ping_infos *ping)
{
    struct timeval timeout;      
    timeout.tv_sec = RECV_TIMEOUT;
    timeout.tv_usec = 0;
    int fd;

    /*création de la raw socket avec le protocole icmp*/
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1)
        errors("socket", errno);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
        errors("setsockopt (timeout)", errno);
    if (ttl_opt)
        if (setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl_opt, sizeof(ttl_opt)) == -1)
            errors("setsockopt (ttl)", errno);
    bzero(ping, sizeof(ping_infos));
    ping->ping_fd = fd;
    ping->destination_address.sin_family = AF_INET;
    check_host(ping);
    ping->ping_pckt.un.echo.id = htons(getpid());
    ping->ping_pckt.un.echo.sequence = htons(0);
    ping->ping_pckt.checksum = 0;
    ping->ping_pckt.code = 0;
    ping->ping_pckt.type = ICMP_ECHO;
    ping->destination_host_name = hostname;
    ping->packet_transmitted = 0;
    ping->packet_received = 0;
}

void init_stats()
{
    stats = malloc(sizeof(ping_stats));
    if (stats == NULL)
        errors("malloc", errno);
    stats->max_round_trip = DBL_MIN;
    stats->min_round_trip = DBL_MAX;
    stats->squared_of_round_trip = 0.0;
    stats->sum_of_round_trip = 0.0;
}

void init_args()
{
    hostname = NULL;
    stats = NULL;
    verbose_opt = 0;
    ttl_opt = 0;
    sequence = 0;
    cont = 1;
}

void send_ping()
{
    //nécessaire de remttre à 0 pour recalculer
    ping.ping_pckt.checksum = 0;
    ping.ping_pckt.un.echo.sequence = sequence;
    ping.ping_pckt.checksum = checksum((unsigned short *)&ping.ping_pckt, sizeof(ping.ping_pckt));
    memcpy(buffer_to_send, &ping.ping_pckt, sizeof(ping.ping_pckt));
    clock_gettime(CLOCK_MONOTONIC, &tm_send);
    if (sendto(ping.ping_fd, buffer_to_send, 64, 0, (struct sockaddr *)&ping.destination_address, sizeof(ping.destination_address)) == -1)
        errors("sendto", errno);
    ping.packet_transmitted++;
    sequence++;
}

void receive_ping()
{
    socklen_t len_ping_addr = sizeof(ping.ping_address);
    struct timespec tm_to_sleep;
    int len_of_recv = recvfrom(ping.ping_fd, buffer_to_receive, \
        sizeof(buffer_to_receive), 0, (struct sockaddr*)&ping.ping_address, \
        &len_ping_addr);
    clock_gettime(CLOCK_MONOTONIC ,&tm_recv);
    double time_spent = compute_time_spent(&tm_send, &tm_recv, &tm_to_sleep);
    if ( len_of_recv >= 0)
    {
        read_recv_buffer(buffer_to_receive, len_of_recv, time_spent);
    }
    nanosleep(&tm_to_sleep, NULL);
}

void read_recv_buffer(char *recv_buf, int len, double time_spent)
{
    struct iphdr *ip;
    struct icmphdr *icmp;
    int iphdr_len;
    char ip_addr[INET_ADDRSTRLEN];

    ip = (struct iphdr *)recv_buf;
    iphdr_len = ip->ihl * 4;/*taille de l'en-tête ip*/
    icmp = (struct icmphdr *)(recv_buf + iphdr_len);/*struct icmp*/
    len -= iphdr_len;/*taille de notre paquet reçu - taille de l'en-tête ip*/
    inet_ntop(AF_INET, (struct sockaddr_in *)&ip->saddr, ip_addr, INET_ADDRSTRLEN);/*convertir l'adresse ip numérique en représentation x.x.x.x*/
    if (len < (int)ping.ping_datalen)
        fprintf(stderr, "packet too short (%d bytes) from %s\n", len, ip_addr);
    else if (icmp->type == ICMP_ECHOREPLY && icmp->un.echo.id == ping.ping_pckt.un.echo.id )
    {
        int checksum_of_icmp = icmp->checksum;
        icmp->checksum = 0;
        if (checksum_of_icmp != checksum((unsigned short *) icmp, sizeof(*icmp)))
        {
            fprintf(stderr, "checksum mismatch from %s\n", ip_addr);// cela veut dire que la façon dont le calcul a été fait par le serveur cible est différent.
        }
        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", len, ip_addr, icmp->un.echo.sequence, ip->ttl, time_spent);
        stats->max_round_trip = MAX(stats->max_round_trip, time_spent);
        stats->min_round_trip = MIN(stats->min_round_trip, time_spent);
        stats->sum_of_round_trip += time_spent;
        stats->squared_of_round_trip += time_spent * time_spent;
        ping.packet_received++;
    }
    else
    {     
        print_icmp_control_message(icmp->type, icmp->code, len, ip_addr);
        if (verbose_opt)
        {
            struct iphdr *ipp = (struct iphdr *)((unsigned char *)icmp + sizeof(struct icmphdr));
            struct icmphdr *icmpp = (struct icmphdr *)((unsigned char *)ipp + sizeof(struct iphdr));
            print_ip_icmp_headers(ipp, icmpp);
        }
    }
}

int main(int ac, char **av)
{
    init_args();
    argp_parse(&argp, ac, av, 0, 0, NULL);
    init_ping(&ping);
    init_stats();   
    print_intro();
    signal(SIGINT, sig_handler);
    while (cont)
    {
        send_ping();
        receive_ping();
    }
    print_outro();
    free_arg(hostname);
    free_arg(stats);
    return 0;
}

void print_intro()
{
    printf("PING %s (%s): %d data bytes", ping.destination_host_name, ping.destination_ip_addr, DATALEN);
    if (verbose_opt)
        printf(", id 0x%04x = %d", ping.ping_pckt.un.echo.id, ping.ping_pckt.un.echo.id);
    printf("\n");
}

void print_outro()
{   
    printf("--- %s ping statistics ---\n", ping.destination_host_name);
    printf("%ld packets transmitted, %ld packets received,", ping.packet_transmitted, ping.packet_received);
    if (ping.packet_transmitted < ping.packet_received)
        printf (" -- somebody is printing forged packets!\n");
    else
        printf(" %d%% packet loss\n", (int)(((ping.packet_transmitted - ping.packet_received) * 100) / ping.packet_transmitted));
    if (ping.packet_received)
        print_stats();
}

void print_stats()
{
    double av = stats->sum_of_round_trip / ping.packet_received;
    //écart type == racine carrée de la variance, qui est la moyenne des carrés des valeurs - le carré de la moyenne des valeurs
    double stddev = mysqrt(stats->squared_of_round_trip / ping.packet_received - av * av);
    printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", stats->min_round_trip, av, stats->max_round_trip, stddev);
}

/*https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol#Control%20messages*/
void print_icmp_control_message(int type, int code, int len, char *ip_addr)
{
    printf("%d bytes from %s (%s): ", len, ip_addr, ip_addr);
    switch (type)
    {
        case ICMP_ECHOREPLY:
            printf("Echo Reply");
            break;
        case ICMP_DEST_UNREACH:
            switch (code)
            {
                case ICMP_NET_UNREACH:
                    printf("Destination Net Unreachable\n");
                    break;
                case ICMP_HOST_UNREACH:
                    printf("Destination Host Unreachable\n");
                    break;
                case ICMP_PROT_UNREACH:
                    printf("Destination Protocol Unreachable\n");
                    break;
                case ICMP_PORT_UNREACH:
                    printf("Destination Port Unreachable\n");
                    break;
                case ICMP_FRAG_NEEDED:
                    printf("Fragmentation needed and DF set\n");
                    break;
                case ICMP_SR_FAILED:
                    printf("Source Route Failed\n");
                    break;
                case ICMP_NET_UNKNOWN:
                    printf("Network Unknown\n");
                    break;
                case ICMP_HOST_UNKNOWN:
                    printf("Host Unknown\n");
                    break;
                case ICMP_HOST_ISOLATED:
                    printf("Host Isolated\n");
                    break;
                case ICMP_NET_ANO:
                    printf("Network Administratively Prohibited\n");
                    break;
                case ICMP_HOST_ANO:
                    printf("Host Administratively Prohibited\n");
                    break;
                case ICMP_NET_UNR_TOS:
                    printf("Destination Network Unreachable At This TOS\n");
                    break;
                case ICMP_PKT_FILTERED:
                    printf("Packet Filtered\n");
                    break;
                case ICMP_PREC_VIOLATION:
                    printf("Precedence Violation\n");
                    break;
                case ICMP_PREC_CUTOFF:
                    printf("Precedence Cutoff\n");
                    break;
                default:
                    printf("Unknow code for type ICMP_DEST_UNREACH\n");
            }
            break;
        case ICMP_SOURCE_QUENCH:
            printf("Source Quench");
            break;
        case ICMP_REDIRECT:
            switch (code)
            {
                case ICMP_REDIR_NET:
                    printf("Redirect Network\n");
                    break;
                case ICMP_REDIR_HOST:
                    printf("Redirect Host\n");
                    break;
                case ICMP_REDIR_NETTOS:
                    printf("Redirect Type of Service and Network\n");
                    break;
                case ICMP_REDIR_HOSTTOS:
                    printf("Redirect Type of Service and Host\n");
                    break;
                default:
                    printf("Unknow code for ICMP_REDIRECT\n");
                    break;
            }
            break;
        case ICMP_ECHO:
            printf("Echo Request");
            break;
        case ICMP_TIME_EXCEEDED:
            switch (code)
            {
                case ICMP_EXC_TTL:
                    printf("Time to live exceeded\n");
                    break;
                case ICMP_EXC_FRAGTIME:
                    printf("Frag reassembly time exceeded");
                    break;
                default:
                    printf("Unknown code for ICMP_TIME_EXCEEDED\n");
            }
            break;
        case ICMP_PARAMETERPROB:
            printf("Parameter Problem");
            break;
        case ICMP_TIMESTAMP:
            printf("Timestamp");
            break;
        case ICMP_TIMESTAMPREPLY:
            printf("Timestamp Reply");
            break;
        case ICMP_INFO_REQUEST:
            printf("Information Request");
            break;
        case ICMP_INFO_REPLY:
            printf("Information Reply");
            break;
        case ICMP_ADDRESS:
            printf("Address Mask Request");
            break;
        case ICMP_ADDRESSREPLY:
            printf("Address Mask Reply");
            break;
        default:
            printf("Unknown type.\n");
            break;
    }
}

void print_ip_icmp_headers(struct iphdr *ip, struct icmphdr *icmp)
{
    unsigned char *ip_to_print = (unsigned char *)ip;
    char src_addr_st[INET_ADDRSTRLEN];
    char dst_addr_st[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, (struct sockaddr_in *)&ip->saddr, src_addr_st, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, (struct sockaddr_in *)&ip->daddr, dst_addr_st, INET_ADDRSTRLEN);
    ip_to_print = (unsigned char *)ip;
    printf("IP Hdr Dump:\n");
    for (size_t i = 0; i < sizeof(*ip); i ++)
    {
        printf("%02x%s", ip_to_print[i], (i & 1 ? " " : ""));
    }
    printf("\n");
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
    //https://en.wikipedia.org/wiki/IPv4#Flags => on conervse les trois premiers bits les plus significatifs.
    uint32_t fl = ntohs(ip->frag_off) >> 13;
    // https://en.wikipedia.org/wiki/IPv4#Fragment%20offset => les 13 bits de l'offset donc
    uint32_t off = ntohs(ip->frag_off) & 0x1fff;
    printf(" %1x  %1x  %02x %04x %04x", ip->version, ip->ihl, ip->tos, ntohs(ip->tot_len), ntohs(ip->id));
    printf ("   %1x %04x  %02x  %02x %04x",fl, off, ip->ttl, ip->protocol, ntohs(ip->check));
    printf (" %s  %s \n", src_addr_st, dst_addr_st);
    printf ("ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n", \
    icmp->type, icmp->code, ntohs (ip->tot_len) - ip->ihl * 4, icmp->un.echo.id, icmp->un.echo.sequence);
}