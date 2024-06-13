#ifndef MACROS_H
# define MACROS_H

# define TTL_ARG  1000

# define MIN_TTL 1
# define MAX_TTL 255

# define RECV_TIMEOUT 1
# define DATALEN 56//64 - sizeof(icmp_hdr)

# define MIN(x, y) (x < y ? x : y)
# define MAX(x, y) (x > y ? x : y)
#endif