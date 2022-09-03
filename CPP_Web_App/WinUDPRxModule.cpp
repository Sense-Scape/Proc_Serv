#include "WinUDPRxModule.h"

WinUDPRxModule::WinUDPRxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize, unsigned uBufferLen = 512): BaseModule(uMaxInputBufferSize),
																																	m_sIPAddress(sIPAddress),
																																	m_sUDPPort(sUDPPort),
																																	m_uBufferLen(uBufferLen),
																																	m_WinSocket(),
																																	m_WSA(),
																																	m_SocketStruct()
{
	ConnectUDPSocket();

}

WinUDPRxModule::~WinUDPRxModule()
{

}

void WinUDPRxModule::ConnectUDPSocket()
{
	WSADATA wsaData;

	//// Initialize Winsock
	auto iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		
	}

	// Configuring Web Security Appliance
	if (WSAStartup(MAKEWORD(2, 2), &m_WSA) != 0)
	{
		std::cout << "Windows UDP socket WSA Error. Error Code : " + std::to_string(WSAGetLastError()) + "\n";
		throw;
	}

	// Configuring protocol to UDP
	if ((m_WinSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		std::cout << "Windows UDP socket WSA Error. INVALID_SOCKET \n";
		throw;
	}

	//Prepare the sockaddr_in structure
	m_SocketStruct.sin_family = AF_INET;
	m_SocketStruct.sin_addr.s_addr = INADDR_ANY;
	m_SocketStruct.sin_port = htons(std::stoi(m_sUDPPort));

	if (bind(m_WinSocket, (struct sockaddr*)&m_SocketStruct, sizeof(m_SocketStruct)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	std::cout << "Socket binding complete: IP: " + m_sIPAddress + " Port: " + m_sUDPPort + " \n";
}

void WinUDPRxModule::Process()
{
		while (true)
		{
			char buf[512];
			unsigned recv_len = 0;

			int slen = sizeof(m_SocketStruct);
			int s = sizeof(m_SocketStruct);

			//clear the buffer by filling null, it might have previously received data
			memset(buf, '\0', 512);

			////try to receive some data, this is a blocking 
			std::cout << "here" << std::endl;
			if ((recv_len = recvfrom(m_WinSocket, buf, 512, 0, (struct sockaddr*)&m_SocketStruct, &slen)) == SOCKET_ERROR)
			{
				printf("recvfrom() failed with error code : %d", WSAGetLastError());
			}
			std::cout << "here" << std::endl;
			std::cout << recv_len << std::endl;
			//printf("Data: %s\n", buf);
		}
}

