module Svc {

    @ The startup sequence state machine.
    @
    @ Advances through the startup preconditions on each tick: first waiting for the
    @ required time base to be established, then waiting out the startup delay. Once
    @ both hold, the sequence is requested and the machine waits for the paired
    @ sequencer to report the outcome.
    state machine StartupMachine {

        # ------------------------------------------------------------------
        # Signals
        # ------------------------------------------------------------------

        @ Rate-group driven tick
        signal tick

        @ The paired sequencer reported the sequence finished
        signal seqDone: Fw.CmdResponse

        @ An operator issued the RUN_NOW command
        signal cmdRunNow

        @ An operator issued the DISABLE command
        signal cmdDisable

        @ An operator issued the RESET command
        signal cmdReset

        # ------------------------------------------------------------------
        # Guards
        # ------------------------------------------------------------------

        @ The SequenceFile parameter names a sequence, i.e. it is not empty
        guard hasSequenceFile

        @ The current system time is in the required time base, or no time base is required
        guard isTimeBaseReady

        @ The time spent waiting for the required time base has reached the maximum wait
        guard isTimeBaseWaitExpired

        @ The startup delay has elapsed since the time base precondition was satisfied
        guard isDelayElapsed

        @ The sequencer reported that the sequence succeeded
        guard isSeqSuccess: Fw.CmdResponse

        # ------------------------------------------------------------------
        # Actions
        # ------------------------------------------------------------------

        @ Count one elapsed tick and report the elapsed time
        action doCountTick

        @ Mark the point at which the startup delay began, and report the time base
        action doBeginDelay

        @ Report that the component is still waiting for the required time base
        action doReportWaiting

        @ Report that the required time base was not established in time
        action doReportTimeout

        @ Request the startup sequence because the startup delay elapsed
        action doRequestOnDelay

        @ Request the startup sequence because an operator commanded it
        action doRequestOnCommand

        @ Report that the sequence completed successfully
        action doReportCompleted

        @ Report that the sequence failed
        action doReportFailed: Fw.CmdResponse

        @ Report that the pending request was cancelled by command
        action doReportCancelled

        @ Report that the pending request was cancelled because no sequence file is
        @ configured, detected before any waiting began
        action doReportNoSequenceFile

        @ Report that the sequence file was cleared while the startup delay was elapsing
        action doReportNoSequenceFileWhileDelaying

        @ Clear all accumulated wait time and report the re-arming
        action doRearm

        @ Report success for the command being handled
        action doAcceptCommand

        @ Report that the command being handled is not valid in the current state
        action doRejectCommand

        @ Report failure for the command being handled, without an InvalidCommand event
        action doFailCommand


        # ------------------------------------------------------------------
        # Initial transition
        # ------------------------------------------------------------------

        @ Start armed, waiting for the required time base
        initial enter WAITING

        # ------------------------------------------------------------------
        # Choices
        # ------------------------------------------------------------------

        @ Reject an unconfigured component before it waits on anything. Checked on the
        @ first tick so an empty SequenceFile is reported immediately rather than after
        @ the full time base wait has expired
        choice CHECK_CONFIGURED {
            if hasSequenceFile enter CHECK_TIME_BASE \
                else do { doReportNoSequenceFile } enter DISABLED
        }

        @ Re-check, on each tick of the startup delay, that a sequence file is still
        @ configured. Distinct from CHECK_CONFIGURED so that re-checking does not
        @ re-enter the time base check and restart the delay
        choice CHECK_STILL_CONFIGURED {
            if hasSequenceFile enter CHECK_DELAY \
                else do { doReportNoSequenceFileWhileDelaying } enter DISABLED
        }

        @ Advance to the startup delay once the required time base is established
        choice CHECK_TIME_BASE {
            if isTimeBaseReady do { doBeginDelay } enter CHECK_DELAY \
                else enter CHECK_TIME_BASE_TIMEOUT
        }

        @ Give up if the required time base was never established
        choice CHECK_TIME_BASE_TIMEOUT {
            if isTimeBaseWaitExpired do { doReportTimeout } enter TIMED_OUT \
                else do { doReportWaiting } enter WAITING
        }

        @ Request the sequence once the startup delay has elapsed. A zero delay is
        @ already elapsed on entry, so the sequence is requested on the same tick that
        @ satisfied the time base precondition
        choice CHECK_DELAY {
            if isDelayElapsed do { doRequestOnDelay } enter STARTED \
                else enter DELAYING
        }

        @ A commanded request must still have a sequence to run
        choice CHECK_RUN_NOW {
            if hasSequenceFile do { doRequestOnCommand, doAcceptCommand } enter STARTED \
                else do { doReportNoSequenceFile, doFailCommand } enter DISABLED
        }

        @ Record the outcome reported by the sequencer
        choice CHECK_SEQ_RESULT {
            if isSeqSuccess do { doReportCompleted } enter COMPLETED \
                else do { doReportFailed } enter FAILED
        }

        # ------------------------------------------------------------------
        # States
        # ------------------------------------------------------------------

        @ Armed and waiting for the required time base to be established
        state WAITING {
            on tick do { doCountTick } enter CHECK_CONFIGURED
            on cmdRunNow enter CHECK_RUN_NOW
            on cmdDisable do { doReportCancelled, doAcceptCommand } enter DISABLED
            on cmdReset do { doRearm, doAcceptCommand } enter WAITING
        }

        @ The time base precondition is satisfied; waiting out the startup delay
        state DELAYING {
            # Re-check the sequence file rather than entering CHECK_DELAY directly: the
            # parameter can be cleared by command while the delay is still elapsing,
            # and requesting an empty file name would make the sequencer run whatever
            # sequence is already resident
            on tick do { doCountTick } enter CHECK_STILL_CONFIGURED
            on cmdRunNow enter CHECK_RUN_NOW
            on cmdDisable do { doReportCancelled, doAcceptCommand } enter DISABLED
            on cmdReset do { doRearm, doAcceptCommand } enter WAITING
        }

        @ The sequence has been requested; waiting for the sequencer to report the outcome.
        @ No command is accepted here: the request cannot be recalled, and re-arming would
        @ allow a second request to be issued into a running sequence
        state STARTED {
            on seqDone enter CHECK_SEQ_RESULT
            # No command is accepted while a requested sequence is still running: the
            # request cannot be recalled, and re-arming would allow a second request
            # to be issued into that sequence
            on cmdRunNow do { doRejectCommand }
            on cmdDisable do { doRejectCommand }
            on cmdReset do { doRejectCommand }
        }

        @ The sequence ran and the sequencer reported success
        state COMPLETED {
            on cmdRunNow enter CHECK_RUN_NOW
            on cmdDisable do { doRejectCommand }
            on cmdReset do { doRearm, doAcceptCommand } enter WAITING
        }

        @ The sequence ran and the sequencer reported failure
        state FAILED {
            on cmdRunNow enter CHECK_RUN_NOW
            on cmdDisable do { doRejectCommand }
            on cmdReset do { doRearm, doAcceptCommand } enter WAITING
        }

        @ The required time base was not established within the maximum wait; the
        @ sequence was not requested
        state TIMED_OUT {
            on cmdRunNow enter CHECK_RUN_NOW
            on cmdDisable do { doRejectCommand }
            on cmdReset do { doRearm, doAcceptCommand } enter WAITING
        }

        @ The pending request was cancelled by command, or no sequence file is
        @ configured. Applies to this run of the software only
        state DISABLED {
            on cmdRunNow enter CHECK_RUN_NOW
            on cmdDisable do { doRejectCommand }
            on cmdReset do { doRearm, doAcceptCommand } enter WAITING
        }

    }

    @ A component that requests a command sequence be run at software startup,
    @ optionally waiting until time in a specified time base has been established.
    @
    @ All component state is held in memory only. Commands affect the current run of
    @ the software and are not persistent: after a restart the component is armed
    @ again and will request its sequence as configured. To prevent the startup
    @ sequence from running on future restarts, clear the SequenceFile parameter and
    @ save it to the parameter database.
    active component StartupSeq {

        # ----------------------------------------------------------------------
        # State machine
        # ----------------------------------------------------------------------

        @ The startup state machine. Signals are queued on the component queue, so the
        @ rate-group tick, the sequence-done report and the commands are all serialized
        state machine instance startupMachine: StartupMachine

        # ----------------------------------------------------------------------
        # Special ports
        # ----------------------------------------------------------------------

        @ Command receive port
        command recv port cmdIn

        @ Command registration port
        command reg port cmdRegOut

        @ Command response port
        command resp port cmdResponseOut

        @ Event port
        event port logOut

        @ Text event port
        text event port logTextOut

        @ Telemetry port
        telemetry port tlmOut

        @ Time get port
        time get port timeCaller

        @ Parameter get port
        param get port prmGetOut

        @ Parameter set port
        param set port prmSetOut

        # ----------------------------------------------------------------------
        # General ports
        # ----------------------------------------------------------------------

        @ Rate group cycle input. Drives the startup state machine
        async input port schedIn: Svc.Sched

        @ Port for requesting that the startup sequence be run
        output port seqRunOut: Svc.CmdSeqIn

        @ Port on which the paired sequencer reports that the startup sequence finished.
        @ Must be connected, or the component remains in the STARTED state after
        @ requesting its sequence
        async input port seqDoneIn: Fw.CmdResponse

        @ Ping in port
        async input port pingIn: Svc.Ping

        @ Ping out port
        output port pingOut: Svc.Ping

        # ----------------------------------------------------------------------
        # Parameters
        #
        # The required time base and the rate group period are static properties of
        # a given system, so they are supplied to the implementation's configure()
        # method rather than being declared as ground-settable parameters here.
        # ----------------------------------------------------------------------

        @ The file name of the sequence to run at startup. An empty string disables startup.
        @
        @ Sized to FW_PARAM_STRING_MAX_SIZE rather than FileNameStringSize because the
        @ generated parameter storage is an Fw::ParamString: a longer declaration would
        @ be silently truncated to that size when the parameter is loaded
        param SequenceFile: string size FW_PARAM_STRING_MAX_SIZE \
            default ""

        @ The maximum number of seconds to wait for the required time base to be
        @ established before timing out. Not applied while the startup delay elapses
        param MaxTimeBaseWaitSeconds: U32 \
            default 60

        @ The number of seconds to delay after the required time base is established,
        @ and before the startup sequence is requested. Zero requests the sequence immediately
        param StartupDelaySeconds: U32 \
            default 0

        # ----------------------------------------------------------------------
        # Commands
        # ----------------------------------------------------------------------

        @ Request the startup sequence immediately, bypassing the time base wait and the
        @ startup delay. Rejected while a requested sequence is still running
        async command RUN_NOW

        @ Cancel the pending startup sequence request for this run of the software only.
        @
        @ This is a one-shot inhibit of the request that is currently pending: it stops
        @ the sequence from being requested now. It does NOT disable startup behavior for
        @ future restarts, and does not change any parameter. After a restart the
        @ component is armed again and will request its sequence as configured. To
        @ suppress the startup sequence across restarts, clear the SequenceFile parameter
        @ and save it to the parameter database.
        @
        @ Valid only while a request is pending, i.e. in the WAITING or DELAYING state
        async command DISABLE

        @ Re-arm the component, clearing all accumulated wait time. Used to retry startup
        @ behavior, or to undo a DISABLE command, without restarting the software.
        @ Rejected while a requested sequence is still running
        async command RESET

        # ----------------------------------------------------------------------
        # Events
        # ----------------------------------------------------------------------

        @ The startup sequence was requested because the startup delay elapsed
        event SequenceRequested(
                                 fileName: string size FW_PARAM_STRING_MAX_SIZE @< The sequence file requested
                               ) \
            severity activity high \
            format "Requested startup sequence {}"

        @ The startup sequence was requested because an operator commanded it
        event SequenceRequestedByCommand(
                                          fileName: string size FW_PARAM_STRING_MAX_SIZE @< The sequence file requested
                                        ) \
            severity activity high \
            format "Requested startup sequence {} by command"

        @ The startup sequence completed successfully
        event SequenceCompleted \
            severity activity high \
            format "Startup sequence completed successfully"

        @ The startup sequence failed
        event SequenceFailed(
                              response: Fw.CmdResponse @< The response reported by the sequencer
                            ) \
            severity warning high \
            format "Startup sequence failed with response {}"

        @ The required time base was established; the startup delay is now elapsing
        event TimeBaseEstablished(
                                   required: TimeBase @< The required time base. TB_DONT_CARE
                                                      @< when no time base was required
                                   delaySeconds: U32 @< The configured startup delay
                                 ) \
            severity activity high \
            format "Time base precondition {} satisfied; delaying {} seconds before startup sequence"

        @ Waiting for the required time base to be established
        event WaitingForTimeBase(
                                  required: TimeBase @< The required time base
                                  current: TimeBase @< The current time base
                                  waitedSeconds: U32 @< Seconds waited so far
                                ) \
            severity activity low \
            format "Waiting for time base {}, currently {} ({} seconds elapsed)" \
            throttle 5

        @ The required time base was not established within the maximum wait
        event TimeBaseWaitTimeout(
                                   required: TimeBase @< The required time base
                                   current: TimeBase @< The current time base
                                   waitedSeconds: U32 @< Seconds waited before timing out
                                 ) \
            severity warning high \
            format "Timed out waiting for time base {} (currently {}) after {} seconds. Startup sequence not run" \
            throttle 1

        @ The pending startup sequence request was cancelled by command
        event StartupCancelled \
            severity warning high \
            format "Startup sequence cancelled by command for this run. Startup behavior is unchanged for future restarts"

        @ No sequence file is configured, so no startup sequence can be requested
        event NoSequenceFile \
            severity warning high \
            format "No startup sequence file is configured; startup sequence not run"

        @ The sequence file was cleared while the startup delay was elapsing
        event SequenceFileClearedWhileDelaying \
            severity warning high \
            format "Startup sequence file was cleared during the startup delay; startup sequence not run"

        @ The component was re-armed by command
        event StartupReset \
            severity activity high \
            format "Startup re-armed; waiting for time base"

        @ A command was rejected because too many commands are already outstanding
        event TooManyCommands(
                               $opcode: FwOpcodeType @< The opcode of the rejected command
                             ) \
            severity warning high \
            format "Command with opcode {} rejected: too many commands outstanding"

        @ A command was rejected because it is invalid in the current state
        event InvalidCommand(
                              $opcode: FwOpcodeType @< The opcode of the rejected command
                              $state: StartupMachine.State @< The state the component was in
                            ) \
            severity warning high \
            format "Command with opcode {} rejected in state {}"

        # ----------------------------------------------------------------------
        # Telemetry
        # ----------------------------------------------------------------------

        @ The current startup state
        telemetry State: StartupMachine.State \
            update on change

        @ Seconds elapsed since the component was armed
        telemetry ElapsedSeconds: U32 \
            update on change

    }

}
