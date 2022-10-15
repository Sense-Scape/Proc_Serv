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

int main()
{
	// General Config
	std::string sAudioFilePath = "D:/Recordings/";

	// Creating pipeline modules
    auto pUDPRXModule = std::make_shared<WinUDPRxModule>("192.168.0.105", "8080", 100, 512);
	auto pWAVSessionProcModule = std::make_shared<SessionProcModule>(100);
	auto pSessionChunkRouter = std::make_shared<RouterModule>(100);
	auto pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(2, 100);
	auto pWAVWriterModule = std::make_shared<WAVWriterModule>(sAudioFilePath, 100);

	// Connection pipeline modules
	pUDPRXModule->SetNextModule(pWAVSessionProcModule);
	pWAVSessionProcModule->SetNextModule(pSessionChunkRouter);
	pSessionChunkRouter->SetNextModule(nullptr); // Note: this module needs registered outputs not set outputs as it is a one to many
	pWAVWriterModule->SetNextModule(nullptr); // Note: This is a termination module so has no next module

	pSessionChunkRouter->RegisterOutputModule(pWAVAccumulatorModule, ChunkType::WAVChunk);
	pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);

	// Starting pipeline modules
	pUDPRXModule->StartProcessing();
	pWAVSessionProcModule->StartProcessing();
	pSessionChunkRouter->StartProcessing();
	pWAVAccumulatorModule->StartProcessing();
	pWAVWriterModule->StartProcessing();

	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
