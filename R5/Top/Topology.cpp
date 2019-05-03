#include <Components.hpp>

#include <Fw/Types/Assert.hpp>
#include <Os/Task.hpp>
#include <Os/Log.hpp>
#include <Os/File.hpp>
#include <Fw/Types/MallocAllocator.hpp>

// Component instances

// Rate Group Dividers for 10Hz and 1Hz

static NATIVE_INT_TYPE rgDivs[] = {100};
Svc::RateGroupDriverImpl rateGroupDriverComp("RGDRV",rgDivs,FW_NUM_ARRAY_ELEMENTS(rgDivs));

static NATIVE_UINT_TYPE rg1HzContext[] = {0,0,0,0,0,0,0,0,0,0};

Svc::ActiveRateGroupImpl rateGroup1HzComp("RG1Hz",rg1HzContext,FW_NUM_ARRAY_ELEMENTS(rg1HzContext));

// Command Components
Svc::ActiveLoggerImpl eventLogger("ELOG");

Svc::TlmChanImpl chanTlm("TLM");

Svc::CommandDispatcherImpl cmdDisp("CMDDISP");

Svc::HealthImpl health("health");

Svc::AssertFatalAdapterComponentImpl fatalAdapter("fatalAdapter");


void constructApp(void) {

    // Initialize rate group driver
    rateGroupDriverComp.init();

    // Initialize the rate groups
    rateGroup1HzComp.init(10,1);
    
    eventLogger.init(10,0);

    chanTlm.init(10,0);

    cmdDisp.init(20,0);

    fatalAdapter.init(0);
    health.init(25,0);

    /* Register commands */
    cmdDisp.regCommands();
    eventLogger.regCommands();
    health.regCommands();

    // set health ping entries

    // This list has to match the connections in RPITopologyAppAi.xml

//    Svc::HealthImpl::PingEntry pingEntries[] = {
//        {3,5,cmdDisp.getObjName()}, // 2
//        {3,5,cmdSeq.getObjName()}, // 3
//        {3,5,chanTlm.getObjName()}, // 4
//        {3,5,eventLogger.getObjName()}, // 5
//    };

    // register ping table
//    health.setPingEntries(pingEntries,FW_NUM_ARRAY_ELEMENTS(pingEntries),0x123);

    // load parameters

    // Active component startup
    // start rate groups
    rateGroup1HzComp.start(0, 119,10 * 1024);
    // start dispatcher
    cmdDisp.start(0,101,10*1024);
    // start telemetry
    eventLogger.start(0,98,10*1024);
    chanTlm.start(0,97,10*1024);

}

void exitTasks(void) {
    rateGroup1HzComp.exit();
    cmdDisp.exit();
    eventLogger.exit();
    chanTlm.exit();
}

extern "C" {
  void fsw_main(void);
}

void fsw_main(void) {
    constructApp();
}

