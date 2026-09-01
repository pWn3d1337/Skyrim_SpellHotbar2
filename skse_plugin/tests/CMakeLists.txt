cmake_minimum_required(VERSION 3.21)

project(SpellHotbar2InputEventAdapterTests LANGUAGES CXX)

enable_testing()

add_executable(input_event_adapter_tests input_event_adapter_tests.cpp)
target_compile_features(input_event_adapter_tests PRIVATE cxx_std_23)
target_include_directories(input_event_adapter_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_test(NAME input_event_adapter_tests COMMAND input_event_adapter_tests)

add_executable(lifecycle_policy_tests lifecycle_policy_tests.cpp)
target_compile_features(lifecycle_policy_tests PRIVATE cxx_std_23)
target_include_directories(lifecycle_policy_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_test(NAME lifecycle_policy_tests COMMAND lifecycle_policy_tests)

add_executable(runtime_state_reset_tests runtime_state_reset_tests.cpp)
target_compile_features(runtime_state_reset_tests PRIVATE cxx_std_23)
target_include_directories(runtime_state_reset_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_test(NAME runtime_state_reset_tests COMMAND runtime_state_reset_tests)

add_executable(mcp_contract_tests mcp_contract_tests.cpp)
target_compile_features(mcp_contract_tests PRIVATE cxx_std_23)
target_include_directories(mcp_contract_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_test(NAME mcp_contract_tests COMMAND mcp_contract_tests)
