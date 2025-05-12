/* Standard Includes */
#include <fstream>

/* Custom Includes */
#include "BaseModule.h"
#include "SessionProcModule.h"
#include "RouterModule.h"
#include "WAVAccumulator.h"
#include "TimeToWAVModule.h"
#include "ToJSONModule.h"
#include "ChunkToBytesModule.h"
#include "FFTModule.h"
#include "LinuxMultiClientTCPRxModule.h"
#include "WAVWriterModule.h"
#include "TCPTxModule.h"
#include "EnergyDetectionModule.h"
#include "DirectionFindingModule.h"
#include "TracerModule.h"
#include "RateLimitingModule.h"
#include "TimeChunkSynchronisationModule.h"
#include "TCPRxModule.h"

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
		std::transform(strRequestLogLevel.begin(), strRequestLogLevel.end(), strRequestLogLevel.begin(), [](unsigned char c)
					   { return std::toupper(c); });

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
		std::transform(strLogToFile.begin(), strLogToFile.end(), strLogToFile.begin(), [](unsigned char c)
					   { return std::toupper(c); });

		if (strLogToFile == "TRUE")
		{
			// Get the time for the log file
			time_t lTime;
			time(&lTime);
			auto strTime = std::to_string((long long)lTime);
			std::string strLogFileName = "Proc_Serv_" + strTime + ".txt";

			// The create and add the appender
			static plog::RollingFileAppender<plog::CsvFormatter> fileAppender(strLogFileName.c_str(), 50'000'000, 2);
			logger.addAppender(&fileAppender);
		}

		// And to console
		std::string strLogToConsole = jsonConfig["LoggingConfig"]["LogToConsole"];
		std::transform(strLogToConsole.begin(), strLogToConsole.end(), strLogToConsole.begin(), [](unsigned char c)
					   { return std::toupper(c); });

		if (strLogToConsole == "TRUE")
		{
			static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
			logger.addAppender(&consoleAppender);
		}
	}
	catch (const std::exception &e)
	{
		PLOG_ERROR << e.what();
		throw;
	}

	// -------------------------
	// Pipeline Config Variables
	// -------------------------

	// TCP Rx
	std::string strTCPRxIP;
	uint16_t u16TCPRxPort;

	// UDP Tx
	std::string strTCPTxIP;
	uint16_t u16TCPTxPort;

	// WAV Sub Pipeline
	bool bEnableWAVRecording;
	float fAccumulationPeriod_sec;
	double dContinuityThresholdFactor;
	std::string strRecordingFilePath;

	// Energy Detection Module
	float fDetectionThreshold_db;

	// Direction Finding Module
	double dPropogationVelocity_mps;
	double dBaseLineLength_m;

	// Hard coded defaults
	uint16_t u16DefaultModuleBufferSize = 250;
	uint16_t u16DefualtNetworkDataTransmissionSize = 512;

	try // To load in config file
	{
		// Updating config variables
		// TCP Rx Module Config
		strTCPRxIP = jsonConfig["PipelineConfig"]["TCPRxModule"]["IP"];
		u16TCPRxPort = jsonConfig["PipelineConfig"]["TCPRxModule"]["Port"];
		// TCP Tx Module Config
		strTCPTxIP = jsonConfig["PipelineConfig"]["TCPTxModule"]["IP"];
		u16TCPTxPort = jsonConfig["PipelineConfig"]["TCPTxModule"]["Port"];

		// WAV Accumulator config
		std::string strEnableWAVRecording = jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]["WAVWriterModule"]["RecordData"];
		std::transform(strEnableWAVRecording.begin(), strEnableWAVRecording.end(), strEnableWAVRecording.begin(), [](unsigned char c)
					   { return std::toupper(c); });

		bEnableWAVRecording = (strEnableWAVRecording == "TRUE");
		fAccumulationPeriod_sec = jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]["WAVAccumulatorModule"]["RecordingPeriod"];
		dContinuityThresholdFactor = jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]["WAVAccumulatorModule"]["ContinuityThresholdFactor"];
		strRecordingFilePath = jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]["WAVWriterModule"]["RecordingPath"];
		
		// Detection config
		fDetectionThreshold_db = std::stod(std::string(jsonConfig["PipelineConfig"]["EnergyDetectionModule"]["Threshold_dB"]));
		
		// DF config
		dPropogationVelocity_mps = std::stod(std::string(jsonConfig["PipelineConfig"]["DirectionFindingModule"]["PropogationVelovity_mps"]));
		dBaseLineLength_m = std::stod(std::string(jsonConfig["PipelineConfig"]["DirectionFindingModule"]["BaseLineLength_m"]));
		
	}
	catch (const std::exception &e)
	{
		PLOG_ERROR << e.what();
		throw;
	}

	// ====================================
	// 			Receive Chain
	// ====================================
	//auto pt = std::make_shared<TracerModule>("sss");

	auto pTCPRXModule = std::make_shared<LinuxMultiClientTCPRxModule>(strTCPRxIP, u16TCPRxPort, u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
	auto pSessionProcModule = std::make_shared<SessionProcModule>(u16DefaultModuleBufferSize);
	auto pSessionChunkRouter = std::make_shared<RouterModule>(u16DefaultModuleBufferSize);

	pTCPRXModule->SetNextModule(pSessionProcModule);
	pSessionProcModule->SetNextModule(pSessionChunkRouter);
	pSessionChunkRouter->SetNextModule(nullptr); // Note: this module needs registered outputs not set outputs as it is a one to many

	pTCPRXModule->StartProcessing();
	pSessionProcModule->StartProcessing();
	pSessionChunkRouter->StartProcessing();

	pSessionProcModule->StartReporting();
	// ====================================


	// ====================================
	// 			Status Streaming Chain
	// ====================================

	auto u16StatusQueueSize = 25; // Limiting memory usage
	auto u16StatusPort = 1996;

	auto pReportingToJsonModule = std::make_shared<ToJSONModule>(u16StatusQueueSize);
	auto pStatusChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16StatusQueueSize, u16DefualtNetworkDataTransmissionSize);
	auto pStatusTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16StatusPort, u16StatusQueueSize, u16DefualtNetworkDataTransmissionSize,"Listen");

	pReportingToJsonModule->SetNextModule(pStatusChunkToBytesModule);
	pStatusChunkToBytesModule->SetNextModule(pStatusTCPExportModule);
	pStatusTCPExportModule->SetNextModule(nullptr);

	pReportingToJsonModule->StartProcessing();
	pStatusChunkToBytesModule->StartProcessing();
	pStatusTCPExportModule->StartProcessing();

	// Enable to stream time data
	pSessionChunkRouter->RegisterOutputModule(pReportingToJsonModule, ChunkType::JSONChunk);


	// ====================================
	// 			Time Sync Chain
	// ====================================

	auto u16TimeSyncPort = 1999;
	
	auto pTimeSyncModule = std::make_shared<TimeChunkSynchronisationModule>(u16DefaultModuleBufferSize, 5000000, 5e9);
	auto pTimeSyncChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
	auto pTimeSyncTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16TimeSyncPort, u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize,"Listen");

	pTimeSyncModule->SetNextModule(pTimeSyncChunkToBytesModule);
	pTimeSyncChunkToBytesModule->SetNextModule(pTimeSyncTCPExportModule);
	pTimeSyncTCPExportModule->SetNextModule(nullptr);

	pTimeSyncModule->StartProcessing();
	pTimeSyncChunkToBytesModule->StartProcessing();
	pTimeSyncTCPExportModule->StartProcessing();


	pTimeSyncModule->SetReportingNextModule(pReportingToJsonModule);
	pTimeSyncModule->SetReportingDescriptors("Server","0");
	pTimeSyncModule->StartReporting();

	// Enable to time sync time data
	// pSessionChunkRouter->RegisterOutputModule(pTimeSyncModule, ChunkType::TimeChunk);
	// pSessionChunkRouter->RegisterOutputModule(pTimeSyncModule, ChunkType::GPSChunk);

	// ====================================


	// ====================================
	// 			Time Streaming Chain
	// ====================================

	auto u16TimeQueueSize = 100; // Limiting memory usage
	auto u16TimePort = 1998;

	auto pTimeRateLimitingModule = std::make_shared<RateLimitingModule>(u16TimeQueueSize);
	auto pTimeChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16TimeQueueSize, u16DefualtNetworkDataTransmissionSize);
	auto pTimeTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16TimePort, u16TimeQueueSize, u16DefualtNetworkDataTransmissionSize, "Listen");

	pTimeRateLimitingModule->SetNextModule(pTimeChunkToBytesModule);
	pTimeChunkToBytesModule->SetNextModule(pTimeTCPExportModule);
	pTimeTCPExportModule->SetNextModule(nullptr);

	pTimeRateLimitingModule->SetChunkRateLimitInUsec(ChunkType::TimeChunk, 1'000'000'000/(100)); // 100Hz

	pTimeRateLimitingModule->StartProcessing();
	pTimeChunkToBytesModule->StartProcessing();
	pTimeTCPExportModule->StartProcessing();

	pTimeRateLimitingModule->SetReportingNextModule(pReportingToJsonModule);
	pTimeRateLimitingModule->SetReportingDescriptors("Server","TimeStream");
	pTimeRateLimitingModule->StartReporting();

	pTimeChunkToBytesModule->SetReportingNextModule(pReportingToJsonModule);
	pTimeChunkToBytesModule->SetReportingDescriptors("Server","TimeStream");
	pTimeChunkToBytesModule->StartReporting();

	pTimeTCPExportModule->SetReportingNextModule(pReportingToJsonModule);
	pTimeTCPExportModule->SetReportingDescriptors("Server","TimeStream");
	pTimeTCPExportModule->StartReporting();

	// Enable to stream time data
	pSessionChunkRouter->RegisterOutputModule(pTimeRateLimitingModule, ChunkType::TimeChunk);

	// ====================================
	// 			GPS Streaming Chain
	// ====================================

	auto u16GPSQueueSize = 25; // Limiting memory usage
	auto u16GPSPort = 1997;

	auto pGPSChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16GPSQueueSize, u16DefualtNetworkDataTransmissionSize);
	auto pGPSTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16GPSPort, u16GPSQueueSize, u16DefualtNetworkDataTransmissionSize, "Listen");

	pGPSChunkToBytesModule->SetNextModule(pGPSTCPExportModule);
	pGPSTCPExportModule->SetNextModule(nullptr);

	pGPSChunkToBytesModule->StartProcessing();
	pGPSTCPExportModule->StartProcessing();

	pGPSChunkToBytesModule->SetReportingNextModule(pReportingToJsonModule);
	pGPSChunkToBytesModule->SetReportingDescriptors("Server","GPSStream");
	pGPSChunkToBytesModule->StartReporting();

	pGPSTCPExportModule->SetReportingNextModule(pReportingToJsonModule);
	pGPSTCPExportModule->SetReportingDescriptors("Server","GPSStream");
	pGPSTCPExportModule->StartReporting();

	// Enable to stream time data
	pSessionChunkRouter->RegisterOutputModule(pGPSChunkToBytesModule, ChunkType::GPSChunk);

    // ====================================
	// 			Time Classifier TX Chain
	// ====================================

	auto u16ClassifierQueueSize = 100; // Limiting memory usage
	auto u16ClassifierPort = 1995;

	auto pClassifierChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16ClassifierQueueSize, u16DefualtNetworkDataTransmissionSize);
	auto pClassifierTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16ClassifierPort, u16TimeQueueSize, u16DefualtNetworkDataTransmissionSize, "Listen");

	pClassifierChunkToBytesModule->SetNextModule(pClassifierTCPExportModule);
	pClassifierTCPExportModule->SetNextModule(nullptr);

	pClassifierChunkToBytesModule->StartProcessing();
	pClassifierTCPExportModule->StartProcessing();

	pClassifierChunkToBytesModule->SetReportingNextModule(pReportingToJsonModule);
	pClassifierChunkToBytesModule->SetReportingDescriptors("Server","ClassifierStream");
	pClassifierChunkToBytesModule->StartReporting();

	pClassifierTCPExportModule->SetReportingNextModule(pReportingToJsonModule);
	pClassifierTCPExportModule->SetReportingDescriptors("Server","ClassifierStream");
	pClassifierTCPExportModule->StartReporting();

	// Enable to stream time data
	pSessionChunkRouter->RegisterOutputModule(pClassifierChunkToBytesModule, ChunkType::TimeChunk);

	// ====================================
	// 			Time Classifier RX/TX Chain
	// ====================================

	auto u16ClassifierRxQueueSize = 100; 
	auto u16ClassifierRxPort = 1994;
	auto u16ClassifierTxPort = 1993;

	auto pClassifierRxModule = std::make_shared<TCPRxModule>("0.0.0.0", u16ClassifierRxPort,u16ClassifierRxQueueSize,u16DefualtNetworkDataTransmissionSize,"Connect");
	auto pClassifierSessionProcModule = std::make_shared<SessionProcModule>(u16DefaultModuleBufferSize);
	auto pClassifierResultChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16GPSQueueSize, u16DefualtNetworkDataTransmissionSize);
	auto pClassifierResultTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16ClassifierTxPort, u16GPSQueueSize, u16DefualtNetworkDataTransmissionSize, "Listen");

	pClassifierRxModule->SetNextModule(pClassifierSessionProcModule);
	pClassifierSessionProcModule->SetNextModule(pClassifierResultChunkToBytesModule);
	
	pClassifierResultChunkToBytesModule->SetNextModule(pClassifierResultTCPExportModule);
	pClassifierResultTCPExportModule->SetNextModule(nullptr);
	
	pClassifierRxModule->StartProcessing();
	pClassifierSessionProcModule->StartProcessing();
	pClassifierResultChunkToBytesModule->StartProcessing();
	pClassifierResultTCPExportModule->StartProcessing();

	// ====================================
	// 			Audio Stream TX Chain
	// ====================================

	auto u16AudioQueueSize = 100; // Limiting memory usage
	auto u16AudioPort = 1992;

	auto pAudioChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16AudioQueueSize, u16DefualtNetworkDataTransmissionSize);
	auto pAudioTCPExportModule = std::make_shared<TCPTxModule>(strTCPTxIP, u16AudioPort, u16TimeQueueSize, u16DefualtNetworkDataTransmissionSize, "Listen");

	pAudioChunkToBytesModule->SetNextModule(pAudioTCPExportModule);
	pAudioTCPExportModule->SetNextModule(nullptr);

	pAudioChunkToBytesModule->StartProcessing();
	pAudioTCPExportModule->StartProcessing();

	pAudioChunkToBytesModule->SetReportingNextModule(pReportingToJsonModule);
	pAudioChunkToBytesModule->SetReportingDescriptors("Server","AudioStream");
	pAudioChunkToBytesModule->StartReporting();

	pAudioTCPExportModule->SetReportingNextModule(pReportingToJsonModule);
	pAudioTCPExportModule->SetReportingDescriptors("Server","AudioStream");
	pAudioTCPExportModule->StartReporting();

	// Enable to stream time data
	pSessionChunkRouter->RegisterOutputModule(pAudioChunkToBytesModule, ChunkType::TimeChunk);

	// ====================================
	// 			Audio Recording Chain
	// ====================================

		// Constructing WAV Subpipeline
	std::shared_ptr<WAVAccumulator> pWAVAccumulatorModule;
	std::shared_ptr<TimeToWAVModule> pTimeToWAVModule;
	std::shared_ptr<WAVWriterModule> pWAVWriterModule;

	if (bEnableWAVRecording)
	{
		std::string strInfo = std::string(__FUNCTION__) + " Constructing WAV sub pipeline";
		PLOG_INFO << strInfo;

		// WAV Processing Chain
		pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(fAccumulationPeriod_sec, dContinuityThresholdFactor, u16DefaultModuleBufferSize);
		pTimeToWAVModule = std::make_shared<TimeToWAVModule>(u16DefaultModuleBufferSize);
		pWAVWriterModule = std::make_shared<WAVWriterModule>(strRecordingFilePath, u16DefaultModuleBufferSize);

		// WAV Chain connections
		pSessionChunkRouter->RegisterOutputModule(pTimeToWAVModule, ChunkType::TimeChunk);
		pTimeToWAVModule->SetNextModule(pWAVAccumulatorModule);
		pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);
		pWAVWriterModule->SetNextModule(nullptr); // Note: This is a termination module so has no next module

		// Starting WAV Chain
		pWAVWriterModule->StartProcessing();
		pTimeToWAVModule->StartProcessing();
		pWAVAccumulatorModule->StartProcessing();
	}

	// ====================================
	


	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
