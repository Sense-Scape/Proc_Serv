// CPP_Web_App.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "BaseModule.h"
#include "WinUDPRxModule.h"

int main()
{
    auto a = std::make_shared<WinUDPRxModule>("192.168.1.35", "8080", 2, 512);
    
	a->StartProcessing();

	while (1)
	{

	}
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
