// CPP_Web_App.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "BaseModule.h"
#include "WinUDPRxModule.h"
#include "SessionProcModule.h"

int main()
{
	// Creating pipeline modules
    auto pUDPRXModule = std::make_shared<WinUDPRxModule>("192.168.0.105", "8080", 2, 512);
	auto pWAVSessionProcModule = std::make_shared<SessionProcModule>(2);

	// Connection pipeline modules
	pUDPRXModule->SetNextModule(pWAVSessionProcModule);
	pWAVSessionProcModule->SetNextModule(nullptr);

	// Starting pipeline modules
	pUDPRXModule->StartProcessing();
	pWAVSessionProcModule->StartProcessing();

	while (1)
	{

	}
}
