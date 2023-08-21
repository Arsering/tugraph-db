#include "lgraph/logger_C.h"

#include "lgraph/lgraph_yz.h"

#ifdef __cplusplus
extern "C" {
#endif

// 因为我们将使用C++的编译方式，用g++编译器来编译 robot_c_api.cpp 这个文件，
// 所以在这个文件中我们可以用C++代码去定义函数 void Robot_sayHi(const char
// *name)（在函数中使用C++的类 Robot）， 最后我们用 extern "C" 来告诉g++编译器，不要对
// Robot_sayHi(const char *name) 函数进行name mangling 这样最终生成的动态链接库中，函数
// Robot_sayHi(const char *name) 将生成 C 编译器的符号表示。

int log_breakdown_c(char* log) {
    std::string log_info = log;
    lgraph_api::yz_logger::log_breakdown(log_info);
    return 0;
}

#ifdef __cplusplus
}
#endif