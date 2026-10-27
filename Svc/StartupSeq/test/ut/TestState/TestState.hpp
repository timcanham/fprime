// ======================================================================
// \title  TestState.hpp
// \author tcanham
// \brief  Shadow state model for StartupSeq rule-based testing
//
// StartupSeqTestState holds only shadow-model data and state-only helpers.
// Rule preconditions and actions are implemented on StartupSeqTester.
//
// Shadow state
// ------------
// The shadow is an independent software model of the component's startup
// state machine: which state it should be in, how many ticks have elapsed,
// and the configuration it should be acting on. It is deliberately written
// from the requirements rather than from the component's source, so that a
// disagreement between the two indicates a defect in one of them rather
// than a transcription of the same mistake twice.
//
// Precondition methods read the shadow to decide whether a rule may fire;
// action methods advance the shadow in lockstep with the component. After
// every action the shadow's expected state and the component's reported
// state must agree.
// ======================================================================

#ifndef Svc_StartupSeq_StartupSeqTestState_HPP
#define Svc_StartupSeq_StartupSeqTestState_HPP

#include "Svc/StartupSeq/StartupMachineStateMachineAc.hpp"
#include "Svc/StartupSeq/StartupSeq.hpp"

namespace Svc {

class StartupSeqTestState {
  public:
    //! The generated state machine's state enum (top-level, not the component's
    //! protected nested instance type)
    using State = Svc::StartupMachine_State;

    // ----------------------------------------------------------------------
    // Configuration the component was given
    // ----------------------------------------------------------------------

    //! Required time base supplied to configure()
    TimeBase shadow_requiredTimeBase = TimeBase::TB_DONT_CARE;

    //! Rate group period supplied to configure(), in milliseconds
    U32 shadow_rateGroupPeriodMs = 1000;

    //! The SequenceFile parameter value the component should be acting on
    Fw::ParamString shadow_sequenceFile;

    //! The MaxTimeBaseWaitSeconds parameter value
    U32 shadow_maxWaitSeconds = 60;

    //! The StartupDelaySeconds parameter value
    U32 shadow_delaySeconds = 0;

    // ----------------------------------------------------------------------
    // Component "shadow" state
    // ----------------------------------------------------------------------

    //! The state the component is expected to be in
    State shadow_state = State::WAITING;

    //! Ticks counted since the component was armed
    U32 shadow_elapsedCycles = 0;

    //! Value of shadow_elapsedCycles when the startup delay began
    U32 shadow_delayStartCycles = 0;

    //! The time base the simulated time source currently reports
    TimeBase shadow_currentTimeBase = TimeBase::TB_NONE;

    //! Whether a parameter was changed since the last tick. The component
    //! applies updates on the next tick (REQ-StartupSeq-038), not instantly
    bool shadow_parametersStale = false;

  public:
    // ----------------------------------------------------------------------
    // State-only helpers
    // ----------------------------------------------------------------------

    //! Convert a tick count to whole seconds the way the component must
    U32 shadow_cyclesToSeconds(U32 cycles) const {
        const U64 elapsedMs = static_cast<U64>(cycles) * static_cast<U64>(this->shadow_rateGroupPeriodMs);
        return static_cast<U32>(elapsedMs / 1000u);
    }

    //! Whether a sequence file is configured, from the component's point of view
    //!
    //! Reflects the value the component is acting on, which lags a parameter
    //! change until the next tick
    bool shadow_hasSequenceFile() const { return this->shadow_sequenceFile.length() > 0; }

    //! Whether the time base precondition is satisfied
    bool shadow_isTimeBaseReady() const {
        return (this->shadow_requiredTimeBase == TimeBase::TB_DONT_CARE) ||
               (this->shadow_currentTimeBase == this->shadow_requiredTimeBase);
    }

    //! Whether the time base wait has reached the configured maximum
    bool shadow_isTimeBaseWaitExpired() const {
        return this->shadow_cyclesToSeconds(this->shadow_elapsedCycles) >= this->shadow_maxWaitSeconds;
    }

    //! Whether the startup delay has elapsed
    bool shadow_isDelayElapsed() const {
        const U32 cyclesInDelay = this->shadow_elapsedCycles - this->shadow_delayStartCycles;
        return this->shadow_cyclesToSeconds(cyclesInDelay) >= this->shadow_delaySeconds;
    }

    //! Whether a startup sequence request is still pending, i.e. cancellable
    bool shadow_isRequestPending() const {
        return (this->shadow_state == State::WAITING) || (this->shadow_state == State::DELAYING);
    }

    //! Whether the component is waiting for the sequencer to report an outcome
    bool shadow_isSequenceRunning() const { return this->shadow_state == State::STARTED; }

    //! Re-arm the shadow, as the RESET command does
    void shadow_rearm() {
        this->shadow_state = State::WAITING;
        this->shadow_elapsedCycles = 0;
        this->shadow_delayStartCycles = 0;
    }
};

}  // namespace Svc

#endif
