// ======================================================================
// \title  MyDriver.cpp
// \author tcanham
// \brief  cpp file for MyDriver component implementation class
// ======================================================================

#include "Drv/MyDriver/MyDriver.hpp"

// define some macros for register writes
#define INT_EN *(reinterpret_cast<U32*>(0x1000U))

namespace Drv {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

MyDriver ::MyDriver(const char* const compName) : MyDriverComponentBase(compName) {}

MyDriver ::~MyDriver() {}

void MyDriver ::enableDriver() {

    // enable interrupts
    INT_EN = 0x7;

}


// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void MyDriver ::run_handler(FwIndexType portNum, U32 context) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void MyDriver ::IsrReport_internalInterfaceHandler(U32 interrupts) {
    // TODO
}

void MyDriver ::driverISR(int vector, void* user_ctx) {

}

}  // namespace Drv
