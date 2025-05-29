add_executable(test_jitter test_jitter.c)
target_link_libraries(test_jitter ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS test_jitter DESTINATION bin)

## add shadow tests that will use test_jitter
add_shadow_tests(BASENAME jitter_test SHADOW_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/jitter_test.yaml)