cmake_minimum_required(VERSION 3.16)

project(pcsc-cpp)

# The libpcsc-cpp library itself.

add_library(${PROJECT_NAME}
  STATIC
  include/${PROJECT_NAME}/${PROJECT_NAME}.hpp
  include/${PROJECT_NAME}/${PROJECT_NAME}-utils.hpp
  include/${PROJECT_NAME}/comp_winscard.hpp
  include/flag-set-cpp/flag_set.hpp
  include/magic_enum/magic_enum.hpp
  src/Context.hpp
  src/Reader.cpp
  src/SCardCall.hpp
  src/SmartCard.cpp
  src/listReaders.cpp
  src/utils.cpp
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include
)

target_compile_features(${PROJECT_NAME}
  PUBLIC
  cxx_std_17
)

target_compile_options(${PROJECT_NAME} PUBLIC
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<CXX_COMPILER_ID:MSVC>:WIN32_LEAN_AND_MEAN;UNICODE;_CRT_SECURE_NO_WARNINGS>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:Ws2_32>
)

# PC/SC API dependencies.

add_library(pcsc INTERFACE)
if(MSVC)
  target_link_libraries(pcsc INTERFACE winscard)
elseif(APPLE)
  target_link_libraries(pcsc INTERFACE "-framework PCSC")
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(PCSC libpcsclite REQUIRED IMPORTED_TARGET)
  target_include_directories(${PROJECT_NAME} PRIVATE ${PCSC_INCLUDE_DIRS})
  target_link_libraries(pcsc INTERFACE PkgConfig::PCSC)
endif()

# Common testing options.

enable_testing()

find_package(GTest REQUIRED)

# Mock tests that use libpcsc-mock to mock PC/SC API calls.

set(MOCK_TEST_EXE lib${PROJECT_NAME}-test-mock)

add_subdirectory(tests/lib/libpcsc-mock)

add_executable(${MOCK_TEST_EXE}
  tests/mock/test-select-card-reader-and-card.cpp
  tests/mock/test-connect-to-card-transmit-apdus.cpp
)

target_link_libraries(${MOCK_TEST_EXE}
  ${PROJECT_NAME}
  pcsc-mock
  GTest::Main
)

add_test(${MOCK_TEST_EXE} ${MOCK_TEST_EXE})

# Integration tests that use the real operating system PC/SC service.

set(INTEGRATION_TEST_EXE lib${PROJECT_NAME}-test-integration)

add_executable(${INTEGRATION_TEST_EXE}
  tests/integration/test-${PROJECT_NAME}.cpp
)

target_link_libraries(${INTEGRATION_TEST_EXE}
  ${PROJECT_NAME}
  pcsc
  GTest::Main
)
