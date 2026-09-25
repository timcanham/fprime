module Ref {

  @ Component for testing command status responses in sequences
  active component CmdStatusTester {

    # ----------------------------------------------------------------------
    # Special ports
    # ----------------------------------------------------------------------

    @ Command receive port
    command recv port CmdDisp

    @ Command registration port
    command reg port CmdReg

    @ Command response port
    command resp port CmdStatus

    @ Event port
    event port Log

    @ Text event port
    text event port LogText

    @ Time get port
    time get port Time

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Command that always succeeds
    async command TEST_CMD_SUCCESS(
      arg1: U32 @< First argument
      arg2: F32 @< Second argument
      arg3: bool @< Third argument
    ) \
      opcode 0

    @ Command that always fails
    async command TEST_CMD_FAIL(
      arg1: U32 @< First argument
      arg2: I16 @< Second argument
      arg3: string size 20 @< Third argument
    ) \
      opcode 1

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Command executed successfully
    event TEST_CommandSuccess(
      arg1: U32 @< First argument
      arg2: F32 @< Second argument
      arg3: bool @< Third argument
    ) \
      severity activity high \
      id 0 \
      format "TEST_CMD_SUCCESS executed: arg1={}, arg2={f}, arg3={}"

    @ Command execution failed
    event TEST_CommandFailed(
      arg1: U32 @< First argument
      arg2: I16 @< Second argument
      arg3: string size 20 @< Third argument
    ) \
      severity warning high \
      id 1 \
      format "TEST_CMD_FAIL executed: arg1={}, arg2={}, arg3={}"

  }

}
