/*Standard Includes*/
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <fstream>

/*Custom Includes*/
#include "BaseModule.h"
#include "WinUDPRxModule.h"
#include "SessionProcModule.h"
#include "RouterModule.h"
#include "WAVAccumulator.h"
#include "WAVWriterModule.h"
#include "TimeToWAVModule.h"
#include "HPFModule.h"
#include "WinUDPTxModule.h"

/*External Libraries*/
#include "json.hpp"

int main()
{

	// ----------------
	// Config Variables
	// ----------------

	// UDP Rx
	std::string strUDPRxIP;
	std::string strUDPRxPort;

	// UDP Rx
	std::string strUDPTxIP;
	std::string strUDPTxPort;
	
	// Other
	float fAccumulationPeriod_sec;
	std::string strRecordingFilePath;

	try
	{	
		// Reading and parsing JSON config
		std::ifstream file("./Config.json");
		std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		nlohmann::json jsonConfig = nlohmann::json::parse(jsonString);

		// Updating config variables 
		strUDPRxIP = jsonConfig["Config"]["Network"]["UDPRx"]["IP"];
		strUDPRxPort = jsonConfig["Config"]["Network"]["UDPRx"]["Port"];
		strUDPTxIP = jsonConfig["Config"]["Network"]["UDPTx"]["IP"];
		strUDPTxPort = jsonConfig["Config"]["Network"]["UDPTx"]["Port"];
		strRecordingFilePath = jsonConfig["Config"]["FileSystem"]["RecordingPath"];
		fAccumulationPeriod_sec = jsonConfig["Config"]["RecordingParameters"]["RecordingPeriod"];
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		throw;
	}
	
	// ------------
	// Construction
	// ------------

	// Start of Processing Chain
    auto pUDPRXModule = std::make_shared<WinUDPRxModule>(strUDPRxIP, strUDPRxPort, 100, 512);
	auto pWAVSessionProcModule = std::make_shared<SessionProcModule>(100);	
	auto pSessionChunkRouter = std::make_shared<RouterModule>(100);
	
	// WAV Processing Chain
	auto pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(fAccumulationPeriod_sec,100);
	//auto pWAVHPFModule = std::make_shared<HPFModule>(100,10,8000,5); // Filter blocking to low atm - need to calculate actual values
	auto pTimeToWAVModule = std::make_shared<TimeToWAVModule>(100);
	auto pWAVWriterModule = std::make_shared<WAVWriterModule>(strRecordingFilePath, 100);

	// UDP Streaming 
	auto pUDPTXModule = std::make_shared<WinUDPTxModule>(strUDPTxIP, strUDPTxPort, 100, 1024);

	// ------------
	// Connection
	// ------------

	// Connection pipeline modules
	pUDPRXModule->SetNextModule(pWAVSessionProcModule);
	pWAVSessionProcModule->SetNextModule(pSessionChunkRouter);
	pSessionChunkRouter->SetNextModule(nullptr); // Note: this module needs registered outputs not set outputs as it is a one to many
	
	// Registering outputs
	pSessionChunkRouter->RegisterOutputModule(pUDPTXModule, ChunkType::TimeChunk);
	pSessionChunkRouter->RegisterOutputModule(pTimeToWAVModule, ChunkType::TimeChunk);

	// WAV Chain connections
	pTimeToWAVModule->SetNextModule(pWAVAccumulatorModule);
	pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);
	//pWAVHPFModule->SetNextModule(pWAVWriterModule);
	pWAVWriterModule->SetNextModule(nullptr); // Note: This is a termination module so has no next module
	
	// Streaming connections
	pUDPTXModule->SetNextModule(nullptr);

	// ------------
	// Start-Up
	// ------------

	// Starting chain from its end to start
	pWAVWriterModule->StartProcessing();
	//pUDPTXModule->StartProcessing();
	//pWAVHPFModule->StartProcessing();
	pTimeToWAVModule->StartProcessing();
	pWAVAccumulatorModule->StartProcessing();
	pSessionChunkRouter->StartProcessing();
	pWAVSessionProcModule->StartProcessing();
	pUDPRXModule->StartProcessing();
	
	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
