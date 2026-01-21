// ======================================================================
// \title  MyDriver.cpp
// \author tcanham
// \brief  cpp file for MyDriver component implementation class
// ======================================================================

#include "Drv/MyDriver/MyDriver.hpp"

// define some macros for register writes
#define INT_EN *(reinterpret_cast<U32*>(0x1000U))
#define INT_PEND *(reinterpret_cast<U32*>(0x1004U))
#define TIMER_VAL *(reinterpret_cast<U32*>(0x1008U))
#define TIMER_CNTL *(reinterpret_cast<U32*>(0x100BU))
#define BUFF_A_FIFO *(reinterpret_cast<U32*>(0x1010U))
#define BUFF_B_FIFO *(reinterpret_cast<U32*>(0x1014U))

static constexpr int DRIVER_VECTOR = 0x100;
static constexpr FwSizeType FIFO_DEPTH = 16;

static constexpr U32 INT_EN_TIMER = 0x1 << 0;
static constexpr U32 INT_EN_BUFFA_FULL = 0x1 << 1;
static constexpr U32 INT_EN_BUFFB_FULL = 0x1 << 2;

static constexpr U32 INT_PEND_TIMER = 0x1 << 0;
static constexpr U32 INT_PEND_BUFFA_FULL = 0x1 << 1;
static constexpr U32 INT_PEND_BUFFB_FULL = 0x1 << 2;

static constexpr U32 TIMER_CNTL_ENABLE = 0x1 << 0;

// prototype for ISR
extern "C" {
typedef void (*isr_func)(int vec, void* user_ctx);
int registerISR(int vec, isr_func isr, void* user_ctx);
};

namespace Drv {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

MyDriver::MyDriver(const char* const compName) : MyDriverComponentBase(compName), m_dataBytes(0), m_timerTicks(0) {}

MyDriver::~MyDriver() {}

void MyDriver::enableDriver(const U32 timerVal) {
    // clear pending interrupts
    INT_PEND = 0x0;
    // set timer value
    TIMER_VAL = timerVal;
    // enable timer
    TIMER_CNTL = TIMER_CNTL_ENABLE;
    // enable interrupts
    INT_EN = INT_EN_TIMER | INT_EN_BUFFA_FULL | INT_EN_BUFFB_FULL;
    // register ISR
    registerISR(DRIVER_VECTOR, MyDriver::driverISR, this);
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void MyDriver::run_handler(FwIndexType portNum, U32 context) {
    this->tlmWrite_DataBytes(this->m_dataBytes);
    this->tlmWrite_TimerTicks(this->m_timerTicks);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void MyDriver::IsrReport_internalInterfaceHandler(U32 interrupts) {
    // if the interrupt reported from the ISR is the buffer done interrupt,
    // copy the data
    if ((interrupts&INT_EN_BUFFA_FULL) or (interrupts&INT_EN_BUFFB_FULL)) {
        // get a buffer to fill
        Fw::Buffer buff;
        this->AllocateBuffer_out(0,FIFO_DEPTH);
        FW_ASSERT(buff.getSize() == FIFO_DEPTH,static_cast<FwAssertArgType>(buff.getSize()));
        FW_ASSERT(buff.getData());
        Fw::ExternalSerializeBufferWithMemberCopy serTo = buff.getSerializer();
        for (FwSizeType word = 0; word < FIFO_DEPTH/sizeof(U32); word++) {
            Fw::SerializeStatus stat;
            if (interrupts&INT_EN_BUFFA_FULL) {
                stat = serTo.serializeFrom(BUFF_A_FIFO);
            } else {
                stat = serTo.serializeFrom(BUFF_B_FIFO);
            }
            // There should always be room
            FW_ASSERT(stat = Fw::FW_SERIALIZE_OK,stat);
        }
        // send copied data to user
        this->SendBuffer_out(0,buff);
        // add data to counter
        this->m_dataBytes += FIFO_DEPTH;
    }
}

void MyDriver::driverISR(int vector, void* user_ctx) {
    FW_ASSERT(user_ctx);
    MyDriver* comp_ptr = static_cast<MyDriver*>(user_ctx);
    comp_ptr->doISR(vector);
}

void MyDriver::doISR(int vector) {
    // make sure we're getting the interrupt we expect
    FW_ASSERT(DRIVER_VECTOR == vector, vector);

    // get interrupts
    U32 ints = INT_PEND;
    // write back bits to clear interrupts
    INT_PEND = ints;
    // dispatch calls based on interrupts
    if (ints & INT_PEND_TIMER) {
        this->m_timerTicks++;
        this->TimerPort_out(0);
    } else { // if not tick, report the interrupt to
        // the driver thread for further processing
        this->IsrReport_internalInterfaceInvoke(ints);
    }
}

}  // namespace Drv
