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
#include "BaseModule.h"
#include "WinUDPRxModule.h"
#include "SessionProcModule.h"
#include "RouterModule.h"
#include "WAVAccumulator.h"
#include "WAVWriterModule.h"
#include "TimeToWAVModule.h"
#include "HPFModule.h"
#include "WinUDPTxModule.h"

int main()
{
	// General Config
	std::string sAudioFilePath = "D:/Recordings_Pond/";
	
	// ------------
	// Construction
	// ------------

	// Start of Processing Chain
    auto pUDPRXModule = std::make_shared<WinUDPRxModule>("192.168.208.165", "8080", 100, 512);
	auto pWAVSessionProcModule = std::make_shared<SessionProcModule>(100);
	auto pSessionChunkRouter = std::make_shared<RouterModule>(100);
	
	// WAV Processing Chain
	auto pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(10,100);
	//auto pWAVHPFModule = std::make_shared<HPFModule>(100,10,8000,5); // Filter bloicking to low atm - need to calculate actual values
	auto pTimeToWAVModule = std::make_shared<TimeToWAVModule>(100);
	auto pWAVWriterModule = std::make_shared<WAVWriterModule>(sAudioFilePath, 100);

	// UDP Streaming 
	auto pUDPTXModule = std::make_shared<WinUDPTxModule>("127.0.0.1", "8081", 100, 512);

	// ------------
	// Connection
	// ------------

	// Connection pipeline modules
	pUDPRXModule->SetNextModule(pWAVSessionProcModule);
	pWAVSessionProcModule->SetNextModule(pSessionChunkRouter);
	pSessionChunkRouter->SetNextModule(nullptr); // Note: this module needs registered outputs not set outputs as it is a one to many
	
	// Registering outputs
	//pSessionChunkRouter->RegisterOutputModule(pUDPTXModule, ChunkType::TimeChunk);
	pSessionChunkRouter->RegisterOutputModule(pTimeToWAVModule, ChunkType::TimeChunk);

	// WAV Chain connections
	pTimeToWAVModule->SetNextModule(pWAVAccumulatorModule);
	pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);
	//pWAVHPFModule->SetNextModule(pWAVWriterModule);
	pWAVWriterModule->SetNextModule(nullptr); // Note: This is a termination module so has no next module
	
	// Streaming connections
	//pUDPTXModule->SetNextModule(nullptr);

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
