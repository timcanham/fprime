// ======================================================================
// \title  Directives.cpp
// \author Generated
// \brief  Test sequence directive functionality
//
// \copyright
// Copyright (C) 2009-2026 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
// ======================================================================

#include "Svc/CmdSequencer/test/ut/Directives.hpp"
#include "Svc/CmdSequencer/test/ut/CommandBuffers.hpp"
#include "Svc/CmdSequencer/test/ut/SequenceFiles/Buffers.hpp"
#include "Svc/CmdSequencer/test/ut/SequenceFiles/FPrime/FPrime.hpp"
#include <Fw/Com/ComBuffer.hpp>

namespace Svc {

namespace Directives {

// ----------------------------------------------------------------------
// Helper functions for creating directive buffers
// ----------------------------------------------------------------------

static void serializeLabel(const char* labelName, Fw::ComBuffer& comBuffer) {
    comBuffer.resetSer();
    // Directive ID = LABEL (0)
    U8 directiveId = CmdSequencerComponentImpl::Sequence::Record::LABEL;
    comBuffer.serialize(directiveId);
    // Label name
    U8 labelLen = static_cast<U8>(strlen(labelName));
    comBuffer.serialize(labelLen);
    comBuffer.serialize(reinterpret_cast<const U8*>(labelName), labelLen, Fw::Serialization::OMIT_LENGTH);
}

static void serializeJCF(const char* targetLabel, Fw::ComBuffer& comBuffer) {
    comBuffer.resetSer();
    // Directive ID = JCF (1)
    U8 directiveId = CmdSequencerComponentImpl::Sequence::Record::JCF;
    comBuffer.serialize(directiveId);
    // Target label name
    U8 labelLen = static_cast<U8>(strlen(targetLabel));
    comBuffer.serialize(labelLen);
    comBuffer.serialize(reinterpret_cast<const U8*>(targetLabel), labelLen, Fw::Serialization::OMIT_LENGTH);
}

static void serializeExit(U8 status, Fw::ComBuffer& comBuffer) {
    comBuffer.resetSer();
    // Directive ID = EXIT (2)
    U8 directiveId = CmdSequencerComponentImpl::Sequence::Record::EXIT;
    comBuffer.serialize(directiveId);
    // Exit status
    comBuffer.serialize(status);
}

static void serializeJCS(const char* targetLabel, Fw::ComBuffer& comBuffer) {
    comBuffer.resetSer();
    // Directive ID = JCS (3)
    U8 directiveId = CmdSequencerComponentImpl::Sequence::Record::JCS;
    comBuffer.serialize(directiveId);
    // Target label name
    U8 labelLen = static_cast<U8>(strlen(targetLabel));
    comBuffer.serialize(labelLen);
    comBuffer.serialize(reinterpret_cast<const U8*>(targetLabel), labelLen, Fw::Serialization::OMIT_LENGTH);
}

static void serializeDirectiveRecord(const Fw::ComBuffer& directiveBuffer, Fw::LinearBufferBase& buffer) {
    // Descriptor = SEQUENCE_DIRECTIVE (3)
    U8 descriptor = CmdSequencerComponentImpl::Sequence::Record::SEQUENCE_DIRECTIVE;
    buffer.serialize(descriptor);
    // Time (8 bytes, ignored for directives)
    U32 seconds = 0;
    U32 useconds = 0;
    buffer.serialize(seconds);
    buffer.serialize(useconds);
    // Record size
    U32 recordSize = directiveBuffer.getBuffLength();
    buffer.serialize(recordSize);
    // Directive buffer
    buffer.serialize(directiveBuffer.getBuffAddr(), recordSize, Fw::Serialization::OMIT_LENGTH);
}

// ----------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------

CmdSequencerTester ::CmdSequencerTester(const SequenceFiles::File::Format::t a_format)
    : ImmediateBase::CmdSequencerTester(a_format) {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void CmdSequencerTester ::Label() {
    // Create a sequence with just a LABEL directive followed by commands
    // The label should be a no-op
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 3;
    const U32 numRecords = numCommands + 1;  // 1 label + 3 commands

    // Calculate data size
    Fw::ComBuffer labelBuf;
    serializeLabel("START", labelBuf);
    const U32 labelRecordSize = 1 + 8 + 4 + labelBuf.getBuffLength();  // descriptor + time + size + data
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = labelRecordSize + (numCommands * commandRecordSize);
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // LABEL directive
    serializeDirectiveRecord(labelBuf, buffer);

    // Commands
    for (U32 i = 0; i < numCommands; i++) {
        Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
        const FwOpcodeType opcode = i;
        const U32 argument = i + 1;
        SequenceFiles::FPrime::Records::serialize(
            CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, opcode, argument, buffer);
    }

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "label_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->parameterizedAutoByCommand(fileName, numCommands, numCommands);
}

void CmdSequencerTester ::JumpOnFailure() {
    // Create sequence: CMD0, JCF "ERROR", CMD1 (will fail), CMD2, LABEL "ERROR", CMD3
    // When CMD1 fails, should jump to ERROR label and execute CMD3
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 4;  // CMD0, CMD1, CMD2, CMD3
    const U32 numDirectives = 2;  // JCF, LABEL
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcfBuf, labelBuf;
    serializeJCF("ERROR", jcfBuf);
    serializeLabel("ERROR", labelBuf);
    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 labelRecordSize = 1 + 8 + 4 + labelBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize =
        (numCommands * commandRecordSize) + jcfRecordSize + labelRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCF "ERROR"
    serializeDirectiveRecord(jcfBuf, buffer);

    // CMD1 (will fail)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // CMD2 (should be skipped)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // LABEL "ERROR"
    serializeDirectiveRecord(labelBuf, buffer);

    // CMD3 (recovery command)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 3, 4, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcf_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    // Verify sequence loaded
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it fail
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearAndDispatch();

    // Should log error and jump to label (CS_SequenceCanceled event)
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandError_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
    this->clearEvents();

    // Execute CMD3 (recovery command, skip CMD2)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(3, 4).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 3, Fw::CmdResponse::OK);
    this->clearAndDispatch();

    // Sequence should complete successfully
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);
}

void CmdSequencerTester ::NoJumpOnSuccess() {
    // Create sequence: CMD0, JCF "ERROR", CMD1 (will succeed), CMD2, LABEL "ERROR", CMD3
    // When CMD1 succeeds, should continue normally and skip the error handler
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 4;
    const U32 numDirectives = 2;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size (same as JumpOnFailure)
    Fw::ComBuffer jcfBuf, labelBuf;
    serializeJCF("ERROR", jcfBuf);
    serializeLabel("ERROR", labelBuf);
    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 labelRecordSize = 1 + 8 + 4 + labelBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize =
        (numCommands * commandRecordSize) + jcfRecordSize + labelRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCF "ERROR"
    serializeDirectiveRecord(jcfBuf, buffer);

    // CMD1 (will succeed)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // CMD2 (should execute)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // LABEL "ERROR"
    serializeDirectiveRecord(labelBuf, buffer);

    // CMD3 (should NOT execute)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 3, 4, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcf_success_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence - should execute CMD0, CMD1, CMD2 and complete (skip error handler)
    this->parameterizedAutoByCommand(fileName, 3, 3);  // Only 3 commands execute
}

void CmdSequencerTester ::JCFBeforeCommand() {
    // Create sequence with JCF before any command - should error
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numRecords = 1;

    // Calculate data size
    Fw::ComBuffer jcfBuf;
    serializeJCF("ERROR", jcfBuf);
    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 dataSize = jcfRecordSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // JCF "ERROR" (before any command)
    serializeDirectiveRecord(jcfBuf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcf_before_cmd_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence - should error
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    ASSERT_EVENTS_CS_InvalidMode_SIZE(1);  // Error: JCF before command
}

void CmdSequencerTester ::JCFLabelNotFound() {
    // Create sequence: CMD0, JCF "NONEXISTENT", CMD1 (will fail)
    // When CMD1 fails, should try to jump but label not found - abort
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 2;
    const U32 numDirectives = 1;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcfBuf;
    serializeJCF("NONEXISTENT", jcfBuf);
    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = (numCommands * commandRecordSize) + jcfRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCF "NONEXISTENT"
    serializeDirectiveRecord(jcfBuf, buffer);

    // CMD1 (will fail)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcf_not_found_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it fail
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearAndDispatch();

    // Should abort - label not found
    ASSERT_EVENTS_SIZE(3);
    ASSERT_EVENTS_CS_CommandError_SIZE(2);  // Error for failed command + label not found
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
}

void CmdSequencerTester ::ExitOK() {
    // Create sequence: CMD0, EXIT(0)
    // Should complete with OK status
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 1;
    const U32 numDirectives = 1;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer exitBuf;
    serializeExit(0, exitBuf);
    const U32 exitRecordSize = 1 + 8 + 4 + exitBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = commandRecordSize + exitRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // EXIT(0)
    serializeDirectiveRecord(exitBuf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "exit_ok_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // EXIT directive executes immediately and completes sequence
    this->clearAndDispatch();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);

    // Verify seqDone port was called with OK status
    ASSERT_from_seqDone_SIZE(1);
    ASSERT_from_seqDone(0, 0, 0, Fw::CmdResponse::OK);
}

void CmdSequencerTester ::ExitError() {
    // Create sequence: CMD0, EXIT(1)
    // Should complete with ERROR status
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 1;
    const U32 numDirectives = 1;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer exitBuf;
    serializeExit(1, exitBuf);
    const U32 exitRecordSize = 1 + 8 + 4 + exitBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = commandRecordSize + exitRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // EXIT(1)
    serializeDirectiveRecord(exitBuf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "exit_error_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // EXIT directive executes and completes with ERROR
    this->clearAndDispatch();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);

    // Verify seqDone port was called with ERROR status
    ASSERT_from_seqDone_SIZE(1);
    ASSERT_from_seqDone(0, 0, 0, Fw::CmdResponse::EXECUTION_ERROR);
}

void CmdSequencerTester ::BackwardJump() {
    // Create sequence: LABEL "LOOP", CMD0, CMD1, JCF "LOOP", CMD2 (fail), CMD3
    // CMD2 fails, jumps back to LOOP, executes CMD0, CMD1 again
    // This test just verifies backward jump works - doesn't test infinite loop
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 4;
    const U32 numDirectives = 2;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcfBuf, labelBuf;
    serializeJCF("LOOP", jcfBuf);
    serializeLabel("LOOP", labelBuf);
    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 labelRecordSize = 1 + 8 + 4 + labelBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize =
        (numCommands * commandRecordSize) + jcfRecordSize + labelRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // LABEL "LOOP"
    serializeDirectiveRecord(labelBuf, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // CMD1
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // JCF "LOOP"
    serializeDirectiveRecord(jcfBuf, buffer);

    // CMD2 (will fail and trigger backward jump)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // CMD3 (should not execute in this test)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 3, 4, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "backward_jump_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0 (first time)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 (first time)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD2 and make it fail - should jump back to LOOP
    this->invoke_to_comCmdOut(0, CommandBuffers::create(2, 3).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 2, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearAndDispatch();

    // Should log error and jump
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandError_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
    this->clearEvents();

    // Execute CMD0 (second time after backward jump)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
}

void CmdSequencerTester ::ForwardJump() {
    // Already tested in JumpOnFailure - this is just an alias
    this->JumpOnFailure();
}

void CmdSequencerTester ::ConsecutiveJCF() {
    // Create sequence: CMD0, JCF "LABEL1", JCF "LABEL2", CMD1 (fail)
    // Only the last JCF should be active - should jump to LABEL2
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 3;  // CMD0, CMD1, CMD2
    const U32 numDirectives = 4;  // JCF1, JCF2, LABEL1, LABEL2
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcf1Buf, jcf2Buf, label1Buf, label2Buf;
    serializeJCF("LABEL1", jcf1Buf);
    serializeJCF("LABEL2", jcf2Buf);
    serializeLabel("LABEL1", label1Buf);
    serializeLabel("LABEL2", label2Buf);

    const U32 jcf1RecordSize = 1 + 8 + 4 + jcf1Buf.getBuffLength();
    const U32 jcf2RecordSize = 1 + 8 + 4 + jcf2Buf.getBuffLength();
    const U32 label1RecordSize = 1 + 8 + 4 + label1Buf.getBuffLength();
    const U32 label2RecordSize = 1 + 8 + 4 + label2Buf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = (numCommands * commandRecordSize) + jcf1RecordSize +
                                jcf2RecordSize + label1RecordSize + label2RecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCF "LABEL1" (will be overwritten)
    serializeDirectiveRecord(jcf1Buf, buffer);

    // JCF "LABEL2" (this one should be active)
    serializeDirectiveRecord(jcf2Buf, buffer);

    // CMD1 (will fail)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // LABEL "LABEL1" (should NOT jump here)
    serializeDirectiveRecord(label1Buf, buffer);

    // CMD2 (should not execute)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // LABEL "LABEL2" (should jump here)
    serializeDirectiveRecord(label2Buf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "consecutive_jcf_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it fail
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearAndDispatch();

    // Should jump to LABEL2 (not LABEL1) and complete since there are no more commands
    ASSERT_EVENTS_SIZE(3);
    ASSERT_EVENTS_CS_CommandError_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);
}

void CmdSequencerTester ::JumpOnSuccess() {
    // Create sequence: CMD0, JCS "SUCCESS", CMD1 (will succeed), CMD2, LABEL "SUCCESS", CMD3
    // When CMD1 succeeds, should jump to SUCCESS label and execute CMD3
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 4;  // CMD0, CMD1, CMD2, CMD3
    const U32 numDirectives = 2;  // JCS, LABEL
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcsBuf, labelBuf;
    serializeJCS("SUCCESS", jcsBuf);
    serializeLabel("SUCCESS", labelBuf);
    const U32 jcsRecordSize = 1 + 8 + 4 + jcsBuf.getBuffLength();
    const U32 labelRecordSize = 1 + 8 + 4 + labelBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize =
        (numCommands * commandRecordSize) + jcsRecordSize + labelRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCS "SUCCESS"
    serializeDirectiveRecord(jcsBuf, buffer);

    // CMD1 (will succeed)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // CMD2 (should be skipped)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // LABEL "SUCCESS"
    serializeDirectiveRecord(labelBuf, buffer);

    // CMD3 (jump target command)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 3, 4, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcs_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    // Verify sequence loaded
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it succeed - should jump
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::OK);
    this->clearAndDispatch();

    // Should log complete and jump to label (CS_SequenceCanceled event)
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
    this->clearEvents();

    // Execute CMD3 (jump target command, skip CMD2)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(3, 4).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 3, Fw::CmdResponse::OK);
    this->clearAndDispatch();

    // Sequence should complete successfully
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);
}

void CmdSequencerTester ::NoJumpOnFailure() {
    // Create sequence: CMD0, JCS "SUCCESS", CMD1 (will fail), CMD2
    // When CMD1 fails, should NOT jump (JCS only jumps on success), sequence aborts
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 3;
    const U32 numDirectives = 1;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcsBuf;
    serializeJCS("SUCCESS", jcsBuf);
    const U32 jcsRecordSize = 1 + 8 + 4 + jcsBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = (numCommands * commandRecordSize) + jcsRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCS "SUCCESS"
    serializeDirectiveRecord(jcsBuf, buffer);

    // CMD1 (will fail)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // CMD2 (should not execute)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcs_failure_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it fail - should abort (no jump)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearAndDispatch();

    // Should abort - JCS doesn't jump on failure
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandError_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
}

void CmdSequencerTester ::JCSBeforeCommand() {
    // Create sequence with JCS before any command - should error
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numRecords = 1;

    // Calculate data size
    Fw::ComBuffer jcsBuf;
    serializeJCS("SUCCESS", jcsBuf);
    const U32 jcsRecordSize = 1 + 8 + 4 + jcsBuf.getBuffLength();
    const U32 dataSize = jcsRecordSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // JCS "SUCCESS" (before any command)
    serializeDirectiveRecord(jcsBuf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcs_before_cmd_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence - should error
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    ASSERT_EVENTS_CS_InvalidMode_SIZE(1);  // Error: JCS before command
}

void CmdSequencerTester ::JCSLabelNotFound() {
    // Create sequence: CMD0, JCS "NONEXISTENT", CMD1 (will succeed)
    // When CMD1 succeeds, should try to jump but label not found - abort
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 2;
    const U32 numDirectives = 1;
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcsBuf;
    serializeJCS("NONEXISTENT", jcsBuf);
    const U32 jcsRecordSize = 1 + 8 + 4 + jcsBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = (numCommands * commandRecordSize) + jcsRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCS "NONEXISTENT"
    serializeDirectiveRecord(jcsBuf, buffer);

    // CMD1 (will succeed)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcs_not_found_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it succeed
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::OK);
    this->clearAndDispatch();

    // Should abort - label not found
    ASSERT_EVENTS_SIZE(3);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    ASSERT_EVENTS_CS_CommandError_SIZE(1);  // Label not found
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
}

void CmdSequencerTester ::JCFAndJCSFailure() {
    // Create sequence: CMD0, JCF "ERROR", JCS "SUCCESS", CMD1 (will fail)
    // Both directives active, command fails - should use JCF
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 3;  // CMD0, CMD1, CMD2
    const U32 numDirectives = 4;  // JCF, JCS, LABEL ERROR, LABEL SUCCESS
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcfBuf, jcsBuf, labelErrorBuf, labelSuccessBuf;
    serializeJCF("ERROR", jcfBuf);
    serializeJCS("SUCCESS", jcsBuf);
    serializeLabel("ERROR", labelErrorBuf);
    serializeLabel("SUCCESS", labelSuccessBuf);

    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 jcsRecordSize = 1 + 8 + 4 + jcsBuf.getBuffLength();
    const U32 labelErrorRecordSize = 1 + 8 + 4 + labelErrorBuf.getBuffLength();
    const U32 labelSuccessRecordSize = 1 + 8 + 4 + labelSuccessBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = (numCommands * commandRecordSize) + jcfRecordSize +
                                jcsRecordSize + labelErrorRecordSize + labelSuccessRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCF "ERROR"
    serializeDirectiveRecord(jcfBuf, buffer);

    // JCS "SUCCESS"
    serializeDirectiveRecord(jcsBuf, buffer);

    // CMD1 (will fail)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // LABEL "ERROR"
    serializeDirectiveRecord(labelErrorBuf, buffer);

    // CMD2 (error handler - should execute)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // LABEL "SUCCESS"
    serializeDirectiveRecord(labelSuccessBuf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcf_jcs_failure_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it fail - should jump to ERROR (JCF)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::EXECUTION_ERROR);
    this->clearAndDispatch();

    // Should use JCF and jump to ERROR
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandError_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
    this->clearEvents();

    // Execute CMD2 (error handler)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(2, 3).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 2, Fw::CmdResponse::OK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);
}

void CmdSequencerTester ::JCFAndJCSSuccess() {
    // Create sequence: CMD0, JCF "ERROR", JCS "SUCCESS", CMD1 (will succeed)
    // Both directives active, command succeeds - should use JCS
    SequenceFiles::Buffers buffers;
    Fw::LinearBufferBase& buffer = buffers.get(this->format);

    const U32 numCommands = 3;  // CMD0, CMD1, CMD2
    const U32 numDirectives = 4;  // JCF, JCS, LABEL ERROR, LABEL SUCCESS
    const U32 numRecords = numCommands + numDirectives;

    // Calculate data size
    Fw::ComBuffer jcfBuf, jcsBuf, labelErrorBuf, labelSuccessBuf;
    serializeJCF("ERROR", jcfBuf);
    serializeJCS("SUCCESS", jcsBuf);
    serializeLabel("ERROR", labelErrorBuf);
    serializeLabel("SUCCESS", labelSuccessBuf);

    const U32 jcfRecordSize = 1 + 8 + 4 + jcfBuf.getBuffLength();
    const U32 jcsRecordSize = 1 + 8 + 4 + jcsBuf.getBuffLength();
    const U32 labelErrorRecordSize = 1 + 8 + 4 + labelErrorBuf.getBuffLength();
    const U32 labelSuccessRecordSize = 1 + 8 + 4 + labelSuccessBuf.getBuffLength();
    const U32 commandRecordSize = SequenceFiles::FPrime::Records::STANDARD_SIZE;
    const U32 recordDataSize = (numCommands * commandRecordSize) + jcfRecordSize +
                                jcsRecordSize + labelErrorRecordSize + labelSuccessRecordSize;
    const U32 dataSize = recordDataSize + SequenceFiles::FPrime::CRCs::SIZE;

    // Header
    const TimeBase timeBase = TimeBase::TB_WORKSTATION_TIME;
    const U32 timeContext = 0;
    SequenceFiles::FPrime::Headers::serialize(dataSize, numRecords, timeBase, timeContext, buffer);

    // CMD0
    Fw::Time t(TimeBase::TB_WORKSTATION_TIME, 0, 0);
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 0, 1, buffer);

    // JCF "ERROR"
    serializeDirectiveRecord(jcfBuf, buffer);

    // JCS "SUCCESS"
    serializeDirectiveRecord(jcsBuf, buffer);

    // CMD1 (will succeed)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 1, 2, buffer);

    // LABEL "ERROR"
    serializeDirectiveRecord(labelErrorBuf, buffer);

    // CMD2 (error handler - should NOT execute)
    SequenceFiles::FPrime::Records::serialize(
        CmdSequencerComponentImpl::Sequence::Record::RELATIVE, t, 2, 3, buffer);

    // LABEL "SUCCESS"
    serializeDirectiveRecord(labelSuccessBuf, buffer);

    // CRC
    SequenceFiles::FPrime::CRCs::serialize(buffer);

    // Write file
    const char* fileName = "jcf_jcs_success_test.seq";
    SequenceFiles::File::write(fileName, this->format, buffer);

    // Run sequence
    this->sendCmd_CS_RUN(0, 0, fileName, Svc::BlockState::NO_BLOCK);
    this->clearAndDispatch();

    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceLoaded(0, fileName);
    this->clearEvents();

    // Execute CMD0
    this->invoke_to_comCmdOut(0, CommandBuffers::create(0, 1).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 0, Fw::CmdResponse::OK);
    this->clearAndDispatch();
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    this->clearEvents();

    // Execute CMD1 and make it succeed - should jump to SUCCESS (JCS)
    this->invoke_to_comCmdOut(0, CommandBuffers::create(1, 2).getComBuffer(), 0);
    this->clearAndDispatch();
    this->sendCmd_Response(0, 1, Fw::CmdResponse::OK);
    this->clearAndDispatch();

    // Should use JCS and jump to SUCCESS
    ASSERT_EVENTS_SIZE(2);
    ASSERT_EVENTS_CS_CommandComplete_SIZE(1);
    ASSERT_EVENTS_CS_SequenceCanceled_SIZE(1);
    this->clearEvents();

    // Should complete immediately (no commands after SUCCESS label in this test)
    this->clearAndDispatch();
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_CS_SequenceComplete_SIZE(1);
}

}  // namespace Directives

}  // namespace Svc
