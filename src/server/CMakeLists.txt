#SRC_LIST变量包含当前目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
#通过指定的源文件生成可执行文件
add_executable(chatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
#指定可执行文件链接时需要依赖的库文件
target_link_libraries(chatServer muduo_net muduo_base mysqlclient hiredis pthread)