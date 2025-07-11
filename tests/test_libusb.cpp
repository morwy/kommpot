// clazy:skip
// NOLINTBEGIN

#include "libkommpot.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

TEST(kommpot_libusb, empty_device_list)
{
    const std::vector<std::shared_ptr<kommpot::device_communication>> device_list =
        kommpot::devices();
    ASSERT_THAT(device_list.size(), Eq(0));
}

// NOLINTEND
