#ifndef HEADER_H
#define HEADER_H 

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>


inline int LOG_LEVEL = 0;
#define TRACE   if (LOG_LEVEL > 5) { std::cout << "TRACE: "
#define DEBUG   if (LOG_LEVEL > 4) { std::cout << "DEBUG: "
#define INFO    if (LOG_LEVEL > 3) { std::cout << "INFO: "
#define WARNING if (LOG_LEVEL > 2) { std::cout << "WARNING: "
#define ERROR   if (LOG_LEVEL > 1) { std::cout << "ERROR: "
#define FATAL   if (LOG_LEVEL > 0) { std::cout << "FATAL: "
#define ENDL  " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; }

#define BUFFER_LEN 64
#define MAX_REQUESTS 30
#define START_TTL 2
#define RESPONSE_TIMEOUT 15
#define SELECT_TIMEOUT 5

uint16_t checksum(unsigned short *buffer, int size);

#endif
