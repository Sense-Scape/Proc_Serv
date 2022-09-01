#ifndef COMPLEXTIMECHUNK
#define COMPLEXTIMECHUNK

/*Standard Includes*/
#include <memory>
#include <vector>
#include <complex>

/*Custom Includes*/
#include "./BaseChunk.h"

/**
 * @brief Complex Time Data Chunk used to store all samples data from the
 *          The active ADCs and their respective channels
 */
class TimeChunk : public BaseChunk
{
private:
public:
    double m_dChunkSize;                                           ///< Number of samples contained in a single chunk
    double m_dSampleRate;                                          ///< Sample rate used to obtain data in chunk
    double m_dTimeStamp;                                           ///< Timestamp of when chunk was taken
    unsigned m_uBits;                                              ///< Bits of ADC used to produce chunk
    unsigned m_uNumChannels;                                       ///< Number of audion channels in chunk
    unsigned m_uNumBytes;                                          ///< Number of bytes in single sample
    std::vector<std::vector<std::vector<double>>> m_vvvdTimeChunk; ///< Vector of ADCChannelSamples corresponding to active ADCs

    /**
     * @brief Construct a new Base Chunk object
     *
     * @param dChunkSize The number of samples contained in each ADC channel chunk
     * @param dSampleRate The sample rate used to generate all data within the chunk
     * @param dTimeStamp The time the chunk was created
     * @param uBits Bits of ADC used to produce chunk
     */
    TimeChunk(double dChunkSize, double dSampleRate, double dTimeStamp, unsigned uBits, unsigned uNumBytes);

    /**
     * @brief Construct a new Time Chunk object
     *
     * @param timeChunk
     */
    TimeChunk(const TimeChunk& timeChunk);
    virtual ~TimeChunk() {};

    /**
     * @brief Get the Chunk Type object
     *
     * @return ChunkType
     */
    ChunkType getChunkType() override { return ChunkType::TimeChunkWAV; };
};

#endif