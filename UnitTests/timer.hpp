#pragma once

#include "../time/time.hpp"

#define TIMER_PRINT(timerName) for(int64_t start = sprawl::time::Now(), end = start, dummy = 0; start == end; end = sprawl::time::Now(), dummy = printf("%f", double(end - start) / sprawl::time::Resolution::Milliseconds))

#define TIMER_COLLECT(storageLocation) for(int64_t start = sprawl::time::Now(), end = start; start == end; end = sprawl::time::Now(), storageLocation = end - start )
