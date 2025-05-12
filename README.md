# Proc_Serv

## Summary

This is a processing server which sends is designed to recieve data from either the [simulator module](https://github.com/Sense-Scape/Windows_Sensor_Sim/tree/main) or the [ESP32 sensor](https://github.com/Sense-Scape/Acoustic_Sensor_ESP32).
This module is configured using the ```Config.json``` file. 

## Block Diagram

``` mermaid 
graph TD; 
  pTCPRXModule-->pSessionProcModule
  pSessionProcModule-->pSessionChunkRouter
  pReportingToJsonModule-->pStatusChunkToBytesModule
  pStatusChunkToBytesModule-->pStatusTCPExportModule
  pTimeSyncModule-->pTimeSyncChunkToBytesModule
  pTimeSyncChunkToBytesModule-->pTimeSyncTCPExportModule
  pTimeRateLimitingModule-->pTimeChunkToBytesModule
  pTimeChunkToBytesModule-->pTimeTCPExportModule
  pGPSChunkToBytesModule-->pGPSTCPExportModule
  pClassifierChunkToBytesModule-->pClassifierTCPExportModule
  pClassifierRxModule-->pClassifierSessionProcModule
  pClassifierSessionProcModule-->pClassifierResultChunkToBytesModule
  pClassifierResultChunkToBytesModule-->pClassifierResultTCPExportModule
  pAudioChunkToBytesModule-->pAudioTCPExportModule
  	pTimeToWAVModule-->pWAVAccumulatorModule
  	pWAVAccumulatorModule-->pWAVWriterModule
  pSessionChunkRouter-- JSONChunk -->pReportingToJsonModule
  pSessionChunkRouter-- TimeChunk -->pTimeSyncModule
  pSessionChunkRouter-- GPSChunk -->pTimeSyncModule
  pSessionChunkRouter-- TimeChunk -->pTimeRateLimitingModule
  pSessionChunkRouter-- GPSChunk -->pGPSChunkToBytesModule
  pSessionChunkRouter-- TimeChunk -->pClassifierChunkToBytesModule
  pSessionChunkRouter-- TimeChunk -->pAudioChunkToBytesModule
  	pSessionChunkRouter-- TimeChunk -->pTimeToWAVModule
```