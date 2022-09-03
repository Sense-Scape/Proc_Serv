#include "./BaseModule.h"

BaseModule::BaseModule(unsigned uMaxInputBufferSize) : m_pNextModule(nullptr),
m_uMaxInputBufferSize(uMaxInputBufferSize),
m_cbBaseChunkBuffer(uMaxInputBufferSize)
{
}

void BaseModule::Process()
{
    while (true)
    {
        std::shared_ptr<BaseChunk> pBaseChunk;
        if (TakeFromBuffer(pBaseChunk))
        {
            // In derived classes processing should be completed here

            // if next module nullptr then this is a "terminating module"
            // and the pointer to the message will be droppped
            if (m_pNextModule != nullptr)
                TryPassChunk(pBaseChunk);
        }
    }  
}

void BaseModule::StartProcessing()
{
    m_thread = std::thread([this]
        { Process(); });
}

void BaseModule::SetNextModule(std::shared_ptr<BaseModule> pNextModule)
{
    m_pNextModule = pNextModule;
}

bool BaseModule::TryPassChunk(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Allow module that one is passing to to facilitate its own locking procedures
    return m_pNextModule->TakeChunkFromModule(pBaseChunk);
}

bool BaseModule::TakeChunkFromModule(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Maintain lock to prevent another module trying to access buffer
    // in the multi input buffer configuration
    std::lock_guard<std::mutex> BufferStateLock(m_BufferStateMutex);

    // Check if next module queue is full
    if (m_cbBaseChunkBuffer.size() < m_uMaxInputBufferSize)
    {
        m_cbBaseChunkBuffer.put(pBaseChunk);
        return true;
    }
    return false;
}

bool BaseModule::TakeFromBuffer(std::shared_ptr<BaseChunk> pBaseChunk)
{
    // Maintain lock to prevent another module trying to access buffer
    // in the multi input buffer configuration
    std::lock_guard<std::mutex> BufferStateLock(m_BufferStateMutex);

    if (!m_cbBaseChunkBuffer.empty())
    {
        pBaseChunk = m_cbBaseChunkBuffer.get();
        return true;
    }

    return false;
}

