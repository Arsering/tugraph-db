#include "lgraph/logger_C.h"

#include "lgraph/lgraph_yz.h"

#ifdef __cplusplus
extern "C" {
#endif

void log_breakdown_c(char* log) { lgraph_api::yz_logger::log_breakdown(log); }

#ifdef __cplusplus
}
#endif