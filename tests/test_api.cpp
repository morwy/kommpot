// clazy:skip
// NOLINTBEGIN

#include "libkommpot.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

TEST(kommpot_tests, library_version)
{
    const kommpot::version version = kommpot::get_version();
    ASSERT_THAT(version.to_string().size(), Gt(0));
}

TEST(kommpot_tests, empty_device_list)
{
    const std::vector<kommpot::device_communication> device_list = kommpot::get_device_list(0);
    ASSERT_THAT(device_list.size(), Eq(0));
}

// NOLINTEND