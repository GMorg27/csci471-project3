#include "traceroute.h"

// ****************************************************************************
// * Compute the Internet Checksum over an arbitrary buffer.
// * (written with the help of ChatGPT 3.5)
// ****************************************************************************
uint16_t checksum(unsigned short *buffer, int size) {
    unsigned long sum = 0;
    while (size > 1) {
        sum += *buffer++;
        size -= 2;
    }
    if (size == 1) {
        sum += *(unsigned char *) buffer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short) (~sum);
}


int main (int argc, char *argv[]) {
	std::string destIP;

	// ********************************************************************
	// * Process the command line arguments
	// ********************************************************************
	int opt = 0;
	while ((opt = getopt(argc,argv,"t:d:")) != -1) {
		switch (opt) {
		case 't':
			destIP = optarg;
			break;
		case 'd':
			LOG_LEVEL = atoi(optarg);;
			break;
		case ':':
		case '?':
		default:
			std::cout << "useage: " << argv[0] << " -t [target ip] -d [Debug Level]" << std::endl;
			exit(-1);
		}
	}
}
