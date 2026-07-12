add_executable(webplayground_test webplayground_test.cpp)
target_link_libraries(webplayground_test PRIVATE Ui Qt6::Test)

zeal_add_test(webplayground_test)

add_executable(developerterminal_test developerterminal_test.cpp)
target_link_libraries(developerterminal_test PRIVATE Ui Core Qt6::Test)

zeal_add_test(developerterminal_test)

add_executable(learningnotesstore_test learningnotesstore_test.cpp)
target_link_libraries(learningnotesstore_test PRIVATE Ui Qt6::Sql Qt6::Test)

zeal_add_test(learningnotesstore_test)
