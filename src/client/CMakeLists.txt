#SRC_LIST包含当前路径下的所有源文件
aux_source_directory(. SRC_LIST)
#指定生成可执行文件需要的源文件
add_executable(chatClient ${SRC_LIST})
#指定依赖的库文件
target_link_libraries(chatClient pthread)
