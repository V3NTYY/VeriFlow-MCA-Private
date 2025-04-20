# CMakeList.txt : CMake project for MCA_VeriFlow, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MCA_VeriFlow")

# Add source to this project's executable.
add_executable (MCA_VeriFlow "MCA_VeriFlow.cpp" "MCA_VeriFlow.h"  "Controller.cpp" "Controller.h" "Flow.cpp" "Flow.h" "OpenFlowMessage.h" "OpenFlowMessage.cpp" "Topology.h" "Topology.cpp" "Node.h" "Node.cpp" "json.hpp" "Digest.h" "Digest.cpp" "Log.h" "TCPAnalyzer.h" )

# Link pthread library
find_package(Threads REQUIRED)
target_link_libraries(MCA_VeriFlow PRIVATE Threads::Threads)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MCA_VeriFlow PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
