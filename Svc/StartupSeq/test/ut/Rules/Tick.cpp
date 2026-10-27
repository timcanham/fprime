// ======================================================================
// \title  Tick.cpp
// \author tcanham
// \brief  Rule implementations for the Tick rule group
//
// These rules exercise the schedIn port, which drives the startup state
// machine, and the establishment of the required time base.
// ======================================================================

#include "Svc/StartupSeq/test/ut/StartupSeqTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Tick.Advance
//
// Invoke schedIn once from any state and check that the component's state
// agrees with the shadow model's independent prediction of the transition.
// ----------------------------------------------------------------------

bool StartupSeqTester::Tick__Advance__precondition() const {
    // A tick is always legal; only WAITING and DELAYING act on it
    return true;
}

void StartupSeqTester::Tick__Advance__action() {
    this->clearHistory();

    const State before = this->shadow.shadow_state;

    // Predict the transition from the requirements, independently of the
    // component, then compare
    if (before == State::WAITING) {
        this->shadow.shadow_elapsedCycles++;
        if (!this->shadow.shadow_hasSequenceFile()) {
            this->shadow.shadow_state = State::DISABLED;
        } else if (this->shadow.shadow_isTimeBaseReady()) {
            this->shadow.shadow_delayStartCycles = this->shadow.shadow_elapsedCycles;
            if (this->shadow.shadow_isDelayElapsed()) {
                this->shadow.shadow_state = State::STARTED;
            } else {
                this->shadow.shadow_state = State::DELAYING;
            }
        } else if (this->shadow.shadow_isTimeBaseWaitExpired()) {
            this->shadow.shadow_state = State::TIMED_OUT;
        }
        // else: stays in WAITING
    } else if (before == State::DELAYING) {
        this->shadow.shadow_elapsedCycles++;
        if (!this->shadow.shadow_hasSequenceFile()) {
            // REQ-StartupSeq-037: the file can be cleared mid-delay
            this->shadow.shadow_state = State::DISABLED;
        } else if (this->shadow.shadow_isDelayElapsed()) {
            this->shadow.shadow_state = State::STARTED;
        }
        // else: stays in DELAYING
    }
    // All other states ignore the tick (REQ-StartupSeq-013)

    this->tick();

    this->assertStateMatchesShadow("Tick.Advance");

    // A sequence may be requested only on the transition into STARTED
    if ((this->shadow.shadow_state == State::STARTED) && (before != State::STARTED)) {
        this->assertSequenceRequested(this->shadow.shadow_sequenceFile.toChar());
    } else {
        this->assertNoSequenceRequested();
    }
}

// ----------------------------------------------------------------------
// Tick.EstablishTimeBase
//
// Make the simulated time source report the required time base, so the
// WAITING -> DELAYING precondition can be satisfied mid-scenario.
// ----------------------------------------------------------------------

bool StartupSeqTester::Tick__EstablishTimeBase__precondition() const {
    // Only meaningful while a time base is actually required and not yet reported
    return (this->shadow.shadow_requiredTimeBase != TimeBase::TB_DONT_CARE) &&
           (this->shadow.shadow_currentTimeBase != this->shadow.shadow_requiredTimeBase);
}

void StartupSeqTester::Tick__EstablishTimeBase__action() {
    this->clearHistory();

    // Changing the reported time base alone must not move the state machine:
    // the component only observes it on a tick
    const State before = this->shadow.shadow_state;
    this->setCurrentTimeBase(this->shadow.shadow_requiredTimeBase);

    ASSERT_EQ(this->component.startupMachine_getState(), before)
        << "Establishing the time base must not change state until the next tick";
    this->assertNoSequenceRequested();
}

}  // namespace Svc
