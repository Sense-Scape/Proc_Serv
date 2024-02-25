# Proc_Serv

## Summary

This is a processing server which sends is designed to recieve data from either the [simulator module](https://github.com/Sense-Scape/Windows_Sensor_Sim/tree/main) or the [ESP32 sensor](https://github.com/Sense-Scape/Acoustic_Sensor_ESP32).
This module is configured uising the ```Config.json``` file. 

## Block Diagram

``` mermaid 
graph TD; 
  pMultiClientTCPRXModule-->pWAVSessionProcModule
  pWAVSessionProcModule-->pSessionChunkRouter
  pFFTProcModule-->pToJSONModule
  pToJSONModule-->pChunkToBytesModule
  pChunkToBytesModule-->pTCPTXModule
  	pTimeToWAVModule-->pWAVAccumulatorModule
  	pWAVAccumulatorModule-->pWAVWriterModule
  pSessionChunkRouter-- TimeChunk -->pToJSONModule
  pSessionChunkRouter-- TimeChunk -->pFFTProcModule
  	pSessionChunkRouter-- TimeChunk -->pTimeToWAVModule
```
