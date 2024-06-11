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
static char doc[] = "A program that partially reimplements the ping program from inetutils.";
static char args_doc[] = "HOST";

static struct argp_option options[] = {
  {"verbose",  'v', 0, 0, "verbose output" },
  {"ttl", TTL_ARG, "N", 0, "specify N as time-to-live"},
  {0}
};


static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch(key)
    {
        case 'v':
            verbose_opt = 1;
            break;
        case TTL_ARG:
            ttl_opt = check_ttl_value(arg);
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
                argp_usage(state);
            hostname = strdup(arg);
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

static struct argp argp = { options, parse_opt, args_doc, doc };

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
        if (setsockopt(fd, SOL_IP, IP_TTL, &ttl_opt, sizeof(ttl_opt)) == -1)
            errors("setsockopt (ttl)", errno);
    bzero(ping, sizeof(ping_infos));
    ping->ping_fd = fd;
    ping->destination_address.sin_family = AF_INET;
    check_host(ping);
    ping->ping_pckt.un.echo.id = htons(getpid());
    ping->ping_pckt.un.echo.sequence = 0;
    ping->ping_pckt.checksum = 0;
    ping->ping_pckt.code = 0;
    ping->ping_pckt.type = ICMP_ECHO;
    ping->destination_host_name = hostname;
    ping->packet_emitted = 0;
    ping->packet_received = 0;
    ping->packet_duplicated = 0;
}

void init_stats()
{
    stats = malloc(sizeof(ping_stats));
    if (stats == NULL)
        errors("malloc", errno);
    stats->max_round_trip = 0.0;
    stats->min_round_trip = 0.0;
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
    ping.packet_emitted++;
    ping.ping_pckt.checksum = 0;
    ping.ping_pckt.un.echo.sequence = sequence;
    ping.ping_pckt.checksum = checksum((unsigned short *)&ping.ping_pckt, sizeof(ping.ping_pckt));
    clock_gettime(CLOCK_MONOTONIC, &tm_send);
    memcpy(buffer_to_send, &ping.ping_pckt, sizeof(ping.ping_pckt));
    if (sendto(ping.ping_fd, buffer_to_send, 64, 0, (struct sockaddr *)&ping.destination_address, sizeof(ping.destination_address)) < 0)
        errors("sendto", errno);
    sequence++;
}

void read_recv_buffer(char *recv_buf, int len, double time_spent)
{
    struct iphdr *ip;
    int iphdr_len;
    struct icmphdr *icmp;
    char ip_addr[INET_ADDRSTRLEN];

    ip = (struct iphdr *)recv_buf;
    iphdr_len = ip->ihl * 4;/*taille de l'en-tête ip*/
    icmp = (struct icmphdr *)(recv_buf + iphdr_len);/*struct icmp*/
    len -= iphdr_len;/*taille de notre paquet de réception - taille de l'en-tête ip*/
    inet_ntop(AF_INET, (struct sockaddr_in *)&ip->saddr, ip_addr, INET_ADDRSTRLEN);
    if (len < ping.ping_datalen)
    {
        fprintf(stderr, "packet too short (%d bytes) from %s\n", len, inet_ntoa(ping.destination_address.sin_addr));
    }
    else if (icmp->type == ICMP_ECHOREPLY && icmp->un.echo.id == ping.ping_pckt.un.echo.id )
    {
        int checksum_of_icmp = icmp->checksum;
        icmp->checksum = 0;
        if (checksum_of_icmp != checksum((unsigned short *) icmp, sizeof(*icmp)))
        {
            fprintf(stderr, "checksum mismatch from %s\n", inet_ntoa(ping.destination_address.sin_addr));
        }
        printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", len, ip_addr, icmp->un.echo.sequence, ip->ttl, time_spent);
        stats->max_round_trip = MAX(stats->max_round_trip, time_spent);
        stats->min_round_trip = MIN(stats->min_round_trip, time_spent);
        stats->sum_of_round_trip += time_spent;
        ping.packet_received++;
    }
    else if (icmp->type == ICMP_TIME_EXCEEDED)
    {
        printf("%d bytes from %s: Time to live exceeded\n", len, ip_addr);
    }
}

void receive_ping()
{
        int len_ping_addr = sizeof(ping.ping_address);
        int len_of_recv = recvfrom(ping.ping_fd, buffer_to_receive, \
        sizeof(buffer_to_receive), MSG_DONTWAIT, (struct sockaddr*)&ping.ping_address, \
        (socklen_t *)&len_ping_addr);
        if ( len_of_recv >= 0)
        {
            clock_gettime(CLOCK_MONOTONIC ,&tm_recv);            
            read_recv_buffer(buffer_to_receive, len_of_recv, compute_time_spent(&tm_send, &tm_recv));
        }
}

void print_intro()
{
    printf("PING %s (%s): 56 data bytes", ping.destination_host_name, ping.destination_ip_addr);
    if (verbose_opt)
        printf(", id %#x = %d", ping.ping_pckt.un.echo.id, ping.ping_pckt.un.echo.id);
    putchar('\n');
}

int main(int ac, char **av)
{
    fd_set fdset;
    struct timeval tv;
    int sel;

    init_args();
    argp_parse(&argp, ac, av, 0, 0, NULL);
    init_ping(&ping);
    init_stats();
    
    print_intro();
    signal(SIGINT, sig_handler);
    send_ping();
    while (cont)
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fdset);
        FD_SET(ping.ping_fd, &fdset);
        sel = select(ping.ping_fd + 1, &fdset, NULL, NULL, &tv);
        if (sel < 0)
        {
            if (errno != EINTR)
                errors("select", errno);
            continue;
        }
        else if (sel == 1)
        {
            if (FD_ISSET(ping.ping_fd, &fdset))
                receive_ping();
        }
        else
            send_ping();
    }
    printf("--- %s ping statistics ---\n", ping.destination_host_name);
    printf("%ld packets transmitted, %ld packets received, %d%% packet loss\n", ping.packet_emitted, ping.packet_received, (int)(((ping.packet_emitted - ping.packet_received) * 100)/ping.packet_emitted));
    free_arg(hostname);
    free_arg(stats);
    return 0;
}
