// clazy:skip
// NOLINTBEGIN

#include <libkommpot.h>

#include <communications/ethernet/ethernet_address.h>
#include <communications/ethernet/ethernet_address_factory.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <limits>
#include <memory>
#include <string>

#ifdef _WIN32
// clang-format off
#    include <ws2tcpip.h>
// clang-format on
#else
#    include <arpa/inet.h>
#    include <netinet/in.h>
#endif

using namespace testing;

/*******************************************************************************
 *
 * Helper functions.
 *
 *******************************************************************************/
static std::shared_ptr<ethernet_ip_address> make_shared_ipv4(const std::string &dotted)
{
    auto addr = ethernet_address_factory::from_string(dotted);

    EXPECT_TRUE(addr.has_value());

    if (!addr.has_value())
    {
        return nullptr;
    }

    return *addr;
}

static std::shared_ptr<ethernet_ip_address> make_shared_ipv6(const std::string &colon_hex)
{
    auto addr = ethernet_address_factory::from_string(colon_hex);

    EXPECT_TRUE(addr.has_value());

    if (!addr.has_value())
    {
        return nullptr;
    }

    return *addr;
}

static std::shared_ptr<ethernet_ip_address> make_shared_ipv4_from_uint32(uint32_t v)
{
    auto addr = ethernet_address_factory::from_uint32_t(v);

    EXPECT_TRUE(addr.has_value());

    if (!addr.has_value())
    {
        return nullptr;
    }

    return *addr;
}

/*******************************************************************************
 *
 * from_string — type routing (dot > IPv4, colon > IPv6).
 *
 *******************************************************************************/
TEST(factory_from_string, returns_true_for_valid_ipv4)
{
    auto addr = ethernet_address_factory::from_string("192.168.1.1");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, returns_true_for_valid_ipv6)
{
    auto addr = ethernet_address_factory::from_string("2001:db8::1");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, returns_false_for_no_delimiter)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("hello").has_value());
}

TEST(factory_from_string, returns_false_for_empty_string)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("").has_value());
}

TEST(factory_from_string, returns_false_for_plain_number)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("12345").has_value());
}

TEST(factory_from_string, returns_false_for_whitespace_only)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("   ").has_value());
}

TEST(factory_from_string, output_is_not_null_on_ipv4_success)
{
    auto addr = ethernet_address_factory::from_string("10.0.0.1");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, output_is_not_null_on_ipv6_success)
{
    auto addr = ethernet_address_factory::from_string("::1");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, output_is_ipv4_type_for_dotted_input)
{
    auto addr = ethernet_address_factory::from_string("192.168.0.1");
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr).get()), nullptr);
}

TEST(factory_from_string, output_is_not_ipv6_type_for_dotted_input)
{
    auto addr = ethernet_address_factory::from_string("192.168.0.1");
    EXPECT_EQ(dynamic_cast<ethernet_ipv6_address *>((*addr).get()), nullptr);
}

TEST(factory_from_string, output_is_ipv6_type_for_colon_input)
{
    auto addr = ethernet_address_factory::from_string("2001:db8::1");
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*addr).get()), nullptr);
}

TEST(factory_from_string, output_is_not_ipv4_type_for_colon_input)
{
    auto addr = ethernet_address_factory::from_string("2001:db8::1");
    EXPECT_EQ(dynamic_cast<ethernet_ipv4_address *>((*addr).get()), nullptr);
}

/*******************************************************************************
 *
 * from_string — output parameter is unchanged on failure.
 *
 *******************************************************************************/
TEST(factory_from_string, does_not_modify_address_on_no_delimiter_failure)
{
    auto original = make_shared_ipv4("10.0.0.1");
    auto result = ethernet_address_factory::from_string("invalid");
    EXPECT_FALSE(result.has_value());
}

TEST(factory_from_string, does_not_modify_address_on_ipv4_parse_failure)
{
    auto original = make_shared_ipv4("10.0.0.1");
    auto result = ethernet_address_factory::from_string("999.999.999.999");
    EXPECT_FALSE(result.has_value());
}

TEST(factory_from_string, does_not_modify_address_on_ipv6_parse_failure)
{
    auto original = make_shared_ipv6("::1");
    auto result = ethernet_address_factory::from_string("::::");
    EXPECT_FALSE(result.has_value());
}

TEST(factory_from_string, null_output_stays_null_on_failure)
{
    auto addr = ethernet_address_factory::from_string("garbage");
    EXPECT_FALSE(addr.has_value());
}

/*******************************************************************************
 *
 * from_string — mixed delimiter input (both dots and colons).
 *
 *******************************************************************************/
TEST(factory_from_string, dot_and_colon_mixed_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1.2:3.4").has_value());
}

TEST(factory_from_string, ipv4_mapped_ipv6_notation_returns_true)
{
    auto addr = ethernet_address_factory::from_string("::ffff:192.168.1.1");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, port_suffix_on_ipv4_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("10.0.0.1:8080").has_value());
}

/*******************************************************************************
 *
 * from_string — overwrite behavior on success.
 *
 *******************************************************************************/
TEST(factory_from_string, overwrites_previous_ipv4_with_new_ipv4)
{
    auto addr = ethernet_address_factory::from_string("1.1.1.1");
    ASSERT_TRUE(addr.has_value());
    EXPECT_EQ((*addr)->to_string(), "1.1.1.1");

    auto addr2 = ethernet_address_factory::from_string("2.2.2.2");
    ASSERT_TRUE(addr2.has_value());
    EXPECT_EQ((*addr2)->to_string(), "2.2.2.2");
}

TEST(factory_from_string, overwrites_ipv4_with_ipv6)
{
    auto addr = ethernet_address_factory::from_string("1.2.3.4");
    ASSERT_TRUE(addr.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr).get()), nullptr);

    auto addr2 = ethernet_address_factory::from_string("::1");
    ASSERT_TRUE(addr2.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*addr2).get()), nullptr);
}

TEST(factory_from_string, overwrites_ipv6_with_ipv4)
{
    auto addr = ethernet_address_factory::from_string("::1");
    ASSERT_TRUE(addr.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*addr).get()), nullptr);

    auto addr2 = ethernet_address_factory::from_string("10.0.0.1");
    ASSERT_TRUE(addr2.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr2).get()), nullptr);
}

/*******************************************************************************
 *
 * from_uint32_t — return value and type.
 *
 *******************************************************************************/
TEST(factory_from_uint32, always_returns_true)
{
    auto addr = ethernet_address_factory::from_uint32_t(0);
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_uint32, returns_true_for_max_value)
{
    auto addr = ethernet_address_factory::from_uint32_t(0xFFFFFFFFu);
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_uint32, produces_ipv4_type)
{
    auto addr = ethernet_address_factory::from_uint32_t(0);
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr).get()), nullptr);
}

TEST(factory_from_uint32, does_not_produce_ipv6_type)
{
    auto addr = ethernet_address_factory::from_uint32_t(0);
    EXPECT_EQ(dynamic_cast<ethernet_ipv6_address *>((*addr).get()), nullptr);
}

TEST(factory_from_uint32, output_is_not_null)
{
    auto addr = ethernet_address_factory::from_uint32_t(0x0A000001u);
    EXPECT_TRUE(addr.has_value());
}

/*******************************************************************************
 *
 * from_uint32_t — boundary values.
 *
 *******************************************************************************/
TEST(factory_from_uint32, zero_produces_all_zeros)
{
    auto addr = make_shared_ipv4_from_uint32(0x00000000u);
    EXPECT_EQ(addr->to_string(), "0.0.0.0");
}

TEST(factory_from_uint32, max_produces_broadcast)
{
    auto addr = make_shared_ipv4_from_uint32(0xFFFFFFFFu);
    EXPECT_EQ(addr->to_string(), "255.255.255.255");
}

TEST(factory_from_uint32, loopback)
{
    auto addr = make_shared_ipv4_from_uint32(0x7F000001u);
    EXPECT_EQ(addr->to_string(), "127.0.0.1");
}

TEST(factory_from_uint32, first_octet_max)
{
    auto addr = make_shared_ipv4_from_uint32(0xFF000000u);
    EXPECT_EQ(addr->to_string(), "255.0.0.0");
}

TEST(factory_from_uint32, second_octet_max)
{
    auto addr = make_shared_ipv4_from_uint32(0x00FF0000u);
    EXPECT_EQ(addr->to_string(), "0.255.0.0");
}

TEST(factory_from_uint32, third_octet_max)
{
    auto addr = make_shared_ipv4_from_uint32(0x0000FF00u);
    EXPECT_EQ(addr->to_string(), "0.0.255.0");
}

TEST(factory_from_uint32, fourth_octet_max)
{
    auto addr = make_shared_ipv4_from_uint32(0x000000FFu);
    EXPECT_EQ(addr->to_string(), "0.0.0.255");
}

TEST(factory_from_uint32, one)
{
    auto addr = make_shared_ipv4_from_uint32(0x00000001u);
    EXPECT_EQ(addr->to_string(), "0.0.0.1");
}

TEST(factory_from_uint32, high_bit_only)
{
    auto addr = make_shared_ipv4_from_uint32(0x80000000u);
    EXPECT_EQ(addr->to_string(), "128.0.0.0");
}

TEST(factory_from_uint32, class_a_private)
{
    auto addr = make_shared_ipv4_from_uint32(0x0A000001u);
    EXPECT_EQ(addr->to_string(), "10.0.0.1");
}

TEST(factory_from_uint32, class_c_private)
{
    auto addr = make_shared_ipv4_from_uint32(0xC0A80164u);
    EXPECT_EQ(addr->to_string(), "192.168.1.100");
}

/*******************************************************************************
 *
 * from_uint32_t — round-trip consistency.
 *
 *******************************************************************************/
TEST(factory_from_uint32, round_trip_to_uint32_zero)
{
    auto addr = ethernet_address_factory::from_uint32_t(0);

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>((*addr).get());
    ASSERT_NE(ipv4, nullptr);
    EXPECT_EQ(ipv4->to_uint32(), 0u);
}

TEST(factory_from_uint32, round_trip_to_uint32_max)
{
    auto addr = ethernet_address_factory::from_uint32_t(0xFFFFFFFFu);

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>((*addr).get());
    ASSERT_NE(ipv4, nullptr);
    EXPECT_EQ(ipv4->to_uint32(), 0xFFFFFFFFu);
}

TEST(factory_from_uint32, round_trip_to_uint32_arbitrary)
{
    const uint32_t values[] = {0x01020304u, 0x0A141E28u, 0xC0A8FEFFu, 0x7F000001u};

    for (uint32_t v : values)
    {
        auto addr = ethernet_address_factory::from_uint32_t(v);

        auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>((*addr).get());
        ASSERT_NE(ipv4, nullptr);
        EXPECT_EQ(ipv4->to_uint32(), v);
    }
}

TEST(factory_from_uint32, consistent_with_from_string)
{
    auto from_str = ethernet_address_factory::from_string("192.168.1.1");

    auto from_u32 = ethernet_address_factory::from_uint32_t(0xC0A80101u);

    EXPECT_EQ((*from_str)->to_string(), (*from_u32)->to_string());
}

/*******************************************************************************
 *
 * from_uint32_t — overwrites previous value.
 *
 *******************************************************************************/
TEST(factory_from_uint32, overwrites_previous_value)
{
    auto addr = ethernet_address_factory::from_uint32_t(0x01020304u);
    EXPECT_EQ((*addr)->to_string(), "1.2.3.4");

    auto addr2 = ethernet_address_factory::from_uint32_t(0x0A000001u);
    EXPECT_EQ((*addr2)->to_string(), "10.0.0.1");
}

/*******************************************************************************
 *
 * calculate_base_address — IPv4.
 *
 *******************************************************************************/
TEST(factory_calculate_base_address, ipv4_class_c_mask)
{
    auto ip = make_shared_ipv4("192.168.1.100");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "192.168.1.0");
}

TEST(factory_calculate_base_address, ipv4_class_b_mask)
{
    auto ip = make_shared_ipv4("172.16.30.40");
    auto mask = make_shared_ipv4_from_uint32(0xFFFF0000u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "172.16.0.0");
}

TEST(factory_calculate_base_address, ipv4_class_a_mask)
{
    auto ip = make_shared_ipv4("10.20.30.40");
    auto mask = make_shared_ipv4_from_uint32(0xFF000000u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "10.0.0.0");
}

TEST(factory_calculate_base_address, ipv4_host_mask_returns_same)
{
    auto ip = make_shared_ipv4("192.168.1.42");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFFFFu);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "192.168.1.42");
}

TEST(factory_calculate_base_address, ipv4_zero_mask_returns_zeros)
{
    auto ip = make_shared_ipv4("192.168.1.42");
    auto mask = make_shared_ipv4_from_uint32(0x00000000u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "0.0.0.0");
}

TEST(factory_calculate_base_address, ipv4_all_zeros_address)
{
    auto ip = make_shared_ipv4("0.0.0.0");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "0.0.0.0");
}

TEST(factory_calculate_base_address, ipv4_broadcast_with_mask)
{
    auto ip = make_shared_ipv4("255.255.255.255");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "255.255.255.0");
}

TEST(factory_calculate_base_address, ipv4_result_is_ipv4_type)
{
    auto ip = make_shared_ipv4("10.0.0.1");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*base).get()), nullptr);
}

TEST(factory_calculate_base_address, ipv4_slash_25_mask)
{
    auto ip = make_shared_ipv4("192.168.1.200");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF80u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "192.168.1.128");
}

TEST(factory_calculate_base_address, ipv4_slash_20_mask)
{
    auto ip = make_shared_ipv4("10.0.15.255");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFF000u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "10.0.0.0");
}

/*******************************************************************************
 *
 * calculate_base_address — IPv6.
 *
 *******************************************************************************/
TEST(factory_calculate_base_address, ipv6_slash_64_mask)
{
    auto ip = make_shared_ipv6("2001:db8::abcd:1234");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "2001:db8:0:0:0:0:0:0");
}

TEST(factory_calculate_base_address, ipv6_slash_128_mask_returns_same)
{
    auto ip = make_shared_ipv6("2001:db8::1");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "2001:db8:0:0:0:0:0:1");
}

TEST(factory_calculate_base_address, ipv6_zero_mask_returns_zeros)
{
    auto ip = make_shared_ipv6("2001:db8::1");
    auto mask = make_shared_ipv6("::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "0:0:0:0:0:0:0:0");
}

TEST(factory_calculate_base_address, ipv6_slash_48_mask)
{
    auto ip = make_shared_ipv6("2001:db8:abcd:ef01:1234:5678:9abc:def0");
    auto mask = make_shared_ipv6("ffff:ffff:ffff::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "2001:db8:abcd:0:0:0:0:0");
}

TEST(factory_calculate_base_address, ipv6_all_zeros_address)
{
    auto ip = make_shared_ipv6("::");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "0:0:0:0:0:0:0:0");
}

TEST(factory_calculate_base_address, ipv6_all_max_with_mask)
{
    auto ip = make_shared_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "ffff:ffff:ffff:ffff:0:0:0:0");
}

TEST(factory_calculate_base_address, ipv6_result_is_ipv6_type)
{
    auto ip = make_shared_ipv6("::1");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*base).get()), nullptr);
}

/*******************************************************************************
 *
 * calculate_base_address — mismatched / invalid arguments.
 *
 *******************************************************************************/
TEST(factory_calculate_base_address, ipv4_address_with_ipv6_mask_returns_false)
{
    auto ip = make_shared_ipv4("192.168.1.1");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    EXPECT_FALSE(ethernet_address_factory::calculate_base_address(ip, mask).has_value());
}

TEST(factory_calculate_base_address, ipv6_address_with_ipv4_mask_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    EXPECT_FALSE(ethernet_address_factory::calculate_base_address(ip, mask).has_value());
}

/*******************************************************************************
 *
 * calculate_new_address — IPv4.
 *
 *******************************************************************************/
TEST(factory_calculate_new_address, ipv4_offset_zero)
{
    auto base = make_shared_ipv4("192.168.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "192.168.0.0");
}

TEST(factory_calculate_new_address, ipv4_offset_one)
{
    auto base = make_shared_ipv4("192.168.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "192.168.0.1");
}

TEST(factory_calculate_new_address, ipv4_offset_causes_octet_carry)
{
    auto base = make_shared_ipv4("10.0.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 256);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "10.0.1.0");
}

TEST(factory_calculate_new_address, ipv4_offset_large)
{
    auto base = make_shared_ipv4("10.0.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 65536);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "10.1.0.0");
}

TEST(factory_calculate_new_address, ipv4_from_zero_offset_one)
{
    auto base = make_shared_ipv4("0.0.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0.0.0.1");
}

TEST(factory_calculate_new_address, ipv4_from_zero_offset_max_uint32)
{
    auto base = make_shared_ipv4("0.0.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 0xFFFFFFFFu);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "255.255.255.255");
}

TEST(factory_calculate_new_address, ipv4_result_is_ipv4_type)
{
    auto base = make_shared_ipv4("10.0.0.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*result).get()), nullptr);
}

TEST(factory_calculate_new_address, ipv4_offset_to_adjacent)
{
    auto base = make_shared_ipv4("192.168.1.0");

    auto result = ethernet_address_factory::calculate_new_address(base, 254);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "192.168.1.254");
}

/*******************************************************************************
 *
 * calculate_new_address — IPv6.
 *
 *******************************************************************************/
TEST(factory_calculate_new_address, ipv6_offset_zero)
{
    auto base = make_shared_ipv6("2001:db8::");

    auto result = ethernet_address_factory::calculate_new_address(base, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "2001:db8:0:0:0:0:0:0");
}

TEST(factory_calculate_new_address, ipv6_offset_one)
{
    auto base = make_shared_ipv6("2001:db8::");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "2001:db8:0:0:0:0:0:1");
}

TEST(factory_calculate_new_address, ipv6_offset_causes_segment_carry)
{
    auto base = make_shared_ipv6("2001:db8::ffff");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "2001:db8:0:0:0:0:1:0");
}

TEST(factory_calculate_new_address, ipv6_offset_large_across_segments)
{
    auto base = make_shared_ipv6("::");

    auto result = ethernet_address_factory::calculate_new_address(base, 0x10000);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:0:0:0:1:0");
}

TEST(factory_calculate_new_address, ipv6_offset_multiple_carries)
{
    auto base = make_shared_ipv6("::ffff:ffff");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:0:0:1:0:0");
}

TEST(factory_calculate_new_address, ipv6_from_zero_offset_one)
{
    auto base = make_shared_ipv6("::");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:0:0:0:0:1");
}

TEST(factory_calculate_new_address, ipv6_result_is_ipv6_type)
{
    auto base = make_shared_ipv6("2001:db8::");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*result).get()), nullptr);
}

TEST(factory_calculate_new_address, ipv6_offset_0xFFFF)
{
    auto base = make_shared_ipv6("::");

    auto result = ethernet_address_factory::calculate_new_address(base, 0xFFFF);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:0:0:0:0:ffff");
}

TEST(factory_calculate_new_address, ipv6_offset_0x10001)
{
    auto base = make_shared_ipv6("::");

    auto result = ethernet_address_factory::calculate_new_address(base, 0x10001);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:0:0:0:1:1");
}

TEST(factory_calculate_new_address, ipv6_preserves_prefix)
{
    auto base = make_shared_ipv6("fe80::");

    auto result = ethernet_address_factory::calculate_new_address(base, 42);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "fe80:0:0:0:0:0:0:2a");
}

/*******************************************************************************
 *
 * calculate_mask — IPv4, valid prefixes.
 *
 *******************************************************************************/
TEST(factory_calculate_mask, ipv4_prefix_0)
{
    auto ip = make_shared_ipv4("10.0.0.1");
    ASSERT_FALSE(ethernet_address_factory::calculate_mask(ip, 0).has_value());
}

TEST(factory_calculate_mask, ipv4_prefix_1)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 1);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "128.0.0.0");
}

TEST(factory_calculate_mask, ipv4_prefix_8)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 8);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.0.0.0");
}

TEST(factory_calculate_mask, ipv4_prefix_16)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 16);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.255.0.0");
}

TEST(factory_calculate_mask, ipv4_prefix_24)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 24);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.255.255.0");
}

TEST(factory_calculate_mask, ipv4_prefix_25)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 25);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.255.255.128");
}

TEST(factory_calculate_mask, ipv4_prefix_30)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 30);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.255.255.252");
}

TEST(factory_calculate_mask, ipv4_prefix_31)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 31);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.255.255.254");
}

TEST(factory_calculate_mask, ipv4_prefix_32)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 32);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "255.255.255.255");
}

TEST(factory_calculate_mask, ipv4_result_is_ipv4_type)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 24);
    ASSERT_TRUE(mask.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*mask).get()), nullptr);
}

/*******************************************************************************
 *
 * calculate_mask — IPv4, invalid prefix.
 *
 *******************************************************************************/
TEST(factory_calculate_mask, ipv4_prefix_33_returns_false)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_mask(ip, 33).has_value());
}

TEST(factory_calculate_mask, ipv4_prefix_64_returns_false)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_mask(ip, 64).has_value());
}

TEST(factory_calculate_mask, ipv4_prefix_max_uint32_returns_false)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_mask(ip, std::numeric_limits<uint32_t>::max())
            .has_value());
}

/*******************************************************************************
 *
 * calculate_mask — IPv6, 16-bit aligned prefixes.
 *
 *******************************************************************************/
TEST(factory_calculate_mask, ipv6_prefix_0)
{
    auto ip = make_shared_ipv6("2001:db8::1");
    ASSERT_FALSE(ethernet_address_factory::calculate_mask(ip, 0).has_value());
}

TEST(factory_calculate_mask, ipv6_prefix_16)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 16);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:0:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_32)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 32);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_48)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 48);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_64)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 64);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_80)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 80);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:ffff:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_96)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 96);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_112)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 112);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:0");
}

TEST(factory_calculate_mask, ipv6_prefix_128)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 128);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
}

TEST(factory_calculate_mask, ipv6_result_is_ipv6_type)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 64);
    ASSERT_TRUE(mask.has_value());
    EXPECT_NE(dynamic_cast<ethernet_ipv6_address *>((*mask).get()), nullptr);
}

/*******************************************************************************
 *
 * calculate_mask — IPv6, non-16-bit aligned prefixes.
 *
 *******************************************************************************/
TEST(factory_calculate_mask, ipv6_prefix_1)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 1);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "8000:0:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_8)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 8);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ff00:0:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_10)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 10);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffc0:0:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_12)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 12);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "fff0:0:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_24)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 24);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ff00:0:0:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_56)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 56);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ff00:0:0:0:0");
}

TEST(factory_calculate_mask, ipv6_prefix_120)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 120);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ff00");
}

TEST(factory_calculate_mask, ipv6_prefix_127)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto mask = ethernet_address_factory::calculate_mask(ip, 127);
    ASSERT_TRUE(mask.has_value());
    EXPECT_EQ((*mask)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe");
}

/*******************************************************************************
 *
 * calculate_mask — IPv6, invalid prefix.
 *
 *******************************************************************************/
TEST(factory_calculate_mask, ipv6_prefix_129_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    EXPECT_FALSE(ethernet_address_factory::calculate_mask(ip, 129).has_value());
}

TEST(factory_calculate_mask, ipv6_prefix_256_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    EXPECT_FALSE(ethernet_address_factory::calculate_mask(ip, 256).has_value());
}

TEST(factory_calculate_mask, ipv6_prefix_max_uint32_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    EXPECT_FALSE(ethernet_address_factory::calculate_mask(ip, std::numeric_limits<uint32_t>::max())
            .has_value());
}

/*******************************************************************************
 *
 * calculate_mask_prefix — IPv4.
 *
 *******************************************************************************/
TEST(factory_calculate_mask_prefix, ipv4_all_zeros)
{
    auto mask = make_shared_ipv4_from_uint32(0x00000000u);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 0u);
}

TEST(factory_calculate_mask_prefix, ipv4_all_ones)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFFFFu);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 32u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_24)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 24u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_16)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFF0000u);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 16u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_8)
{
    auto mask = make_shared_ipv4_from_uint32(0xFF000000u);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 8u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_1)
{
    auto mask = make_shared_ipv4_from_uint32(0x80000000u);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 1u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_25)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF80u);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 25u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_30)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFFFCu);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 30u);
}

TEST(factory_calculate_mask_prefix, ipv4_slash_31)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFFFEu);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 31u);
}

TEST(factory_calculate_mask_prefix, ipv4_non_contiguous_counts_leading_ones_only)
{
    auto mask = make_shared_ipv4_from_uint32(0xFF00FF00u);
    ASSERT_FALSE(ethernet_address_factory::calculate_mask_prefix(mask).has_value());
}

TEST(factory_calculate_mask_prefix, ipv4_single_trailing_zero)
{
    // 0xFFFFFFFE
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFFFEu);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 31u);
}

/*******************************************************************************
 *
 * calculate_mask_prefix — IPv4 round-trip with calculate_mask.
 *
 *******************************************************************************/
TEST(factory_calculate_mask_prefix, ipv4_round_trip_prefix_8)
{
    auto ip = make_shared_ipv4("10.0.0.1");
    auto mask = ethernet_address_factory::calculate_mask(ip, 8);
    ASSERT_TRUE(mask.has_value());

    auto prefix = ethernet_address_factory::calculate_mask_prefix(*mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 8u);
}

TEST(factory_calculate_mask_prefix, ipv4_round_trip_prefix_24)
{
    auto ip = make_shared_ipv4("10.0.0.1");
    auto mask = ethernet_address_factory::calculate_mask(ip, 24);
    ASSERT_TRUE(mask.has_value());

    auto prefix = ethernet_address_factory::calculate_mask_prefix(*mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 24u);
}

TEST(factory_calculate_mask_prefix, ipv4_round_trip_prefix_32)
{
    auto ip = make_shared_ipv4("10.0.0.1");
    auto mask = ethernet_address_factory::calculate_mask(ip, 32);
    ASSERT_TRUE(mask.has_value());

    auto prefix = ethernet_address_factory::calculate_mask_prefix(*mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 32u);
}

TEST(factory_calculate_mask_prefix, ipv4_round_trip_all_prefixes)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    for (uint32_t expected = 0; expected <= 32; ++expected)
    {
        if (expected == 0)
        {
            ASSERT_FALSE(ethernet_address_factory::calculate_mask(ip, expected).has_value());
        }
        else
        {
            auto mask = ethernet_address_factory::calculate_mask(ip, expected);
            ASSERT_TRUE(mask.has_value());

            auto actual = ethernet_address_factory::calculate_mask_prefix(*mask);
            ASSERT_TRUE(actual.has_value());
            EXPECT_EQ(*actual, expected) << "Failed round-trip for IPv4 prefix " << expected;
        }
    }
}

/*******************************************************************************
 *
 * calculate_mask_prefix — IPv6.
 *
 *******************************************************************************/
TEST(factory_calculate_mask_prefix, ipv6_all_zeros)
{
    auto mask = make_shared_ipv6("::");

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 0u);
}

TEST(factory_calculate_mask_prefix, ipv6_all_ones)
{
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 128u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_64)
{
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 64u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_48)
{
    auto mask = make_shared_ipv6("ffff:ffff:ffff::");

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 48u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_16)
{
    auto mask = make_shared_ipv6("ffff::");

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 16u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_112)
{
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff::");

    // Note: "ffff::" as the last segment > that's 0x0000, so mask is
    // ffff:ffff:ffff:ffff:ffff:ffff:ffff:0000 > /112

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 112u);
}

/*******************************************************************************
 *
 * calculate_mask_prefix — IPv6
 * testing non-16-bit aligned masks constructed correctly via from_sockaddr_in.
 *
 *******************************************************************************/
static std::shared_ptr<ethernet_ip_address> make_ipv6_mask(uint32_t prefix_length)
{
    // Build a correct IPv6 mask in network byte order, then use from_sockaddr_in
    // which properly converts to host byte order via ntohs().
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;

    uint32_t remaining = prefix_length;
    for (int i = 0; i < 16; ++i)
    {
        if (remaining >= 8)
        {
            sa6.sin6_addr.s6_addr[i] = 0xFF;
            remaining -= 8;
        }
        else if (remaining > 0)
        {
            sa6.sin6_addr.s6_addr[i] = static_cast<uint8_t>(0xFF << (8 - remaining));
            remaining = 0;
        }
        else
        {
            sa6.sin6_addr.s6_addr[i] = 0x00;
        }
    }

    auto mask_opt =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa6));
    return mask_opt.value_or(nullptr);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_1_via_correct_mask)
{
    auto mask = make_ipv6_mask(1);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 1u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_8_via_correct_mask)
{
    auto mask = make_ipv6_mask(8);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 8u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_10_via_correct_mask)
{
    auto mask = make_ipv6_mask(10);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 10u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_56_via_correct_mask)
{
    auto mask = make_ipv6_mask(56);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 56u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_120_via_correct_mask)
{
    auto mask = make_ipv6_mask(120);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 120u);
}

TEST(factory_calculate_mask_prefix, ipv6_slash_127_via_correct_mask)
{
    auto mask = make_ipv6_mask(127);

    auto prefix = ethernet_address_factory::calculate_mask_prefix(mask);
    ASSERT_TRUE(prefix.has_value());
    EXPECT_EQ(*prefix, 127u);
}

/*******************************************************************************
 *
 * calculate_max_hosts — IPv4.
 *
 *******************************************************************************/
TEST(factory_calculate_max_hosts, ipv4_prefix_24)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 24);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 256u);
}

TEST(factory_calculate_max_hosts, ipv4_prefix_32)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 32);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 1u);
}

TEST(factory_calculate_max_hosts, ipv4_prefix_1)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 1);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, (1ULL << 31));
}

TEST(factory_calculate_max_hosts, ipv4_prefix_16)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 16);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 65536u);
}

TEST(factory_calculate_max_hosts, ipv4_prefix_8)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 8);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, (1ULL << 24));
}

TEST(factory_calculate_max_hosts, ipv4_prefix_31)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 31);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 2u);
}

TEST(factory_calculate_max_hosts, ipv4_prefix_30)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 30);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 4u);
}

TEST(factory_calculate_max_hosts, ipv4_prefix_25)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 25);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 128u);
}

/*******************************************************************************
 *
 * calculate_max_hosts — IPv4, invalid prefixes.
 *
 *******************************************************************************/
TEST(factory_calculate_max_hosts, ipv4_prefix_0_returns_false)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_address_count(ip, 0).has_value());
}

TEST(factory_calculate_max_hosts, ipv4_prefix_33_returns_false)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_address_count(ip, 33).has_value());
}

TEST(factory_calculate_max_hosts, ipv4_prefix_64_returns_false)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_address_count(ip, 64).has_value());
}

/*******************************************************************************
 *
 * calculate_max_hosts — IPv6.
 *
 *******************************************************************************/
TEST(factory_calculate_max_hosts, ipv6_prefix_128)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 128);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 1u);
}

TEST(factory_calculate_max_hosts, ipv6_prefix_127)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 127);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 2u);
}

TEST(factory_calculate_max_hosts, ipv6_prefix_120)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 120);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 256u);
}

TEST(factory_calculate_max_hosts, ipv6_prefix_112)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 112);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, 65536u);
}

TEST(factory_calculate_max_hosts, ipv6_prefix_96)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 96);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, (1ULL << 32));
}

TEST(factory_calculate_max_hosts, ipv6_prefix_65)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 65);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, (1ULL << 63));
}

TEST(factory_calculate_max_hosts, ipv6_prefix_64_capped_at_uint64_max)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 64);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, std::numeric_limits<uint64_t>::max());
}

TEST(factory_calculate_max_hosts, ipv6_prefix_1_capped_at_uint64_max)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 1);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, std::numeric_limits<uint64_t>::max());
}

TEST(factory_calculate_max_hosts, ipv6_prefix_48_capped_at_uint64_max)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 48);
    ASSERT_TRUE(max_hosts.has_value());
    EXPECT_EQ(*max_hosts, std::numeric_limits<uint64_t>::max());
}

/*******************************************************************************
 *
 * calculate_max_hosts — IPv6, invalid prefixes.
 *
 *******************************************************************************/
TEST(factory_calculate_max_hosts, ipv6_prefix_0_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    EXPECT_FALSE(ethernet_address_factory::calculate_address_count(ip, 0).has_value());
}

TEST(factory_calculate_max_hosts, ipv6_prefix_129_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    EXPECT_FALSE(ethernet_address_factory::calculate_address_count(ip, 129).has_value());
}

TEST(factory_calculate_max_hosts, ipv6_prefix_256_returns_false)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    EXPECT_FALSE(ethernet_address_factory::calculate_address_count(ip, 256).has_value());
}

/*******************************************************************************
 *
 * calculate_max_hosts — does not modify output on failure.
 *
 *******************************************************************************/
TEST(factory_calculate_max_hosts, ipv4_does_not_modify_output_on_failure)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 0);
    EXPECT_FALSE(max_hosts.has_value());
}

TEST(factory_calculate_max_hosts, ipv6_does_not_modify_output_on_failure)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    auto max_hosts = ethernet_address_factory::calculate_address_count(ip, 0);
    EXPECT_FALSE(max_hosts.has_value());
}

/*******************************************************************************
 *
 * Integration: calculate_base_address + calculate_new_address (IPv4).
 *
 *******************************************************************************/
TEST(factory_integration, ipv4_base_then_new_address)
{
    auto ip = make_shared_ipv4("192.168.1.100");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "192.168.1.0");

    auto new_addr = ethernet_address_factory::calculate_new_address(*base, 42);
    ASSERT_TRUE(new_addr.has_value());
    EXPECT_EQ((*new_addr)->to_string(), "192.168.1.42");
}

TEST(factory_integration, ipv4_base_then_enumerate_range)
{
    auto ip = make_shared_ipv4("10.0.0.50");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());

    for (uint64_t i = 1; i < 5; ++i)
    {
        auto new_addr = ethernet_address_factory::calculate_new_address(*base, i);
        ASSERT_TRUE(new_addr.has_value());
        EXPECT_EQ((*new_addr)->to_string(), "10.0.0." + std::to_string(i));
    }
}

/*******************************************************************************
 *
 * Integration: calculate_base_address + calculate_new_address (IPv6).
 *
 *******************************************************************************/
TEST(factory_integration, ipv6_base_then_new_address)
{
    auto ip = make_shared_ipv6("2001:db8::abcd");
    auto mask = make_shared_ipv6("ffff:ffff:ffff:ffff::");

    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "2001:db8:0:0:0:0:0:0");

    auto new_addr = ethernet_address_factory::calculate_new_address(*base, 1);
    ASSERT_TRUE(new_addr.has_value());
    EXPECT_EQ((*new_addr)->to_string(), "2001:db8:0:0:0:0:0:1");
}

/*******************************************************************************
 *
 * Integration: calculate_mask + calculate_base_address (IPv4).
 *
 *******************************************************************************/
TEST(factory_integration, ipv4_mask_then_base_address)
{
    auto ip = make_shared_ipv4("172.16.30.40");

    auto mask = ethernet_address_factory::calculate_mask(ip, 16);
    ASSERT_TRUE(mask.has_value());

    auto base = ethernet_address_factory::calculate_base_address(ip, *mask);
    ASSERT_TRUE(base.has_value());
    EXPECT_EQ((*base)->to_string(), "172.16.0.0");
}

/*******************************************************************************
 *
 * Integration: calculate_mask + calculate_max_hosts (IPv4).
 *
 *******************************************************************************/
TEST(factory_integration, ipv4_mask_and_max_hosts_are_consistent)
{
    auto ip = make_shared_ipv4("10.0.0.1");

    for (uint32_t prefix = 1; prefix <= 32; ++prefix)
    {
        auto max_hosts = ethernet_address_factory::calculate_address_count(ip, prefix);
        ASSERT_TRUE(max_hosts.has_value());
        EXPECT_EQ(*max_hosts, (1ULL << (32 - prefix))) << "Mismatch for IPv4 prefix " << prefix;
    }
}

/*******************************************************************************
 *
 * Integration: calculate_mask + calculate_max_hosts (IPv6).
 *
 *******************************************************************************/
TEST(factory_integration, ipv6_mask_and_max_hosts_small_host_part)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    for (uint32_t prefix = 65; prefix <= 128; ++prefix)
    {
        auto max_hosts = ethernet_address_factory::calculate_address_count(ip, prefix);
        ASSERT_TRUE(max_hosts.has_value());
        EXPECT_EQ(*max_hosts, (1ULL << (128 - prefix))) << "Mismatch for IPv6 prefix " << prefix;
    }
}

TEST(factory_integration, ipv6_mask_and_max_hosts_capped)
{
    auto ip = make_shared_ipv6("2001:db8::1");

    for (uint32_t prefix = 1; prefix <= 64; ++prefix)
    {
        auto max_hosts = ethernet_address_factory::calculate_address_count(ip, prefix);
        ASSERT_TRUE(max_hosts.has_value());
        EXPECT_EQ(*max_hosts, std::numeric_limits<uint64_t>::max())
            << "Should be capped for IPv6 prefix " << prefix;
    }
}

/*******************************************************************************
 *
 * from_string — IPv4 leading zeros rejection.
 *
 *******************************************************************************/
TEST(factory_from_string, ipv4_leading_zero_in_first_octet_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("01.0.0.1").has_value());
}

TEST(factory_from_string, ipv4_leading_zero_in_second_octet_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1.01.0.1").has_value());
}

TEST(factory_from_string, ipv4_leading_zero_in_third_octet_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1.0.01.1").has_value());
}

TEST(factory_from_string, ipv4_leading_zero_in_fourth_octet_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1.0.0.01").has_value());
}

TEST(factory_from_string, ipv4_all_octets_with_leading_zeros_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("001.002.003.004").has_value());
}

TEST(factory_from_string, ipv4_double_leading_zeros_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("00.0.0.1").has_value());
}

TEST(factory_from_string, ipv4_triple_leading_zeros_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("000.0.0.1").has_value());
}

TEST(factory_from_string, ipv4_octal_like_notation_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("010.010.010.010").has_value());
}

TEST(factory_from_string, ipv4_single_zero_octet_is_valid)
{
    auto addr = ethernet_address_factory::from_string("0.0.0.0");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, ipv4_single_zero_octet_mixed_with_nonzero_is_valid)
{
    auto addr = ethernet_address_factory::from_string("10.0.0.1");
    EXPECT_TRUE(addr.has_value());
}

TEST(factory_from_string, ipv4_leading_zero_does_not_modify_output)
{
    auto original = make_shared_ipv4("10.0.0.1");
    auto result = ethernet_address_factory::from_string("01.0.0.1");
    EXPECT_FALSE(result.has_value());
}

/*******************************************************************************
 *
 * calculate_new_address — IPv4 overflow detection.
 *
 *******************************************************************************/
TEST(factory_calculate_new_address, ipv4_overflow_broadcast_plus_one_returns_false)
{
    auto base = make_shared_ipv4("255.255.255.255");

    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(base, 1).has_value());
}

TEST(factory_calculate_new_address, ipv4_overflow_high_base_plus_large_offset_returns_false)
{
    auto base = make_shared_ipv4("255.255.255.254");

    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(base, 2).has_value());
}

TEST(factory_calculate_new_address, ipv4_overflow_boundary_no_overflow)
{
    auto base = make_shared_ipv4("255.255.255.254");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "255.255.255.255");
}

TEST(factory_calculate_new_address, ipv4_overflow_half_range_base_plus_half_range_offset)
{
    auto base = make_shared_ipv4_from_uint32(0x80000000u);

    auto result = ethernet_address_factory::calculate_new_address(base, 0x7FFFFFFFu);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "255.255.255.255");
}

TEST(factory_calculate_new_address, ipv4_overflow_half_range_base_plus_half_range_plus_one)
{
    auto base = make_shared_ipv4_from_uint32(0x80000000u);

    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(base, 0x80000000u).has_value());
}

TEST(factory_calculate_new_address, ipv4_overflow_max_host_index_returns_false)
{
    auto base = make_shared_ipv4("0.0.0.1");

    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(base, 0xFFFFFFFFu).has_value());
}

TEST(factory_calculate_new_address, ipv4_overflow_u64_host_index_returns_false)
{
    auto base = make_shared_ipv4("0.0.0.0");

    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(base, 0x100000000ULL).has_value());
}

TEST(factory_calculate_new_address, ipv4_overflow_does_not_modify_output)
{
    auto base = make_shared_ipv4("255.255.255.255");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    EXPECT_FALSE(result.has_value());
}

/*******************************************************************************
 *
 * calculate_new_address — IPv6 overflow detection.
 *
 *******************************************************************************/
TEST(factory_calculate_new_address, ipv6_overflow_all_max_plus_one_returns_false)
{
    auto base = make_shared_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(base, 1).has_value());
}

TEST(factory_calculate_new_address, ipv6_overflow_boundary_no_overflow)
{
    auto base = make_shared_ipv6("ffff:ffff:ffff:ffff:ffff:ffff:ffff:fffe");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
}

TEST(factory_calculate_new_address, ipv6_overflow_last_four_segments_max_plus_one)
{
    auto base = make_shared_ipv6("::ffff:ffff:ffff:ffff");

    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:1:0:0:0:0");
}

TEST(factory_calculate_new_address, ipv6_overflow_max_u64_offset_from_zero)
{
    auto base = make_shared_ipv6("::");

    auto result =
        ethernet_address_factory::calculate_new_address(base, std::numeric_limits<uint64_t>::max());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "0:0:0:0:ffff:ffff:ffff:ffff");
}

/*******************************************************************************
 *
 * from_array — MAC address factory method.
 *
 *******************************************************************************/
TEST(factory_from_array, returns_true_for_valid_array)
{
    const uint8_t data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00};

    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    EXPECT_TRUE(addr.has_value());
    EXPECT_EQ(addr->to_string(), "AA:BB:CC:DD:EE:FF");
}

TEST(factory_from_array, returns_false_for_nullptr)
{
    EXPECT_FALSE(ethernet_address_factory::from_array(nullptr, 8).has_value());
}

TEST(factory_from_array, returns_false_for_length_less_than_six)
{
    const uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};

    EXPECT_FALSE(ethernet_address_factory::from_array(data, sizeof(data)).has_value());
}

TEST(factory_from_array, returns_true_for_exactly_six_bytes)
{
    const uint8_t data[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    EXPECT_TRUE(addr.has_value());
    EXPECT_EQ(addr->to_string(), "11:22:33:44:55:66");
}

TEST(factory_from_array, ignores_bytes_beyond_six)
{
    const uint8_t data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xFF, 0xFF};

    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());
    EXPECT_EQ(addr->to_string(), "12:34:56:78:9A:BC");
}

TEST(factory_from_array, all_zeros_produces_empty_mac)
{
    const uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());
    EXPECT_TRUE(addr->empty());
    EXPECT_EQ(addr->to_string(), "00:00:00:00:00:00");
}

TEST(factory_from_array, broadcast_mac)
{
    const uint8_t data[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());
    EXPECT_FALSE(addr->empty());
    EXPECT_EQ(addr->to_string(), "FF:FF:FF:FF:FF:FF");
}

TEST(factory_from_array, does_not_modify_address_on_nullptr_failure)
{
    const uint8_t data[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());

    EXPECT_FALSE(ethernet_address_factory::from_array(nullptr, 8).has_value());
    EXPECT_EQ(addr->to_string(), "AA:BB:CC:DD:EE:FF");
}

/*******************************************************************************
 *
 * Null shared_ptr tests for calculate_base_address, calculate_mask, calculate_new_address.
 *
 *******************************************************************************/
TEST(factory_nullptr, calculate_base_address_null_ip_address)
{
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);
    EXPECT_FALSE(ethernet_address_factory::calculate_base_address(nullptr, mask).has_value());
}

TEST(factory_nullptr, calculate_base_address_null_mask)
{
    auto ip = make_shared_ipv4("192.168.1.1");
    EXPECT_FALSE(ethernet_address_factory::calculate_base_address(ip, nullptr).has_value());
}

TEST(factory_nullptr, calculate_base_address_null_output)
{
    auto ip = make_shared_ipv4("192.168.1.1");
    auto mask = make_shared_ipv4_from_uint32(0xFFFFFF00u);
    // Should still return a value
    auto base = ethernet_address_factory::calculate_base_address(ip, mask);
    EXPECT_TRUE(base.has_value());
}

TEST(factory_nullptr, calculate_mask_null_ip_address)
{
    EXPECT_FALSE(ethernet_address_factory::calculate_mask(nullptr, 24).has_value());
}

TEST(factory_nullptr, calculate_mask_null_output)
{
    auto ip = make_shared_ipv4("10.0.0.1");
    auto mask = ethernet_address_factory::calculate_mask(ip, 24);
    EXPECT_TRUE(mask.has_value());
}

TEST(factory_nullptr, calculate_new_address_null_base_address)
{
    EXPECT_FALSE(ethernet_address_factory::calculate_new_address(nullptr, 1).has_value());
}

TEST(factory_nullptr, calculate_new_address_null_output)
{
    auto base = make_shared_ipv4("10.0.0.1");
    auto result = ethernet_address_factory::calculate_new_address(base, 1);
    EXPECT_TRUE(result.has_value());
}

TEST(factory_from_sockaddr_in, returns_false_for_nullptr)
{
    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(nullptr).has_value());
}

TEST(factory_from_sockaddr_in, returns_false_for_zero_length)
{
    // Simulate a zero-length sockaddr by passing a valid pointer but not enough data.
    // Since the function signature does not take a length, this test is not directly applicable.
    // However, we can simulate a "zeroed" sockaddr structure.
    sockaddr sa = {};
    sa.sa_family = AF_UNSPEC; // Not AF_INET or AF_INET6
    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(&sa).has_value());
}

TEST(factory_from_sockaddr_in, returns_true_for_valid_ipv4)
{
    sockaddr_in sa4 = {};
    sa4.sin_family = AF_INET;
    sa4.sin_addr.s_addr = htonl(0xC0A80101); // 192.168.1.1
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<sockaddr *>(&sa4));
    EXPECT_TRUE(addr.has_value());
    EXPECT_EQ((*addr)->to_string(), "192.168.1.1");
}

TEST(factory_from_sockaddr_in, returns_true_for_valid_ipv6)
{
    sockaddr_in6 sa6 = {};
    sa6.sin6_family = AF_INET6;
    // 2001:db8::1
    sa6.sin6_addr.s6_addr[0] = 0x20;
    sa6.sin6_addr.s6_addr[1] = 0x01;
    sa6.sin6_addr.s6_addr[2] = 0x0d;
    sa6.sin6_addr.s6_addr[3] = 0xb8;
    sa6.sin6_addr.s6_addr[15] = 0x01;
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<sockaddr *>(&sa6));
    EXPECT_TRUE(addr.has_value());
    EXPECT_EQ((*addr)->to_string(), "2001:db8:0:0:0:0:0:1");
}

TEST(factory_from_sockaddr_in, does_not_modify_output_on_failure)
{
    sockaddr sa = {};
    sa.sa_family = AF_UNSPEC;
    auto original = make_shared_ipv4("10.0.0.1");
    auto result = ethernet_address_factory::from_sockaddr_in(&sa);
    EXPECT_FALSE(result.has_value());
}

// NOLINTEND
