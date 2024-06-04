# include "ft_ping.h"

const char *argp_program_version = "ft_ping 1.0";
static char doc[] = "A program that reimplements ping.";
static char args_doc[] = "ADDRESS";

ping_infos *ping;
// ping_stats *ping_stats;

static struct argp_option options[] = {
  {"verbose",  'v', 0,      0,  "Produce verbose output" },
  {"usage",    '?', 0,      0,  "Don't produce any output" },
  { 0 }
};

struct arguments
{
    char *address;
    int verbose;
    int usage;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *args = state->input;

    switch(key)
    {
        case 'v':
            args->verbose = 1;
            break;
        case '?' :
            args->usage = 1;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num >= 1)
                argp_usage(state);
            args->address = arg;
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

ping_infos *init_ping(int packet_type, int pid)
{
    ping_infos *ping_tmp;
    int fd;

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1)
    {
        int err = errno;
        fprintf(stderr, "ping: %s\n", strerror(err));
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
}

int main(int ac, char **av)
{
    struct arguments args;
    args.verbose = 0;
    args.usage = 0;
    argp_parse(&argp, ac, av, 0, 0, &args);

    return 0;
}
