/* Standard Includes */
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <fstream>

/* Custom Includes */
#include "BaseModule.h"
#include "WinUDPRxModule.h"
#include "SessionProcModule.h"
#include "RouterModule.h"
#include "WAVAccumulator.h"
#include "WAVWriterModule.h"
#include "TimeToWAVModule.h"
#include "HPFModule.h"
#include "WinUDPTxModule.h"
#include "WinTCPRxModule.h"
#include "WinTCPTxModule.h"
#include "ToJSONModule.h"
#include "WinTCPTxModule.h"
#include "ChunkToBytesModule.h"

/* External Libraries */
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include "json.hpp"

int main()
{

	// ----------------
	// Config Variables
	// ----------------

	// TCP Rx
	std::string strTCPRxIP;
	std::string strTCPRxPort;

	// UDP Tx
	std::string strTCPTxIP = "127.0.0.1";
	std::string strTCPTxPort = "10005";

	// Other
	float fAccumulationPeriod_sec;
	double dContinuityThresholdFactor;
	std::string strRecordingFilePath;

	// Reading and parsing JSON config
	std::ifstream file("./Config.json");
	std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	nlohmann::json jsonConfig = nlohmann::json::parse(jsonString);

	try // To load in config file
	{
		// Updating config variables 
		// TCP Module Config
		strTCPRxIP = jsonConfig["PipelineConfig"]["TCPRxModule"]["IP"];
		strTCPRxPort = jsonConfig["PipelineConfig"]["TCPRxModule"]["Port"];
		// WAV Accumulator config
		fAccumulationPeriod_sec = jsonConfig["PipelineConfig"]["WAVAccumulatorModule"]["RecordingPeriod"];
		dContinuityThresholdFactor = jsonConfig["PipelineConfig"]["WAVAccumulatorModule"]["ContinuityThresholdFactor"];
		// WAV Writer condig
		strRecordingFilePath = jsonConfig["PipelineConfig"]["WAVWriterModule"]["RecordingPath"];

	}
	catch (const std::exception& e)
	{
		PLOG_ERROR << e.what();
		throw;
	}

	try // To configure logging
	{
		auto eLogLevel = plog::debug;
		std::string strRequestLogLevel = jsonConfig["LoggingConfig"]["LoggingLevel"];
		std::transform(strRequestLogLevel.begin(), strRequestLogLevel.end(), strRequestLogLevel.begin(), [](unsigned char c) {
			return std::toupper(c);
			});

		// Select log level
		if (strRequestLogLevel == "DEBUG")
			eLogLevel = plog::debug;
		else if (strRequestLogLevel == "INFO")
			eLogLevel = plog::info;
		else if (strRequestLogLevel == "WARNING")
			eLogLevel = plog::warning;
		if (strRequestLogLevel == "ERROR")
			eLogLevel = plog::error;
		else
			eLogLevel = plog::debug;

		// Get the time for the log file
		time_t lTime;
		time(&lTime);
		auto strTime = std::to_string((long long)lTime);
		std::string strLogFileName = "PL_Windows_Proc_Serv_" + strTime + ".txt";
		
		// And create loggers
		static plog::RollingFileAppender<plog::CsvFormatter> fileAppender(strLogFileName.c_str(), 50'000'000, 2); // Create the 1st appender.
		static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender; // Create the 2nd appender.
		plog::init(eLogLevel, &fileAppender).addAppender(&consoleAppender); // Initialize the logger with the both appenders.
	}
	catch (const std::exception& e)
	{
		PLOG_ERROR << e.what();
		throw;
	}

	// ------------
	// Construction
	// ------------

	// Start of Processing Chain
	auto pTCPRXModule = std::make_shared<WinTCPRxModule>(strTCPRxIP, strTCPRxPort, 100, 512);
	auto pWAVSessionProcModule = std::make_shared<SessionProcModule>(100);
	auto pSessionChunkRouter = std::make_shared<RouterModule>(100);

	// WAV Processing Chain
	auto pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(fAccumulationPeriod_sec, dContinuityThresholdFactor, 100);
	auto pTimeToWAVModule = std::make_shared<TimeToWAVModule>(100);
	auto pWAVWriterModule = std::make_shared<WAVWriterModule>(strRecordingFilePath, 100);

	// To Go Adapter
	auto pToJSONModule = std::make_shared<ToJSONModule>();
	auto pChunkToBytesModule = std::make_shared<ChunkToBytesModule>(100, 512);
	auto pTCPTXModule = std::make_shared<WinTCPTxModule>(strTCPTxIP, strTCPTxPort, 100, 512);

	// ------------
	// Connection
	// ------------

	// Connection pipeline modules
	pTCPRXModule->SetNextModule(pWAVSessionProcModule);
	pWAVSessionProcModule->SetNextModule(pSessionChunkRouter);
	pSessionChunkRouter->SetNextModule(nullptr); // Note: this module needs registered outputs not set outputs as it is a one to many

	// Registering outputs;
	pSessionChunkRouter->RegisterOutputModule(pTimeToWAVModule, ChunkType::TimeChunk);
	pSessionChunkRouter->RegisterOutputModule(pToJSONModule, ChunkType::TimeChunk);

	// WAV Chain connections
	pTimeToWAVModule->SetNextModule(pWAVAccumulatorModule);
	pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);
	pWAVWriterModule->SetNextModule(nullptr); // Note: This is a termination module so has no next module

	// To Go adapter
	pToJSONModule->SetNextModule(pChunkToBytesModule);
	pChunkToBytesModule->SetNextModule(pTCPTXModule);
	pTCPTXModule->SetNextModule(nullptr);

	// ------------
	// Start-Up
	// ------------

	// Starting chain from its end to start
	pWAVWriterModule->StartProcessing();
	pTimeToWAVModule->StartProcessing();
	pWAVAccumulatorModule->StartProcessing();
	pSessionChunkRouter->StartProcessing();
	pWAVSessionProcModule->StartProcessing();
	pTCPRXModule->StartProcessing();
	pTCPTXModule->StartProcessing();
	pChunkToBytesModule->StartProcessing();
	pToJSONModule->StartProcessing();

	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}