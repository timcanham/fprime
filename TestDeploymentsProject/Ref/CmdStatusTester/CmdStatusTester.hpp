// ======================================================================
// \title  CmdStatusTester.hpp
// \author Generated
// \brief  hpp file for CmdStatusTester component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef Ref_CmdStatusTester_HPP
#define Ref_CmdStatusTester_HPP

#include "Ref/CmdStatusTester/CmdStatusTesterComponentAc.hpp"

namespace Ref {

class CmdStatusTester final : public CmdStatusTesterComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object CmdStatusTester
    //!
    CmdStatusTester(const char* const compName /*!< The component name*/
    );

    //! Destroy object CmdStatusTester
    //!
    ~CmdStatusTester();

  private:
    // ----------------------------------------------------------------------
    // Command handler implementations
    // ----------------------------------------------------------------------

    //! Implementation for TEST_CMD_SUCCESS command handler
    //! Command that always succeeds
    void TEST_CMD_SUCCESS_cmdHandler(
        const FwOpcodeType opCode, /*!< The opcode*/
        const U32 cmdSeq, /*!< The command sequence number*/
        U32 arg1, /*!< First argument*/
        F32 arg2, /*!< Second argument*/
        bool arg3 /*!< Third argument*/
    );

    //! Implementation for TEST_CMD_FAIL command handler
    //! Command that always fails
    void TEST_CMD_FAIL_cmdHandler(
        const FwOpcodeType opCode, /*!< The opcode*/
        const U32 cmdSeq, /*!< The command sequence number*/
        U32 arg1, /*!< First argument*/
        I16 arg2, /*!< Second argument*/
        const Fw::CmdStringArg& arg3 /*!< Third argument*/
    );
};

}  // end namespace Ref

#endif
