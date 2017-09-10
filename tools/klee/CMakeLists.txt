#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(klee
  main.cpp
)

set(KLEE_LIBS
  kleeCore
)

find_library(SVF_LIB Svf.so HINTS ${SVF_ROOT_DIR}/build/lib)
find_library(CUDD_LIB Cudd.so HINTS ${SVF_ROOT_DIR}/build/lib/CUDD)
find_library(LLVMDG_LIB LLVMdg HINTS ${DG_ROOT_DIR}/build/src)
find_library(LLVMPTA_LIB LLVMpta HINTS ${DG_ROOT_DIR}/build/src)
find_library(PTA_LIB PTA HINTS ${DG_ROOT_DIR}/build/src)
find_library(RD_LIB RD HINTS ${DG_ROOT_DIR}/build/src)

target_link_libraries(klee
    ${SVF_LIB}
    ${CUDD_LIB}
    ${LLVMDG_LIB}
    ${LLVMPTA_LIB}
    ${PTA_LIB}
    ${RD_LIB}
    ${KLEE_LIBS}
)

install(TARGETS klee RUNTIME DESTINATION bin)

# The KLEE binary depends on the runtimes
add_dependencies(klee BuildKLEERuntimes)
