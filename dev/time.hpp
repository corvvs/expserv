#ifndef TIME_HPP
# define TIME_HPP
# include "types.hpp"
# include <sys/time.h>
# include <ctime>

namespace WSTime {
    t_time_epoch_ms    get_epoch_ms();
}

#endif
