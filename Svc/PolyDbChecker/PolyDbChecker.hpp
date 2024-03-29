// ======================================================================
// \title  PolyDbChecker.hpp
// \author tcanham
// \brief  hpp file for PolyDbChecker component implementation class
// ======================================================================

#ifndef Svc_PolyDbChecker_HPP
#define Svc_PolyDbChecker_HPP

#include "Svc/PolyDbChecker/PolyDbCheckerComponentAc.hpp"

namespace Svc {

  class PolyDbChecker :
    public PolyDbCheckerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Component construction and destruction
      // ----------------------------------------------------------------------

      //! Construct PolyDbChecker object
      PolyDbChecker(
          const char* const compName //!< The component name
      );

      //! Destroy PolyDbChecker object
      ~PolyDbChecker();

  };

}

#endif
