#include <argp.h>

/*
FOr parsing arguments
*/
const char *argp_program_version = "ft_ping 1.0";
static char doc[] = "A program that reimplements ping.";
static char args_doc[] = "ADDRESS";

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

int main(int ac, char **av)
{
    struct arguments args;
    args.verbose = 0;
    args.usage = 0;
    argp_parse(&argp, ac, av, 0, 0, &args);
    return 0;
}
