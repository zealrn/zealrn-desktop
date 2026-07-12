add_executable(webplayground_test webplayground_test.cpp)
target_link_libraries(webplayground_test PRIVATE Ui Qt6::Test)

zeal_add_test(webplayground_test)

add_executable(developerterminal_test developerterminal_test.cpp)
target_link_libraries(developerterminal_test PRIVATE Ui Qt6::Test)

zeal_add_test(developerterminal_test)
