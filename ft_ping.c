# include "ft_ping.h"


char *hostname;
char buffer_to_send[4096];
char buffer_to_receive[4096];
int verbose_opt;
int quiet_opt;
int ttl_opt;
int count_opt;
int timeout_opt;
int usage_opt;

const char *argp_program_version = "ft_ping 1.0";
static char doc[] = "A program that reimplements the ping program from inetutils.";
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
    /*hostent est une structure qui nous permettra
    de retrouver une adresse ip à partir d'un nom 
    de domaine*/
    struct hostent *host_entity = NULL;
    struct timeval timeout;      
    timeout.tv_sec = RECV_TIMEOUT;
    timeout.tv_usec = 0;
    char *ip_temp;
    int fd;

    /*création de la raw socket avec le protocole icmp*/
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1)
    {
        fprintf(stderr, "ft_ping: %s\n", strerror(errno));
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
    {
        fprintf(stderr, "ft_ping: setsockopt failed.\n");
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
    host_entity = gethostbyname(hostname);    
    if (host_entity == NULL)
    {
        fprintf(stderr, "ft_ping: unknown host\n");
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
    ping->destination_host_name = hostname;
    ip_temp = inet_ntoa(*((struct in_addr*) host_entity->h_addr_list[0]));
    if (strlen(ip_temp) > 15)
    {
        fprintf(stderr, "ft_ping: bad ip len.\n");
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
    strcpy(ping->destination_ip_addr, ip_temp);
    ping->destination_ip_addr[strlen(ip_temp)] = 0;
    if (inet_aton(ip_temp, &ping->destination_address.sin_addr) == 0)
    {
        fprintf(stderr, "ft_ping: inet_aton failed. Bad ip notation ?\n");
        free_arg(hostname);
        exit(EXIT_FAILURE);
    }
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
    timeout_opt = 1;
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
    ping.packet_emitted++;
    ping.ping_pckt.checksum = cal_chksum((unsigned short *)&ping.ping_pckt, 64);
    memcpy(buffer_to_send, &ping.ping_pckt, sizeof(ping.ping_pckt));
    if (sendto(ping.ping_fd, buffer_to_send, sizeof(buffer_to_send), 0, (struct sockaddr *)&ping.destination_address, sizeof(ping.destination_address)) < 0)
    {
        fprintf(stderr, "Error with sendto : %s\n", strerror(errno));
        ping.packet_emitted--;
    }
    ping.ping_pckt.un.echo.sequence++;
}

void receive_ping()
{
    while (ping.packet_received < ping.packet_emitted)
    {
        printf("Dans receive ping & fd  %d\n", ping.ping_fd);
        int len_ping_addr = sizeof(ping.ping_address);
        if (recvfrom(ping.ping_fd, buffer_to_receive, \
        sizeof(buffer_to_receive), 0, (struct sockaddr*)&ping.ping_address, \
        (socklen_t *)&len_ping_addr) < 0)
        {
            int err = errno;
            if (err == EAGAIN || err == EWOULDBLOCK || err == EINPROGRESS)
            {
                printf("Timeout\n");
                continue;
            }
            continue;
        }
        else
        {
            printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%d\n", ping.ping_datalen - 8, ping.destination_ip_addr, ping.ping_pckt.un.echo.sequence, 116, 1);
            ping.packet_received++;
            sleep(1);
        }
    }
}

int main(int ac, char **av)
{
    init_args();
    argp_parse(&argp, ac, av, 0, 0, NULL);
    init_ping(&ping);
    while (1)
    {
        send_ping();
        receive_ping();
    }
    return 0;
}
