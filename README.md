# Proc_Serv

## Summary

- This server is configured using the ```Config.json``` file
- It is able to store and write received WAV data
- It retransmits data received (GPS, synchronised time channels, single time stream) from sensor clients to third party clients (such as a UI) 
- It is able to connect to a local classifier and stream data to it and forward results to additional clients

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