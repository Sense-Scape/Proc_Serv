#ifndef BASECHUNK
#define BASECHUNK

#include "ChunkTypes.h"

/**
 * @brief BaseChunk used to store the basic data storage component metadata
 */
class BaseChunk
{

private:

public:
    /**
     * @brief Default constructor for a new Base Chunk object
     *
     */
    BaseChunk();

    /**
     * @brief Construct a new Base Chunk object
     *
     * @param baseChunk Reference to another BaseChunk
     */
    BaseChunk(const BaseChunk& baseChunk);

    virtual ~BaseChunk() {};

    /**
     * @brief Get the Type object
     *
     * @return ChunkType ChunkType of chunk
     */
    virtual ChunkType getChunkType() { return ChunkType::ChunkBase; };
};

#endif