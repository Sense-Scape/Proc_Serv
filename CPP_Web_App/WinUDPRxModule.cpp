#include "WinUDPRxModule.h"

WinUDPRxModule::WinUDPRxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize): BaseModule(uMaxInputBufferSize),
																											m_sIPAddress(sIPAddress),
																											m_sUDPPort(sUDPPort),
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