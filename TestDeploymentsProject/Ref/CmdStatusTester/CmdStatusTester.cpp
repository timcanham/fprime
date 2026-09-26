// ======================================================================
// \title  CmdStatusTester.cpp
// \author Generated
// \brief  cpp file for CmdStatusTester component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <Ref/CmdStatusTester/CmdStatusTester.hpp>

namespace Ref {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

CmdStatusTester::CmdStatusTester(const char* const compName)
    : CmdStatusTesterComponentBase(compName) {}

CmdStatusTester::~CmdStatusTester() {}

// ----------------------------------------------------------------------
// Command handler implementations
// ----------------------------------------------------------------------

void CmdStatusTester::TEST_CMD_SUCCESS_cmdHandler(const FwOpcodeType opCode,
                                                   const U32 cmdSeq,
                                                   U32 arg1,
                                                   F32 arg2,
                                                   bool arg3) {
    // Log the successful command execution
    this->log_ACTIVITY_HI_TEST_CommandSuccess(arg1, arg2, arg3);

    // Send successful command response
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void CmdStatusTester::TEST_CMD_FAIL_cmdHandler(const FwOpcodeType opCode,
                                                const U32 cmdSeq,
                                                U32 arg1,
                                                I16 arg2,
                                                const Fw::CmdStringArg& arg3) {
    // Log the failed command execution
    this->log_WARNING_HI_TEST_CommandFailed(arg1, arg2, arg3);

    // Send failed command response
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
}

}  // end namespace Ref
