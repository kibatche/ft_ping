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
    unsigned short chksum = 0;

    while(nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if( nleft == 1) {
        *(unsigned char *)(&chksum) = *(unsigned char *)w;
        sum += chksum;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    chksum = ~sum;
    return chksum;
}

double compute_time_spent(struct timespec *start, struct timespec *end, struct timespec *to_sleep)
{
    long time_spent_sec = end->tv_sec - start->tv_sec;
    long time_spent_nsec = end->tv_nsec - start->tv_nsec;
    
    
    to_sleep->tv_sec = 0;
    to_sleep->tv_nsec = 1000000000;
    /*Si les nanosecondes de recv sont plus grandes que celles de l'envoi*/
    if (time_spent_nsec < 0)
    {
        time_spent_sec -= 1;
        time_spent_nsec += 1000000000;
    }
    if (time_spent_sec >= 1 || time_spent_nsec >= 1000000000)
        to_sleep->tv_nsec = 0;
    else
        to_sleep->tv_nsec -= time_spent_nsec;
    return time_spent_sec * 1000.0 + time_spent_nsec / 1000000.0;
}

double myabs(double nb)
{
    return (nb < 0 ? nb * -1 : nb);
}

double
nsqrt (double a, double prec)
{
  double x0, x1;

  if (a < 0)
    return 0;
  if (a < prec)
    return 0;
  x1 = a / 2;
  do
    {
      x0 = x1;
      x1 = (x0 + a / x0) / 2;
    }
  while (myabs (x1 - x0) > prec);

  return x1;
}

double mysqrt(double nb)
{
    double tol = 0.0005;//tolerance pour la précision, c'est la même que pour le vrai ping inetutils

    double guess = nb / 2.0;
    while (myabs(guess * guess - nb) > tol) {
        guess = (guess + nb / guess) / 2.0;
    }
    printf("Me : %.3f Inetutils : %.3f\n", guess, nsqrt(nb, 0.0005));
    return guess;
}