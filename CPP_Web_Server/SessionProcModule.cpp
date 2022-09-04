#include "SessionProcModule.h"

SessionProcModule::SessionProcModule(unsigned uBufferSize) : BaseModule(uBufferSize),
                                                            m_uSequenceNumber(),
                                                            m_uPreviousSequenceNumber(),
                                                            m_uTransmissionState(),
                                                            m_uTransmissionSize()
{
}

SessionProcModule::~SessionProcModule()
{
}

void SessionProcModule::Process()
{
    while (true)
    {
        std::shared_ptr<BaseChunk> pBaseChunk;
        if (TakeFromBuffer(pBaseChunk))
        {
            auto pUDPChunk = std::dynamic_pointer_cast<UDPChunk>(pBaseChunk);

            // 1. Extract data header

            // 2. Check squence number
            // check n = n{-1} + 1

            // 3. Store data

            // 4. final transmission?
            // if so pass data to next module 

            // 5. clear local storage and reinstantiate data

            if (m_pNextModule != nullptr)
                TryPassChunk(pBaseChunk);
        }
    }
}

void SessionProcModule::ExtractDataHeader(std::shared_ptr<UDPChunk> pUDPChunk)
{

    // TODO: Find a nive way to generalise this funvtion to generic header description

    // Extract bytes and conver according to header desription
    SessionMode_1 sHeaderDescriptions;

    //m_uSequenceNumber = &reinterpret_cast<char*>(&pUDPChunk->m_cvDataChunk[sHeaderDescriptions.m_uSequenceNumber]);
    m_uTransmissionSize;
    m_uTransmissionState;
    
}