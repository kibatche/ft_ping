#ifndef UTILS_H
# define  UTILS_H

# include <time.h>

unsigned long int check_ttl_value(const char *ttl);
unsigned short checksum(unsigned short *addr, int len);
double mysqrt(double nb);
double myabs(double nb);
double compute_time_spent(struct timespec *start, struct timespec *end, struct timespec *to_sleep);
void free_arg(void *arg);

#endif