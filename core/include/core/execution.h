#ifndef EXECUTION_H
#define EXECUTION_H

#if defined(__APPLE__)
#define STD_EXECUTION_PAR_UNSEQ
#else
#include <execution>

#define STD_EXECUTION_PAR_UNSEQ std::execution::par_unseq,
#endif

#endif // EXECUTION_H
