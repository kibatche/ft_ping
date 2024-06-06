#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Execution
int main()
{
    char host_name[1024];
    char *IP_address;

    // // Retrieving hostname using gethostname function
    // int temp= gethostname(host_name, sizeof(host_name));

    // // To check whether the hostname is copied to host_name or not
    // if(temp==-1)
    // {
    //     perror("gethostname");
    //     exit(1);
    // }

    // Retrieving host information using gethostbyname function
    struct hostent *entry = gethostbyname("google.com");

    // Check whether the entry is not NULL or not
    if(!entry)
    {
        perror("inet_ntoa");
        exit(1);
    }

    // To convert an IP address into an ASCII string in dotted decimal format.
    IP_address = inet_ntoa(*((struct in_addr*) entry->h_addr_list[0]));

    printf("The name of the Host is:  %s\n", host_name);
    printf("The IP Address of the host is:  %s", IP_address);

    return 0;
}