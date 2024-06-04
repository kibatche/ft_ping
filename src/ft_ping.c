# include "ft_ping.h"


char *hostname;
int verbose_opt;
int quiet_opt;
int ttl_opt;
int count_opt;
int timeout_opt;
int usage_opt;

const char *argp_program_version = "ft_ping 1.0";
static char doc[] = "A program that reimplements ping.";
static char args_doc[] = "ADDRESS";

ping_infos *ping;
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
            hostname = arg;
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

int sig_handler(int sig)
{
//TODO
}

ping_infos *init_ping(int packet_type, int pid, struct arguments *args)
{
    ping_infos *ping_tmp;
    int fd;

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1)
    {
        fprintf(stderr, "ping: %s\n", strerror(errno));
    }
    ping_tmp = malloc(sizeof(ping_infos));
    if (ping_tmp == NULL)
    {
        close(fd);
        return NULL;
    }
    bzero(ping_tmp, sizeof(ping_infos));
    ping_tmp->ping_fd = fd;
    ping_tmp->ping_type = packet_type;
    ping_tmp->ping_id = htons(pid);
    ping_tmp->ping_datalen = 64;
    ping_tmp->packet_emitted = 0;
    ping_tmp->packet_received = 0;
    ping_tmp->packet_duplicated = 0;
    ping_tmp->ping_interval = 1000;
    gettimeofday(&ping_tmp->ping_start_time, NULL);
    return ping_tmp;
}


int main(int ac, char **av)
{
    struct arguments args;
    args.verbose = 0;
    args.usage = 0;
    args.address = NULL;
    argp_parse(&argp, ac, av, 0, 0, &args);
    ping = init_ping(ICMP_ECHO, getpid(), &args);
    if (ping == NULL)
        exit(EXIT_FAILURE);
    
    return 0;
}
