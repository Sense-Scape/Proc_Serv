#ifndef BASEMODULE
#define BASEMODULE

/*Standard Includes*/
#include <memory>
#include <vector>
#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <chrono>

/*Custom Includes*/
#include "BaseChunk.h"
#include "CircularBuffer.h"

/**
 * @brief Base processing class containing a threaded process and thread safe input buffer.
 *
 */
class BaseModule
{
private:
    std::shared_ptr<BaseModule> m_pNextModule; ///< Shared pointer to next module into which messages are passed
    size_t m_uMaxInputBufferSize;              ///< Max size of the class input buffer

protected:
    CircularBuffer<std::shared_ptr<BaseChunk>> m_cbBaseChunkBuffer; ///< Input buffer of module
    std::thread m_thread;                                           ///< Thread object for module processing
    std::mutex m_BufferStateMutex;                                  ///< Mutex to facilitate multi module buffer size checking

    /**
     * @brief Returns true if a message pointer had been retrieved an passed on to next module.
     *          If no pointer in queue then returns false
     */
    virtual void Process();

    /**
     * @brief Passes base chunk pointer to next module
     *
     * @param pBaseChunk
     * @return true if message was sucessfully inserted into queue
     * @return false if message was unsucessfully inserted into queue
     */
    bool TryPassChunk(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Recieves base chunk pointer from previous module
     *
     * @param pBaseChunk
     * @return true if message was sucessfully inserted into the buffer
     * @return false if message was unsucessfully inserted into the buffer
     */
    bool TakeChunkFromModule(std::shared_ptr<BaseChunk> pBaseChunk);

    /**
     * @brief Tries to extract a message from the input buffer
     *
     * @return true if message was sucessfully removed from the buffer
     * @return false if message was unsucessfully removed from the buffer
     */
    bool TakeFromBuffer(std::shared_ptr<BaseChunk> pBaseChunk);

public:
    /**
     * @brief Construct a new Base Module object
     *
     */
    BaseModule(unsigned uMaxInputBufferSize = 1);
    virtual ~BaseModule() {};

    /**
     * @brief Starts the  process on its own thread
     *
     */
    void StartProcessing();

    /**
     * @brief Set the Next Module object to pass chunks along to
     *
     * @param m_NextModule pointer to the next module to pass chunk to
     */
    void SetNextModule(std::shared_ptr<BaseModule> pNextModule);
};

#endif