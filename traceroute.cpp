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
	std::string destIP = "";

	// ********************************************************************
	// * Process the command line arguments.
	// ********************************************************************
	int opt = 0;
	while ((opt = getopt(argc, argv, "t:d:")) != -1) {
		switch (opt) {
		case 't':
			destIP = optarg;
			break;
		case 'd':
			LOG_LEVEL = atoi(optarg);
			break;
		case ':':
		case '?':
		default:
			std::cerr << "useage: " << argv[0] << " -t [target ip] -d [debug level]" << std::endl;
			exit(-1);
		}
	}

	if (destIP == "") {
		std::cerr << "Invalid command line arguments: -t [target ip] is required" << std::endl;
        exit(-1);
	}
	uint32_t destInetAddr = inet_addr(destIP.c_str());
	if (destInetAddr == -1) {
		std::cerr << "Invalid IP address " << destIP << std::endl;
		exit(-1);
	}
	struct sockaddr_in destAddr;
	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.s_addr = destInetAddr;

	// ********************************************************************
	// * Initialize sockets and buffers for sending/receiving.
	// ********************************************************************
	int sendFd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (sendFd == -1) {
		FATAL << "Error creating send socket: " << std::strerror(errno) << ENDL;
		exit(-1);
	}
	int recvFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (recvFd == -1) {
		FATAL << "Error creating receive socket: " << std::strerror(errno) << ENDL;
		close(sendFd);
		exit(-1);
	}

	uint8_t *sendBuffer = new uint8_t[BUFFER_LEN];
	uint8_t *recvBuffer = new uint8_t[BUFFER_LEN];

	// ********************************************************************
	// * Create the ICMP echo request packet.
	// ********************************************************************
	memset(sendBuffer, 'A', BUFFER_LEN); // Fill payload
	
	struct iphdr *ipHeader = (struct iphdr *)sendBuffer;
	memset(sendBuffer, 0, sizeof(struct iphdr));
	ipHeader->ihl = 5; // Header length / 4
	ipHeader->version = 4;
	ipHeader->tos = 0;
	ipHeader->protocol = IPPROTO_ICMP;
	ipHeader->daddr = destAddr.sin_addr.s_addr;

	uint8_t *icmpPacket = sendBuffer + sizeof(struct iphdr);
	struct icmphdr *icmpHeader = (struct icmphdr *)(icmpPacket);
	memset(icmpPacket, 0, sizeof(struct icmphdr));
	icmpHeader->type = ICMP_ECHO;
	icmpHeader->code = 0;

	// ********************************************************************
	// * Send echo requests with increasing TTL.
	// ********************************************************************
	unsigned int ttl = START_TTL;
	for (unsigned int req = 0; req < MAX_REQUESTS; req++) {
		ipHeader->ttl = ttl;
		icmpHeader->checksum = 0;
		uint16_t calculatedChecksum = checksum((unsigned short *)icmpPacket, BUFFER_LEN - sizeof(struct iphdr));
		icmpHeader->checksum = calculatedChecksum;

		DEBUG << "Sending packet with TTL = " << ttl << ", checksum = " << icmpHeader->checksum << ENDL;
		int bytesSent = 0;
		if ((bytesSent = sendto(sendFd, sendBuffer, BUFFER_LEN, 0, (struct sockaddr *)&destAddr, sizeof(destAddr))) == -1) {
			ERROR << "sendto error: " << strerror(errno) << ENDL;
			break;
		}
		DEBUG << "bytesSent = " << bytesSent << ENDL;

		// ********************************************************************
		// * Non-blocking read.
		// ********************************************************************
		bool ready = false;
		int selectReturned = 0;
		fd_set readFdSet;
		struct timeval timeout;
		unsigned int totalTimeout = 0;
		while (!ready && totalTimeout < RESPONSE_TIMEOUT) {
			totalTimeout += SELECT_TIMEOUT;
			timeout.tv_sec = SELECT_TIMEOUT;
			timeout.tv_usec = 0;

			// Set the bit corresponding to recvFd
			FD_ZERO(&readFdSet);
			FD_SET(recvFd, &readFdSet);

			DEBUG << "Waiting up to " << SELECT_TIMEOUT << " seconds for data to become available" << ENDL;
			if ((selectReturned = select(recvFd + 1, &readFdSet, NULL, NULL, &timeout)) < 0) {
				FATAL << "select error: " << std::strerror(errno) << ENDL;
				break;
			}
			DEBUG << "selectReturned = " << selectReturned << ENDL;

			// Confirm that select returned because the fd is set, not because of a timeout
			if (FD_ISSET(recvFd, &readFdSet)) {
				DEBUG << "Bit #" << recvFd << " is set" << ENDL;
				ready = true;
			}
		}

		if (ready) {
			struct sockaddr_in recvAddr;
			socklen_t recvAddrLen = sizeof(recvAddr);
			if (recvfrom(recvFd, recvBuffer, BUFFER_LEN, 0, (struct sockaddr *)&recvAddr, &recvAddrLen) < 0) {
				ERROR << "recvfrom error: " << std::strerror(errno) << ENDL;
				continue;
			}

			struct iphdr *ipResp = (struct iphdr *)recvBuffer;
			if (ipResp->protocol == IPPROTO_ICMP) {
				struct icmphdr *icmpResp = (struct icmphdr *)(recvBuffer + ipResp->ihl * 4);
				if (icmpResp->type == ICMP_ECHOREPLY) {
					DEBUG << "ICMP echo response received" << ENDL;
					char respIp[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &recvAddr.sin_addr, respIp, INET_ADDRSTRLEN);
					std::cout << respIp << std::endl;
					break;
				}
				else if (icmpResp->type == ICMP_TIME_EXCEEDED && icmpResp->code == ICMP_EXC_TTL) {
					DEBUG << "ICMP TTL exceeded response received" << ENDL;
					char respIp[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &recvAddr.sin_addr, respIp, INET_ADDRSTRLEN);
					std::cout << respIp << std::endl;
				}
				else {
					WARNING << "Other ICMP type/subtype received" << ENDL;
				}
			}
			else {
				WARNING << "Non-ICMP protocol received" << ENDL;
			}
		}
		else {
			std::cout << "No response with TTL of " << ttl << std::endl;
		}

		ttl++;
	}

	// ********************************************************************
	// * Cleanup.
	// ********************************************************************
	close(sendFd);
	close(recvFd);
	delete[] sendBuffer;
	delete[] recvBuffer;

	return 0;
}
