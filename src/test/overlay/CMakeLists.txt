add_executable(test-overlay test_overlay.c)
add_linux_tests(BASENAME overlay COMMAND test-overlay)
add_shadow_tests(BASENAME overlay)
