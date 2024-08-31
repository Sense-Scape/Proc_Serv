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
	std::string strTCPRxPort;

	// UDP Tx
	bool bEnableTxSubPipeline;
	std::string strTCPTxIP;
	std::string strTCPTxPort;

	// WAV Sub Pipeline
	bool bEnableWAVSubPipeline;
	float fAccumulationPeriod_sec;
	double dContinuityThresholdFactor;
	std::string strRecordingFilePath;

	// Energy Detection Module
	float fDetectionThreshold_db;

	// Direction Finding Module
	double dPropogationVelocity_mps;
	double dBaseLineLength_m;

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
		std::string strEnableTXSubPipeline = jsonConfig["PipelineConfig"]["TCPTxModule"]["Enable"];
		std::transform(strEnableTXSubPipeline.begin(), strEnableTXSubPipeline.end(), strEnableTXSubPipeline.begin(), [](unsigned char c)
					   { return std::toupper(c); });
		bEnableTxSubPipeline = (strEnableTXSubPipeline == "TRUE");
		 
		// WAV Accumulator config
		std::string strEnableWAVSubPipeline = jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]["EnableSubPipeline"];
		std::transform(strEnableWAVSubPipeline.begin(), strEnableWAVSubPipeline.end(), strEnableWAVSubPipeline.begin(), [](unsigned char c)
					   { return std::toupper(c); });

		bEnableWAVSubPipeline = (strEnableWAVSubPipeline == "TRUE");
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

	// ------------
	// Construction
	// ------------

	// Start of Processing Chain
	auto pTCPRXModule = std::make_shared<LinuxMultiClientTCPRxModule>(strTCPRxIP, strTCPRxPort, u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
	auto pSessionProcModule = std::make_shared<SessionProcModule>(u16DefaultModuleBufferSize);
	auto pSessionChunkRouter = std::make_shared<RouterModule>(u16DefaultModuleBufferSize);

	// // FFT proc
	auto pFFTProcModule = std::make_shared<FFTModule>(u16DefaultModuleBufferSize);
	auto pEnergyDetectionModule = std::make_shared<EnergyDetectionModule>(u16DefaultModuleBufferSize, fDetectionThreshold_db);
	auto pDirectionFindingModule = std::make_shared<DirectionFindingModule>(u16DefaultModuleBufferSize, dPropogationVelocity_mps, dBaseLineLength_m);

	// To Go Adapter
	auto pRateLimitingModule = std::make_shared<RateLimitingModule>(u16DefaultModuleBufferSize);
	auto pToJSONModule = std::make_shared<ToJSONModule>(u16DefaultModuleBufferSize);
	auto pChunkToBytesModule = std::make_shared<ChunkToBytesModule>(u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
	auto pTCPTXModule = std::make_shared<TCPTxModule>(strTCPTxIP, strTCPTxPort, u16DefaultModuleBufferSize, u16DefualtNetworkDataTransmissionSize);
		
	// ------------
	// Connection
	// ------------

	// Connection pipeline modules
	pTCPRXModule->SetNextModule(pSessionProcModule);
	pSessionProcModule->SetNextModule(pSessionChunkRouter);
	pSessionChunkRouter->SetNextModule(nullptr); // Note: this module needs registered outputs not set outputs as it is a one to many

	if(bEnableTxSubPipeline)
	{
		pSessionChunkRouter->RegisterOutputModule(pToJSONModule, ChunkType::GPSChunk);
		pSessionChunkRouter->RegisterOutputModule(pToJSONModule, ChunkType::QueueLengthChunk);
		pSessionChunkRouter->RegisterOutputModule(pFFTProcModule, ChunkType::TimeChunk);

		// FFT Proc Chain
		pFFTProcModule->SetNextModule(pEnergyDetectionModule);
		pFFTProcModule->SetGenerateMagnitudeData(true);
		pEnergyDetectionModule->SetNextModule(pDirectionFindingModule);
		pDirectionFindingModule->SetNextModule(pRateLimitingModule);
		
		uint64_t u64RateLimitPeriod_ns = 1'000'000'000/60;
		pRateLimitingModule->SetChunkRateLimitInUsec(ChunkType::TimeChunk, u64RateLimitPeriod_ns);
		pRateLimitingModule->SetChunkRateLimitInUsec(ChunkType::FFTChunk, u64RateLimitPeriod_ns);
		pRateLimitingModule->SetChunkRateLimitInUsec(ChunkType::FFTMagnitudeChunk, u64RateLimitPeriod_ns);
		pRateLimitingModule->SetChunkRateLimitInUsec(ChunkType::DirectionBinChunk, u64RateLimitPeriod_ns);

		// To Go adapter
		pRateLimitingModule->SetNextModule(pToJSONModule);	
		pToJSONModule->SetNextModule(pChunkToBytesModule);
		pChunkToBytesModule->SetNextModule(pTCPTXModule);
		pTCPTXModule->SetNextModule(nullptr);
		
		pRateLimitingModule->StartReporting();
		pFFTProcModule->StartReporting();
		pEnergyDetectionModule->StartReporting();
		pDirectionFindingModule->StartReporting();

		pTCPTXModule->StartProcessing();
		pChunkToBytesModule->StartProcessing();
		pRateLimitingModule->StartProcessing();
		pToJSONModule->StartProcessing();
		pDirectionFindingModule->StartProcessing();

		pFFTProcModule->StartProcessing();
		pEnergyDetectionModule->StartProcessing();
	}

	// Constructing WAV Subpipeline
	std::shared_ptr<WAVAccumulator> pWAVAccumulatorModule;
	std::shared_ptr<TimeToWAVModule> pTimeToWAVModule;
	std::shared_ptr<WAVWriterModule> pWAVWriterModule;

	if (bEnableWAVSubPipeline)
	{
		std::string strInfo = std::string(__FUNCTION__) + " Constructing WAV sub pipeline";
		PLOG_INFO << strInfo;

		// WAV Processing Chain
		pTimeToWAVModule = std::make_shared<TimeToWAVModule>(u16DefaultModuleBufferSize);
		pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(fAccumulationPeriod_sec, dContinuityThresholdFactor, u16DefaultModuleBufferSize);
		pWAVWriterModule = std::make_shared<WAVWriterModule>(strRecordingFilePath, u16DefaultModuleBufferSize);

		
		// pTimeToWAVModule->TrackProcessTime(true, "pTimeToWAVModule");
		// pWAVAccumulatorModule->TrackProcessTime(true, "pWAVAccumulatorModule");
		// pWAVWriterModule->TrackProcessTime(true, "pWAVWriterModule");

		// WAV Chain connections
		pSessionChunkRouter->RegisterOutputModule(pTimeToWAVModule, ChunkType::TimeChunk);
		pTimeToWAVModule->SetNextModule(pWAVAccumulatorModule);
		pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);
		pWAVWriterModule->SetNextModule(nullptr); // Note: This is a termination module so has no next module

		// Starting WAV Chain
		pTimeToWAVModule->StartProcessing();
		pWAVAccumulatorModule->StartProcessing();
		pWAVWriterModule->StartProcessing();
		
	}

	// ------------
	// Start-Up
	// ------------
	// Starting chain from its end to start
	pSessionProcModule->StartProcessing();
	pSessionChunkRouter->StartProcessing();
	pTCPRXModule->StartProcessing();


	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}