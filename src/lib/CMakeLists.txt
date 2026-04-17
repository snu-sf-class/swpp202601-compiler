cmake_minimum_required(VERSION 3.28.0)

add_library(SCPrintIR print_ir.cpp)
target_include_directories(SCPrintIR PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(printer_llvm_libs core support transformutils)
target_link_libraries(SCPrintIR ${printer_llvm_libs})

add_library(SCOpt opt.cpp)
target_include_directories(SCOpt PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(opt_llvm_libs analysis passes scalaropts transformutils)
target_link_libraries(SCOpt PRIVATE SCPrintIR OptPasses ${opt_llvm_libs})

add_library(SCBackend backend.cpp)
target_include_directories(SCBackend PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(backend_llvm_libs Analysis CodeGen CodeGenTypes Core MC Passes Support Target TargetParser GlobalISel SelectionDAG ScalarOpts)
target_link_libraries(SCBackend PRIVATE SCPrintIR SCBackendConstExprElimPass
  SCBackendGEPElimPass SCBackendGVElimPass SCBackendSextEliminatePass SCBackendTruncAdjustPass
  SCBackendMIRTranslatorPass SCBackendMIRPrinterPass SCBackendConstCopyEliminatePass
  SWPPFrameLowering SWPPTargetLowering SWPPRegisterInfo SWPPInstrInfo SWPPTargetInfo SWPPMCTargetDesc SWPPSubtarget SWPPTargetMachine 
  ${backend_llvm_libs})

add_subdirectory(backend)
add_subdirectory(opt)
