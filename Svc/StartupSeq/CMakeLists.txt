####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
# UT_SOURCE_FILES: list of source files for unit tests
#
####

register_fprime_module(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/StartupSeq.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/StartupSeq.cpp"
  HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/StartupSeq.hpp"
  DEPENDS
    Svc_Seq
    Svc_Sched
    Svc_Ping
)

### Unit Tests ###
register_fprime_ut(
  AUTOCODER_INPUTS
    "${CMAKE_CURRENT_LIST_DIR}/StartupSeq.fpp"
  SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/StartupSeqTestMain.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/StartupSeqTester.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/TestState/TestState.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/Tick.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/Commands.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/test/ut/Rules/SeqDone.cpp"
  DEPENDS
    STest
  UT_AUTO_HELPERS
)
