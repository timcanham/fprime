// ======================================================================
// \title  CmdSequencerImpl.cpp
// \author Bocchino/Canham
// \brief  cpp file for CmdDispatcherComponentBase component implementation class
//
// Copyright (C) 2009-2018 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.

#include <Fw/Com/ComPacket.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/SerialBuffer.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Svc/CmdSequencer/CmdSequencerImpl.hpp>
#include <Utils/Hash/Hash.hpp>
#include <config/CommandDispatcherImplCfg.hpp>

namespace Svc {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

CmdSequencerComponentImpl::CmdSequencerComponentImpl(const char* name)
    : CmdSequencerComponentBase(name),
      m_FPrimeSequence(*this),
      m_sequence(&this->m_FPrimeSequence),
      m_loadCmdCount(0),
      m_cancelCmdCount(0),
      m_errorCount(0),
      m_runMode(STOPPED),
      m_stepMode(AUTO),
      m_executedCount(0),
      m_totalExecutedCount(0),
      m_sequencesCompletedCount(0),
      m_timeout(0),
      m_blockState(Svc::BlockState::NO_BLOCK),
      m_opCode(0),
      m_cmdSeq(0),
      m_join_waiting(false),
      m_jcfActive(false),
      m_jcfTarget(""),
      m_jcsActive(false),
      m_jcsTarget(""),
      m_errorMode(true) {}  // Default: error mode ON (abort on error)

void CmdSequencerComponentImpl::setTimeout(const U32 timeout) {
    this->m_timeout = timeout;
}

void CmdSequencerComponentImpl ::setSequenceFormat(Sequence& sequence) {
    this->m_sequence = &sequence;
}

void CmdSequencerComponentImpl ::allocateBuffer(const FwEnumStoreType identifier,
                                                Fw::MemAllocator& allocator,
                                                const FwSizeType bytes) {
    this->m_sequence->allocateBuffer(identifier, allocator, bytes);
}

void CmdSequencerComponentImpl ::loadSequence(const Fw::ConstStringBase& fileName) {
    FW_ASSERT(this->m_runMode == STOPPED, this->m_runMode);
    if (not this->loadFile(fileName)) {
        this->m_sequence->clear();
    }
}

void CmdSequencerComponentImpl ::deallocateBuffer(Fw::MemAllocator& allocator) {
    this->m_sequence->deallocateBuffer(allocator);
}

CmdSequencerComponentImpl::~CmdSequencerComponentImpl() {}

// ----------------------------------------------------------------------
// Handler implementations
// ----------------------------------------------------------------------

void CmdSequencerComponentImpl::CS_RUN_cmdHandler(FwOpcodeType opCode,
                                                  U32 cmdSeq,
                                                  const Fw::CmdStringArg& fileName,
                                                  const Svc::BlockState& block) {
    if (not this->requireRunMode(STOPPED)) {
        if (m_join_waiting) {
            // Inform user previous seq file is not complete
            this->log_WARNING_HI_CS_JoinWaitingNotComplete();
        }
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    if ((Svc::BlockState::BLOCK == block.e) && (MANUAL == this->m_stepMode)) {
        // In MANUAL mode nothing executes until CS_STEP, so a BLOCK response could never be sent
        this->log_WARNING_HI_CS_InvalidMode();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->m_blockState = block.e;
    this->m_cmdSeq = cmdSeq;
    this->m_opCode = opCode;

    // load commands
    if (not this->loadFile(fileName)) {
        // Clear the recorded command state so a later port-driven run cannot
        // emit a duplicate response for this already-answered command
        this->m_blockState = Svc::BlockState::NO_BLOCK;
        this->m_opCode = 0;
        this->m_cmdSeq = 0;
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->m_executedCount = 0;

    // Check the step mode. If it is auto, start the sequence
    if (AUTO == this->m_stepMode) {
        this->m_runMode = RUNNING;
        this->tlmWrite_CS_CurrentSequence(this->m_sequence->getStringFileName());
        if (this->isConnected_seqStartOut_OutputPort(0)) {
            // Create empty SeqArgs as placeholder
            // Use parameterized constructor to ensure m_size is initialized to 0
            Svc::SeqArgs emptyArgs{0, 0};
            this->seqStartOut_out(0, this->m_sequence->getStringFileName(), emptyArgs);
        }
        this->performCmd_Step();
    }

    if (Svc::BlockState::NO_BLOCK == this->m_blockState) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}

void CmdSequencerComponentImpl::CS_VALIDATE_cmdHandler(FwOpcodeType opCode,
                                                       U32 cmdSeq,
                                                       const Fw::CmdStringArg& fileName) {
    FW_ASSERT(this->m_sequence != nullptr);
    if (!this->requireRunMode(STOPPED)) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    // load commands
    if (not this->loadFile(fileName)) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    // clear the buffer
    this->m_sequence->clear();

    this->log_ACTIVITY_HI_CS_SequenceValid(this->m_sequence->getLogFileName());

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

//! Handler for input port seqRunIn
void CmdSequencerComponentImpl::doSequenceRun(const Fw::StringBase& filename) {
    if (MANUAL == this->m_stepMode) {
        // In MANUAL mode nothing executes until CS_STEP, so a port-driven run would wedge
        this->log_WARNING_HI_CS_InvalidMode();
        this->seqDone_out(0, 0, 0, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->requireRunMode(STOPPED)) {
        this->seqDone_out(0, 0, 0, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    // If file name is non-empty, load a file.
    // Empty file name means don't load.
    if (filename != "") {
        Fw::CmdStringArg cmdStr(filename);
        const bool status = this->loadFile(cmdStr);
        if (!status) {
            this->seqDone_out(0, 0, 0, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }
    } else if (not this->m_sequence->hasMoreRecords()) {
        // No sequence loaded
        this->log_WARNING_LO_CS_NoSequenceActive();
        this->error();
        this->seqDone_out(0, 0, 0, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->m_executedCount = 0;

    // Check the step mode. If it is auto, start the sequence
    if (AUTO == this->m_stepMode) {
        this->m_runMode = RUNNING;
        this->tlmWrite_CS_CurrentSequence(this->m_sequence->getStringFileName());
        if (this->isConnected_seqStartOut_OutputPort(0)) {
            // Create empty SeqArgs as placeholder
            // Use parameterized constructor to ensure m_size is initialized to 0
            Svc::SeqArgs emptyArgs{0, 0};
            this->seqStartOut_out(0, this->m_sequence->getStringFileName(), emptyArgs);
        }
        this->performCmd_Step();
    }

    this->log_ACTIVITY_HI_CS_PortSequenceStarted(this->m_sequence->getLogFileName());
}

void CmdSequencerComponentImpl::seqRunIn_handler(FwIndexType portNum,
                                                 const Fw::StringBase& filename,
                                                 const Svc::SeqArgs& args) {
    (void)args;  // Suppress unused parameter warning
    this->doSequenceRun(filename);
}

void CmdSequencerComponentImpl::seqDispatchIn_handler(FwIndexType portNum, Fw::StringBase& file_name) {
    this->doSequenceRun(file_name);
}

void CmdSequencerComponentImpl ::seqCancelIn_handler(const FwIndexType portNum) {
    if (RUNNING == this->m_runMode) {
        this->performCmd_Cancel();
        this->log_ACTIVITY_HI_CS_SequenceCanceled(this->m_sequence->getLogFileName());
        ++this->m_cancelCmdCount;
        this->tlmWrite_CS_CancelCommands(this->m_cancelCmdCount);
    } else {
        this->log_WARNING_LO_CS_NoSequenceActive();
    }
}

void CmdSequencerComponentImpl::CS_CANCEL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (RUNNING == this->m_runMode) {
        this->performCmd_Cancel();
        this->log_ACTIVITY_HI_CS_SequenceCanceled(this->m_sequence->getLogFileName());
        ++this->m_cancelCmdCount;
        this->tlmWrite_CS_CancelCommands(this->m_cancelCmdCount);
    } else {
        this->log_WARNING_LO_CS_NoSequenceActive();
    }
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void CmdSequencerComponentImpl::CS_JOIN_WAIT_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq) {
    // If there is no running sequence do not wait
    if (m_runMode != RUNNING) {
        this->log_WARNING_LO_CS_NoSequenceActive();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    } else if ((Svc::BlockState::BLOCK == this->m_blockState) || this->m_join_waiting) {
        // A command response is already owed to a BLOCK-mode CS_RUN caller or a
        // previous CS_JOIN_WAIT caller. Reject rather than overwrite that state,
        // which would leave the original caller without a completion response.
        this->log_WARNING_HI_CS_JoinWaitingNotComplete();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    } else {
        m_join_waiting = true;
        Fw::LogStringArg& logFileName = this->m_sequence->getLogFileName();
        this->log_ACTIVITY_HI_CS_JoinWaiting(logFileName, m_cmdSeq, CmdDispatcherCfg::getEventOpcode(m_opCode));
        m_cmdSeq = cmdSeq;
        m_opCode = opCode;
    }
}

// ----------------------------------------------------------------------
// Private helper methods
// ----------------------------------------------------------------------

bool CmdSequencerComponentImpl ::loadFile(const Fw::ConstStringBase& fileName) {
    const bool status = this->m_sequence->loadFile(fileName);
    if (status) {
        Fw::LogStringArg& logFileName = this->m_sequence->getLogFileName();
        this->log_ACTIVITY_LO_CS_SequenceLoaded(logFileName);
        ++this->m_loadCmdCount;
        this->tlmWrite_CS_LoadCommands(this->m_loadCmdCount);
    } else {
        // A partial load may have populated m_buffer before an intermediate
        // validation step (e.g. CRC, time, record structure) failed. Without
        // an explicit reset, FPrimeSequence::hasMoreRecords() still returns
        // true because getDeserializeSizeLeft() > 0, so a subsequent CS_START
        // bypasses the "no sequence active" guard and reaches
        // FPrimeSequence::nextRecord, which asserts on the failed deserialize
        // and aborts the FSW. Clearing the sequence on every load failure
        // closes that window.
        this->m_sequence->clear();
    }
    return status;
}

void CmdSequencerComponentImpl::error() {
    ++this->m_errorCount;
    this->tlmWrite_CS_Errors(m_errorCount);
}

void CmdSequencerComponentImpl::performCmd_Cancel() {
    FW_ASSERT(this->m_sequence != nullptr);
    this->m_sequence->reset();
    this->m_runMode = STOPPED;
    this->m_cmdTimer.clear();
    this->m_cmdTimeoutTimer.clear();
    this->m_executedCount = 0;

    // Clear JCF and JCS state
    this->m_jcfActive = false;
    this->m_jcfTarget = "";
    this->m_jcsActive = false;
    this->m_jcsTarget = "";

    // Reset error mode to default (ON)
    this->m_errorMode = true;

    // write sequence done port with error, if connected
    if (this->isConnected_seqDone_OutputPort(0)) {
        this->seqDone_out(0, 0, 0, Fw::CmdResponse::EXECUTION_ERROR);
    }

    if (Svc::BlockState::BLOCK == this->m_blockState || m_join_waiting) {
        // Do not wait if sequence was canceled or a cmd failed
        this->m_join_waiting = false;
        this->cmdResponse_out(this->m_opCode, this->m_cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }

    this->m_blockState = Svc::BlockState::NO_BLOCK;
}

void CmdSequencerComponentImpl ::cmdResponseIn_handler(FwIndexType portNum,
                                                       FwOpcodeType opcode,
                                                       U32 cmdSeq,
                                                       const Fw::CmdResponse& response) {
    if (this->m_runMode == STOPPED) {
        // Sequencer is not running
        this->log_WARNING_HI_CS_UnexpectedCompletion(CmdDispatcherCfg::getEventOpcode(opcode));
    } else {
        // clear command timeout
        this->m_cmdTimeoutTimer.clear();
        if (response != Fw::CmdResponse::OK) {
            // Command failed
            if (this->m_jcfActive) {
                // Jump Command Failure is active, attempt to jump to label
                this->commandError(this->m_executedCount, opcode, response.e);

                if (this->jumpToLabel(this->m_jcfTarget)) {
                    // Successfully jumped to label
                    this->log_ACTIVITY_HI_CS_SequenceCanceled(this->m_sequence->getLogFileName());

                    // Clear JCF state
                    this->m_jcfActive = false;
                    this->m_jcfTarget = "";

                    // Continue execution from label if in auto mode
                    if (this->m_runMode == RUNNING && this->m_stepMode == AUTO) {
                        if (this->m_sequence->hasMoreRecords()) {
                            this->performCmd_Step();
                        } else {
                            this->m_runMode = STOPPED;
                            this->sequenceComplete();
                        }
                    }
                } else {
                    // Label not found, abort sequence
                    this->log_WARNING_HI_CS_CommandError(this->m_sequence->getLogFileName(),
                                                         this->m_executedCount,
                                                         CmdDispatcherCfg::getEventOpcode(opcode),
                                                         response.e);
                    this->m_jcfActive = false;
                    this->m_jcfTarget = "";
                    this->performCmd_Cancel();
                }
            } else if (this->m_errorMode) {
                // Error mode is ON (abort on error) and no JCF active, abort sequence
                this->commandError(this->m_executedCount, opcode, response.e);
                this->performCmd_Cancel();
            } else {
                // Error mode is OFF (continue on error), log error but continue
                this->commandError(this->m_executedCount, opcode, response.e);

                if (this->m_runMode == RUNNING && this->m_stepMode == AUTO) {
                    // Auto mode - continue to next command
                    if (not this->m_sequence->hasMoreRecords()) {
                        // No data left
                        this->m_runMode = STOPPED;
                        this->sequenceComplete();
                    } else {
                        this->performCmd_Step();
                    }
                } else {
                    // Manual step mode - wait for next step command
                    if (not this->m_sequence->hasMoreRecords()) {
                        this->m_runMode = STOPPED;
                        this->sequenceComplete();
                    }
                }
            }
        } else {
            // Command succeeded
            // Clear any active JCF
            this->m_jcfActive = false;
            this->m_jcfTarget = "";

            // Check if JCS (Jump Command Success) is active
            if (this->m_jcsActive) {
                // Jump Command Success is active, attempt to jump to label
                this->commandComplete(opcode);

                if (this->jumpToLabel(this->m_jcsTarget)) {
                    // Successfully jumped to label
                    this->log_ACTIVITY_HI_CS_SequenceCanceled(this->m_sequence->getLogFileName());

                    // Clear JCS state
                    this->m_jcsActive = false;
                    this->m_jcsTarget = "";

                    // Continue execution from label if in auto mode
                    if (this->m_runMode == RUNNING && this->m_stepMode == AUTO) {
                        if (this->m_sequence->hasMoreRecords()) {
                            this->performCmd_Step();
                        } else {
                            this->m_runMode = STOPPED;
                            this->sequenceComplete();
                        }
                    }
                } else {
                    // Label not found, abort sequence
                    this->log_WARNING_HI_CS_CommandError(this->m_sequence->getLogFileName(),
                                                         this->m_executedCount,
                                                         CmdDispatcherCfg::getEventOpcode(opcode),
                                                         0);  // No error code for success case
                    this->m_jcsActive = false;
                    this->m_jcsTarget = "";
                    this->performCmd_Cancel();
                }
            } else {
                // No JCS active, continue normally
                if (this->m_runMode == RUNNING && this->m_stepMode == AUTO) {
                    // Auto mode
                    this->commandComplete(opcode);
                    if (not this->m_sequence->hasMoreRecords()) {
                        // No data left
                        this->m_runMode = STOPPED;
                        this->sequenceComplete();
                    } else {
                        this->performCmd_Step();
                    }
                } else {
                    // Manual step mode
                    this->commandComplete(opcode);
                    if (not this->m_sequence->hasMoreRecords()) {
                        this->m_runMode = STOPPED;
                        this->sequenceComplete();
                    }
                }
            }
        }
    }
}

void CmdSequencerComponentImpl ::schedIn_handler(FwIndexType portNum, U32 order) {
    Fw::Time currTime = this->getTime();
    // check to see if a command time is pending
    if (this->m_cmdTimer.isExpiredAt(currTime)) {
        this->comCmdOut_out(0, m_record.m_command, 0);
        this->m_cmdTimer.clear();
        // start command timeout timer
        this->setCmdTimeout(currTime);
    } else if (this->m_cmdTimeoutTimer.isExpiredAt(this->getTime())) {  // check for command timeout
        this->log_WARNING_HI_CS_SequenceTimeout(m_sequence->getLogFileName(), this->m_executedCount);
        // If there is a command timeout, cancel the sequence
        this->performCmd_Cancel();
    }
}

void CmdSequencerComponentImpl ::CS_START_cmdHandler(FwOpcodeType opcode, U32 cmdSeq) {
    if (not this->m_sequence->hasMoreRecords()) {
        // No sequence loaded
        this->log_WARNING_LO_CS_NoSequenceActive();
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }
    if (!this->requireRunMode(STOPPED)) {
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    }

    this->m_blockState = Svc::BlockState::NO_BLOCK;
    this->m_runMode = RUNNING;
    this->tlmWrite_CS_CurrentSequence(this->m_sequence->getStringFileName());
    this->log_ACTIVITY_HI_CS_CmdStarted(this->m_sequence->getLogFileName());
    this->performCmd_Step();
    if (this->isConnected_seqStartOut_OutputPort(0)) {
        // Create empty SeqArgs as placeholder
        Svc::SeqArgs emptyArgs{0, 0};
        this->seqStartOut_out(0, this->m_sequence->getStringFileName(), emptyArgs);
    }
    this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::OK);
}

void CmdSequencerComponentImpl ::CS_STEP_cmdHandler(FwOpcodeType opcode, U32 cmdSeq) {
    FW_ASSERT(this->m_sequence != nullptr);
    if (this->requireRunMode(RUNNING)) {
        if (MANUAL != this->m_stepMode) {
            // CS_STEP is valid only in MANUAL step mode
            this->log_WARNING_HI_CS_InvalidMode();
            this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }
        if (not this->m_sequence->hasMoreRecords()) {
            // A sequence with no end-of-sequence record leaves nothing to step; stepping anyway
            // asserts in the sequence reader
            this->log_WARNING_LO_CS_NoSequenceActive();
            this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }
        this->performCmd_Step();
        // check for special case where end of sequence entry was encountered
        if (this->m_runMode != STOPPED) {
            this->log_ACTIVITY_HI_CS_CmdStepped(this->m_sequence->getLogFileName(), this->m_executedCount);
        }
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

void CmdSequencerComponentImpl ::CS_AUTO_cmdHandler(FwOpcodeType opcode, U32 cmdSeq) {
    if (this->requireRunMode(STOPPED)) {
        this->m_stepMode = AUTO;
        this->log_ACTIVITY_HI_CS_ModeSwitched(CmdSequencer_SeqMode::AUTO);
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

void CmdSequencerComponentImpl ::CS_MANUAL_cmdHandler(FwOpcodeType opcode, U32 cmdSeq) {
    if (this->requireRunMode(STOPPED)) {
        this->m_stepMode = MANUAL;
        this->log_ACTIVITY_HI_CS_ModeSwitched(CmdSequencer_SeqMode::STEP);
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::OK);
    } else {
        this->cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

// ----------------------------------------------------------------------
// Helper methods
// ----------------------------------------------------------------------

bool CmdSequencerComponentImpl::requireRunMode(RunMode mode) {
    if (this->m_runMode == mode) {
        return true;
    } else {
        this->log_WARNING_HI_CS_InvalidMode();
        return false;
    }
}

void CmdSequencerComponentImpl ::commandError(const U32 number, const FwOpcodeType opCode, const U32 error) {
    this->log_WARNING_HI_CS_CommandError(this->m_sequence->getLogFileName(), number,
                                         CmdDispatcherCfg::getEventOpcode(opCode), error);
    this->error();
}

void CmdSequencerComponentImpl::performCmd_Step() {
    this->m_sequence->nextRecord(m_record);
    // set clock time base and context from value set when sequence was loaded
    const Sequence::Header& header = this->m_sequence->getHeader();
    this->m_record.m_timeTag.setTimeBase(header.m_timeBase);
    this->m_record.m_timeTag.setTimeContext(header.m_timeContext);

    Fw::Time currentTime = this->getTime();
    switch (this->m_record.m_descriptor) {
        case Sequence::Record::END_OF_SEQUENCE:
            this->m_runMode = STOPPED;
            this->sequenceComplete();
            break;
        case Sequence::Record::RELATIVE:
            this->performCmd_Step_RELATIVE(currentTime);
            break;
        case Sequence::Record::ABSOLUTE:
            this->performCmd_Step_ABSOLUTE(currentTime);
            break;
        case Sequence::Record::SEQUENCE_DIRECTIVE:
            // Execute the directive
            if (!this->executeDirective(m_record)) {
                // Directive execution failed, abort sequence
                this->performCmd_Cancel();
            } else if (this->m_runMode == RUNNING && this->m_stepMode == AUTO) {
                // Directive executed successfully in auto mode, continue to next record
                if (this->m_sequence->hasMoreRecords()) {
                    this->performCmd_Step();
                } else {
                    this->m_runMode = STOPPED;
                    this->sequenceComplete();
                }
            }
            break;
        default:
            FW_ASSERT(false, m_record.m_descriptor);
    }
}

void CmdSequencerComponentImpl::sequenceComplete() {
    this->sequenceComplete(Fw::CmdResponse::OK);
}

void CmdSequencerComponentImpl::sequenceComplete(const Fw::CmdResponse& status) {
    FW_ASSERT(this->m_sequence != nullptr);
    ++this->m_sequencesCompletedCount;
    // reset buffer
    this->m_sequence->clear();
    this->log_ACTIVITY_HI_CS_SequenceComplete(this->m_sequence->getLogFileName());
    this->tlmWrite_CS_SequencesCompleted(this->m_sequencesCompletedCount);
    this->m_executedCount = 0;

    // Reset error mode to default (ON)
    this->m_errorMode = true;

    // write sequence done port, if connected
    if (this->isConnected_seqDone_OutputPort(0)) {
        this->seqDone_out(0, 0, 0, status);
    }

    if (Svc::BlockState::BLOCK == this->m_blockState || m_join_waiting) {
        this->cmdResponse_out(this->m_opCode, this->m_cmdSeq, status);
    }

    m_join_waiting = false;
    this->m_blockState = Svc::BlockState::NO_BLOCK;
    this->tlmWrite_CS_CurrentSequence(NO_SEQ);
}

void CmdSequencerComponentImpl::commandComplete(const FwOpcodeType opcode) {
    this->log_ACTIVITY_LO_CS_CommandComplete(this->m_sequence->getLogFileName(), this->m_executedCount,
                                             CmdDispatcherCfg::getEventOpcode(opcode));
    ++this->m_executedCount;
    ++this->m_totalExecutedCount;
    this->tlmWrite_CS_CommandsExecuted(this->m_totalExecutedCount);
}

void CmdSequencerComponentImpl ::performCmd_Step_RELATIVE(Fw::Time& currentTime) {
    this->m_record.m_timeTag.add(currentTime.getSeconds(), currentTime.getUSeconds());
    this->performCmd_Step_ABSOLUTE(currentTime);
}

void CmdSequencerComponentImpl ::performCmd_Step_ABSOLUTE(Fw::Time& currentTime) {
    if (currentTime >= this->m_record.m_timeTag) {
        this->comCmdOut_out(0, m_record.m_command, 0);
        this->setCmdTimeout(currentTime);
    } else {
        this->m_cmdTimer.set(this->m_record.m_timeTag);
    }
}

void CmdSequencerComponentImpl ::pingIn_handler(FwIndexType portNum, /*!< The port number*/
                                                U32 key              /*!< Value to return to pinger*/
) {
    // send ping response
    this->pingOut_out(0, key);
}

void CmdSequencerComponentImpl ::setCmdTimeout(const Fw::Time& currentTime) {
    // start timeout timer if enabled and not in step mode
    if ((this->m_timeout > 0) and (AUTO == this->m_stepMode)) {
        Fw::Time expTime = currentTime;
        expTime.add(this->m_timeout, 0);
        this->m_cmdTimeoutTimer.set(expTime);
    }
}

bool CmdSequencerComponentImpl ::executeDirective(const Sequence::Record& record) {
    // Extract directive ID from command buffer
    Fw::ExternalSerializeBuffer dirBuf(const_cast<U8*>(record.m_command.getBuffAddr()),
                                       record.m_command.getSize());
    dirBuf.setBuffLen(record.m_command.getSize());

    U8 directiveId;
    Fw::SerializeStatus status = dirBuf.deserializeTo(directiveId);
    if (status != Fw::FW_SERIALIZE_OK) {
        this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
        this->error();
        return false;
    }

    // Validate directive ID
    if (directiveId > Sequence::Record::ERROR_MODE) {
        this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, directiveId);
        this->error();
        return false;
    }

    Sequence::Record::DirectiveId directive = static_cast<Sequence::Record::DirectiveId>(directiveId);

    switch (directive) {
        case Sequence::Record::LABEL: {
            // LABEL is a no-op at execution time
            // It just marks a position for jumping
            break;
        }
        case Sequence::Record::JCF: {
            // JCF: Jump Command Failure
            // Check if this is before any command has executed
            if (this->m_executedCount == 0) {
                this->log_WARNING_HI_CS_InvalidMode();
                this->error();
                return false;
            }

            // Extract the target label name
            U8 labelLen;
            status = dirBuf.deserializeTo(labelLen);
            if (status != Fw::FW_SERIALIZE_OK) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
                this->error();
                return false;
            }

            // Read label string
            char labelBuf[21];  // Max 20 chars + null terminator
            FwSizeType readSize = labelLen;
            if (readSize > 20) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, static_cast<I32>(readSize));
                this->error();
                return false;
            }

            status = dirBuf.deserializeTo(reinterpret_cast<U8*>(labelBuf), readSize, Fw::Serialization::OMIT_LENGTH);
            if (status != Fw::FW_SERIALIZE_OK) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
                this->error();
                return false;
            }
            labelBuf[readSize] = '\0';

            // Store the JCF target
            this->m_jcfTarget = labelBuf;
            this->m_jcfActive = true;
            break;
        }
        case Sequence::Record::EXIT: {
            // EXIT: Terminate sequence with specified status
            // Extract the status code (0 = OK, 1 = EXECUTION_ERROR)
            U8 exitStatus;
            status = dirBuf.deserializeTo(exitStatus);
            if (status != Fw::FW_SERIALIZE_OK) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
                this->error();
                return false;
            }

            // Validate status code
            if (exitStatus > 1) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, exitStatus);
                this->error();
                return false;
            }

            // Stop the sequence and complete with specified status
            this->m_runMode = STOPPED;
            Fw::CmdResponse exitResponse = (exitStatus == 0) ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR;
            this->sequenceComplete(exitResponse);
            break;
        }
        case Sequence::Record::JCS: {
            // JCS: Jump Command Success
            // Check if this is before any command has executed
            if (this->m_executedCount == 0) {
                this->log_WARNING_HI_CS_InvalidMode();
                this->error();
                return false;
            }

            // Extract the target label name
            U8 labelLen;
            status = dirBuf.deserializeTo(labelLen);
            if (status != Fw::FW_SERIALIZE_OK) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
                this->error();
                return false;
            }

            // Read label string
            char labelBuf[21];  // Max 20 chars + null terminator
            FwSizeType readSize = labelLen;
            if (readSize > 20) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, static_cast<I32>(readSize));
                this->error();
                return false;
            }

            status = dirBuf.deserializeTo(reinterpret_cast<U8*>(labelBuf), readSize, Fw::Serialization::OMIT_LENGTH);
            if (status != Fw::FW_SERIALIZE_OK) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
                this->error();
                return false;
            }
            labelBuf[readSize] = '\0';

            // Store the JCS target
            this->m_jcsTarget = labelBuf;
            this->m_jcsActive = true;
            break;
        }
        case Sequence::Record::ERROR_MODE: {
            // ERROR_MODE: Control whether sequence aborts on command failure
            // Extract the mode (0 = off/continue, 1 = on/abort)
            U8 mode;
            status = dirBuf.deserializeTo(mode);
            if (status != Fw::FW_SERIALIZE_OK) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, status);
                this->error();
                return false;
            }

            // Validate mode
            if (mode > 1) {
                this->log_WARNING_HI_CS_RecordInvalid(this->m_sequence->getLogFileName(), this->m_executedCount, mode);
                this->error();
                return false;
            }

            // Set error mode: 1 = ON (abort on error), 0 = OFF (continue on error)
            this->m_errorMode = (mode == 1);
            break;
        }
        default:
            FW_ASSERT(false, directive);
    }

    return true;
}

bool CmdSequencerComponentImpl ::jumpToLabel(const Fw::StringBase& labelName) {
    FW_ASSERT(this->m_sequence != nullptr);

    // Reset to beginning of sequence to search for label
    this->m_sequence->reset();

    // Search through all records looking for matching LABEL directive
    while (this->m_sequence->hasMoreRecords()) {
        Sequence::Record searchRecord;
        this->m_sequence->nextRecord(searchRecord);

        if (searchRecord.m_descriptor == Sequence::Record::SEQUENCE_DIRECTIVE) {
            // Parse the directive
            Fw::ExternalSerializeBuffer dirBuf(const_cast<U8*>(searchRecord.m_command.getBuffAddr()),
                                               searchRecord.m_command.getSize());
            dirBuf.setBuffLen(searchRecord.m_command.getSize());

            U8 directiveId;
            Fw::SerializeStatus status = dirBuf.deserializeTo(directiveId);
            if (status != Fw::FW_SERIALIZE_OK) {
                continue;  // Skip malformed directive
            }

            if (directiveId == Sequence::Record::LABEL) {
                // Extract label name
                U8 labelLen;
                status = dirBuf.deserializeTo(labelLen);
                if (status != Fw::FW_SERIALIZE_OK || labelLen > 20) {
                    continue;
                }

                char labelBuf[21];
                FwSizeType readSize = labelLen;
                status = dirBuf.deserializeTo(reinterpret_cast<U8*>(labelBuf), readSize,
                                              Fw::Serialization::OMIT_LENGTH);
                if (status != Fw::FW_SERIALIZE_OK) {
                    continue;
                }
                labelBuf[readSize] = '\0';

                // Check if this is the label we're looking for
                if (labelName == labelBuf) {
                    // Found it! The next record will be executed
                    return true;
                }
            }
        }
    }

    // Label not found
    return false;
}

}  // namespace Svc
