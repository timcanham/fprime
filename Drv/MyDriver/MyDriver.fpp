module Drv {

    @ Timer output port type - notification, no arguments
    port TimerPort

    @ Example data driver
    active component MyDriver {

        @ Internal interface/port for ISR reporting
        internal port IsrReport(interrupts: U32)

        @ Data counter
        telemetry DataBytes: U64
        @ Tick counter
        telemetry TimerTicks: U64

        @ Scheduler port
        async input port run: Svc.Sched

        @ Port to allocate a data buffer
        output port AllocateBuffer: Fw.BufferGet

        @ Port to send buffer with received data
        output port SendBuffer: Fw.BufferSend
        
        @ Port to send timer ticks (NOTE: runs in ISR context!)
        output port TimerPort: TimerPort

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Enables telemetry channels handling
        import Fw.Channel

    }
}