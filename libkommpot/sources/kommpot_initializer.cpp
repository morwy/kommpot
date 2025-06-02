#include <kommpot_initializer.h>

#include <kommpot_core.h>

kommpot_initializer::kommpot_initializer()
{
    kommpot_core::instance();
}
