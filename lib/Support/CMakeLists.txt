#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeSupport
  CompressionStream.cpp
  ErrorHandling.cpp
  MemoryUsage.cpp
  PrintVersion.cpp
  RNG.cpp
  Time.cpp
  Timer.cpp
  TreeStream.cpp
)

target_link_libraries(kleeSupport PRIVATE ${ZLIB_LIBRARIES})

set(LLVM_COMPONENTS
  support
)
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeSupport PUBLIC ${LLVM_LIBS})
target_include_directories(kleeSupport PUBLIC
    ${SVF_ROOT_DIR}/include
    ${DG_ROOT_DIR}/src
    ${DG_ROOT_DIR}/tools
)
