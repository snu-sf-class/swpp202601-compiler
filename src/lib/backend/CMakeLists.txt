cmake_minimum_required(VERSION 3.28.0)

function(add_backend_pass pass_name file_name)
    add_library(${pass_name} OBJECT ${file_name})
    target_include_directories(${pass_name} PRIVATE ${LLVM_INCLUDE_DIRS})
endfunction()

add_backend_pass(SCBackendConstExprElimPass const_expr_eliminate.cpp)
add_backend_pass(SCBackendGEPElimPass gep_eliminate.cpp)
add_backend_pass(SCBackendGVElimPass gv_eliminate.cpp)
add_backend_pass(SCBackendSextEliminatePass sext_eliminate.cpp)
add_backend_pass(SCBackendTruncAdjustPass trunc_adjust.cpp)
add_backend_pass(SCBackendMIRTranslatorPass mir_translator.cpp)
add_backend_pass(SCBackendMIRPrinterPass mir_printer.cpp)
add_backend_pass(SCBackendConstCopyEliminatePass const_copy_eliminate.cpp)

target_link_libraries(SCBackendConstCopyEliminatePass PRIVATE SWPPInstrInfo)
target_link_libraries(SCBackendMIRPrinterPass PRIVATE SWPPInstrInfo)
target_link_libraries(SCBackendMIRTranslatorPass PRIVATE SWPPInstrInfo)

add_subdirectory(SWPP)
