#ifndef WIN_UDP_RX_MODULE
#define WIN_UDP_RX_MODULE

/*Standard Includes*/
#include <string>
#include <winsock2.h>

/*Custom Includes*/
#include "BaseModule.h"


/**
 * @brief Windows UDP Receiving Module to receive data from a UDP port
 * 
 * Class input format:
 * Class output format:
 *
 */
class WinUDPRxModule :
	public BaseModule
{
private:
	std::string m_sIPAddress;	///< string format of host IP address
	std::string m_sUDPPort;		///< string format of port to listen on
    unsigned m_uBufferLen;      ///< Maxmimum UDP buffer length
    SOCKET m_Socket;            ///< Windows socket
    WSADATA m_WSA;              ///< Web Security Appliance for Windows socket

    /**
     * @brief Creates the classes windows socket
     *
     */
    void MakeUDPSocket();

protected:

public:

    /**
     * @brief WinUDPRxModule constructor
     *
     * @param sIPAddress string format of host IP address
     * @param sUDPPort string format of port to listen on
     *
     */
	WinUDPRxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize);
    ~WinUDPRxModule();
};

#endif
