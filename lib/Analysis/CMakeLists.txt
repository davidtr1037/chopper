#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

klee_add_component(kleeAnalysis
    ReachabilityAnalysis.cpp
    Inliner.cpp
    AAPass.cpp
    ModRefAnalysis.cpp
    Annotator.cpp
    Cloner.cpp
    Slicer.cpp
    SVFPointerAnalysis.cpp
    SliceGenerator.cpp
)

# TODO: Work out what the correct LLVM components are for kleeCore.
set(LLVM_COMPONENTS
  core
  support
)

if ("${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "3.6" OR
    "${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.6")
  list(APPEND LLVM_COMPONENTS mcjit executionengine native)
else()
  list(APPEND LLVM_COMPONENTS jit engine)
endif()

find_library(SVF_LIB Svf.so HINTS ${SVF_ROOT_DIR}/build/lib)
find_library(CUDD_LIB Cudd.so HINTS ${SVF_ROOT_DIR}/build/lib/CUDD)
find_library(LLVMDG_LIB LLVMdg HINTS ${DG_ROOT_DIR}/build/src)
find_library(LLVMPTA_LIB LLVMpta HINTS ${DG_ROOT_DIR}/build/src)
find_library(PTA_LIB PTA HINTS ${DG_ROOT_DIR}/build/src)
find_library(RD_LIB RD HINTS ${DG_ROOT_DIR}/build/src)

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeAnalysis PUBLIC
    ${SVF_LIB}
    ${CUDD_LIB}
    ${LLVMDG_LIB}
    ${LLVMPTA_LIB}
    ${PTA_LIB}
    ${RD_LIB}
    ${LLVM_LIBS}
)
target_include_directories(kleeAnalysis PUBLIC
    ${SVF_ROOT_DIR}/include
    ${DG_ROOT_DIR}/src
    ${DG_ROOT_DIR}/tools
)
