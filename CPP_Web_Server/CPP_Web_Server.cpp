/*Standard Includes*/
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>

/*Custom Includes*/
#include "Cpp_Module_Types/Cpp_Module_Types/BaseModule.h"
#include "Cpp_Module_Types/Cpp_Module_Types/WinUDPRxModule.h"
#include "Cpp_Module_Types/Cpp_Module_Types/SessionProcModule.h"

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
