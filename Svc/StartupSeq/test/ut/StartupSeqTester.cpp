// ======================================================================
// \title  StartupSeqTester.cpp
// \author tcanham
// \brief  cpp file for StartupSeq component test harness implementation class
// ======================================================================

#include "STest/Pick/Pick.hpp"
#include "Svc/StartupSeq/test/ut/StartupSeqTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

StartupSeqTester ::StartupSeqTester()
    : StartupSeqGTestBase("StartupSeqTester", StartupSeqTester::MAX_HISTORY_SIZE),
      component("StartupSeq"),
      cmdSeq(0) {
    this->initComponents();
    this->connectPorts();
}

StartupSeqTester ::~StartupSeqTester() {
    // Tear down the message queue allocated by component.init(), matching the
    // convention of the other active-component testers in Svc
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Setup helpers
// ----------------------------------------------------------------------

void StartupSeqTester ::setup(TimeBase requiredTimeBase,
                              const char* sequenceFile,
                              U32 maxWaitSeconds,
                              U32 delaySeconds,
                              U32 rateGroupPeriodMs) {
    // Configure the component and mirror the same values into the shadow
    this->component.configure(requiredTimeBase, rateGroupPeriodMs);
    this->shadow.shadow_requiredTimeBase = requiredTimeBase;
    this->shadow.shadow_rateGroupPeriodMs = rateGroupPeriodMs;

    // Seed the parameter database before loading, so the component starts from
    // these values rather than from the model defaults
    const Fw::ParamString fileParam(sequenceFile);
    this->paramSet_SequenceFile(fileParam, Fw::ParamValid::VALID);
    this->paramSet_MaxTimeBaseWaitSeconds(maxWaitSeconds, Fw::ParamValid::VALID);
    this->paramSet_StartupDelaySeconds(delaySeconds, Fw::ParamValid::VALID);
    this->component.loadParameters();

    this->shadow.shadow_sequenceFile = fileParam;
    this->shadow.shadow_maxWaitSeconds = maxWaitSeconds;
    this->shadow.shadow_delaySeconds = delaySeconds;

    // The time source reports no time base until a test establishes one
    this->setCurrentTimeBase(TimeBase::TB_NONE);

    this->shadow.shadow_state = State::WAITING;
    this->shadow.shadow_elapsedCycles = 0;
    this->shadow.shadow_delayStartCycles = 0;
    this->shadow.shadow_parametersStale = false;
}

void StartupSeqTester ::setCurrentTimeBase(TimeBase timeBase) {
    Fw::Time timeTag;
    timeTag.set(timeBase, 0, 0, 0);
    this->setTestTime(timeTag);
    this->shadow.shadow_currentTimeBase = timeBase;
}

void StartupSeqTester ::setSequenceFileParam(const char* sequenceFile) {
    const Fw::ParamString fileParam(sequenceFile);
    this->paramSet_SequenceFile(fileParam, Fw::ParamValid::VALID);
    this->paramSend_SequenceFile(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
    this->drainQueue();
    // The component picks the new value up on its next tick, so the shadow keeps
    // acting on the old value until then
    this->shadow.shadow_parametersStale = true;
}

void StartupSeqTester ::setDelayParam(U32 delaySeconds) {
    this->paramSet_StartupDelaySeconds(delaySeconds, Fw::ParamValid::VALID);
    this->paramSend_StartupDelaySeconds(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
    this->drainQueue();
    this->shadow.shadow_parametersStale = true;
}

void StartupSeqTester ::setMaxWaitParam(U32 maxWaitSeconds) {
    this->paramSet_MaxTimeBaseWaitSeconds(maxWaitSeconds, Fw::ParamValid::VALID);
    this->paramSend_MaxTimeBaseWaitSeconds(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
    this->drainQueue();
    this->shadow.shadow_parametersStale = true;
}

// ----------------------------------------------------------------------
// Action helpers
// ----------------------------------------------------------------------

void StartupSeqTester ::drainQueue() {
    // Each port invocation or command is one queued message, and the state machine
    // signal it sends is a second. Dispatch until the queue is empty so a single
    // test action runs to completion.
    for (FwSizeType i = 0; i < StartupSeqTester::MAX_DISPATCH_ITERATIONS; i++) {
        if (this->component.m_queue.getMessagesAvailable() == 0) {
            return;
        }
        this->component.doDispatch();
    }
    FW_ASSERT(0, static_cast<FwAssertArgType>(StartupSeqTester::MAX_DISPATCH_ITERATIONS));
}

void StartupSeqTester ::tick() {
    this->invoke_to_schedIn(0, 0);
    this->drainQueue();
}

void StartupSeqTester ::tickTimes(U32 n) {
    for (U32 i = 0; i < n; i++) {
        this->tick();
    }
}

void StartupSeqTester ::reportSequenceDone(Fw::CmdResponse response) {
    this->invoke_to_seqDoneIn(0, 0, 0, response);
    this->drainQueue();
}

void StartupSeqTester ::sendRunNow() {
    this->sendCmd_RUN_NOW(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
    this->drainQueue();
}

void StartupSeqTester ::sendDisable() {
    this->sendCmd_DISABLE(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
    this->drainQueue();
}

void StartupSeqTester ::sendReset() {
    this->sendCmd_RESET(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
    this->drainQueue();
}

void StartupSeqTester ::sendPing(U32 key) {
    this->invoke_to_pingIn(0, key);
    this->drainQueue();
}

// ----------------------------------------------------------------------
// Assertion helpers
// ----------------------------------------------------------------------

void StartupSeqTester ::assertStateMatchesShadow(const char* context) const {
    const State actual = this->component.startupMachine_getState();
    ASSERT_EQ(actual, this->shadow.shadow_state)
        << context << ": component state " << static_cast<U32>(actual) << " != expected "
        << static_cast<U32>(this->shadow.shadow_state);
}

void StartupSeqTester ::assertNoSequenceRequested() const {
    ASSERT_from_seqRunOut_SIZE(0);
}

void StartupSeqTester ::assertSequenceRequested(const char* expectedFile) const {
    ASSERT_from_seqRunOut_SIZE(1);
    const Svc::SeqArgs emptyArgs;
    ASSERT_from_seqRunOut(0, Fw::String(expectedFile), emptyArgs);
}

void StartupSeqTester ::assertCommandAccepted(FwOpcodeType opCode) const {
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, opCode, this->cmdSeq - 1, Fw::CmdResponse::OK);
    ASSERT_EVENTS_InvalidCommand_SIZE(0);
}

void StartupSeqTester ::assertCommandRejected(FwOpcodeType opCode, State expectedState) const {
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, opCode, this->cmdSeq - 1, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_InvalidCommand_SIZE(1);
    ASSERT_EVENTS_InvalidCommand(0, opCode, expectedState);
}

// ----------------------------------------------------------------------
// Traditional tests
// ----------------------------------------------------------------------

void StartupSeqTester ::testDontCareRunsImmediately() {
    // TB_DONT_CARE and a zero delay: the first tick should request the sequence
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 0);
    this->clearHistory();

    this->tick();

    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
    ASSERT_EVENTS_SequenceRequested_SIZE(1);
    ASSERT_EVENTS_SequenceRequested(0, TEST_SEQUENCE_FILE);
    // No time base was waited for, so no waiting event should appear
    ASSERT_EVENTS_WaitingForTimeBase_SIZE(0);
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(0);
}

void StartupSeqTester ::testWaitsForRequiredTimeBase() {
    // Require TB_SC_TIME, which the time source does not report yet
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 60, 0);
    this->clearHistory();

    // Several ticks with the wrong time base must not request anything
    this->tickTimes(3);
    ASSERT_EQ(this->component.startupMachine_getState(), State::WAITING);
    this->assertNoSequenceRequested();
    ASSERT_EVENTS_SequenceRequested_SIZE(0);

    // Establish the required time base; the next tick should start the sequence
    this->setCurrentTimeBase(TimeBase::TB_SC_TIME);
    this->clearHistory();
    this->tick();

    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
    ASSERT_EVENTS_TimeBaseEstablished_SIZE(1);
    // The event must report the REQUIRED time base
    ASSERT_EVENTS_TimeBaseEstablished(0, TimeBase::TB_SC_TIME, 0);
}

void StartupSeqTester ::testTimeBaseWaitTimeout() {
    // Max wait of 3 s at 1000 ms per tick: the 3rd tick should time out
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 3, 0);
    this->clearHistory();

    this->tickTimes(2);
    ASSERT_EQ(this->component.startupMachine_getState(), State::WAITING)
        << "Must still be waiting before the max wait elapses";

    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::TIMED_OUT);
    this->assertNoSequenceRequested();
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(1);
    ASSERT_EVENTS_TimeBaseWaitTimeout(0, TimeBase::TB_SC_TIME, TimeBase::TB_NONE, 3);

    // Further ticks must not act (terminal state)
    this->clearHistory();
    this->tickTimes(5);
    ASSERT_EQ(this->component.startupMachine_getState(), State::TIMED_OUT);
    this->assertNoSequenceRequested();
}

void StartupSeqTester ::testStartupDelay() {
    // No time base precondition, 3 s delay at 1000 ms per tick
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 3);
    this->clearHistory();

    // First tick satisfies the time base precondition and begins the delay
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::DELAYING);
    this->assertNoSequenceRequested();
    ASSERT_EVENTS_TimeBaseEstablished_SIZE(1);
    ASSERT_EVENTS_TimeBaseEstablished(0, TimeBase::TB_DONT_CARE, 3);

    // Two more ticks: still delaying (2 s of a 3 s delay)
    this->tickTimes(2);
    ASSERT_EQ(this->component.startupMachine_getState(), State::DELAYING);
    this->assertNoSequenceRequested();

    // Third tick of the delay completes it
    this->clearHistory();
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
}

void StartupSeqTester ::testZeroDelayRequestsOnSameTick() {
    // A zero delay must mean no delay, not one tick of delay
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 60, 0);
    this->setCurrentTimeBase(TimeBase::TB_SC_TIME);
    this->clearHistory();

    this->tick();

    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED)
        << "A zero delay must request on the same tick that satisfies the time base";
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
}

void StartupSeqTester ::testEmptySequenceFileDisables() {
    // An unconfigured component must report itself on the first tick, not after
    // the full time base wait
    this->setup(TimeBase::TB_SC_TIME, "", 60, 0);
    this->clearHistory();

    this->tick();

    ASSERT_EQ(this->component.startupMachine_getState(), State::DISABLED);
    this->assertNoSequenceRequested();
    ASSERT_EVENTS_NoSequenceFile_SIZE(1);
    // The component must not claim to be waiting for a time base it will never use
    ASSERT_EVENTS_WaitingForTimeBase_SIZE(0);
}

void StartupSeqTester ::testSequenceFileClearedDuringDelay() {
    // Clearing the sequence file mid-delay must prevent the request: an empty
    // name makes the sequencer run whichever sequence is already resident
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 5);

    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::DELAYING);

    // Clear the parameter while the delay is still running
    this->setSequenceFileParam("");
    this->clearHistory();

    // The next tick must detect the cleared file and refuse to request
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::DISABLED);
    this->assertNoSequenceRequested();
    ASSERT_EVENTS_SequenceFileClearedWhileDelaying_SIZE(1);

    // Even after the delay would have elapsed, nothing is requested
    this->clearHistory();
    this->tickTimes(10);
    this->assertNoSequenceRequested();
}

void StartupSeqTester ::testSequenceCompleted() {
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 0);
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);

    this->clearHistory();
    this->reportSequenceDone(Fw::CmdResponse::OK);

    ASSERT_EQ(this->component.startupMachine_getState(), State::COMPLETED);
    ASSERT_EVENTS_SequenceCompleted_SIZE(1);
    ASSERT_EVENTS_SequenceFailed_SIZE(0);
}

void StartupSeqTester ::testSequenceFailed() {
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 0);
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);

    this->clearHistory();
    this->reportSequenceDone(Fw::CmdResponse::EXECUTION_ERROR);

    ASSERT_EQ(this->component.startupMachine_getState(), State::FAILED);
    ASSERT_EVENTS_SequenceFailed_SIZE(1);
    ASSERT_EVENTS_SequenceFailed(0, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_SequenceCompleted_SIZE(0);
}

void StartupSeqTester ::testOneShotPerArming() {
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 0);
    this->clearHistory();

    this->tick();
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);

    // Many further ticks, in STARTED and then in COMPLETED, must not re-request
    this->clearHistory();
    this->tickTimes(10);
    this->assertNoSequenceRequested();

    this->reportSequenceDone(Fw::CmdResponse::OK);
    this->clearHistory();
    this->tickTimes(10);
    this->assertNoSequenceRequested();
    ASSERT_EQ(this->component.startupMachine_getState(), State::COMPLETED);
}

void StartupSeqTester ::testRunNowFromTerminalStates() {
    // From TIMED_OUT: RUN_NOW is the documented recovery path
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 1, 0);
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::TIMED_OUT);

    this->clearHistory();
    this->sendRunNow();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
    ASSERT_EVENTS_SequenceRequestedByCommand_SIZE(1);
    this->assertCommandAccepted(StartupSeqComponentBase::OPCODE_RUN_NOW);

    // From COMPLETED: RUN_NOW re-runs the sequence
    this->reportSequenceDone(Fw::CmdResponse::OK);
    ASSERT_EQ(this->component.startupMachine_getState(), State::COMPLETED);
    this->clearHistory();
    this->sendRunNow();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);

    // From FAILED
    this->reportSequenceDone(Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->component.startupMachine_getState(), State::FAILED);
    this->clearHistory();
    this->sendRunNow();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);

    // From DISABLED
    this->reportSequenceDone(Fw::CmdResponse::OK);
    this->sendReset();
    this->sendDisable();
    ASSERT_EQ(this->component.startupMachine_getState(), State::DISABLED);
    this->clearHistory();
    this->sendRunNow();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
}

void StartupSeqTester ::testCommandsRejectedWhileStarted() {
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 0);
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);

    // All three commands must be rejected, each with a response so none hangs
    this->clearHistory();
    this->sendRunNow();
    this->assertCommandRejected(StartupSeqComponentBase::OPCODE_RUN_NOW, State::STARTED);
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);

    this->clearHistory();
    this->sendDisable();
    this->assertCommandRejected(StartupSeqComponentBase::OPCODE_DISABLE, State::STARTED);
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);

    this->clearHistory();
    this->sendReset();
    this->assertCommandRejected(StartupSeqComponentBase::OPCODE_RESET, State::STARTED);
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);

    // No command may have caused a second request
    this->assertNoSequenceRequested();
}

void StartupSeqTester ::testDisableRejectedWithNothingPending() {
    // In COMPLETED there is nothing pending to cancel
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 0);
    this->tick();
    this->reportSequenceDone(Fw::CmdResponse::OK);
    ASSERT_EQ(this->component.startupMachine_getState(), State::COMPLETED);

    this->clearHistory();
    this->sendDisable();
    this->assertCommandRejected(StartupSeqComponentBase::OPCODE_DISABLE, State::COMPLETED);
    ASSERT_EQ(this->component.startupMachine_getState(), State::COMPLETED)
        << "A rejected DISABLE must not change state";
}

void StartupSeqTester ::testResetReArmsAndClearsAccumulators() {
    // Accumulate some wait, then re-arm and confirm the wait restarts from zero
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 5, 0);
    this->tickTimes(3);
    ASSERT_EQ(this->component.startupMachine_getState(), State::WAITING);

    this->clearHistory();
    this->sendReset();
    ASSERT_EQ(this->component.startupMachine_getState(), State::WAITING);
    ASSERT_EVENTS_StartupReset_SIZE(1);
    this->assertCommandAccepted(StartupSeqComponentBase::OPCODE_RESET);

    // Had the accumulator not been cleared, 2 more ticks would hit the 5 s timeout
    this->clearHistory();
    this->tickTimes(2);
    ASSERT_EQ(this->component.startupMachine_getState(), State::WAITING)
        << "RESET must clear the time base wait accumulator";
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(0);
}

void StartupSeqTester ::testResetClearsThrottles() {
    // Time out once, which latches the throttled timeout event
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 1, 0);
    this->clearHistory();
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::TIMED_OUT);
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(1);

    // Re-arm and time out again: the event must be reported a second time
    this->sendReset();
    this->clearHistory();
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::TIMED_OUT);
    // RESET must clear the timeout throttle, so a second timeout is reported
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(1);
}

void StartupSeqTester ::testParameterUpdateAppliedOnNextTick() {
    // Start with a long delay so the component stays in DELAYING
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, 60, 10);
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::DELAYING);

    // Change the sequence file; the new value must be used for the request
    const char* const newFile = "/seq/other.bin";
    this->setSequenceFileParam(newFile);

    // Shorten the delay so the next ticks complete it
    this->setDelayParam(0);
    this->clearHistory();

    // The next tick picks up both updates and requests the NEW file
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(newFile);
}

void StartupSeqTester ::testPingResponds() {
    this->setup();
    this->clearHistory();

    const U32 key = STest::Pick::any();
    this->sendPing(key);

    ASSERT_from_pingOut_SIZE(1);
    ASSERT_from_pingOut(0, key);
}

void StartupSeqTester ::testTelemetryReportsState() {
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 60, 2);
    this->clearHistory();

    // While waiting, the State channel must report WAITING
    this->tick();
    ASSERT_TLM_State_SIZE(1);
    ASSERT_TLM_State(0, State::WAITING);

    // Establish the time base and tick into DELAYING
    this->setCurrentTimeBase(TimeBase::TB_SC_TIME);
    this->tick();
    this->clearHistory();

    // The channel reports the state as of the start of the tick
    this->tick();
    ASSERT_TLM_State_SIZE(1);
    ASSERT_TLM_State(0, State::DELAYING);

    // Elapsed seconds must advance with the ticks
    ASSERT_TLM_ElapsedSeconds_SIZE(1);
}

void StartupSeqTester ::testMaxWaitDoesNotApplyToDelay() {
    // A startup delay (10 s) deliberately longer than the time base timeout (2 s).
    // The timeout must bound only the time base wait, so a long delay must not be
    // misreported as a time base failure.
    this->setup(TimeBase::TB_DONT_CARE, TEST_SEQUENCE_FILE, /* maxWait */ 2, /* delay */ 10);
    this->clearHistory();

    // First tick satisfies the time base precondition immediately
    this->tick();
    ASSERT_EQ(this->component.startupMachine_getState(), State::DELAYING);

    // Tick well past the 2 s timeout but short of the 10 s delay
    this->tickTimes(5);
    ASSERT_EQ(this->component.startupMachine_getState(), State::DELAYING)
        << "The time base timeout must not fire while the startup delay is elapsing";
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(0);
    this->assertNoSequenceRequested();

    // Complete the delay: 1 tick for the time base + 10 ticks of delay
    this->tickTimes(5);
    ASSERT_EQ(this->component.startupMachine_getState(), State::STARTED);
    this->assertSequenceRequested(TEST_SEQUENCE_FILE);
    ASSERT_EVENTS_TimeBaseWaitTimeout_SIZE(0);
}

void StartupSeqTester ::testRunNowWithNoSequenceFile() {
    // RUN_NOW bypasses the time base wait and the delay, but it still needs a
    // sequence to run. With no file configured the command must fail, and the
    // reason must be NoSequenceFile rather than InvalidCommand.
    this->setup(TimeBase::TB_DONT_CARE, "", 60, 0);
    this->clearHistory();

    this->sendRunNow();

    ASSERT_EQ(this->component.startupMachine_getState(), State::DISABLED);
    this->assertNoSequenceRequested();
    ASSERT_EVENTS_NoSequenceFile_SIZE(1);
    // The command failed, but not because it was invalid for the state
    ASSERT_EVENTS_InvalidCommand_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, StartupSeqComponentBase::OPCODE_RUN_NOW, this->cmdSeq - 1,
                        Fw::CmdResponse::EXECUTION_ERROR);
}

void StartupSeqTester ::testTooManyOutstandingCommands() {
    // The component tracks a bounded number of commands awaiting a state machine
    // response. Dispatch more than that without letting it drain, and the excess
    // must be answered BUSY rather than dropped (REQ-StartupSeq-036): a command
    // with no response hangs in the dispatcher.
    this->setup(TimeBase::TB_SC_TIME, TEST_SEQUENCE_FILE, 600, 0);
    this->clearHistory();

    // PENDING_COMMAND_DEPTH is 8; send more than that with no dispatch in between.
    // Rotate across all three opcodes so every handler's overflow path is exercised.
    const U32 numCommands = 12;
    for (U32 i = 0; i < numCommands; i++) {
        switch (i % 3) {
            case 0:
                this->sendCmd_RESET(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
                break;
            case 1:
                this->sendCmd_RUN_NOW(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
                break;
            default:
                this->sendCmd_DISABLE(StartupSeqTester::TEST_INSTANCE_ID, this->cmdSeq++);
                break;
        }
    }

    // Every command must be answered, whether accepted or refused
    this->drainQueue();
    ASSERT_CMD_RESPONSE_SIZE(numCommands);

    // The overflow must be reported, and refused with BUSY
    ASSERT_EVENTS_TooManyCommands_SIZE(numCommands - 8);

    U32 busyCount = 0;
    for (U32 i = 0; i < numCommands; i++) {
        if (this->cmdResponseHistory->at(i).response == Fw::CmdResponse::BUSY) {
            busyCount++;
        }
    }
    ASSERT_EQ(busyCount, numCommands - 8) << "Commands beyond the pending depth must be refused BUSY";
}

}  // namespace Svc
