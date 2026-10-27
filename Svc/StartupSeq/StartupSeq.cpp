// ======================================================================
// \title  StartupSeq.cpp
// \author tcanham
// \brief  cpp file for StartupSeq component implementation class
// ======================================================================

#include "Svc/StartupSeq/StartupSeq.hpp"
#include <limits>
#include "Fw/Types/Assert.hpp"

namespace Svc {

namespace {
//! Milliseconds per second, used to convert rate group cycles into seconds
constexpr U32 MS_PER_SECOND = 1000;

//! Default rate group period assumed before configure() is called
constexpr U32 DEFAULT_RATE_GROUP_PERIOD_MS = 1000;
}  // namespace

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

StartupSeq ::StartupSeq(const char* const compName)
    : StartupSeqComponentBase(compName),
      m_requiredTimeBase(TimeBase::TB_DONT_CARE),
      m_rateGroupPeriodMs(DEFAULT_RATE_GROUP_PERIOD_MS),
      m_configured(false),
      m_elapsedCycles(0),
      m_delayStartCycles(0),
      m_cachedSequenceFile(""),
      m_cachedMaxWaitSeconds(0),
      m_cachedDelaySeconds(0),
      m_parametersStale(false),
      m_pendingCommands() {}

StartupSeq ::~StartupSeq() {}

// ----------------------------------------------------------------------
// Public configuration
// ----------------------------------------------------------------------

void StartupSeq ::configure(TimeBase requiredTimeBase, U32 rateGroupPeriodMs) {
    // Topology setup code is trusted, and a zero period would be a divide-by-zero
    // when converting cycles to seconds
    FW_ASSERT(rateGroupPeriodMs > 0, static_cast<FwAssertArgType>(rateGroupPeriodMs));
    this->m_requiredTimeBase = requiredTimeBase;
    this->m_rateGroupPeriodMs = rateGroupPeriodMs;
    this->m_configured = true;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void StartupSeq ::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(portNum, key);
}

void StartupSeq ::schedIn_handler(FwIndexType portNum, U32 context) {
    // A missing configure() is a topology programming error: the component would run
    // with an assumed rate group period and so with a silently wrong timeout and delay
    FW_ASSERT(this->m_configured);
    // Pick up any parameter update that arrived on another thread. Done here so the
    // cache is only ever written on this thread, where the guards read it
    if (this->m_parametersStale.exchange(false)) {
        this->cacheParameters();
    }
    // Report before signalling. State machine actions cannot report the state they are
    // entering: the generated code runs both transition and entry actions *before*
    // assigning the new state, so getState() inside an action still returns the
    // previous state. Reporting here instead always observes a settled state. Both
    // channels are "update on change", so reporting every cycle costs nothing while
    // the component is idle in a terminal state.
    this->reportState();
    this->startupMachine_sendSignal_tick();
}

void StartupSeq ::seqDoneIn_handler(FwIndexType portNum,
                                    FwOpcodeType opCode,
                                    U32 cmdSeq,
                                    const Fw::CmdResponse& response) {
    this->startupMachine_sendSignal_seqDone(response);
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void StartupSeq ::RUN_NOW_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->saveCommand(opCode, cmdSeq)) {
        return;
    }
    this->startupMachine_sendSignal_cmdRunNow();
}

void StartupSeq ::DISABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->saveCommand(opCode, cmdSeq)) {
        return;
    }
    this->startupMachine_sendSignal_cmdDisable();
}

void StartupSeq ::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if (!this->saveCommand(opCode, cmdSeq)) {
        return;
    }
    this->startupMachine_sendSignal_cmdReset();
}

// ----------------------------------------------------------------------
// Parameter hooks
// ----------------------------------------------------------------------

void StartupSeq ::parametersLoaded() {
    // Called during initialization, before the component's thread is running, so the
    // cache can be populated directly
    this->cacheParameters();
}

void StartupSeq ::parameterUpdated(FwPrmIdType id) {
    // PARAM_SET is dispatched inline in cmdIn_handlerBase rather than queued, so this
    // runs on the CommandDispatcher's thread, not the component's. Refreshing the
    // cache here would race with the state machine guards reading it. Flag it instead
    // and refresh on the next tick, on the component thread.
    this->m_parametersStale = true;
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void StartupSeq ::Svc_StartupMachine_action_doCountTick(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    // Saturate rather than wrap: a wrapped counter would restart the timeout and the
    // startup delay, and the component is not expected to wait anywhere near this long
    if (this->m_elapsedCycles < std::numeric_limits<FwSizeType>::max()) {
        this->m_elapsedCycles++;
    }
}

void StartupSeq ::Svc_StartupMachine_action_doBeginDelay(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->m_delayStartCycles = this->m_elapsedCycles;
    // Report the required time base, not the current one: with TB_DONT_CARE the
    // current time base is whatever the time source happens to report (often TB_NONE),
    // which would read as "TB_NONE established"
    this->log_ACTIVITY_HI_TimeBaseEstablished(this->m_requiredTimeBase, this->m_cachedDelaySeconds);
}

void StartupSeq ::Svc_StartupMachine_action_doReportWaiting(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_ACTIVITY_LO_WaitingForTimeBase(this->m_requiredTimeBase, this->getTime().getTimeBase(),
                                             this->cyclesToSeconds(this->m_elapsedCycles));
}

void StartupSeq ::Svc_StartupMachine_action_doReportTimeout(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_WARNING_HI_TimeBaseWaitTimeout(this->m_requiredTimeBase, this->getTime().getTimeBase(),
                                             this->cyclesToSeconds(this->m_elapsedCycles));
}

void StartupSeq ::Svc_StartupMachine_action_doRequestOnDelay(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->requestSequence();
    this->log_ACTIVITY_HI_SequenceRequested(this->m_cachedSequenceFile);
}

void StartupSeq ::Svc_StartupMachine_action_doRequestOnCommand(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->requestSequence();
    this->log_ACTIVITY_HI_SequenceRequestedByCommand(this->m_cachedSequenceFile);
}

void StartupSeq ::Svc_StartupMachine_action_doReportCompleted(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_ACTIVITY_HI_SequenceCompleted();
}

void StartupSeq ::Svc_StartupMachine_action_doReportFailed(SmId smId,
                                                           Svc_StartupMachine::Signal signal,
                                                           const Fw::CmdResponse& value) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_WARNING_HI_SequenceFailed(value);
}

void StartupSeq ::Svc_StartupMachine_action_doReportCancelled(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_WARNING_HI_StartupCancelled();
}

void StartupSeq ::Svc_StartupMachine_action_doReportNoSequenceFile(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_WARNING_HI_NoSequenceFile();
}

void StartupSeq ::Svc_StartupMachine_action_doReportNoSequenceFileWhileDelaying(
    SmId smId,
    Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_WARNING_HI_SequenceFileClearedWhileDelaying();
}

void StartupSeq ::Svc_StartupMachine_action_doRearm(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->m_elapsedCycles = 0;
    this->m_delayStartCycles = 0;
    // Clear the throttles so a re-armed component can report a subsequent wait and
    // timeout rather than being silenced by the previous arming
    this->log_ACTIVITY_LO_WaitingForTimeBase_ThrottleClear();
    this->log_WARNING_HI_TimeBaseWaitTimeout_ThrottleClear();
    this->log_ACTIVITY_HI_StartupReset();
}


void StartupSeq ::Svc_StartupMachine_action_doAcceptCommand(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->respondToCommand(Fw::CmdResponse::OK);
}

void StartupSeq ::Svc_StartupMachine_action_doRejectCommand(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    this->log_WARNING_HI_InvalidCommand(this->peekPendingOpCode(), this->startupMachine_getState());
    this->respondToCommand(Fw::CmdResponse::EXECUTION_ERROR);
}

void StartupSeq ::Svc_StartupMachine_action_doFailCommand(SmId smId, Svc_StartupMachine::Signal signal) {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    // The reason has already been reported by the action that ran before this one
    this->respondToCommand(Fw::CmdResponse::EXECUTION_ERROR);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine guards
// ----------------------------------------------------------------------

bool StartupSeq ::Svc_StartupMachine_guard_hasSequenceFile(SmId smId, Svc_StartupMachine::Signal signal) const {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    return this->m_cachedSequenceFile.length() > 0;
}

bool StartupSeq ::Svc_StartupMachine_guard_isTimeBaseReady(SmId smId, Svc_StartupMachine::Signal signal) const {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    // TB_DONT_CARE means no time base precondition, so the delay begins immediately
    if (this->m_requiredTimeBase == TimeBase::TB_DONT_CARE) {
        return true;
    }
    return this->getTime().getTimeBase() == this->m_requiredTimeBase;
}

bool StartupSeq ::Svc_StartupMachine_guard_isTimeBaseWaitExpired(SmId smId, Svc_StartupMachine::Signal signal) const {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    return this->cyclesToSeconds(this->m_elapsedCycles) >= this->m_cachedMaxWaitSeconds;
}

bool StartupSeq ::Svc_StartupMachine_guard_isDelayElapsed(SmId smId, Svc_StartupMachine::Signal signal) const {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    FW_ASSERT(this->m_elapsedCycles >= this->m_delayStartCycles,
              static_cast<FwAssertArgType>(this->m_elapsedCycles),
              static_cast<FwAssertArgType>(this->m_delayStartCycles));
    const FwSizeType cyclesInDelay = this->m_elapsedCycles - this->m_delayStartCycles;
    return this->cyclesToSeconds(cyclesInDelay) >= this->m_cachedDelaySeconds;
}

bool StartupSeq ::Svc_StartupMachine_guard_isSeqSuccess(SmId smId,
                                                        Svc_StartupMachine::Signal signal,
                                                        const Fw::CmdResponse& value) const {
    FW_ASSERT(smId == SmId::startupMachine, static_cast<FwAssertArgType>(smId));
    return value == Fw::CmdResponse::OK;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void StartupSeq ::cacheParameters() {
    // The state machine guards are const, so they cannot call paramGet_*. Cache the
    // values here instead. An invalid parameter keeps the model default, which is the
    // inert configuration: no sequence file, so nothing is requested.
    // paramGet_* returns the model default when a parameter is unset or invalid, which
    // is the inert configuration, so the validity flag needs no separate handling. Each
    // read gets its own flag rather than sharing one that the next read would overwrite.
    Fw::ParamValid sequenceFileValid = Fw::ParamValid::INVALID;
    Fw::ParamValid maxWaitValid = Fw::ParamValid::INVALID;
    Fw::ParamValid delayValid = Fw::ParamValid::INVALID;

    this->m_cachedSequenceFile = this->paramGet_SequenceFile(sequenceFileValid);
    this->m_cachedMaxWaitSeconds = this->paramGet_MaxTimeBaseWaitSeconds(maxWaitValid);
    this->m_cachedDelaySeconds = this->paramGet_StartupDelaySeconds(delayValid);
}

bool StartupSeq ::saveCommand(FwOpcodeType opCode, U32 cmdSeq) {
    // The state machine decides whether a command is valid, so the response is sent
    // from an action rather than from the handler. Several commands may be dispatched
    // before their signals are processed, so the pending commands are queued in order.
    const PendingCommand pending = {opCode, cmdSeq};
    if (this->m_pendingCommands.enqueue(pending) != Fw::Success::SUCCESS) {
        // More commands are outstanding than the component can track. Answer this one
        // directly so it does not hang in the dispatcher
        this->log_WARNING_HI_TooManyCommands(opCode);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return false;
    }
    return true;
}

void StartupSeq ::respondToCommand(Fw::CmdResponse response) {
    PendingCommand pending = {0, 0};
    const Fw::Success status = this->m_pendingCommands.dequeue(pending);
    // Every command signal is queued by a handler that first enqueued its command, so
    // a signal handled here always has a matching pending command
    FW_ASSERT(status == Fw::Success::SUCCESS, static_cast<FwAssertArgType>(status));
    this->cmdResponse_out(pending.opCode, pending.cmdSeq, response);
}

FwOpcodeType StartupSeq ::peekPendingOpCode() const {
    PendingCommand pending = {0, 0};
    const Fw::Success status = this->m_pendingCommands.peek(pending, 0);
    FW_ASSERT(status == Fw::Success::SUCCESS, static_cast<FwAssertArgType>(status));
    return pending.opCode;
}

void StartupSeq ::requestSequence() {
    // Every path into this function passes through the hasSequenceFile guard on the
    // same tick, so the cached name is non-empty. Asserted because an empty name would
    // make the sequencer run whichever sequence is already resident
    FW_ASSERT(this->m_cachedSequenceFile.length() > 0);
    Svc::SeqArgs args;
    this->seqRunOut_out(0, this->m_cachedSequenceFile, args);
}

U32 StartupSeq ::cyclesToSeconds(FwSizeType cycles) const {
    // Integer division truncates, so a partial second does not count. This makes both
    // the timeout and the startup delay a lower bound: the component waits at least
    // the configured number of seconds.
    FW_ASSERT(this->m_rateGroupPeriodMs > 0, static_cast<FwAssertArgType>(this->m_rateGroupPeriodMs));
    const U64 elapsedMs = static_cast<U64>(cycles) * static_cast<U64>(this->m_rateGroupPeriodMs);
    const U64 elapsedSeconds = elapsedMs / static_cast<U64>(MS_PER_SECOND);
    // Clamp rather than truncate the high bits: a wrapped value would read as a short
    // wait and could satisfy the timeout or delay comparison spuriously
    const U64 maxU32 = static_cast<U64>(std::numeric_limits<U32>::max());
    return static_cast<U32>((elapsedSeconds > maxU32) ? maxU32 : elapsedSeconds);
}

void StartupSeq ::reportState() {
    this->tlmWrite_State(this->startupMachine_getState());
    this->tlmWrite_ElapsedSeconds(this->cyclesToSeconds(this->m_elapsedCycles));
}

}  // namespace Svc
