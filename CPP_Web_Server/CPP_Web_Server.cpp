// CPP_Web_App.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "BaseModule.h"
#include "WinUDPRxModule.h"

int main()
{
    auto pUDPRXModule = std::make_shared<WinUDPRxModule>("192.168.1.35", "8080", 2, 512);
    
	pUDPRXModule->StartProcessing();

	while (1)
	{

	}
}
