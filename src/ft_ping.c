# include "ft_ping.h"


char *hostname;
char packet_to_send[4096];
char buffer_to_receive[4096];
int verbose_opt;
int quiet_opt;
int ttl_opt;
int count_opt;
int timeout_opt;
int usage_opt;

const char *argp_program_version = "ft_ping 1.0";
static char doc[] = "A program that reimplements ping.";
static char args_doc[] = "ADDRESS";

ping_infos ping;
// ping_stats *ping_stats;

static struct argp_option options[] = {
  {"verbose",  'v', 0, 0, "Produce verbose output" },
  {"quiet", 'q', 0, 0, "Produce a quiet output"},
  {"ttl", TTL_ARG, "N", 0, "specify N as time-to-live"},
  {"count", 'c', "NUMBER", 0, "stop after sending NUMBER packets"},
  {"timeout", 'w', "N", 0, "stop after N seconds"},
  {"usage", '?', 0, 0, "Don't produce any output" },
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
        case 'q':
            quiet_opt = 1;
            break;
        case 'w':
            timeout_opt = 1; /*A IMPLEMENTER*/
            break;
        case 'c':
            count_opt = 1;  /*A IMPLEMENTER*/
            break;
        case TTL_ARG:
            ttl_opt = 1; /*A IMPLEMENTER*/
        case '?' :
            usage_opt = 1;
            break;
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

void free_arg(void *arg)
{
    if (arg != NULL)
    {
        free(arg);
        arg = NULL;
    }
}

int sig_handler(int sig)
{
//TODO
}

void init_ping(ping_infos *ping)
{
    
    struct protoent *proto;
    /*hostent est une structure qui nous permettra
    de retrouver une adresse ip à partir d'un nom 
    de domaine*/
    struct hostent *host_entity = NULL;
    int fd;

    host_entity = gethostbyname(hostname);
    if (host_entity == NULL)
    {
        fprintf(stderr, "ping: unkown host\n");
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
    /*COnnaître le numéro du protocole (en l'occurrence toujours icmp)*/
    proto = getprotobyname("icmp");
    if (proto == NULL)
    {
        fprintf(stderr, "ping: protocol icmp unknown by getprotobyname\n");
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
    /*création de la raw socket avec le protocole icmp*/
    fd = socket(AF_INET, SOCK_RAW, proto->p_proto);
    if (fd == -1)
    {
        fprintf(stderr, "ping: %s\n", strerror(errno));
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
    bzero(ping, sizeof(ping_infos));
    ping->ping_fd = fd;
    ping->ping_pckt.un.echo.id = htons(getpid());
    ping->ping_pckt.un.echo.sequence = 0;
    ping->ping_pckt.checksum = 0;
    ping->ping_pckt.code = 0;
    ping->ping_pckt.type = ICMP_ECHO;
    ping->ping_datalen = 64;/*ICMP_DATA_LEN (56) for the icmp req + 8 for ip header*/
    ping->destination_address.sin_family = AF_INET;
    ping->destination_address.sin_addr.s_addr = inet_addr(host_entity->h_name);
    ping->packet_emitted = 0;
    ping->packet_received = 0;
    ping->packet_duplicated = 0;
    ping->ping_interval = 1000;
    gettimeofday(&ping->ping_start_time, NULL);

}

void init_args()
{
    hostname = NULL;
    verbose_opt = 0;
    quiet_opt = 0;
    ttl_opt = 0;
    count_opt = 0;
    timeout_opt = 0;
    usage_opt = 0;
}

unsigned short cal_chksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    /* 
     * The checksum is the 16-bit ones's complement of the one's
     * complement sum of the ICMP message starting with the ICMP Type.
     * For computing the checksum , the checksum field should be zero. 
     */

    while(nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if( nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

void send_ping()
{
    ping.ping_pckt.un.echo.id++;
    ping.packet_emitted++;
    ping.ping_pckt.checksum = cal_chksum((unsigned short *)&ping.ping_pckt, 64);
    if (sendto(ping.ping_fd, &ping.ping_pckt,sizeof(ping.ping_pckt), 0, (struct sockaddr *)&ping.destination_address, sizeof(ping.destination_address)) < 0)
    {
        fprintf(stderr, "Error with sendto : %s", strerror(errno));
        ping.packet_emitted--;
    }
}

void receive_ping()
{
    while (ping.packet_received < ping.packet_emitted)
    {
        ssize_t n = recvfrom(ping.ping_fd, buffer_to_receive, \
        sizeof(buffer_to_receive), 0, (struct sockaddr*)&ping.ping_address, \
        (socklen_t *)sizeof(ping.ping_address));
        if (n < 0)
        {
            if (errno = EINTR)
                continue;
            fprintf(stderr, "recvfrom error");
            continue;
        }
        printf("OK\n");
    }
}

int main(int ac, char **av)
{
    init_args();
    argp_parse(&argp, ac, av, 0, 0, NULL);
    init_ping(&ping);
    int size = 1024 * 5;
    setsockopt(ping.ping_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    send_ping();
    receive_ping();
    return 0;
}
