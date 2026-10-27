// ======================================================================
// \title  StartupSeqTestMain.cpp
// \author tcanham
// \brief  cpp file for StartupSeq component test main function
// ======================================================================

#include <Fw/Test/UnitTest.hpp>

#include "STest/Random/Random.hpp"
#include "STest/Scenario/BoundedScenario.hpp"
#include "STest/Scenario/RandomScenario.hpp"
#include "Svc/StartupSeq/test/ut/StartupSeqTester.hpp"

using Svc::StartupSeqTester;

// ----------------------------------------------------------------------
// Nominal startup paths
// ----------------------------------------------------------------------

TEST(Nominal, DontCareRunsImmediately) {
    COMMENT("TB_DONT_CARE requests the sequence on the first tick, with no time base wait");
    REQUIREMENT("StartupSeq-007");
    StartupSeqTester tester;
    tester.testDontCareRunsImmediately();
}

TEST(Nominal, WaitsForRequiredTimeBase) {
    COMMENT("The sequence is withheld until the current time base matches the required one");
    REQUIREMENT("StartupSeq-005");
    REQUIREMENT("StartupSeq-006");
    StartupSeqTester tester;
    tester.testWaitsForRequiredTimeBase();
}

TEST(Nominal, StartupDelay) {
    COMMENT("The sequence is withheld until the configured startup delay has elapsed");
    REQUIREMENT("StartupSeq-004");
    REQUIREMENT("StartupSeq-008");
    REQUIREMENT("StartupSeq-009");
    StartupSeqTester tester;
    tester.testStartupDelay();
}

TEST(Nominal, ZeroDelayRequestsOnSameTick) {
    COMMENT("A zero delay requests the sequence on the tick that satisfies the time base");
    REQUIREMENT("StartupSeq-010");
    StartupSeqTester tester;
    tester.testZeroDelayRequestsOnSameTick();
}

TEST(Nominal, SequenceCompleted) {
    COMMENT("A successful sequence outcome moves the component to COMPLETED");
    REQUIREMENT("StartupSeq-031");
    REQUIREMENT("StartupSeq-032");
    StartupSeqTester tester;
    tester.testSequenceCompleted();
}

TEST(Nominal, OneShotPerArming) {
    COMMENT("The sequence is requested at most once per arming");
    REQUIREMENT("StartupSeq-012");
    REQUIREMENT("StartupSeq-013");
    StartupSeqTester tester;
    tester.testOneShotPerArming();
}

TEST(Nominal, TelemetryReportsState) {
    COMMENT("The State and ElapsedSeconds channels track the startup progress");
    REQUIREMENT("StartupSeq-022");
    REQUIREMENT("StartupSeq-023");
    StartupSeqTester tester;
    tester.testTelemetryReportsState();
}

TEST(Nominal, PingResponds) {
    COMMENT("The health ping is answered on the same port index");
    StartupSeqTester tester;
    tester.testPingResponds();
}

// ----------------------------------------------------------------------
// Off-nominal paths
// ----------------------------------------------------------------------

TEST(OffNominal, TimeBaseWaitTimeout) {
    COMMENT("The component gives up, without requesting the sequence, after the max wait");
    REQUIREMENT("StartupSeq-003");
    REQUIREMENT("StartupSeq-014");
    REQUIREMENT("StartupSeq-015");
    StartupSeqTester tester;
    tester.testTimeBaseWaitTimeout();
}

TEST(OffNominal, MaxWaitDoesNotApplyToDelay) {
    COMMENT("A startup delay longer than the time base timeout does not trigger a timeout");
    REQUIREMENT("StartupSeq-016");
    StartupSeqTester tester;
    tester.testMaxWaitDoesNotApplyToDelay();
}

TEST(OffNominal, EmptySequenceFileDisables) {
    COMMENT("An unconfigured sequence file is reported and disables startup");
    REQUIREMENT("StartupSeq-017");
    StartupSeqTester tester;
    tester.testEmptySequenceFileDisables();
}

TEST(OffNominal, SequenceFileClearedDuringDelay) {
    COMMENT("Clearing the sequence file mid-delay prevents an empty-name request");
    REQUIREMENT("StartupSeq-037");
    StartupSeqTester tester;
    tester.testSequenceFileClearedDuringDelay();
}

TEST(OffNominal, SequenceFailed) {
    COMMENT("A failed sequence outcome moves the component to FAILED and warns");
    REQUIREMENT("StartupSeq-033");
    StartupSeqTester tester;
    tester.testSequenceFailed();
}

// ----------------------------------------------------------------------
// Commands
// ----------------------------------------------------------------------

TEST(Commands, RunNowFromTerminalStates) {
    COMMENT("RUN_NOW requests the sequence from every state but STARTED");
    REQUIREMENT("StartupSeq-019");
    StartupSeqTester tester;
    tester.testRunNowFromTerminalStates();
}

TEST(Commands, RejectedWhileStarted) {
    COMMENT("Every command is rejected while a requested sequence is still running");
    REQUIREMENT("StartupSeq-034");
    REQUIREMENT("StartupSeq-036");
    StartupSeqTester tester;
    tester.testCommandsRejectedWhileStarted();
}

TEST(Commands, DisableRejectedWithNothingPending) {
    COMMENT("DISABLE is rejected when no startup sequence request is pending");
    REQUIREMENT("StartupSeq-035");
    StartupSeqTester tester;
    tester.testDisableRejectedWithNothingPending();
}

TEST(Commands, ResetReArmsAndClearsAccumulators) {
    COMMENT("RESET returns to WAITING and clears both wait accumulators");
    REQUIREMENT("StartupSeq-021");
    StartupSeqTester tester;
    tester.testResetReArmsAndClearsAccumulators();
}

TEST(Commands, ResetClearsThrottles) {
    COMMENT("RESET clears the event throttles so a re-armed component can warn again");
    REQUIREMENT("StartupSeq-021");
    REQUIREMENT("StartupSeq-026");
    StartupSeqTester tester;
    tester.testResetClearsThrottles();
}

TEST(Commands, RunNowWithNoSequenceFile) {
    COMMENT("RUN_NOW fails, reporting NoSequenceFile, when no sequence is configured");
    REQUIREMENT("StartupSeq-017");
    REQUIREMENT("StartupSeq-019");
    StartupSeqTester tester;
    tester.testRunNowWithNoSequenceFile();
}

TEST(Commands, TooManyOutstandingCommands) {
    COMMENT("Commands beyond the pending-response depth are refused BUSY, never dropped");
    REQUIREMENT("StartupSeq-036");
    StartupSeqTester tester;
    tester.testTooManyOutstandingCommands();
}

// ----------------------------------------------------------------------
// Parameters
// ----------------------------------------------------------------------

TEST(Parameters, UpdateAppliedOnNextTick) {
    COMMENT("A parameter update is applied to behavior no later than the next tick");
    REQUIREMENT("StartupSeq-038");
    StartupSeqTester tester;
    tester.testParameterUpdateAppliedOnNextTick();
}

// ----------------------------------------------------------------------
// Rule-based randomized testing
// ----------------------------------------------------------------------

//! Run a randomized scenario against a given configuration
//!
//! The shadow model predicts every transition independently of the component,
//! so any disagreement in state, events or port output fails the run.
static void runRandomScenario(TimeBase requiredTimeBase, U32 delaySeconds, U32 numRulesToApply) {
    StartupSeqTester tester;
    tester.setup(requiredTimeBase, StartupSeqTester::TEST_SEQUENCE_FILE,
                 /* maxWaitSeconds */ 5, delaySeconds);

    StartupSeqTester::Tick__Advance tickAdvance;
    StartupSeqTester::Tick__EstablishTimeBase tickEstablish;
    StartupSeqTester::Commands__RunNow cmdRunNow;
    StartupSeqTester::Commands__Disable cmdDisable;
    StartupSeqTester::Commands__Reset cmdReset;
    StartupSeqTester::Commands__RejectedWhileStarted cmdRejected;
    StartupSeqTester::SeqDone__Success seqSuccess;
    StartupSeqTester::SeqDone__Failure seqFailure;

    STest::Rule<StartupSeqTester>* rules[] = {
        &tickAdvance, &tickEstablish, &cmdRunNow,   &cmdDisable,
        &cmdReset,    &cmdRejected,   &seqSuccess,  &seqFailure,
    };

    STest::RandomScenario<StartupSeqTester> random("Random Rules", rules, FW_NUM_ARRAY_ELEMENTS(rules));
    STest::BoundedScenario<StartupSeqTester> bounded("Bounded Random Rules Scenario", random, numRulesToApply);
    const U32 numSteps = bounded.run(tester);
    printf("Ran %u steps.\n", numSteps);
}

TEST(RuleBased, RandomizedDontCare) {
    COMMENT("Randomized rules with no time base precondition and no delay");
    runRandomScenario(TimeBase::TB_DONT_CARE, 0, 2000);
}

TEST(RuleBased, RandomizedWithTimeBaseAndDelay) {
    COMMENT("Randomized rules that must wait for a time base and then a startup delay");
    runRandomScenario(TimeBase::TB_SC_TIME, 3, 2000);
}

TEST(RuleBased, RandomizedWithTimeBaseNoDelay) {
    COMMENT("Randomized rules that must wait for a time base but have no startup delay");
    runRandomScenario(TimeBase::TB_SC_TIME, 0, 2000);
}

int main(int argc, char** argv) {
    STest::Random::seed();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
