add_executable(webplayground_test webplayground_test.cpp)
target_link_libraries(webplayground_test PRIVATE Ui Qt6::Test)

zeal_add_test(webplayground_test)

add_executable(developerterminal_test developerterminal_test.cpp)
target_link_libraries(developerterminal_test PRIVATE Ui Core Qt6::Test)

zeal_add_test(developerterminal_test)

add_executable(learningnotesstore_test learningnotesstore_test.cpp)
target_link_libraries(learningnotesstore_test PRIVATE Ui Qt6::Sql Qt6::Test)

zeal_add_test(learningnotesstore_test)

add_executable(learningnotespanel_test learningnotespanel_test.cpp)
target_link_libraries(learningnotespanel_test PRIVATE Ui Core Qt6::Sql Qt6::Test Qt6::Widgets)

zeal_add_test(learningnotespanel_test)
set_tests_properties(learningnotespanel_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(learningnotesexporter_test learningnotesexporter_test.cpp)
target_link_libraries(learningnotesexporter_test PRIVATE Ui LibArchive::LibArchive Qt6::PrintSupport Qt6::Sql Qt6::Test)

zeal_add_test(learningnotesexporter_test)
set_tests_properties(learningnotesexporter_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(docsetmetadata_test docsetmetadata_test.cpp)
target_link_libraries(docsetmetadata_test PRIVATE Registry Qt6::Gui Qt6::Test)

zeal_add_test(docsetmetadata_test)
set_tests_properties(docsetmetadata_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(docsetcatalog_test docsetcatalog_test.cpp)
target_link_libraries(docsetcatalog_test PRIVATE Ui Qt6::Gui Qt6::Network Qt6::Test)

zeal_add_test(docsetcatalog_test)
set_tests_properties(docsetcatalog_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
