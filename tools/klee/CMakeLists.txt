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

find_library(SLICING Slicing HINTS /home/david/tau/slicing/se-slicing-3.4)
target_link_libraries(klee ${SLICING} ${KLEE_LIBS})
#target_link_libraries(klee ${KLEE_LIBS})

install(TARGETS klee RUNTIME DESTINATION bin)

# The KLEE binary depends on the runtimes
add_dependencies(klee BuildKLEERuntimes)
