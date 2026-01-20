// ======================================================================
// \title  MyDriver.hpp
// \author tcanham
// \brief  hpp file for MyDriver component implementation class
// ======================================================================

#ifndef Drv_MyDriver_HPP
#define Drv_MyDriver_HPP

#include "Drv/MyDriver/MyDriverComponentAc.hpp"

namespace Drv {

class MyDriver final : public MyDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct MyDriver object
    MyDriver(const char* const compName  //!< The component name
    );

    //! Destroy MyDriver object
    ~MyDriver();

    //! Enable driver
    void enableDriver();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! Scheduler port
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined internal interfaces
    // ----------------------------------------------------------------------

    //! Handler implementation for IsrReport
    //!
    //! Internal interface/port for ISR reporting
    void IsrReport_internalInterfaceHandler(U32 interrupts) override;

    //! static interrupt service routine - required for OS API
    static void driverISR(int vector, void* user_ctx);
};

}  // namespace Drv

#endif
