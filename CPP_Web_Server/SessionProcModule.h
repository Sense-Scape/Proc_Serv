#ifndef SESSION_PROC_MODULE
#define SESSION_PROC_MODULE

/*Standard Includes*/

/*Custom Includes*/
#include "BaseModule.h"
#include "SC_Chunk_Types/CPP_Chunk_Types/UDPChunk.h"
#include "SC_Chunk_Types/CPP_Chunk_Types/SessionModeTypes.h"


class SessionProcModule :
    public BaseModule
{
private:
    unsigned m_uSequenceNumber;
    unsigned m_uPreviousSequenceNumber;
    unsigned m_uTransmissionState;
    unsigned m_uTransmissionSize;

    /*
     * @brief Extract header information from transmitted bytes
     * 
     * @param pUDPChunk pointer to UDPChunk
     */
    void ExtractDataHeader(std::shared_ptr<UDPChunk> pUDPChunk);

public:
    /**
     * @brief Construct a new Session Processing Module to proce UDP data
     * 
     * @param uBufferSize size of processing input buffer
     */
    SessionProcModule(unsigned uBufferSize);
    ~SessionProcModule();

    /*
     * @brief Module process to collect and format UDP data
     */
    void Process() override;

};

#endif