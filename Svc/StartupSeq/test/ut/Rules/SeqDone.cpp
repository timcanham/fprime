// ======================================================================
// \title  SeqDone.cpp
// \author tcanham
// \brief  Rule implementations for the SeqDone rule group
//
// These rules exercise the seqDoneIn port, on which the paired sequencer
// reports the outcome of the requested startup sequence
// (REQ-StartupSeq-031, -032, -033).
// ======================================================================

#include "STest/Pick/Pick.hpp"
#include "Svc/StartupSeq/test/ut/StartupSeqTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// SeqDone.Success
// ----------------------------------------------------------------------

bool StartupSeqTester::SeqDone__Success__precondition() const {
    // Only meaningful while the component is waiting for an outcome
    return this->shadow.shadow_isSequenceRunning();
}

void StartupSeqTester::SeqDone__Success__action() {
    this->clearHistory();

    this->shadow.shadow_state = State::COMPLETED;

    this->reportSequenceDone(Fw::CmdResponse::OK);

    this->assertStateMatchesShadow("SeqDone.Success");
    ASSERT_EVENTS_SequenceCompleted_SIZE(1);
    ASSERT_EVENTS_SequenceFailed_SIZE(0);
    this->assertNoSequenceRequested();
}

// ----------------------------------------------------------------------
// SeqDone.Failure
// ----------------------------------------------------------------------

bool StartupSeqTester::SeqDone__Failure__precondition() const {
    return this->shadow.shadow_isSequenceRunning();
}

void StartupSeqTester::SeqDone__Failure__action() {
    this->clearHistory();

    // Any response other than OK is a failure
    static const Fw::CmdResponse::T failures[] = {
        Fw::CmdResponse::EXECUTION_ERROR,
        Fw::CmdResponse::VALIDATION_ERROR,
        Fw::CmdResponse::INVALID_OPCODE,
        Fw::CmdResponse::BUSY,
    };
    const U32 index = STest::Pick::lowerUpper(0, FW_NUM_ARRAY_ELEMENTS(failures) - 1);
    const Fw::CmdResponse response(failures[index]);

    this->shadow.shadow_state = State::FAILED;

    this->reportSequenceDone(response);

    this->assertStateMatchesShadow("SeqDone.Failure");
    ASSERT_EVENTS_SequenceFailed_SIZE(1);
    ASSERT_EVENTS_SequenceFailed(0, response);
    ASSERT_EVENTS_SequenceCompleted_SIZE(0);
    this->assertNoSequenceRequested();
}

}  // namespace Svc
