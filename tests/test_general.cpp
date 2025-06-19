// clazy:skip
// NOLINTBEGIN

#include "libkommpot.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

TEST(kommpot_general, library_version)
{
    const kommpot::version version = kommpot::get_version();
    ASSERT_THAT(version.to_string().size(), Gt(0));
}

TEST(kommpot_general, empty_device_list)
{
    const std::vector<std::shared_ptr<kommpot::device_communication>> device_list =
        kommpot::devices();
    ASSERT_THAT(device_list.size(), Eq(0));
}

// NOLINTEND
