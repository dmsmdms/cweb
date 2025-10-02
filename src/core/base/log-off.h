#pragma once

#ifdef NO_INFO
    #undef log_info
    #define log_info(fmt, ...) (void)0
#endif
#ifdef NO_DEBUG
    #undef log_debug
    #define log_debug(fmt, ...) (void)0
#endif
