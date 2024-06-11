#include "ft_ping.h"

void free_arg(void *arg)
{
    if (arg != NULL)
    {
        free(arg);
        arg = NULL;
    }
}

unsigned long int check_ttl_value(const char *ttl)
{
    char *p;
    unsigned long int n;

    n = strtoul(ttl, &p, 0);
    if (*p)
    {
        fprintf(stderr, "invalid value (`%s' near `%s')", ttl, p);
        errors(NULL, 0);
    }
    if (n < 1)
    {
        fprintf(stderr, "option value too small: %s\n", ttl);
        errors(NULL, 0);
    }
    if (n > 255)
    {
        fprintf(stderr, "option value too big: %s\n", ttl);
        errors(NULL, 0);
    }
    return n;
}

unsigned short checksum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

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

double compute_time_spent(struct timespec *start, struct timespec *end)
{
    long time_spent_sec = end->tv_sec - start->tv_sec;
    long time_spent_nsec = end->tv_nsec - start->tv_nsec;
    
    /*Si les nanosecondes de recv sont plus grandes que celles de l'envoi*/
    if (time_spent_nsec < 0)
    {
        time_spent_sec -= 1;
        time_spent_nsec += 1000000000;
    }
    return time_spent_sec * 1000.0 + time_spent_nsec / 1000000.0;
}