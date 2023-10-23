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

	// ---------------------
	// Logging Configuration
	// ---------------------

	// Reading and parsing JSON config
	std::ifstream file("./Config.json");
	std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	nlohmann::json jsonConfig = nlohmann::json::parse(jsonString);

	try // To configure logging
	{
		auto eLogLevel = plog::debug;

		// Get log level
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

		// And create loggers the after try append to it
		auto &logger = plog::init(eLogLevel);

		// Check if one should log to files
		std::string strLogToFile = jsonConfig["LoggingConfig"]["LogToTextFile"];
		std::transform(strLogToFile.begin(), strLogToFile.end(), strLogToFile.begin(), [](unsigned char c) {
			return std::toupper(c);
			});

		if (strLogToFile == "TRUE")
		{
			// Get the time for the log file
			time_t lTime;
			time(&lTime);
			auto strTime = std::to_string((long long)lTime);
			std::string strLogFileName = "PL_Windows_Proc_Serv_" + strTime + ".txt";

			// The create and add the appender
			static plog::RollingFileAppender<plog::CsvFormatter> fileAppender(strLogFileName.c_str(), 50'000'000, 2);
			logger.addAppender(&fileAppender);
		}

		// And to console
		std::string strLogToConsole = jsonConfig["LoggingConfig"]["LogToConsole"];
		std::transform(strLogToConsole.begin(), strLogToConsole.end(), strLogToConsole.begin(), [](unsigned char c) {
			return std::toupper(c);
			});

		if (strLogToConsole == "TRUE")
		{
			static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
			logger.addAppender(&consoleAppender);
		}

	}
	catch (const std::exception& e)
	{
		PLOG_ERROR << e.what();
		throw;
	}

	// -------------------------
	// Pipeline Config Variables
	// -------------------------

	// TCP Rx
	std::string strTCPRxIP;
	std::string strTCPRxPort;

	// UDP Tx
	std::string strTCPTxIP;
	std::string strTCPTxPort;

	// Other
	float fAccumulationPeriod_sec;
	double dContinuityThresholdFactor;
	std::string strRecordingFilePath;

	// Hard coded defaults
	uint16_t u16DefaultModuleBufferSize = 100;
	uint16_t u16DefualtNetworkDataTransmissionSize = 512;

	try // To load in config file
	{
		// Updating config variables 
		// TCP Rx Module Config
		strTCPRxIP = jsonConfig["PipelineConfig"]["TCPRxModule"]["IP"];
		strTCPRxPort = jsonConfig["PipelineConfig"]["TCPRxModule"]["Port"];
		// TCP Tx Module Config
		strTCPTxIP = jsonConfig["PipelineConfig"]["TCPTxModule"]["IP"];
		strTCPTxPort = jsonConfig["PipelineConfig"]["TCPTxModule"]["Port"];
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

	// ------------
	// Construction
	// ------------

	// Start of Processing Chain
	auto pTCPRXModule = std::make_shared<WinTCPRxModule>(strTCPRxIP, strTCPRxPort, u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
	auto pWAVSessionProcModule = std::make_shared<SessionProcModule>(u16DefaultModuleBufferSize);
	auto pSessionChunkRouter = std::make_shared<RouterModule>(u16DefaultModuleBufferSize);

	// WAV Processing Chain
	auto pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(fAccumulationPeriod_sec, dContinuityThresholdFactor, u16DefaultModuleBufferSize);
	auto pTimeToWAVModule = std::make_shared<TimeToWAVModule>(u16DefaultModuleBufferSize);
	auto pWAVWriterModule = std::make_shared<WAVWriterModule>(strRecordingFilePath, u16DefaultModuleBufferSize);

	// To Go Adapter
	auto pToJSONModule = std::make_shared<ToJSONModule>();
	auto pChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
	auto pTCPTXModule = std::make_shared<WinTCPTxModule>(strTCPTxIP, strTCPTxPort, u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);

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