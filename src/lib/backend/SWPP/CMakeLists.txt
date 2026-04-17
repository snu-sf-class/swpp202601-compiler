cmake_minimum_required(VERSION 3.28.0)

include(TableGen)
include(AddLLVM)

find_program(LLVM_TABLEGEN_EXE llvm-tblgen HINTS "${LLVM_TOOLS_BINARY_DIR}")
message(STATUS "Found llvm-tblgen : ${LLVM_TABLEGEN_EXE}")

set(SWPP_TD "SWPP.td")
set(LLVM_TARGET_DEFINITIONS ${SWPP_TD})
tablegen(LLVM SWPPGenRegisterInfo.inc --gen-register-info EXTRA_INCLUDES ${LLVM_INCLUDE_DIRS})
add_public_tablegen_target(SWPPGenRegisterInfo)
message(STATUS "Generated ${TABLEGEN_OUTPUT} from ${SWPP_TD}")

function(add_swpp_machine_target target_name file_name)
    add_library(${target_name} OBJECT ${file_name})
    target_include_directories(${target_name} PUBLIC ${LLVM_INCLUDE_DIRS} ${TABLEGEN_INCLUDE_DIR})
endfunction()

add_swpp_machine_target(SWPPFrameLowering SWPPFrameLowering.cpp)
add_swpp_machine_target(SWPPTargetLowering SWPPTargetLowering.cpp)
add_swpp_machine_target(SWPPRegisterInfo SWPPRegisterInfo.cpp)
add_swpp_machine_target(SWPPInstrInfo SWPPInstrInfo.cpp)
add_swpp_machine_target(SWPPSubtarget SWPPSubtarget.cpp)
add_swpp_machine_target(SWPPTargetMachine SWPPTargetMachine.cpp)
add_swpp_machine_target(SWPPTargetInfo SWPPTargetInfo.cpp)
add_swpp_machine_target(SWPPMCTargetDesc SWPPMCTargetDesc.cpp)

add_dependencies(SWPPRegisterInfo SWPPGenRegisterInfo)
add_dependencies(SWPPMCTargetDesc SWPPGenRegisterInfo)
