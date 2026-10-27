// ======================================================================
// \title  Commands.cpp
// \author tcanham
// \brief  Rule implementations for the Commands rule group
//
// These rules exercise the three ground commands and the state-dependent
// accept/reject matrix of REQ-StartupSeq-019, -020, -021, -034 and -035.
// ======================================================================

#include "STest/Pick/Pick.hpp"
#include "Svc/StartupSeq/test/ut/StartupSeqTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Commands.RunNow
//
// RUN_NOW is valid in every state but STARTED (REQ-StartupSeq-019, -034).
// ----------------------------------------------------------------------

bool StartupSeqTester::Commands__RunNow__precondition() const {
    return !this->shadow.shadow_isSequenceRunning();
}

void StartupSeqTester::Commands__RunNow__action() {
    this->clearHistory();

    const bool hasFile = this->shadow.shadow_hasSequenceFile();

    // RUN_NOW bypasses both preconditions, but still needs a sequence to run
    if (hasFile) {
        this->shadow.shadow_state = State::STARTED;
    } else {
        this->shadow.shadow_state = State::DISABLED;
    }

    this->sendRunNow();

    this->assertStateMatchesShadow("Commands.RunNow");

    if (hasFile) {
        this->assertSequenceRequested(this->shadow.shadow_sequenceFile.toChar());
        this->assertCommandAccepted(StartupSeqComponentBase::OPCODE_RUN_NOW);
        ASSERT_EVENTS_SequenceRequestedByCommand_SIZE(1);
    } else {
        // No sequence to run: the command fails, and the reason is reported by
        // NoSequenceFile rather than by InvalidCommand
        this->assertNoSequenceRequested();
        ASSERT_CMD_RESPONSE_SIZE(1);
        ASSERT_CMD_RESPONSE(0, StartupSeqComponentBase::OPCODE_RUN_NOW, this->cmdSeq - 1,
                            Fw::CmdResponse::EXECUTION_ERROR);
        ASSERT_EVENTS_NoSequenceFile_SIZE(1);
    }
}

// ----------------------------------------------------------------------
// Commands.Disable
//
// DISABLE cancels a pending request, and is valid only while one is pending
// (REQ-StartupSeq-020, -035).
// ----------------------------------------------------------------------

bool StartupSeqTester::Commands__Disable__precondition() const {
    return this->shadow.shadow_isRequestPending();
}

void StartupSeqTester::Commands__Disable__action() {
    this->clearHistory();

    this->shadow.shadow_state = State::DISABLED;

    this->sendDisable();

    this->assertStateMatchesShadow("Commands.Disable");
    this->assertCommandAccepted(StartupSeqComponentBase::OPCODE_DISABLE);
    ASSERT_EVENTS_StartupCancelled_SIZE(1);
    this->assertNoSequenceRequested();
}

// ----------------------------------------------------------------------
// Commands.Reset
//
// RESET re-arms and clears both accumulators; valid in every state but
// STARTED (REQ-StartupSeq-021, -034).
// ----------------------------------------------------------------------

bool StartupSeqTester::Commands__Reset__precondition() const {
    return !this->shadow.shadow_isSequenceRunning();
}

void StartupSeqTester::Commands__Reset__action() {
    this->clearHistory();

    this->shadow.shadow_rearm();

    this->sendReset();

    this->assertStateMatchesShadow("Commands.Reset");
    this->assertCommandAccepted(StartupSeqComponentBase::OPCODE_RESET);
    ASSERT_EVENTS_StartupReset_SIZE(1);
    this->assertNoSequenceRequested();

    // The elapsed-time channel must reflect the cleared accumulator on the next tick
    ASSERT_EQ(this->shadow.shadow_elapsedCycles, 0u);
}

// ----------------------------------------------------------------------
// Commands.RejectedWhileStarted
//
// While a requested sequence is in flight, every command is rejected
// (REQ-StartupSeq-034).
// ----------------------------------------------------------------------

bool StartupSeqTester::Commands__RejectedWhileStarted__precondition() const {
    return this->shadow.shadow_isSequenceRunning();
}

void StartupSeqTester::Commands__RejectedWhileStarted__action() {
    // Pick one of the three commands at random; all three must be rejected
    const U32 choice = STest::Pick::lowerUpper(0, 2);

    this->clearHistory();

    FwOpcodeType expectedOpCode = StartupSeqComponentBase::OPCODE_RUN_NOW;
    switch (choice) {
        case 0:
            expectedOpCode = StartupSeqComponentBase::OPCODE_RUN_NOW;
            this->sendRunNow();
            break;
        case 1:
            expectedOpCode = StartupSeqComponentBase::OPCODE_DISABLE;
            this->sendDisable();
            break;
        default:
            expectedOpCode = StartupSeqComponentBase::OPCODE_RESET;
            this->sendReset();
            break;
    }

    // The state must not have moved
    this->assertStateMatchesShadow("Commands.RejectedWhileStarted");
    this->assertCommandRejected(expectedOpCode, State::STARTED);
    this->assertNoSequenceRequested();
}

}  // namespace Svc
