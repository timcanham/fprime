// ======================================================================
// \title  StartupSeq.hpp
// \author tcanham
// \brief  hpp file for StartupSeq component implementation class
// ======================================================================

#ifndef Svc_StartupSeq_HPP
#define Svc_StartupSeq_HPP

#include <atomic>
#include "Fw/DataStructures/FifoQueue.hpp"
#include "Fw/Prm/PrmString.hpp"
#include "Svc/StartupSeq/StartupSeqComponentAc.hpp"

namespace Svc {

class StartupSeq final : public StartupSeqComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct StartupSeq object
    explicit StartupSeq(const char* const compName  //!< The component name
    );

    //! Destroy StartupSeq object
    ~StartupSeq();

    //! Components are not copyable or movable (CPP-17)
    StartupSeq(const StartupSeq&) = delete;
    StartupSeq(StartupSeq&&) = delete;
    StartupSeq& operator=(const StartupSeq&) = delete;
    StartupSeq& operator=(StartupSeq&&) = delete;

    // ----------------------------------------------------------------------
    // Public configuration
    // ----------------------------------------------------------------------

    //! Configure the component's static startup properties
    //!
    //! Both values are fixed properties of the system rather than operational
    //! settings, so they are supplied here rather than as parameters. This must be
    //! called during topology setup, before the component is scheduled.
    void configure(TimeBase requiredTimeBase,  //!< Time base required before the sequence
                                               //!< is requested; TB_DONT_CARE to not wait
                   U32 rateGroupPeriodMs       //!< Period of the rate group driving schedIn,
                                               //!< in milliseconds. Must be non-zero
    );

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for pingIn
    //!
    //! Ping in port
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for schedIn
    //!
    //! Rate group cycle input. Drives the startup state machine
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

    //! Handler implementation for seqDoneIn
    //!
    //! Port on which the paired sequencer reports that the startup sequence finished.
    //! Must be connected, or the component remains in the STARTED state after
    //! requesting its sequence
    void seqDoneIn_handler(FwIndexType portNum,             //!< The port number
                           FwOpcodeType opCode,             //!< Command Op Code
                           U32 cmdSeq,                      //!< Command Sequence
                           const Fw::CmdResponse& response  //!< The command response argument
                           ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RUN_NOW
    //!
    //! Request the startup sequence immediately, bypassing the time base wait and the
    //! startup delay. Rejected while a requested sequence is still running
    void RUN_NOW_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                            U32 cmdSeq            //!< The command sequence number
                            ) override;

    //! Handler implementation for command DISABLE
    //!
    //! Cancel the pending startup sequence request for this run of the software only.
    //! Valid only while a request is pending, i.e. in the WAITING or DELAYING state
    void DISABLE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                            U32 cmdSeq            //!< The command sequence number
                            ) override;

    //! Handler implementation for command RESET
    //!
    //! Re-arm the component, clearing all accumulated wait time. Rejected while a
    //! requested sequence is still running
    void RESET_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                          U32 cmdSeq            //!< The command sequence number
                          ) override;

  private:
    // ----------------------------------------------------------------------
    // Parameter hooks
    // ----------------------------------------------------------------------

    //! Called when all parameters have been loaded at initialization
    //!
    //! Runs before the component's thread is started, so it may refresh the cache
    //! directly.
    void parametersLoaded() override;

    //! Called when a parameter is updated by command
    //!
    //! Runs on the caller's thread (PARAM_SET is handled inline in cmdIn_handlerBase,
    //! not queued), so this only raises a flag. The cache itself is refreshed on the
    //! component thread.
    void parameterUpdated(FwPrmIdType id  //!< The parameter id
                          ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action doCountTick of state machine Svc_StartupMachine
    //!
    //! Count one elapsed tick and report the elapsed time
    void Svc_StartupMachine_action_doCountTick(SmId smId,                         //!< The state machine id
                                               Svc_StartupMachine::Signal signal  //!< The signal
                                               ) override;

    //! Implementation for action doBeginDelay of state machine Svc_StartupMachine
    //!
    //! Mark the point at which the startup delay began, and report the time base
    void Svc_StartupMachine_action_doBeginDelay(SmId smId,                         //!< The state machine id
                                                Svc_StartupMachine::Signal signal  //!< The signal
                                                ) override;

    //! Implementation for action doReportWaiting of state machine Svc_StartupMachine
    //!
    //! Report that the component is still waiting for the required time base
    void Svc_StartupMachine_action_doReportWaiting(SmId smId,                         //!< The state machine id
                                                   Svc_StartupMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action doReportTimeout of state machine Svc_StartupMachine
    //!
    //! Report that the required time base was not established in time
    void Svc_StartupMachine_action_doReportTimeout(SmId smId,                         //!< The state machine id
                                                   Svc_StartupMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action doRequestOnDelay of state machine Svc_StartupMachine
    //!
    //! Request the startup sequence because the startup delay elapsed
    void Svc_StartupMachine_action_doRequestOnDelay(SmId smId,                         //!< The state machine id
                                                    Svc_StartupMachine::Signal signal  //!< The signal
                                                    ) override;

    //! Implementation for action doRequestOnCommand of state machine Svc_StartupMachine
    //!
    //! Request the startup sequence because an operator commanded it
    void Svc_StartupMachine_action_doRequestOnCommand(SmId smId,                         //!< The state machine id
                                                      Svc_StartupMachine::Signal signal  //!< The signal
                                                      ) override;

    //! Implementation for action doReportCompleted of state machine Svc_StartupMachine
    //!
    //! Report that the sequence completed successfully
    void Svc_StartupMachine_action_doReportCompleted(SmId smId,                         //!< The state machine id
                                                     Svc_StartupMachine::Signal signal  //!< The signal
                                                     ) override;

    //! Implementation for action doReportFailed of state machine Svc_StartupMachine
    //!
    //! Report that the sequence failed
    void Svc_StartupMachine_action_doReportFailed(SmId smId,                          //!< The state machine id
                                                  Svc_StartupMachine::Signal signal,  //!< The signal
                                                  const Fw::CmdResponse& value        //!< The value
                                                  ) override;

    //! Implementation for action doReportCancelled of state machine Svc_StartupMachine
    //!
    //! Report that the pending request was cancelled by command
    void Svc_StartupMachine_action_doReportCancelled(SmId smId,                         //!< The state machine id
                                                     Svc_StartupMachine::Signal signal  //!< The signal
                                                     ) override;

    //! Implementation for action doReportNoSequenceFile of state machine Svc_StartupMachine
    //!
    //! Report that the pending request was cancelled because no sequence file is configured
    void Svc_StartupMachine_action_doReportNoSequenceFile(SmId smId,                         //!< The state machine id
                                                          Svc_StartupMachine::Signal signal  //!< The signal
                                                          ) override;

    //! Implementation for action doReportNoSequenceFileWhileDelaying of state machine
    //! Svc_StartupMachine
    //!
    //! Report that the sequence file was cleared while the startup delay was elapsing
    void Svc_StartupMachine_action_doReportNoSequenceFileWhileDelaying(
        SmId smId,                         //!< The state machine id
        Svc_StartupMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action doRearm of state machine Svc_StartupMachine
    //!
    //! Clear all accumulated wait time and report the re-arming
    void Svc_StartupMachine_action_doRearm(SmId smId,                         //!< The state machine id
                                           Svc_StartupMachine::Signal signal  //!< The signal
                                           ) override;

    //! Implementation for action doAcceptCommand of state machine Svc_StartupMachine
    //!
    //! Report success for the command being handled
    void Svc_StartupMachine_action_doAcceptCommand(SmId smId,                         //!< The state machine id
                                                   Svc_StartupMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action doRejectCommand of state machine Svc_StartupMachine
    //!
    //! Report that the command being handled is not valid in the current state
    void Svc_StartupMachine_action_doRejectCommand(SmId smId,                         //!< The state machine id
                                                   Svc_StartupMachine::Signal signal  //!< The signal
                                                   ) override;

    //! Implementation for action doFailCommand of state machine Svc_StartupMachine
    //!
    //! Report failure for the command being handled, without an InvalidCommand event
    void Svc_StartupMachine_action_doFailCommand(SmId smId,                         //!< The state machine id
                                                 Svc_StartupMachine::Signal signal  //!< The signal
                                                 ) override;


  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! Implementation for guard hasSequenceFile of state machine Svc_StartupMachine
    //!
    //! The SequenceFile parameter names a sequence, i.e. it is not empty
    bool Svc_StartupMachine_guard_hasSequenceFile(SmId smId,                         //!< The state machine id
                                                  Svc_StartupMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard isTimeBaseReady of state machine Svc_StartupMachine
    //!
    //! The current system time is in the required time base, or no time base is required
    bool Svc_StartupMachine_guard_isTimeBaseReady(SmId smId,                         //!< The state machine id
                                                  Svc_StartupMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard isTimeBaseWaitExpired of state machine Svc_StartupMachine
    //!
    //! The time spent waiting for the required time base has reached the maximum wait
    bool Svc_StartupMachine_guard_isTimeBaseWaitExpired(SmId smId,                         //!< The state machine id
                                                        Svc_StartupMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard isDelayElapsed of state machine Svc_StartupMachine
    //!
    //! The startup delay has elapsed since the time base precondition was satisfied
    bool Svc_StartupMachine_guard_isDelayElapsed(SmId smId,                         //!< The state machine id
                                                 Svc_StartupMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard isSeqSuccess of state machine Svc_StartupMachine
    //!
    //! The sequencer reported that the sequence succeeded
    bool Svc_StartupMachine_guard_isSeqSuccess(SmId smId,                          //!< The state machine id
                                               Svc_StartupMachine::Signal signal,  //!< The signal
                                               const Fw::CmdResponse& value        //!< The value
    ) const override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Refresh the cached parameter values from the parameter database
    //!
    //! The state machine guards are const and so cannot call the non-const
    //! paramGet_* functions, so the values are cached. MUST be called only from the
    //! component's own thread: the cache is read without a lock by the guards and by
    //! requestSequence().
    void cacheParameters();

    //! Record the command being handled so a state machine action can respond to it
    //!
    //! \return Whether the command was recorded. False if the pending queue is full,
    //!         in which case the caller must respond to the command itself.
    bool saveCommand(FwOpcodeType opCode,  //!< The opcode of the command
                     U32 cmdSeq            //!< The command sequence number
    );

    //! Send the response for the oldest command recorded by saveCommand
    void respondToCommand(Fw::CmdResponse response  //!< The response to send
    );

    //! Get the opcode of the oldest command recorded by saveCommand
    FwOpcodeType peekPendingOpCode() const;

    //! Request the startup sequence from the paired sequencer
    void requestSequence();

    //! Convert a count of elapsed rate group cycles into whole seconds
    //!
    //! Returns U32 to match the parameter and telemetry types the result is compared
    //! and reported against
    U32 cyclesToSeconds(FwSizeType cycles  //!< The cycle count to convert
    ) const;

    //! Emit the State and ElapsedSeconds telemetry channels
    void reportState();

  private:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    //! Maximum number of commands that may await a state machine response at once.
    //! Commands beyond this are rejected rather than dropped
    static constexpr FwSizeType PENDING_COMMAND_DEPTH = 8;

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The time base that must be established before the sequence is requested.
    //! Supplied by configure()
    TimeBase m_requiredTimeBase;

    //! The period of the rate group driving schedIn, in milliseconds.
    //! Supplied by configure()
    U32 m_rateGroupPeriodMs;

    //! Whether configure() has been called
    bool m_configured;

    //! Rate group cycles counted since the component was armed
    //!
    //! FwSizeType rather than a bare U32: this is a tick count whose useful range
    //! depends on the project's rate group period and mission duration (CPP-28)
    FwSizeType m_elapsedCycles;

    //! The value of m_elapsedCycles when the startup delay began
    FwSizeType m_delayStartCycles;

    //! Cached SequenceFile parameter value
    //!
    //! Fw::ParamString, matching the generated parameter storage. The sequence path is
    //! therefore limited to FW_PARAM_STRING_MAX_SIZE
    Fw::ParamString m_cachedSequenceFile;

    //! Cached MaxTimeBaseWaitSeconds parameter value
    U32 m_cachedMaxWaitSeconds;

    //! Cached StartupDelaySeconds parameter value
    U32 m_cachedDelaySeconds;

    //! Set when a parameter was updated and the cache needs refreshing
    //!
    //! Written from parameterUpdated() on the updating component's thread and cleared
    //! from the component thread, so it is atomic rather than a plain bool
    std::atomic<bool> m_parametersStale;

    //! A command awaiting a response from the state machine
    struct PendingCommand {
        FwOpcodeType opCode;  //!< The opcode
        U32 cmdSeq;           //!< The command sequence number
    };

    //! Commands awaiting a response, oldest first
    //!
    //! A command handler records the command here and queues a signal; the action that
    //! handles that signal sends the response. Several commands can be dispatched
    //! before their signals are processed, so the responses must be queued in order
    //! rather than held in a single slot.
    Fw::FifoQueue<PendingCommand, PENDING_COMMAND_DEPTH> m_pendingCommands;
};

}  // namespace Svc

#endif
