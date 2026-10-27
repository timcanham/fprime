// ======================================================================
// \title  StartupSeqTester.hpp
// \author tcanham
// \brief  hpp file for StartupSeq component test harness implementation class
// ======================================================================

#ifndef Svc_StartupSeqTester_HPP
#define Svc_StartupSeqTester_HPP

#include "Svc/StartupSeq/StartupSeq.hpp"
#include "Svc/StartupSeq/StartupSeqGTestBase.hpp"
#include "Svc/StartupSeq/test/ut/TestState/TestState.hpp"
#include "TestUtils/RuleBasedTesting.hpp"

namespace Svc {

class StartupSeqTester : public StartupSeqGTestBase {
  public:
    using State = Svc::StartupMachine_State;

    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    //! Maximum size of histories storing events, telemetry, and port outputs
    //!
    //! Large enough that a randomized scenario never silently overflows a
    //! history between clearHistory() calls
    static const FwSizeType MAX_HISTORY_SIZE = 100;

    //! Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    //! Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 20;

    //! Rate group period used by most tests. 1000 ms makes one tick one second,
    //! so a configured delay in seconds equals that many ticks
    static const U32 TEST_RATE_GROUP_PERIOD_MS = 1000;

    //! A valid sequence file name used by tests that need one
    static constexpr const char* TEST_SEQUENCE_FILE = "/seq/startup.bin";

    //! Bound on drainQueue() iterations, so a test cannot spin forever if the
    //! component ever sends signals indefinitely
    static const FwSizeType MAX_DISPATCH_ITERATIONS = 32;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object StartupSeqTester
    StartupSeqTester();

    //! Destroy object StartupSeqTester
    ~StartupSeqTester();

  public:
    // ----------------------------------------------------------------------
    // Setup helpers
    // ----------------------------------------------------------------------

    //! Configure the component and the shadow with the same values, then load
    //! parameters. Call exactly once, before any tick.
    void setup(TimeBase requiredTimeBase = TimeBase::TB_DONT_CARE,
               const char* sequenceFile = TEST_SEQUENCE_FILE,
               U32 maxWaitSeconds = 60,
               U32 delaySeconds = 0,
               U32 rateGroupPeriodMs = TEST_RATE_GROUP_PERIOD_MS);

    //! Set the time base the simulated time source reports, in both the
    //! component's test time and the shadow
    void setCurrentTimeBase(TimeBase timeBase);

    //! Set the SequenceFile parameter on the component and in the shadow
    //!
    //! The shadow records the change as pending: the component applies parameter
    //! updates on its next tick, not immediately (REQ-StartupSeq-038)
    void setSequenceFileParam(const char* sequenceFile);

    //! Set the StartupDelaySeconds parameter on the component and in the shadow
    void setDelayParam(U32 delaySeconds);

    //! Set the MaxTimeBaseWaitSeconds parameter on the component and in the shadow
    void setMaxWaitParam(U32 maxWaitSeconds);

  public:
    // ----------------------------------------------------------------------
    // Action helpers
    // ----------------------------------------------------------------------

    //! Dispatch until the component's queue is empty
    //!
    //! A port invocation or command is one queued message; the state machine signal
    //! it sends is a second. A single doDispatch() would leave the signal unprocessed
    void drainQueue();

    //! Invoke schedIn once and dispatch the resulting messages
    void tick();

    //! Invoke schedIn n times, dispatching after each
    void tickTimes(U32 n);

    //! Report a sequence outcome on seqDoneIn and dispatch
    void reportSequenceDone(Fw::CmdResponse response);

    //! Send a command and dispatch the resulting messages
    void sendRunNow();
    void sendDisable();
    void sendReset();

    //! Invoke pingIn and dispatch
    void sendPing(U32 key);

  public:
    // ----------------------------------------------------------------------
    // Assertion helpers
    // ----------------------------------------------------------------------

    //! Assert the component reports the state the shadow expects
    void assertStateMatchesShadow(const char* context) const;

    //! Assert that no sequence request was emitted
    void assertNoSequenceRequested() const;

    //! Assert that exactly one sequence request was emitted for the given file
    void assertSequenceRequested(const char* expectedFile) const;

    //! Assert the most recent command response was OK
    void assertCommandAccepted(FwOpcodeType opCode) const;

    //! Assert the most recent command was rejected with an InvalidCommand event
    void assertCommandRejected(FwOpcodeType opCode, State expectedState) const;

  public:
    // ----------------------------------------------------------------------
    // Traditional tests
    // ----------------------------------------------------------------------

    void testDontCareRunsImmediately();
    void testWaitsForRequiredTimeBase();
    void testTimeBaseWaitTimeout();
    void testStartupDelay();
    void testZeroDelayRequestsOnSameTick();
    void testEmptySequenceFileDisables();
    void testSequenceFileClearedDuringDelay();
    void testSequenceCompleted();
    void testSequenceFailed();
    void testOneShotPerArming();
    void testRunNowFromTerminalStates();
    void testCommandsRejectedWhileStarted();
    void testDisableRejectedWithNothingPending();
    void testResetReArmsAndClearsAccumulators();
    void testResetClearsThrottles();
    void testParameterUpdateAppliedOnNextTick();
    void testPingResponds();
    void testTelemetryReportsState();
    void testMaxWaitDoesNotApplyToDelay();
    void testRunNowWithNoSequenceFile();
    void testTooManyOutstandingCommands();

  private:
    // ----------------------------------------------------------------------
    // Helper functions (auto-generated via UT_AUTO_HELPERS)
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  public:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    StartupSeq component;

    //! Shadow state for rule-based testing
    StartupSeqTestState shadow;

    //! Command sequence number, incremented per command sent
    U32 cmdSeq;

  public:
    // ----------------------------------------------------------------------
    // Rule Based Testing
    // ----------------------------------------------------------------------

    //! Rules for the schedIn port
    FW_RBT_DEFINE_RULE(StartupSeqTester, Tick, Advance);
    FW_RBT_DEFINE_RULE(StartupSeqTester, Tick, EstablishTimeBase);

    //! Rules for the three commands
    FW_RBT_DEFINE_RULE(StartupSeqTester, Commands, RunNow);
    FW_RBT_DEFINE_RULE(StartupSeqTester, Commands, Disable);
    FW_RBT_DEFINE_RULE(StartupSeqTester, Commands, Reset);
    FW_RBT_DEFINE_RULE(StartupSeqTester, Commands, RejectedWhileStarted);

    //! Rules for the seqDoneIn port
    FW_RBT_DEFINE_RULE(StartupSeqTester, SeqDone, Success);
    FW_RBT_DEFINE_RULE(StartupSeqTester, SeqDone, Failure);
};

}  // namespace Svc

#endif
