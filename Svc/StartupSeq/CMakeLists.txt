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
