#ifndef UTILS_H
# define  UTILS_H

# include <time.h>

unsigned long int check_ttl_value(const char *ttl);
unsigned short checksum(unsigned short *addr, int len);
double compute_time_spent(struct timespec *start, struct timespec *end);
void free_arg(void *arg);

#endif