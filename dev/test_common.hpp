#ifndef TEST_COMMON_HPP
# define TEST_COMMON_HPP

# include <iostream>
# include <string>
# include <cstdlib>
# ifdef NDEBUG
#  define DXOUT(expr) ((void)0)
#  define DXERR(expr) ((void)0)
# else
#  define DXOUT(expr) do { debug_out(__FILE__, __LINE__, __func__) << expr << std::endl; } while(0)
#  define DXERR(expr) do { debug_err(__FILE__, __LINE__, __func__) << expr << std::endl; } while(0)
#endif

std::ostream&   debug_out(
    const char *filename,
    const int linenumber,
    const char *func
);

std::ostream&   debug_err(
    const char *filename,
    const int linenumber,
    const char *func
);

#endif
