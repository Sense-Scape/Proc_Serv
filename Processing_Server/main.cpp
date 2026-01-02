/* Standard Includes */
#include <fstream>

/* Custom Includes */

#include "ChunkToBytesModule.h"
#include "LinuxMultiClientTCPRxModule.h"
#include "RouterModule.h"
#include "SessionProcModule.h"
#include "TCPRxModule.h"
#include "TCPTxModule.h"
#include "TimeToWAVModule.h"
#include "ToJSONModule.h"
#include "WAVAccumulator.h"
#include "WAVWriterModule.h"
#include <HTTPPostModule.h>
#include <JSONStateAccumulatorModule.h>
#include <TracerModule.h>

/* External Libraries */
#include "json.hpp"
#include "plog/Log.h"
#include <memory>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>

int main() {

  // ---------------------
  // Logging Configuration
  // ---------------------

  // Reading and parsing JSON config
  std::ifstream file("./Config.json");
  std::string jsonString((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  nlohmann::json jsonConfig = nlohmann::json::parse(jsonString);

  try // To configure logging
  {
    auto eLogLevel = plog::debug;

    // Get log level
    std::string strRequestLogLevel =
        jsonConfig["LoggingConfig"]["LoggingLevel"];
    std::transform(strRequestLogLevel.begin(), strRequestLogLevel.end(),
                   strRequestLogLevel.begin(),
                   [](unsigned char c) { return std::toupper(c); });

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
    bool bLogToFile = jsonConfig["LoggingConfig"]["LogToTextFile"];

    if (bLogToFile) {
      // Get the time for the log file
      time_t lTime;
      time(&lTime);
      auto strTime = std::to_string((long long)lTime);
      std::string strLogFileName = "Proc_Serv_" + strTime + ".txt";

      // The create and add the appender
      static plog::RollingFileAppender<plog::CsvFormatter> fileAppender(
          strLogFileName.c_str(), 50'000'000, 2);
      logger.addAppender(&fileAppender);
    }

    // And to console
    bool bLogToConsole = jsonConfig["LoggingConfig"]["LogToConsole"];

    if (bLogToConsole) {
      PLOG_INFO << "Begining Console Logs";
      static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
      logger.addAppender(&consoleAppender);
    } else {
      std::cout << "Console logging disabled in config";
    }
  } catch (const std::exception &e) {
    PLOG_ERROR << e.what();
    throw;
  }

  uint16_t u16DefaultModuleBufferSize = 250;

  // ====================================
  // 			Receive Chain
  // ====================================

  auto pTCPRXModule = std::make_shared<LinuxMultiClientTCPRxModule>(
      u16DefaultModuleBufferSize,
      jsonConfig["PipelineConfig"]["MultiSensorTCPRxModule"]);
  auto pSessionProcModule =
      std::make_shared<SessionProcModule>(u16DefaultModuleBufferSize);
  auto pSessionChunkRouter =
      std::make_shared<RouterModule>(u16DefaultModuleBufferSize);

  pTCPRXModule->SetNextModule(pSessionProcModule);
  pSessionProcModule->SetNextModule(pSessionChunkRouter);
  pSessionChunkRouter->SetNextModule(
      nullptr); // Note: this module needs registered outputs not set outputs as
                // it is a one to many

  pTCPRXModule->StartProcessing();
  pSessionProcModule->StartProcessing();
  pSessionChunkRouter->StartProcessing();

  pSessionProcModule->StartReporting();

  // ====================================

  // ====================================
  // 			Time Classifier TX Chain
  // ====================================

  auto u16ClassifierQueueSize = 100; // Limiting memory usage
  auto u16TimeQueueSize = 25;

  auto pClassifierChunkToBytesModule =
      std::make_shared<ChunkToBytesModule>(u16ClassifierQueueSize, 512);
  auto pClassifierTCPExportModule = std::make_shared<TCPTxModule>(
      u16TimeQueueSize, jsonConfig["PipelineConfig"]["TCPTxClassifierModule"]);

  pClassifierChunkToBytesModule->SetNextModule(pClassifierTCPExportModule);
  pClassifierTCPExportModule->SetNextModule(nullptr);

  pClassifierChunkToBytesModule->StartProcessing();
  pClassifierTCPExportModule->StartProcessing();

  // Enable to stream time data
  pSessionChunkRouter->RegisterOutputModule(pClassifierChunkToBytesModule,
                                            ChunkType::TimeChunk);

  // ====================================
  // 			Time Classifier RX Chain
  // ====================================

  auto u16ClassifierQueueSize2 = 25;

  auto pClassifierRxModule = std::make_shared<TCPRxModule>(
      u16DefaultModuleBufferSize,
      jsonConfig["PipelineConfig"]["TCPRxClassifierModule"]);
  auto pClassifierSessionProcModule =
      std::make_shared<SessionProcModule>(u16DefaultModuleBufferSize);
  auto pClassifierSessionChunkRouter =
      std::make_shared<RouterModule>(u16DefaultModuleBufferSize);

  pClassifierRxModule->SetNextModule(pClassifierSessionProcModule);
  pClassifierSessionProcModule->SetNextModule(pClassifierSessionChunkRouter);

  pClassifierRxModule->StartProcessing();
  pClassifierSessionProcModule->StartProcessing();
  pClassifierSessionChunkRouter->StartProcessing();

  // ====================================
  // 			Map Display Export
  // ====================================

  auto pMapDisplayToJson = std::make_shared<ToJSONModule>(512);
  auto pMapDisplayJSONAccumulator =
      std::make_shared<JSONStateAccumulatorModule>(
          512, jsonConfig["PipelineConfig"]["JSONAccumulator"]);
  auto pHTTPPostToMap = std::make_shared<HTTPPostModule>(
      512, jsonConfig["PipelineConfig"]["HTTPPostModule"]);

  pMapDisplayToJson->SetNextModule(pMapDisplayJSONAccumulator);
  pMapDisplayJSONAccumulator->SetNextModule(pHTTPPostToMap);
  pHTTPPostToMap->SetNextModule(nullptr);

  pMapDisplayToJson->StartProcessing();
  pMapDisplayJSONAccumulator->StartProcessing();
  pHTTPPostToMap->StartProcessing();

  pSessionChunkRouter->RegisterOutputModule(pMapDisplayToJson,
                                            ChunkType::GPSChunk);

  pClassifierSessionChunkRouter->RegisterOutputModule(pMapDisplayToJson,
                                                      ChunkType::JSONChunk);
  // ====================================
  // 			Audio Recording
  // ====================================

  // Constructing WAV Subpipeline
  std::shared_ptr<WAVAccumulator> pWAVAccumulatorModule;
  std::shared_ptr<TimeToWAVModule> pTimeToWAVModule;
  std::shared_ptr<WAVWriterModule> pWAVWriterModule;
  if (jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]
                ["EnableSubPipeline"]) {

    PLOG_INFO << "Enabling wav recording pipeline";
    // WAV Processing Chain
    pWAVAccumulatorModule = std::make_shared<WAVAccumulator>(
        u16DefaultModuleBufferSize,
        jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]
                  ["WAVAccumulatorModule"]);
    pTimeToWAVModule =
        std::make_shared<TimeToWAVModule>(u16DefaultModuleBufferSize);
    pWAVWriterModule = std::make_shared<WAVWriterModule>(
        u16DefaultModuleBufferSize,
        jsonConfig["PipelineConfig"]["WAVSubPipelineConfig"]
                  ["WAVWriterModule"]);

    // WAV Chain connections
    pSessionChunkRouter->RegisterOutputModule(pTimeToWAVModule,
                                              ChunkType::TimeChunk);
    pTimeToWAVModule->SetNextModule(pWAVAccumulatorModule);
    pWAVAccumulatorModule->SetNextModule(pWAVWriterModule);
    pWAVWriterModule->SetNextModule(
        nullptr); // Note: This is a termination module so has no next module

    // Starting WAV Chain
    pWAVWriterModule->StartProcessing();
    pTimeToWAVModule->StartProcessing();
    pWAVAccumulatorModule->StartProcessing();
  }

  // ====================================

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
