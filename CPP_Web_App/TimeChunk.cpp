#include "./TimeChunk.h"

TimeChunk::TimeChunk(double dChunkSize, double dSampleRate, double dTimeStamp, unsigned uBits, unsigned uNumBytes) : BaseChunk(),
m_dChunkSize(dChunkSize),
m_dSampleRate(dSampleRate),
m_dTimeStamp(dTimeStamp),
m_uBits(uBits),
m_uNumBytes(uNumBytes)
{
}

TimeChunk::TimeChunk(const TimeChunk& timeChunk) : BaseChunk()
{
    m_vvvdTimeChunk = timeChunk.m_vvvdTimeChunk;
    m_uBits = timeChunk.m_uBits;
    m_uNumChannels = timeChunk.m_uNumChannels;
}