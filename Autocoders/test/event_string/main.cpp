#include <Autocoders/test/event_string/TestLogImpl.hpp>
#include <Autocoders/test/event_string/TestLogRecvImpl.hpp>
#include <Autocoders/test/time_tester/TestTimeImpl.hpp>
#include <Fw/Obj/SimpleObjRegistry.hpp>

int main(int argc, char* argv[]) {

#if FW_PORT_TRACING
    Fw::PortBase::setTrace(true);
#endif

#if FW_OBJECT_REGISTRATION == 1
    Fw::SimpleObjRegistry objReg;
#endif

    TestLogImpl testImpl("TestLogImpl");
	testImpl.init();
	TestLogRecvImpl logRecv("TestLogRecv");
	logRecv.init();
	TestTimeImpl timeSource("TimeComp");
	timeSource.init();

    testImpl.set_Log_OutputPort(0,logRecv.get_logRecvPort_InputPort(0));
    testImpl.set_Time_OutputPort(0,timeSource.get_timeGetPort_InputPort(0));
    testImpl.set_LogText_OutputPort(0,logRecv.get_textLogRecvPort_InputPort(0));

    timeSource.setTime(Fw::Time(TB_NONE,2,3));
#if FW_OBJECT_REGISTRATION == 1
	objReg.dump();
#endif
	Fw::LogStringArg arg("Log Something");
	testImpl.sendEvent(20,arg,40);
}
