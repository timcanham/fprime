// ======================================================================
// \title  Directives.hpp
// \author Generated
// \brief  Test sequence directive functionality
//
// \copyright
// Copyright (C) 2009-2026 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
// ======================================================================

#ifndef Svc_Directives_HPP
#define Svc_Directives_HPP

#include "Svc/CmdSequencer/test/ut/ImmediateBase.hpp"

namespace Svc {

namespace Directives {

//! Test sequences with directives (LABEL, JCF, EXIT)
class CmdSequencerTester : public ImmediateBase::CmdSequencerTester {
  public:
    // ----------------------------------------------------------------------
    // Constructors
    // ----------------------------------------------------------------------

    //! Construct object CmdSequencerTester
    CmdSequencerTester(const SequenceFiles::File::Format::t a_format =
                           SequenceFiles::File::Format::F_PRIME  //!< The file format to use
    );

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Test LABEL directive (should be no-op)
    void Label();

    //! Test JCF directive with command failure - should jump to label
    void JumpOnFailure();

    //! Test JCF directive with command success - should not jump
    void NoJumpOnSuccess();

    //! Test JCF before any command executed - should error
    void JCFBeforeCommand();

    //! Test JCF with non-existent label - should error
    void JCFLabelNotFound();

    //! Test EXIT directive with OK status
    void ExitOK();

    //! Test EXIT directive with ERROR status
    void ExitError();

    //! Test multiple labels and backward jump
    void BackwardJump();

    //! Test forward jump
    void ForwardJump();

    //! Test consecutive JCF directives (last one wins)
    void ConsecutiveJCF();

    //! Test JCS directive with command success - should jump
    void JumpOnSuccess();

    //! Test JCS directive with command failure - should not jump
    void NoJumpOnFailure();

    //! Test JCS before any command executed - should error
    void JCSBeforeCommand();

    //! Test JCS with non-existent label - should error
    void JCSLabelNotFound();

    //! Test both JCF and JCS active - command fails, should use JCF
    void JCFAndJCSFailure();

    //! Test both JCF and JCS active - command succeeds, should use JCS
    void JCFAndJCSSuccess();

    //! Test ERROR_MODE OFF - sequence continues on error
    void ErrorModeOff();

    //! Test ERROR_MODE ON - sequence aborts on error
    void ErrorModeOn();

    //! Test ERROR_MODE toggle - changes behavior mid-sequence
    void ErrorModeToggle();

    //! Test ERROR_MODE OFF with JCF - JCF takes precedence
    void ErrorModeOffWithJCF();
};

}  // namespace Directives

}  // namespace Svc

#endif
