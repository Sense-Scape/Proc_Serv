#include "WinUDPRxModule.h"

WinUDPRxModule::WinUDPRxModule(std::string sIPAddress, std::string sUDPPort, unsigned uMaxInputBufferSize): BaseModule(uMaxInputBufferSize),
																											m_sIPAddress(sIPAddress),
																											m_sUDPPort(sUDPPort)
{

}

WinUDPRxModule::~WinUDPRxModule()
{

}