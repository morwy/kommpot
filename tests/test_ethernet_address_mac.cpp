// clazy:skip
// NOLINTBEGIN

#include <libkommpot.h>

#include <communications/ethernet/ethernet_address.h>
#include <communications/ethernet/ethernet_address_factory.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

using namespace testing;

/*******************************************************************************
 *
 * Helper: build a MAC address from 6 byte values.
 *
 *******************************************************************************/
static ethernet_mac_address make_mac(
    uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5)
{
    const uint8_t data[8] = {b0, b1, b2, b3, b4, b5, 0, 0};
    auto result = ethernet_address_factory::from_array(data, sizeof(data));
    EXPECT_TRUE(result.has_value());
    if (!result.has_value())
    {
        return {};
    }
    return *result;
}

/*******************************************************************************
 *
 * Default construction.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, default_constructor_to_string_is_all_zeros)
{
    ethernet_mac_address addr;
    EXPECT_EQ(addr.to_string(), "00:00:00:00:00:00");
}

TEST(ethernet_mac_address, default_constructor_is_empty)
{
    ethernet_mac_address addr;
    EXPECT_TRUE(addr.empty());
}

TEST(ethernet_mac_address, default_constructor_has_exactly_five_colons)
{
    ethernet_mac_address addr;
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), ':'), 5);
}

/*******************************************************************************
 *
 * Construction from uint8_t array (via factory).
 *
 *******************************************************************************/
TEST(ethernet_mac_address, construct_from_array_all_zeros)
{
    const uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());

    EXPECT_EQ(addr->to_string(), "00:00:00:00:00:00");
    EXPECT_TRUE(addr->empty());
}

TEST(ethernet_mac_address, construct_from_array_all_ff)
{
    const uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};
    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());

    EXPECT_EQ(addr->to_string(), "FF:FF:FF:FF:FF:FF");
    EXPECT_FALSE(addr->empty());
}

TEST(ethernet_mac_address, construct_from_array_typical_mac)
{
    const uint8_t data[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00};
    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());

    EXPECT_EQ(addr->to_string(), "AA:BB:CC:DD:EE:FF");
    EXPECT_FALSE(addr->empty());
}

TEST(ethernet_mac_address, construct_from_array_only_first_byte_set)
{
    const uint8_t data[8] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());

    EXPECT_EQ(addr->to_string(), "01:00:00:00:00:00");
    EXPECT_FALSE(addr->empty());
}

TEST(ethernet_mac_address, construct_from_array_only_last_byte_set)
{
    const uint8_t data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
    auto addr = ethernet_address_factory::from_array(data, sizeof(data));
    ASSERT_TRUE(addr.has_value());

    EXPECT_EQ(addr->to_string(), "00:00:00:00:00:01");
    EXPECT_FALSE(addr->empty());
}

TEST(ethernet_mac_address, construct_from_array_ignores_bytes_beyond_six)
{
    // Bytes 6 and 7 should not affect the MAC address (only 6 bytes are used)
    const uint8_t data1[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xFF, 0xFF};
    const uint8_t data2[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00};

    auto addr1 = ethernet_address_factory::from_array(data1, sizeof(data1));
    auto addr2 = ethernet_address_factory::from_array(data2, sizeof(data2));
    ASSERT_TRUE(addr1.has_value());
    ASSERT_TRUE(addr2.has_value());

    EXPECT_EQ(addr1->to_string(), addr2->to_string());
}

/*******************************************************************************
 *
 * to_string — corner-case MAC addresses.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, to_string_broadcast)
{
    auto addr = make_mac(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    EXPECT_EQ(addr.to_string(), "FF:FF:FF:FF:FF:FF");
}

TEST(ethernet_mac_address, to_string_all_zeros)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    EXPECT_EQ(addr.to_string(), "00:00:00:00:00:00");
}

TEST(ethernet_mac_address, to_string_each_byte_individually)
{
    EXPECT_EQ(make_mac(0x01, 0x00, 0x00, 0x00, 0x00, 0x00).to_string(), "01:00:00:00:00:00");
    EXPECT_EQ(make_mac(0x00, 0x02, 0x00, 0x00, 0x00, 0x00).to_string(), "00:02:00:00:00:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x03, 0x00, 0x00, 0x00).to_string(), "00:00:03:00:00:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x00, 0x04, 0x00, 0x00).to_string(), "00:00:00:04:00:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x00, 0x00, 0x05, 0x00).to_string(), "00:00:00:00:05:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x06).to_string(), "00:00:00:00:00:06");
}

TEST(ethernet_mac_address, to_string_each_byte_at_max)
{
    EXPECT_EQ(make_mac(0xFF, 0x00, 0x00, 0x00, 0x00, 0x00).to_string(), "FF:00:00:00:00:00");
    EXPECT_EQ(make_mac(0x00, 0xFF, 0x00, 0x00, 0x00, 0x00).to_string(), "00:FF:00:00:00:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0xFF, 0x00, 0x00, 0x00).to_string(), "00:00:FF:00:00:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x00, 0xFF, 0x00, 0x00).to_string(), "00:00:00:FF:00:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x00, 0x00, 0xFF, 0x00).to_string(), "00:00:00:00:FF:00");
    EXPECT_EQ(make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0xFF).to_string(), "00:00:00:00:00:FF");
}

TEST(ethernet_mac_address, to_string_sequential_bytes)
{
    auto addr = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x06);
    EXPECT_EQ(addr.to_string(), "01:02:03:04:05:06");
}

TEST(ethernet_mac_address, to_string_mixed_hex_digits)
{
    auto addr = make_mac(0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56);
    EXPECT_EQ(addr.to_string(), "AB:CD:EF:12:34:56");
}

TEST(ethernet_mac_address, to_string_uses_uppercase_hex)
{
    auto addr = make_mac(0xab, 0xcd, 0xef, 0x00, 0x00, 0x00);
    const std::string s = addr.to_string();

    for (char c : s)
    {
        if (std::isalpha(static_cast<unsigned char>(c)))
        {
            EXPECT_TRUE(std::isupper(static_cast<unsigned char>(c)))
                << "Expected uppercase, got: " << c;
        }
    }
}

TEST(ethernet_mac_address, to_string_pads_single_digit_values_with_zero)
{
    // 0x0A should produce "0A", not "A"
    auto addr = make_mac(0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F);
    EXPECT_EQ(addr.to_string(), "0A:0B:0C:0D:0E:0F");
}

TEST(ethernet_mac_address, to_string_smallest_nonzero_values)
{
    auto addr = make_mac(0x01, 0x01, 0x01, 0x01, 0x01, 0x01);
    EXPECT_EQ(addr.to_string(), "01:01:01:01:01:01");
}

/*******************************************************************************
 *
 * to_string format validation.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, to_string_has_exactly_five_colons)
{
    auto addr = make_mac(0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE);
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), ':'), 5);
}

TEST(ethernet_mac_address, to_string_has_correct_length)
{
    // Format: "XX:XX:XX:XX:XX:XX" = 17 characters
    auto addr = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);
    EXPECT_EQ(addr.to_string().size(), 17u);
}

TEST(ethernet_mac_address, to_string_does_not_start_or_end_with_colon)
{
    auto addr = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    const std::string s = addr.to_string();

    EXPECT_NE(s.front(), ':');
    EXPECT_NE(s.back(), ':');
}

TEST(ethernet_mac_address, to_string_contains_only_hex_digits_and_colons)
{
    auto addr = make_mac(0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45);
    const std::string s = addr.to_string();

    for (char c : s)
    {
        EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)) || c == ':')
            << "Unexpected character: " << c;
    }
}

TEST(ethernet_mac_address, to_string_does_not_contain_whitespace)
{
    auto addr = make_mac(0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01);
    const std::string s = addr.to_string();

    EXPECT_EQ(s.find(' '), std::string::npos);
    EXPECT_EQ(s.find('\t'), std::string::npos);
    EXPECT_EQ(s.find('\n'), std::string::npos);
}

TEST(ethernet_mac_address, to_string_is_not_empty)
{
    ethernet_mac_address addr;
    EXPECT_FALSE(addr.to_string().empty());
}

TEST(ethernet_mac_address, to_string_colons_at_expected_positions)
{
    // "XX:XX:XX:XX:XX:XX" — colons at indices 2, 5, 8, 11, 14
    auto addr = make_mac(0x11, 0x22, 0x33, 0x44, 0x55, 0x66);
    const std::string s = addr.to_string();

    EXPECT_EQ(s[2], ':');
    EXPECT_EQ(s[5], ':');
    EXPECT_EQ(s[8], ':');
    EXPECT_EQ(s[11], ':');
    EXPECT_EQ(s[14], ':');
}

/*******************************************************************************
 *
 * empty() — boundary conditions.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, empty_returns_true_for_default_constructed)
{
    ethernet_mac_address addr;
    EXPECT_TRUE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_true_for_explicit_all_zeros)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    EXPECT_TRUE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_for_broadcast)
{
    auto addr = make_mac(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_when_only_first_byte_nonzero)
{
    auto addr = make_mac(0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_when_only_second_byte_nonzero)
{
    auto addr = make_mac(0x00, 0x01, 0x00, 0x00, 0x00, 0x00);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_when_only_third_byte_nonzero)
{
    auto addr = make_mac(0x00, 0x00, 0x01, 0x00, 0x00, 0x00);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_when_only_fourth_byte_nonzero)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x01, 0x00, 0x00);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_when_only_fifth_byte_nonzero)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x01, 0x00);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_when_only_sixth_byte_nonzero)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, empty_returns_false_for_smallest_nonzero_last_byte)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
    EXPECT_FALSE(addr.empty());
}

/*******************************************************************************
 *
 * Well-known MAC addresses.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, well_known_broadcast)
{
    auto addr = make_mac(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    EXPECT_EQ(addr.to_string(), "FF:FF:FF:FF:FF:FF");
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, well_known_ipv4_multicast_base)
{
    // IPv4 multicast base: 01:00:5E:00:00:00
    auto addr = make_mac(0x01, 0x00, 0x5E, 0x00, 0x00, 0x00);
    EXPECT_EQ(addr.to_string(), "01:00:5E:00:00:00");
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, well_known_ipv6_multicast_base)
{
    // IPv6 multicast base: 33:33:00:00:00:00
    auto addr = make_mac(0x33, 0x33, 0x00, 0x00, 0x00, 0x00);
    EXPECT_EQ(addr.to_string(), "33:33:00:00:00:00");
    EXPECT_FALSE(addr.empty());
}

TEST(ethernet_mac_address, well_known_spanning_tree)
{
    // Spanning Tree Protocol: 01:80:C2:00:00:00
    auto addr = make_mac(0x01, 0x80, 0xC2, 0x00, 0x00, 0x00);
    EXPECT_EQ(addr.to_string(), "01:80:C2:00:00:00");
    EXPECT_FALSE(addr.empty());
}

/*******************************************************************************
 *
 * Unicast vs Multicast bit (LSB of first octet).
 *
 *******************************************************************************/
TEST(ethernet_mac_address, unicast_bit_clear)
{
    // First octet even = unicast
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
    const std::string s = addr.to_string();
    EXPECT_EQ(s.substr(0, 2), "00");
}

TEST(ethernet_mac_address, multicast_bit_set)
{
    // First octet odd = multicast
    auto addr = make_mac(0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
    const std::string s = addr.to_string();
    EXPECT_EQ(s.substr(0, 2), "01");
}

/*******************************************************************************
 *
 * Locally Administered bit (second LSB of first octet).
 *
 *******************************************************************************/
TEST(ethernet_mac_address, locally_administered_bit_set)
{
    // 0x02 = locally administered, unicast
    auto addr = make_mac(0x02, 0x00, 0x00, 0x00, 0x00, 0x01);
    EXPECT_EQ(addr.to_string(), "02:00:00:00:00:01");
}

TEST(ethernet_mac_address, globally_unique_bit_clear)
{
    // 0x00 = globally unique, unicast
    auto addr = make_mac(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);
    EXPECT_EQ(addr.to_string(), "00:1A:2B:3C:4D:5E");
}

/*******************************************************************************
 *
 * Copy semantics.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, copy_constructor_preserves_value)
{
    auto original = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    ethernet_mac_address copy(original);

    EXPECT_EQ(copy.to_string(), "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(copy.empty(), original.empty());
}

TEST(ethernet_mac_address, copy_assignment_preserves_value)
{
    auto original = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);
    ethernet_mac_address other;

    other = original;

    EXPECT_EQ(other.to_string(), "12:34:56:78:9A:BC");
    EXPECT_FALSE(other.empty());
}

TEST(ethernet_mac_address, copy_is_independent_of_original)
{
    auto original = make_mac(0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01);
    ethernet_mac_address copy(original);

    EXPECT_EQ(copy.to_string(), original.to_string());
    EXPECT_NE(&copy, &original);
}

TEST(ethernet_mac_address, copy_then_overwrite_does_not_affect_original)
{
    auto original = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    auto copy = original;

    copy = make_mac(0x11, 0x22, 0x33, 0x44, 0x55, 0x66);

    EXPECT_EQ(original.to_string(), "AA:BB:CC:DD:EE:FF");
    EXPECT_EQ(copy.to_string(), "11:22:33:44:55:66");
}

/*******************************************************************************
 *
 * Move semantics.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, move_constructor_preserves_value)
{
    auto original = make_mac(0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x01);
    const std::string expected = original.to_string();

    ethernet_mac_address moved(std::move(original));

    EXPECT_EQ(moved.to_string(), expected);
}

TEST(ethernet_mac_address, move_assignment_preserves_value)
{
    auto original = make_mac(0xFE, 0xED, 0xFA, 0xCE, 0x00, 0x01);
    const std::string expected = original.to_string();
    ethernet_mac_address other;

    other = std::move(original);

    EXPECT_EQ(other.to_string(), expected);
}

/*******************************************************************************
 *
 * Multiple instances are independent.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, two_different_macs_are_independent)
{
    auto addr1 = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01);
    auto addr2 = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02);

    EXPECT_NE(addr1.to_string(), addr2.to_string());
}

TEST(ethernet_mac_address, two_identical_macs_have_same_string)
{
    auto addr1 = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);
    auto addr2 = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);

    EXPECT_EQ(addr1.to_string(), addr2.to_string());
    EXPECT_EQ(addr1.empty(), addr2.empty());
}

/*******************************************************************************
 *
 * Idempotency.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, to_string_is_idempotent)
{
    auto addr = make_mac(0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE);

    const std::string s1 = addr.to_string();
    const std::string s2 = addr.to_string();
    const std::string s3 = addr.to_string();

    EXPECT_EQ(s1, s2);
    EXPECT_EQ(s2, s3);
}

TEST(ethernet_mac_address, empty_is_idempotent)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);

    EXPECT_EQ(addr.empty(), addr.empty());
}

/*******************************************************************************
 *
 * Const correctness.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, const_object_to_string)
{
    const auto addr = make_mac(0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45);
    EXPECT_EQ(addr.to_string(), "AB:CD:EF:01:23:45");
}

TEST(ethernet_mac_address, const_object_empty)
{
    const ethernet_mac_address addr;
    EXPECT_TRUE(addr.empty());
}

TEST(ethernet_mac_address, const_nonzero_object_not_empty)
{
    const auto addr = make_mac(0xFF, 0x00, 0x00, 0x00, 0x00, 0x00);
    EXPECT_FALSE(addr.empty());
}

/*******************************************************************************
 *
 * Type traits / compile-time properties.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, is_default_constructible)
{
    EXPECT_TRUE(std::is_default_constructible<ethernet_mac_address>::value);
}

TEST(ethernet_mac_address, is_copy_constructible)
{
    EXPECT_TRUE(std::is_copy_constructible<ethernet_mac_address>::value);
}

TEST(ethernet_mac_address, is_copy_assignable)
{
    EXPECT_TRUE(std::is_copy_assignable<ethernet_mac_address>::value);
}

TEST(ethernet_mac_address, is_move_constructible)
{
    EXPECT_TRUE(std::is_move_constructible<ethernet_mac_address>::value);
}

TEST(ethernet_mac_address, is_move_assignable)
{
    EXPECT_TRUE(std::is_move_assignable<ethernet_mac_address>::value);
}

TEST(ethernet_mac_address, is_not_abstract)
{
    EXPECT_FALSE(std::is_abstract<ethernet_mac_address>::value);
}

/*******************************************************************************
 *
 * Return types.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, to_string_returns_std_string)
{
    ethernet_mac_address addr;
    EXPECT_TRUE((std::is_same<decltype(addr.to_string()), std::string>::value));
}

TEST(ethernet_mac_address, empty_returns_bool)
{
    ethernet_mac_address addr;
    EXPECT_TRUE((std::is_same<decltype(addr.empty()), bool>::value));
}

/*******************************************************************************
 *
 * Self-assignment safety.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, self_assignment_is_safe)
{
    auto addr = make_mac(0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
    addr = addr;
#pragma GCC diagnostic pop

    EXPECT_EQ(addr.to_string(), "DE:AD:BE:EF:00:01");
}

/*******************************************************************************
 *
 * STL container compatibility.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, storable_in_vector)
{
    std::vector<ethernet_mac_address> addrs;
    addrs.push_back(make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x06));
    addrs.push_back(make_mac(0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F));
    addrs.push_back(make_mac(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF));

    EXPECT_EQ(addrs.size(), 3u);
    EXPECT_EQ(addrs[0].to_string(), "01:02:03:04:05:06");
    EXPECT_EQ(addrs[1].to_string(), "0A:0B:0C:0D:0E:0F");
    EXPECT_EQ(addrs[2].to_string(), "FF:FF:FF:FF:FF:FF");
}

TEST(ethernet_mac_address, storable_in_vector_default_constructed)
{
    std::vector<ethernet_mac_address> addrs(5);

    EXPECT_EQ(addrs.size(), 5u);
    for (const auto &addr : addrs)
    {
        EXPECT_TRUE(addr.empty());
        EXPECT_EQ(addr.to_string(), "00:00:00:00:00:00");
    }
}

/*******************************************************************************
 *
 * All hex digit coverage (0x00–0x0F as individual bytes).
 *
 *******************************************************************************/
TEST(ethernet_mac_address, hex_digits_0_to_5)
{
    auto addr = make_mac(0x00, 0x01, 0x02, 0x03, 0x04, 0x05);
    EXPECT_EQ(addr.to_string(), "00:01:02:03:04:05");
}

TEST(ethernet_mac_address, hex_digits_6_to_B)
{
    auto addr = make_mac(0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B);
    EXPECT_EQ(addr.to_string(), "06:07:08:09:0A:0B");
}

TEST(ethernet_mac_address, hex_digits_C_to_F_and_wrapping)
{
    auto addr = make_mac(0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x7F);
    EXPECT_EQ(addr.to_string(), "0C:0D:0E:0F:10:7F");
}

/*******************************************************************************
 *
 * Comparison operators for ethernet_mac_address.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, equality_operator_identical_addresses)
{
    auto addr1 = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);
    auto addr2 = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);

    EXPECT_TRUE(addr1 == addr2);
    EXPECT_FALSE(addr1 != addr2);
}

TEST(ethernet_mac_address, equality_operator_different_addresses)
{
    auto addr1 = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC);
    auto addr2 = make_mac(0x12, 0x34, 0x56, 0x78, 0x9A, 0xBD);

    EXPECT_FALSE(addr1 == addr2);
    EXPECT_TRUE(addr1 != addr2);
}

TEST(ethernet_mac_address, equality_operator_default_constructed)
{
    ethernet_mac_address addr1;
    ethernet_mac_address addr2;

    EXPECT_TRUE(addr1 == addr2);
    EXPECT_FALSE(addr1 != addr2);
}

TEST(ethernet_mac_address, less_than_operator_orders_lexicographically)
{
    auto addr1 = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
    auto addr2 = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x02);

    EXPECT_TRUE(addr1 < addr2);
    EXPECT_FALSE(addr2 < addr1);
}

TEST(ethernet_mac_address, less_than_operator_first_byte_differs)
{
    auto addr1 = make_mac(0x01, 0x00, 0x00, 0x00, 0x00, 0x00);
    auto addr2 = make_mac(0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

    EXPECT_TRUE(addr1 < addr2);
    EXPECT_FALSE(addr2 < addr1);
}

TEST(ethernet_mac_address, less_than_operator_equal_addresses)
{
    auto addr1 = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);
    auto addr2 = make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF);

    EXPECT_FALSE(addr1 < addr2);
    EXPECT_FALSE(addr2 < addr1);
}

TEST(ethernet_mac_address, greater_than_operator)
{
    auto addr1 = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x02);
    auto addr2 = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);

    EXPECT_TRUE(addr1 > addr2);
    EXPECT_FALSE(addr2 > addr1);
}

TEST(ethernet_mac_address, less_than_or_equal_operator)
{
    auto addr1 = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x06);
    auto addr2 = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x06);
    auto addr3 = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x07);

    EXPECT_TRUE(addr1 <= addr2);
    EXPECT_TRUE(addr1 <= addr3);
    EXPECT_FALSE(addr3 <= addr1);
}

TEST(ethernet_mac_address, greater_than_or_equal_operator)
{
    auto addr1 = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x06);
    auto addr2 = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x06);
    auto addr3 = make_mac(0x01, 0x02, 0x03, 0x04, 0x05, 0x05);

    EXPECT_TRUE(addr1 >= addr2);
    EXPECT_TRUE(addr1 >= addr3);
    EXPECT_FALSE(addr3 >= addr1);
}

TEST(ethernet_mac_address, comparison_with_default_constructed)
{
    auto addr = make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
    ethernet_mac_address def;

    EXPECT_FALSE(addr == def);
    EXPECT_TRUE(addr != def);
    EXPECT_TRUE(def < addr);
    EXPECT_FALSE(addr < def);
}

TEST(ethernet_mac_address, can_be_used_in_std_set)
{
    std::set<ethernet_mac_address> macs;
    macs.insert(make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01));
    macs.insert(make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x02));
    macs.insert(make_mac(0x00, 0x00, 0x00, 0x00, 0x00, 0x01)); // duplicate

    EXPECT_EQ(macs.size(), 2u);
    auto it = macs.begin();
    EXPECT_EQ(it->to_string(), "00:00:00:00:00:01");
    ++it;
    EXPECT_EQ(it->to_string(), "00:00:00:00:00:02");
}

/*******************************************************************************
 *
 * Construction from uint8_t array — edge cases with invalid lengths.
 *
 *******************************************************************************/
TEST(ethernet_mac_address, construct_from_array_with_zero_length)
{
    const uint8_t data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00};
    auto result = ethernet_address_factory::from_array(data, 0);

    EXPECT_FALSE(result.has_value());
}

TEST(ethernet_mac_address, construct_from_array_with_length_5)
{
    const uint8_t data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00};
    auto result = ethernet_address_factory::from_array(data, 5);

    EXPECT_FALSE(result.has_value());
}

// NOLINTEND
