# CmdStatusTester Test Sequences

This directory contains test sequences for validating the sequence directive system in F Prime's CmdSequencer component.

## Test Sequences

### Basic Directive Tests

1. **test_jcf_basic.seq** - Jump on Command Failure (JCF)
   - Tests basic JCF directive functionality
   - Demonstrates jumping to an error handler when a command fails
   - Expected: Executes TEST_CMD_SUCCESS, then TEST_CMD_FAIL triggers jump to ERROR_HANDLER, exits with status 1

2. **test_jcs_basic.seq** - Jump on Command Success (JCS)
   - Tests basic JCS directive functionality
   - Demonstrates jumping to a success path when a command succeeds
   - Expected: TEST_CMD_SUCCESS triggers jump to SUCCESS_PATH, skips intermediate commands, exits with status 0

3. **test_exit_codes.seq** - EXIT Directive
   - Tests EXIT directive with different status codes
   - Demonstrates EXIT 0 (success) and EXIT 1 (error)
   - Expected: Follows success path and exits with status 0

### ERROR_MODE Tests

4. **test_error_mode_off.seq** - ERROR_MODE OFF
   - Tests ERROR_MODE 0 (continue on errors)
   - Demonstrates that sequence continues even when commands fail
   - Expected: All commands execute despite TEST_CMD_FAIL failures, exits with status 0

5. **test_error_mode_toggle.seq** - ERROR_MODE Toggle
   - Tests switching ERROR_MODE between OFF and ON during execution
   - Demonstrates persistent ERROR_MODE state
   - Expected: Failures continue when OFF, then JCF handles failure when mode is ON

6. **test_error_mode_with_jcf.seq** - ERROR_MODE with JCF
   - Tests interaction between ERROR_MODE OFF and JCF directive
   - Demonstrates that JCF takes precedence over ERROR_MODE
   - Expected: JCF jumps even when ERROR_MODE is OFF, but other failures continue

### Advanced Patterns

7. **test_jcf_and_jcs.seq** - Combined JCF and JCS
   - Tests using both JCF and JCS on the same command
   - Demonstrates that only one directive executes based on command result
   - Expected: JCF executes for failing command, JCS executes for successful command

8. **test_retry_pattern.seq** - Retry Pattern
   - Tests a common retry pattern with multiple error handlers
   - Demonstrates chained error handlers for operation retries
   - Expected: First attempt fails and retries, second attempt fails and goes to fatal error

9. **test_forward_backward_jumps.seq** - Forward and Backward Jumps
   - Tests jumping both forward and backward in the sequence
   - Demonstrates that jumps can target labels in either direction
   - Expected: Jumps forward on failure, demonstrates backward jump capability

## Running Test Sequences

### Prerequisites

1. Build the TestDeploymentsProject:
   ```bash
   cd /home/tcanham/source/fprime/TestDeploymentsProject
   fprime-util build
   ```

2. Generate binary sequence files:
   ```bash
   cd /home/tcanham/source/fprime/TestDeploymentsProject/Ref/CmdStatusTester/seq
   
   # Generate all sequences
   for seq in *.seq; do
       fprime-seqgen -d ../../../build-fprime-automatic-native/dict/RefTopologyAppDictionary.xml \
                     "$seq" "${seq%.seq}.bin"
   done
   ```

### Running Sequences

1. Start the Ref deployment:
   ```bash
   cd /home/tcanham/source/fprime/TestDeploymentsProject
   fprime-gds
   ```

2. Load and run a sequence through the GDS:
   - Navigate to Commanding → Sequences
   - Upload a binary sequence file
   - Send the sequence run command

3. Or use command line:
   ```bash
   # Load sequence
   cmdStatusTester.CS_Run("/path/to/sequence.bin")
   ```

## Expected Behaviors

### TEST_CMD_SUCCESS
- Always returns command success status
- Logs TEST_CommandSuccess event with provided arguments
- Can be used to verify normal execution flow

### TEST_CMD_FAIL
- Always returns command failure status (EXECUTION_ERROR)
- Logs TEST_CommandFailed event with provided arguments
- Triggers JCF directive if one is active
- With ERROR_MODE ON, causes sequence abort unless JCF is set
- With ERROR_MODE OFF, sequence continues unless JCF is set

## Directive Behavior Summary

### LABEL "name"
- Marks a jump target location
- No execution behavior
- Referenced by JCF and JCS directives

### JCF "label"
- Sets jump target for next command's failure
- Cleared after next command executes (success or failure)
- Takes precedence over ERROR_MODE setting

### JCS "label"
- Sets jump target for next command's success
- Cleared after next command executes (success or failure)
- Works independently from ERROR_MODE

### EXIT status
- Terminates sequence immediately
- status=0: Success
- status=1: Error

### ERROR_MODE mode
- mode=0 (OFF): Command failures continue sequence execution
- mode=1 (ON): Command failures abort sequence (unless JCF is active)
- Persistent until next ERROR_MODE directive
- Default: ON at sequence start
- Reset to ON when sequence completes or is canceled

## Validation Checklist

When running these sequences, verify:

- [ ] JCF jumps occur on command failures
- [ ] JCS jumps occur on command successes
- [ ] ERROR_MODE OFF allows failures to continue
- [ ] ERROR_MODE ON causes abort on failure (without JCF)
- [ ] JCF takes precedence over ERROR_MODE OFF
- [ ] EXIT directives terminate with correct status
- [ ] Labels can be jumped to from before or after their definition
- [ ] Event logs show correct TEST_CommandSuccess and TEST_CommandFailed events
- [ ] Sequence completion events show correct final status

## Troubleshooting

If sequences don't behave as expected:

1. Check that CmdStatusTester is properly connected in the topology
2. Verify command dispatcher and event logger are working
3. Check sequence binary was generated correctly with fprime-seqgen
4. Review event logs for sequence execution events (CS_*)
5. Ensure time source is working for relative time commands
