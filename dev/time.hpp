#ifndef TIME_HPP
#define TIME_HPP
#include "types.hpp"
#include <ctime>
#include <sys/time.h>

namespace WSTime {
t_time_epoch_ms get_epoch_ms();
}

#endif
