// clazy:skip
// NOLINTBEGIN

#include <libkommpot.h>

#include <communications/ethernet/ethernet_address.h>
#include <communications/ethernet/ethernet_address_factory.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

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
static ethernet_ipv4_address make_ipv4(const std::string &dotted)
{
    auto addr = ethernet_address_factory::from_string(dotted);

    EXPECT_TRUE(addr.has_value());

    if (!addr.has_value())
    {
        return {};
    }

    return *dynamic_cast<ethernet_ipv4_address *>((*addr).get());
}

static ethernet_ipv4_address make_ipv4_from_uint32(uint32_t v)
{
    auto addr = ethernet_address_factory::from_uint32_t(v);

    EXPECT_TRUE(addr.has_value());

    if (!addr.has_value())
    {
        return {};
    }

    return *dynamic_cast<ethernet_ipv4_address *>((*addr).get());
}

static sockaddr_in make_sockaddr_ipv4(const std::string &dotted, uint16_t port = 0)
{
    sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, dotted.c_str(), &sa.sin_addr);
    return sa;
}

/*******************************************************************************
 *
 * Default construction.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, default_constructor_to_string_is_all_zeros)
{
    ethernet_ipv4_address addr;
    EXPECT_EQ(addr.to_string(), "0.0.0.0");
}

TEST(ethernet_ipv4_address, default_constructor_to_uint32_is_zero)
{
    ethernet_ipv4_address addr;
    EXPECT_EQ(addr.to_uint32(), 0u);
}

TEST(ethernet_ipv4_address, default_constructor_has_exactly_three_dots)
{
    ethernet_ipv4_address addr;
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), '.'), 3);
}

/*******************************************************************************
 *
 * Invalid / malformed input.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, invalid_octet_above_255)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("256.0.0.0").has_value());
}

TEST(ethernet_ipv4_address, invalid_second_octet_above_255)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("0.256.0.0").has_value());
}

TEST(ethernet_ipv4_address, invalid_third_octet_above_255)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("0.0.256.0").has_value());
}

TEST(ethernet_ipv4_address, invalid_fourth_octet_above_255)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("0.0.0.256").has_value());
}

TEST(ethernet_ipv4_address, invalid_all_octets_above_255)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("999.999.999.999").has_value());
}

TEST(ethernet_ipv4_address, invalid_negative_octet)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("-1.0.0.0").has_value());
}

TEST(ethernet_ipv4_address, invalid_empty_string)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("").has_value());
}

TEST(ethernet_ipv4_address, invalid_no_dots_no_colons)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("not_an_address").has_value());
}

TEST(ethernet_ipv4_address, invalid_alphabetic_octets)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("abc.def.ghi.jkl").has_value());
}

TEST(ethernet_ipv4_address, invalid_too_few_octets)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("10.0.0").has_value());
}

TEST(ethernet_ipv4_address, invalid_too_many_octets)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("10.0.0.0.1").has_value());
}

TEST(ethernet_ipv4_address, invalid_trailing_dot)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("10.0.0.1.").has_value());
}

TEST(ethernet_ipv4_address, invalid_leading_dot)
{
    EXPECT_FALSE(ethernet_address_factory::from_string(".10.0.0.1").has_value());
}

TEST(ethernet_ipv4_address, invalid_double_dot)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("10..0.1").has_value());
}

TEST(ethernet_ipv4_address, invalid_only_dots)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("...").has_value());
}

TEST(ethernet_ipv4_address, invalid_whitespace_in_address)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("10. 0.0.1").has_value());
}

TEST(ethernet_ipv4_address, invalid_large_number_in_octet)
{
    EXPECT_FALSE(ethernet_address_factory::from_string("1000.0.0.0").has_value());
}

/*******************************************************************************
 *
 * to_string — all IPv4 corner-case addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, to_string_all_zeros)
{
    auto addr = make_ipv4("0.0.0.0");
    EXPECT_EQ(addr.to_string(), "0.0.0.0");
}

TEST(ethernet_ipv4_address, to_string_broadcast)
{
    auto addr = make_ipv4("255.255.255.255");
    EXPECT_EQ(addr.to_string(), "255.255.255.255");
}

TEST(ethernet_ipv4_address, to_string_loopback)
{
    auto addr = make_ipv4("127.0.0.1");
    EXPECT_EQ(addr.to_string(), "127.0.0.1");
}

TEST(ethernet_ipv4_address, to_string_first_octet_max)
{
    auto addr = make_ipv4("255.0.0.0");
    EXPECT_EQ(addr.to_string(), "255.0.0.0");
}

TEST(ethernet_ipv4_address, to_string_second_octet_max)
{
    auto addr = make_ipv4("0.255.0.0");
    EXPECT_EQ(addr.to_string(), "0.255.0.0");
}

TEST(ethernet_ipv4_address, to_string_third_octet_max)
{
    auto addr = make_ipv4("0.0.255.0");
    EXPECT_EQ(addr.to_string(), "0.0.255.0");
}

TEST(ethernet_ipv4_address, to_string_fourth_octet_max)
{
    auto addr = make_ipv4("0.0.0.255");
    EXPECT_EQ(addr.to_string(), "0.0.0.255");
}

TEST(ethernet_ipv4_address, to_string_first_octet_one)
{
    auto addr = make_ipv4("1.0.0.0");
    EXPECT_EQ(addr.to_string(), "1.0.0.0");
}

TEST(ethernet_ipv4_address, to_string_last_octet_one)
{
    auto addr = make_ipv4("0.0.0.1");
    EXPECT_EQ(addr.to_string(), "0.0.0.1");
}

TEST(ethernet_ipv4_address, to_string_class_a_private)
{
    auto addr = make_ipv4("10.20.30.40");
    EXPECT_EQ(addr.to_string(), "10.20.30.40");
}

TEST(ethernet_ipv4_address, to_string_class_b_private)
{
    auto addr = make_ipv4("172.16.0.1");
    EXPECT_EQ(addr.to_string(), "172.16.0.1");
}

TEST(ethernet_ipv4_address, to_string_class_c_private)
{
    auto addr = make_ipv4("192.168.1.100");
    EXPECT_EQ(addr.to_string(), "192.168.1.100");
}

TEST(ethernet_ipv4_address, to_string_link_local)
{
    auto addr = make_ipv4("169.254.1.1");
    EXPECT_EQ(addr.to_string(), "169.254.1.1");
}

TEST(ethernet_ipv4_address, to_string_multicast_low)
{
    auto addr = make_ipv4("224.0.0.1");
    EXPECT_EQ(addr.to_string(), "224.0.0.1");
}

TEST(ethernet_ipv4_address, to_string_multicast_high)
{
    auto addr = make_ipv4("239.255.255.255");
    EXPECT_EQ(addr.to_string(), "239.255.255.255");
}

TEST(ethernet_ipv4_address, to_string_class_e_reserved)
{
    auto addr = make_ipv4("240.0.0.1");
    EXPECT_EQ(addr.to_string(), "240.0.0.1");
}

TEST(ethernet_ipv4_address, to_string_adjacent_to_broadcast)
{
    auto addr = make_ipv4("255.255.255.254");
    EXPECT_EQ(addr.to_string(), "255.255.255.254");
}

TEST(ethernet_ipv4_address, to_string_adjacent_to_zero)
{
    auto addr = make_ipv4("0.0.0.1");
    EXPECT_EQ(addr.to_string(), "0.0.0.1");
}

TEST(ethernet_ipv4_address, to_string_mixed_boundary_octets)
{
    auto addr = make_ipv4("128.64.32.16");
    EXPECT_EQ(addr.to_string(), "128.64.32.16");
}

/*******************************************************************************
 *
 * to_uint32 — all IPv4 corner-case addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, to_uint32_all_zeros)
{
    auto addr = make_ipv4("0.0.0.0");
    EXPECT_EQ(addr.to_uint32(), 0x00000000u);
}

TEST(ethernet_ipv4_address, to_uint32_broadcast)
{
    auto addr = make_ipv4("255.255.255.255");
    EXPECT_EQ(addr.to_uint32(), 0xFFFFFFFFu);
}

TEST(ethernet_ipv4_address, to_uint32_loopback)
{
    auto addr = make_ipv4("127.0.0.1");
    EXPECT_EQ(addr.to_uint32(), 0x7F000001u);
}

TEST(ethernet_ipv4_address, to_uint32_first_octet_only_255)
{
    auto addr = make_ipv4("255.0.0.0");
    EXPECT_EQ(addr.to_uint32(), 0xFF000000u);
}

TEST(ethernet_ipv4_address, to_uint32_second_octet_only_255)
{
    auto addr = make_ipv4("0.255.0.0");
    EXPECT_EQ(addr.to_uint32(), 0x00FF0000u);
}

TEST(ethernet_ipv4_address, to_uint32_third_octet_only_255)
{
    auto addr = make_ipv4("0.0.255.0");
    EXPECT_EQ(addr.to_uint32(), 0x0000FF00u);
}

TEST(ethernet_ipv4_address, to_uint32_fourth_octet_only_255)
{
    auto addr = make_ipv4("0.0.0.255");
    EXPECT_EQ(addr.to_uint32(), 0x000000FFu);
}

TEST(ethernet_ipv4_address, to_uint32_first_octet_only_1)
{
    auto addr = make_ipv4("1.0.0.0");
    EXPECT_EQ(addr.to_uint32(), 0x01000000u);
}

TEST(ethernet_ipv4_address, to_uint32_second_octet_only_1)
{
    auto addr = make_ipv4("0.1.0.0");
    EXPECT_EQ(addr.to_uint32(), 0x00010000u);
}

TEST(ethernet_ipv4_address, to_uint32_third_octet_only_1)
{
    auto addr = make_ipv4("0.0.1.0");
    EXPECT_EQ(addr.to_uint32(), 0x00000100u);
}

TEST(ethernet_ipv4_address, to_uint32_fourth_octet_only_1)
{
    auto addr = make_ipv4("0.0.0.1");
    EXPECT_EQ(addr.to_uint32(), 0x00000001u);
}

TEST(ethernet_ipv4_address, to_uint32_class_c_network)
{
    auto addr = make_ipv4("192.168.1.0");
    // 192=0xC0, 168=0xA8, 1=0x01, 0=0x00
    EXPECT_EQ(addr.to_uint32(), 0xC0A80100u);
}

TEST(ethernet_ipv4_address, to_uint32_mixed_boundary)
{
    auto addr = make_ipv4("128.64.32.16");
    // 128=0x80, 64=0x40, 32=0x20, 16=0x10
    EXPECT_EQ(addr.to_uint32(), 0x80402010u);
}

TEST(ethernet_ipv4_address, to_uint32_adjacent_to_broadcast)
{
    auto addr = make_ipv4("255.255.255.254");
    EXPECT_EQ(addr.to_uint32(), 0xFFFFFFFEu);
}

TEST(ethernet_ipv4_address, to_uint32_adjacent_to_zero)
{
    auto addr = make_ipv4("0.0.0.1");
    EXPECT_EQ(addr.to_uint32(), 0x00000001u);
}

/*******************************************************************************
 *
 * to_string <-> to_uint32 consistency for various addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, to_string_and_to_uint32_are_consistent_for_zeros)
{
    auto addr = make_ipv4("0.0.0.0");
    EXPECT_EQ(addr.to_string(), "0.0.0.0");
    EXPECT_EQ(addr.to_uint32(), 0u);
}

TEST(ethernet_ipv4_address, to_string_and_to_uint32_are_consistent_for_broadcast)
{
    auto addr = make_ipv4("255.255.255.255");
    EXPECT_EQ(addr.to_string(), "255.255.255.255");
    EXPECT_EQ(addr.to_uint32(), 0xFFFFFFFFu);
}

TEST(ethernet_ipv4_address, to_string_and_to_uint32_are_consistent_for_loopback)
{
    auto addr = make_ipv4("127.0.0.1");
    EXPECT_EQ(addr.to_string(), "127.0.0.1");
    EXPECT_EQ(addr.to_uint32(), 0x7F000001u);
}

TEST(ethernet_ipv4_address, to_string_and_to_uint32_are_consistent_for_arbitrary)
{
    // 222.173.190.239 = 0xDEADBEEF
    auto addr = make_ipv4_from_uint32(0xDEADBEEFu);
    EXPECT_EQ(addr.to_string(), "222.173.190.239");
    EXPECT_EQ(addr.to_uint32(), 0xDEADBEEFu);
}

/*******************************************************************************
 *
 * Round-trip: string > uint32 > string.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, roundtrip_string_to_uint32_to_string)
{
    const std::string original = "172.16.254.3";

    auto addr = make_ipv4(original);
    const uint32_t numeric = addr.to_uint32();

    auto rebuilt = make_ipv4_from_uint32(numeric);
    EXPECT_EQ(rebuilt.to_string(), original);
    EXPECT_EQ(rebuilt.to_uint32(), numeric);
}

TEST(ethernet_ipv4_address, roundtrip_uint32_max)
{
    auto addr = make_ipv4_from_uint32(0xFFFFFFFFu);
    EXPECT_EQ(addr.to_string(), "255.255.255.255");
    EXPECT_EQ(addr.to_uint32(), 0xFFFFFFFFu);
}

TEST(ethernet_ipv4_address, roundtrip_uint32_zero)
{
    auto addr = make_ipv4_from_uint32(0x00000000u);
    EXPECT_EQ(addr.to_string(), "0.0.0.0");
    EXPECT_EQ(addr.to_uint32(), 0x00000000u);
}

TEST(ethernet_ipv4_address, roundtrip_uint32_loopback)
{
    auto addr = make_ipv4_from_uint32(0x7F000001u);
    EXPECT_EQ(addr.to_string(), "127.0.0.1");
    EXPECT_EQ(addr.to_uint32(), 0x7F000001u);
}

TEST(ethernet_ipv4_address, roundtrip_uint32_one)
{
    auto addr = make_ipv4_from_uint32(0x00000001u);
    EXPECT_EQ(addr.to_string(), "0.0.0.1");
    EXPECT_EQ(addr.to_uint32(), 0x00000001u);
}

TEST(ethernet_ipv4_address, roundtrip_uint32_high_bit)
{
    auto addr = make_ipv4_from_uint32(0x80000000u);
    EXPECT_EQ(addr.to_string(), "128.0.0.0");
    EXPECT_EQ(addr.to_uint32(), 0x80000000u);
}

/*******************************************************************************
 *
 * to_string format validation with non-default addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, to_string_has_exactly_three_dots_for_nonzero_address)
{
    auto addr = make_ipv4("192.168.1.1");
    const std::string s = addr.to_string();
    EXPECT_EQ(std::count(s.begin(), s.end(), '.'), 3);
}

TEST(ethernet_ipv4_address, to_string_contains_only_digits_and_dots)
{
    auto addr = make_ipv4("255.128.64.1");
    const std::string s = addr.to_string();

    for (char c : s)
    {
        EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(c)) || c == '.')
            << "Unexpected character: " << c;
    }
}

TEST(ethernet_ipv4_address, to_string_does_not_contain_whitespace)
{
    auto addr = make_ipv4("10.20.30.40");
    const std::string s = addr.to_string();

    EXPECT_EQ(s.find(' '), std::string::npos);
    EXPECT_EQ(s.find('\t'), std::string::npos);
    EXPECT_EQ(s.find('\n'), std::string::npos);
}

TEST(ethernet_ipv4_address, to_string_does_not_start_or_end_with_dot)
{
    auto addr = make_ipv4("100.200.50.25");
    const std::string s = addr.to_string();

    EXPECT_NE(s.front(), '.');
    EXPECT_NE(s.back(), '.');
}

TEST(ethernet_ipv4_address, to_string_no_leading_zeros_in_octets)
{
    // "1.2.3.4" should not produce "001.002.003.004"
    auto addr = make_ipv4("1.2.3.4");
    EXPECT_EQ(addr.to_string(), "1.2.3.4");
}

TEST(ethernet_ipv4_address, to_string_is_not_empty)
{
    auto addr = make_ipv4("10.0.0.1");
    EXPECT_FALSE(addr.to_string().empty());
}

/*******************************************************************************
 *
 * Copy semantics with non-default addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, copy_constructor_preserves_nonzero_value)
{
    auto original = make_ipv4("192.168.1.42");
    ethernet_ipv4_address copy(original);

    EXPECT_EQ(copy.to_string(), "192.168.1.42");
    EXPECT_EQ(copy.to_uint32(), original.to_uint32());
}

TEST(ethernet_ipv4_address, copy_assignment_preserves_nonzero_value)
{
    auto original = make_ipv4("10.0.255.128");
    ethernet_ipv4_address other;

    other = original;

    EXPECT_EQ(other.to_string(), "10.0.255.128");
    EXPECT_EQ(other.to_uint32(), original.to_uint32());
}

TEST(ethernet_ipv4_address, copy_is_independent_of_original)
{
    auto original = make_ipv4("192.168.0.1");
    ethernet_ipv4_address copy(original);

    EXPECT_EQ(copy.to_uint32(), original.to_uint32());
    EXPECT_NE(&copy, &original);
}

/*******************************************************************************
 *
 * Move semantics with non-default addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, move_constructor_preserves_value)
{
    auto original = make_ipv4("172.16.0.99");
    const uint32_t expected = original.to_uint32();

    ethernet_ipv4_address moved(std::move(original));

    EXPECT_EQ(moved.to_string(), "172.16.0.99");
    EXPECT_EQ(moved.to_uint32(), expected);
}

TEST(ethernet_ipv4_address, move_assignment_preserves_value)
{
    auto original = make_ipv4("10.255.0.1");
    const uint32_t expected = original.to_uint32();
    ethernet_ipv4_address other;

    other = std::move(original);

    EXPECT_EQ(other.to_string(), "10.255.0.1");
    EXPECT_EQ(other.to_uint32(), expected);
}

/*******************************************************************************
 *
 * Polymorphism / inheritance.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, is_derived_from_ethernet_ip_address)
{
    EXPECT_TRUE((std::is_base_of<ethernet_ip_address, ethernet_ipv4_address>::value));
}

TEST(ethernet_ipv4_address, virtual_dispatch_through_base_pointer)
{
    auto addr = make_ipv4("10.0.0.1");
    ethernet_ip_address *base = &addr;

    EXPECT_EQ(base->to_string(), "10.0.0.1");
}

TEST(ethernet_ipv4_address, virtual_dispatch_through_base_reference)
{
    auto addr = make_ipv4("192.168.100.200");
    ethernet_ip_address &ref = addr;

    EXPECT_EQ(ref.to_string(), "192.168.100.200");
}

TEST(ethernet_ipv4_address, dynamic_cast_from_base_pointer_succeeds)
{
    auto addr = make_ipv4("127.0.0.1");
    ethernet_ip_address *base = &addr;

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(base);
    ASSERT_NE(ipv4, nullptr);
    EXPECT_EQ(ipv4->to_uint32(), 0x7F000001u);
}

TEST(ethernet_ipv4_address, dynamic_cast_to_wrong_derived_type_returns_nullptr)
{
    auto addr = make_ipv4("10.0.0.1");
    ethernet_ip_address *base = &addr;

    auto *ipv6 = dynamic_cast<ethernet_ipv6_address *>(base);
    EXPECT_EQ(ipv6, nullptr);
}

/*******************************************************************************
 *
 * Multiple instances are independent.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, two_different_addresses_are_independent)
{
    auto addr1 = make_ipv4("10.0.0.1");
    auto addr2 = make_ipv4("10.0.0.2");

    EXPECT_NE(addr1.to_uint32(), addr2.to_uint32());
    EXPECT_NE(addr1.to_string(), addr2.to_string());
}

TEST(ethernet_ipv4_address, modifying_copy_does_not_affect_original)
{
    auto original = make_ipv4("192.168.0.1");
    auto copy = original;

    // Overwrite copy with a different address
    copy = make_ipv4("10.0.0.1");

    EXPECT_EQ(original.to_string(), "192.168.0.1");
    EXPECT_EQ(copy.to_string(), "10.0.0.1");
}

/*******************************************************************************
 *
 * Polymorphic smart pointer usage.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, shared_ptr_to_base_holds_nonzero_address)
{
    auto concrete = make_ipv4("192.168.1.1");
    std::shared_ptr<ethernet_ip_address> base = std::make_shared<ethernet_ipv4_address>(concrete);

    EXPECT_EQ(base->to_string(), "192.168.1.1");

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>(base.get());
    ASSERT_NE(ipv4, nullptr);
    EXPECT_EQ(ipv4->to_uint32(), concrete.to_uint32());
}

/*******************************************************************************
 *
 * Idempotency — repeated calls with non-default addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, to_string_is_idempotent)
{
    auto addr = make_ipv4("172.16.254.3");

    const std::string s1 = addr.to_string();
    const std::string s2 = addr.to_string();
    const std::string s3 = addr.to_string();

    EXPECT_EQ(s1, s2);
    EXPECT_EQ(s2, s3);
}

TEST(ethernet_ipv4_address, to_uint32_is_idempotent)
{
    auto addr = make_ipv4("10.20.30.40");

    const uint32_t v1 = addr.to_uint32();
    const uint32_t v2 = addr.to_uint32();
    const uint32_t v3 = addr.to_uint32();

    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v2, v3);
}

/*******************************************************************************
 *
 * Const correctness with non-default addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, const_object_to_string)
{
    const auto addr = make_ipv4("10.0.0.1");
    EXPECT_EQ(addr.to_string(), "10.0.0.1");
}

TEST(ethernet_ipv4_address, const_object_to_uint32)
{
    const auto addr = make_ipv4("255.255.255.255");
    EXPECT_EQ(addr.to_uint32(), 0xFFFFFFFFu);
}

TEST(ethernet_ipv4_address, const_base_pointer_to_string)
{
    const auto addr = make_ipv4("192.168.0.1");
    const ethernet_ip_address *base = &addr;

    EXPECT_EQ(base->to_string(), "192.168.0.1");
}

/*******************************************************************************
 *
 * Type traits / compile-time properties.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, is_default_constructible)
{
    EXPECT_TRUE(std::is_default_constructible<ethernet_ipv4_address>::value);
}

TEST(ethernet_ipv4_address, is_copy_constructible)
{
    EXPECT_TRUE(std::is_copy_constructible<ethernet_ipv4_address>::value);
}

TEST(ethernet_ipv4_address, is_copy_assignable)
{
    EXPECT_TRUE(std::is_copy_assignable<ethernet_ipv4_address>::value);
}

TEST(ethernet_ipv4_address, is_move_constructible)
{
    EXPECT_TRUE(std::is_move_constructible<ethernet_ipv4_address>::value);
}

TEST(ethernet_ipv4_address, is_move_assignable)
{
    EXPECT_TRUE(std::is_move_assignable<ethernet_ipv4_address>::value);
}

TEST(ethernet_ipv4_address, is_not_abstract)
{
    EXPECT_FALSE(std::is_abstract<ethernet_ipv4_address>::value);
}

TEST(ethernet_ipv4_address, is_polymorphic)
{
    EXPECT_TRUE(std::is_polymorphic<ethernet_ipv4_address>::value);
}

/*******************************************************************************
 *
 * Return types.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, to_string_returns_std_string)
{
    ethernet_ipv4_address addr;
    EXPECT_TRUE((std::is_same<decltype(addr.to_string()), std::string>::value));
}

TEST(ethernet_ipv4_address, to_uint32_returns_uint32)
{
    ethernet_ipv4_address addr;
    EXPECT_TRUE((std::is_same<decltype(addr.to_uint32()), uint32_t>::value));
}

/*******************************************************************************
 *
 * Self-assignment safety.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, self_assignment_is_safe)
{
    auto addr = make_ipv4("10.20.30.40");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
    addr = addr;
#pragma GCC diagnostic pop

    EXPECT_EQ(addr.to_string(), "10.20.30.40");
}

/*******************************************************************************
 *
 * STL container compatibility.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, storable_in_vector)
{
    std::vector<ethernet_ipv4_address> addrs;
    addrs.push_back(make_ipv4("10.0.0.1"));
    addrs.push_back(make_ipv4("10.0.0.2"));
    addrs.push_back(make_ipv4("10.0.0.3"));

    EXPECT_EQ(addrs.size(), 3u);
    EXPECT_EQ(addrs[0].to_string(), "10.0.0.1");
    EXPECT_EQ(addrs[1].to_string(), "10.0.0.2");
    EXPECT_EQ(addrs[2].to_string(), "10.0.0.3");
}

/*******************************************************************************
 *
 * from_sockaddr_in — null pointer and unsupported family.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, null_pointer_returns_false)
{
    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(nullptr).has_value());
}

TEST(ethernet_ipv4_from_sockaddr, unsupported_family_returns_false)
{
    sockaddr sa = {};
    sa.sa_family = AF_UNSPEC;

    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(&sa).has_value());
}

TEST(ethernet_ipv4_from_sockaddr, unsupported_unix_family_returns_false)
{
    sockaddr sa = {};
#ifdef _WIN32
    sa.sa_family = AF_UNIX;
#else
    sa.sa_family = AF_UNIX;
#endif

    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(&sa).has_value());
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv4 basic success / return value.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, returns_true_on_valid_ipv4)
{
    auto sa = make_sockaddr_ipv4("192.168.1.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_TRUE(addr.has_value());
}

TEST(ethernet_ipv4_from_sockaddr, output_pointer_is_not_null_on_success)
{
    auto sa = make_sockaddr_ipv4("10.0.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_TRUE(addr.has_value());
}

TEST(ethernet_ipv4_from_sockaddr, result_is_ipv4_type)
{
    auto sa = make_sockaddr_ipv4("10.0.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_NE(dynamic_cast<ethernet_ipv4_address *>((*addr).get()), nullptr);
}

TEST(ethernet_ipv4_from_sockaddr, basic_address_value)
{
    auto sa = make_sockaddr_ipv4("192.168.1.100");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "192.168.1.100");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv4 corner-case addresses.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, all_zeros)
{
    auto sa = make_sockaddr_ipv4("0.0.0.0");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "0.0.0.0");
}

TEST(ethernet_ipv4_from_sockaddr, broadcast)
{
    auto sa = make_sockaddr_ipv4("255.255.255.255");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "255.255.255.255");
}

TEST(ethernet_ipv4_from_sockaddr, loopback)
{
    auto sa = make_sockaddr_ipv4("127.0.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "127.0.0.1");
}

TEST(ethernet_ipv4_from_sockaddr, class_a_private)
{
    auto sa = make_sockaddr_ipv4("10.255.255.254");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "10.255.255.254");
}

TEST(ethernet_ipv4_from_sockaddr, class_b_private)
{
    auto sa = make_sockaddr_ipv4("172.16.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "172.16.0.1");
}

TEST(ethernet_ipv4_from_sockaddr, class_c_private)
{
    auto sa = make_sockaddr_ipv4("192.168.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "192.168.0.1");
}

TEST(ethernet_ipv4_from_sockaddr, link_local)
{
    auto sa = make_sockaddr_ipv4("169.254.1.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "169.254.1.1");
}

TEST(ethernet_ipv4_from_sockaddr, multicast)
{
    auto sa = make_sockaddr_ipv4("224.0.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "224.0.0.1");
}

TEST(ethernet_ipv4_from_sockaddr, class_e_reserved)
{
    auto sa = make_sockaddr_ipv4("240.0.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "240.0.0.1");
}

/*******************************************************************************
 *
 * from_sockaddr_in — IPv4 per-octet boundaries.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, first_octet_max)
{
    auto sa = make_sockaddr_ipv4("255.0.0.0");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "255.0.0.0");
}

TEST(ethernet_ipv4_from_sockaddr, second_octet_max)
{
    auto sa = make_sockaddr_ipv4("0.255.0.0");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "0.255.0.0");
}

TEST(ethernet_ipv4_from_sockaddr, third_octet_max)
{
    auto sa = make_sockaddr_ipv4("0.0.255.0");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "0.0.255.0");
}

TEST(ethernet_ipv4_from_sockaddr, fourth_octet_max)
{
    auto sa = make_sockaddr_ipv4("0.0.0.255");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*addr)->to_string(), "0.0.0.255");
}

/*******************************************************************************
 *
 * from_sockaddr_in — port is ignored.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, port_does_not_affect_address)
{
    auto sa1 = make_sockaddr_ipv4("10.0.0.1", 0);
    auto sa2 = make_sockaddr_ipv4("10.0.0.1", 8080);
    auto sa3 = make_sockaddr_ipv4("10.0.0.1", 65535);

    auto a1 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa1));
    auto a2 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa2));
    auto a3 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa3));

    EXPECT_EQ((*a1)->to_string(), (*a2)->to_string());
    EXPECT_EQ((*a2)->to_string(), (*a3)->to_string());
}

/*******************************************************************************
 *
 * from_sockaddr_in — round-trip and consistency.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, round_trip_matches_from_string)
{
    const std::string ip = "172.31.255.254";

    auto from_str = make_ipv4(ip);

    auto sa = make_sockaddr_ipv4(ip);
    auto from_sa =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    EXPECT_EQ((*from_sa)->to_string(), from_str.to_string());
}

TEST(ethernet_ipv4_from_sockaddr, round_trip_matches_from_uint32)
{
    auto sa = make_sockaddr_ipv4("1.2.3.4");
    auto from_sa =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>((*from_sa).get());
    auto from_u32 = make_ipv4_from_uint32(ipv4->to_uint32());

    EXPECT_EQ(ipv4->to_string(), from_u32.to_string());
}

TEST(ethernet_ipv4_from_sockaddr, to_uint32_matches_from_string)
{
    const std::string ip = "192.168.100.200";

    auto sa = make_sockaddr_ipv4(ip);
    auto from_sa =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));
    auto *sa_ipv4 = dynamic_cast<ethernet_ipv4_address *>((*from_sa).get());

    auto str_ipv4 = make_ipv4(ip);

    EXPECT_EQ(sa_ipv4->to_uint32(), str_ipv4.to_uint32());
}

/*******************************************************************************
 *
 * from_sockaddr_in — virtual dispatch / polymorphism.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, virtual_to_string_via_base_pointer)
{
    auto sa = make_sockaddr_ipv4("8.8.8.8");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));

    ethernet_ip_address *base = (*addr).get();
    EXPECT_EQ(base->to_string(), "8.8.8.8");
}

/*******************************************************************************
 *
 * from_sockaddr_in — independence and overwrite.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_from_sockaddr, two_calls_produce_independent_objects)
{
    auto sa1 = make_sockaddr_ipv4("1.1.1.1");
    auto sa2 = make_sockaddr_ipv4("2.2.2.2");

    auto a1 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa1));
    auto a2 = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa2));

    EXPECT_NE((*a1)->to_string(), (*a2)->to_string());
}

TEST(ethernet_ipv4_from_sockaddr, second_call_overwrites_previous_pointer)
{
    auto sa1 = make_sockaddr_ipv4("1.1.1.1");
    auto sa2 = make_sockaddr_ipv4("2.2.2.2");

    auto addr1 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa1));
    EXPECT_EQ((*addr1)->to_string(), "1.1.1.1");

    auto addr2 =
        ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa2));
    EXPECT_EQ((*addr2)->to_string(), "2.2.2.2");
}

TEST(ethernet_ipv4_from_sockaddr, failure_does_not_overwrite_existing_pointer)
{
    auto sa = make_sockaddr_ipv4("10.0.0.1");
    auto addr = ethernet_address_factory::from_sockaddr_in(reinterpret_cast<const sockaddr *>(&sa));
    ASSERT_TRUE(addr.has_value());
    auto original = *addr;

    EXPECT_FALSE(ethernet_address_factory::from_sockaddr_in(nullptr).has_value());
}

/*******************************************************************************
 *
 * IPv4 overflow: calculate_new_address with host_index that's a full uint64_t max.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, calculate_new_address_overflow_uint64_max)
{
    auto base = ethernet_address_factory::from_string("0.0.0.0");

    uint64_t host_index = std::numeric_limits<uint64_t>::max();

    auto result = ethernet_address_factory::calculate_new_address(*base, host_index);
    EXPECT_FALSE(result.has_value());
}

/*******************************************************************************
 *
 * IPv4 calculate_new_address: valid host_index within range.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, calculate_new_address_valid_index)
{
    auto base = ethernet_address_factory::from_string("10.0.0.1");

    uint64_t host_index = 5;

    auto result = ethernet_address_factory::calculate_new_address(*base, host_index);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "10.0.0.6");
}

/*******************************************************************************
 *
 * IPv4 calculate_new_address: host_index causes wrap-around.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, calculate_new_address_wrap_around)
{
    auto base = ethernet_address_factory::from_string("255.255.255.250");

    uint64_t host_index = 10;

    auto result = ethernet_address_factory::calculate_new_address(*base, host_index);
    EXPECT_FALSE(result.has_value());
}

/*******************************************************************************
 *
 * IPv4 calculate_new_address: host_index zero returns same address.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, calculate_new_address_zero_index)
{
    auto base = ethernet_address_factory::from_string("192.168.1.100");

    uint64_t host_index = 0;

    auto result = ethernet_address_factory::calculate_new_address(*base, host_index);

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ((*result)->to_string(), "192.168.1.100");
}

/*******************************************************************************
 *
 * Comparison operators — IPv4.
 *
 *******************************************************************************/
TEST(factory_ipv4_comparison, equal_addresses_are_equal)
{
    auto a = ethernet_address_factory::from_string("192.168.1.1");
    auto b = ethernet_address_factory::from_string("192.168.1.1");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a == *ipv4_b);
    EXPECT_FALSE(*ipv4_a != *ipv4_b);
}

TEST(factory_ipv4_comparison, different_addresses_are_not_equal)
{
    auto a = ethernet_address_factory::from_string("192.168.1.1");
    auto b = ethernet_address_factory::from_string("192.168.1.2");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_FALSE(*ipv4_a == *ipv4_b);
    EXPECT_TRUE(*ipv4_a != *ipv4_b);
}

TEST(factory_ipv4_comparison, lower_address_is_less_than_higher)
{
    auto a = ethernet_address_factory::from_string("10.0.0.1");
    auto b = ethernet_address_factory::from_string("10.0.0.2");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a < *ipv4_b);
    EXPECT_FALSE(*ipv4_b < *ipv4_a);
}

TEST(factory_ipv4_comparison, equal_addresses_not_less_than)
{
    auto a = ethernet_address_factory::from_string("10.0.0.1");
    auto b = ethernet_address_factory::from_string("10.0.0.1");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_FALSE(*ipv4_a < *ipv4_b);
}

TEST(factory_ipv4_comparison, less_than_or_equal_for_equal)
{
    auto a = ethernet_address_factory::from_string("10.0.0.1");
    auto b = ethernet_address_factory::from_string("10.0.0.1");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a <= *ipv4_b);
}

TEST(factory_ipv4_comparison, less_than_or_equal_for_less)
{
    auto a = ethernet_address_factory::from_string("10.0.0.1");
    auto b = ethernet_address_factory::from_string("10.0.0.2");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a <= *ipv4_b);
    EXPECT_FALSE(*ipv4_b <= *ipv4_a);
}

TEST(factory_ipv4_comparison, greater_than)
{
    auto a = ethernet_address_factory::from_string("10.0.0.2");
    auto b = ethernet_address_factory::from_string("10.0.0.1");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a > *ipv4_b);
    EXPECT_FALSE(*ipv4_b > *ipv4_a);
}

TEST(factory_ipv4_comparison, greater_than_or_equal_for_equal)
{
    auto a = ethernet_address_factory::from_string("10.0.0.1");
    auto b = ethernet_address_factory::from_string("10.0.0.1");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a >= *ipv4_b);
}

TEST(factory_ipv4_comparison, greater_than_or_equal_for_greater)
{
    auto a = ethernet_address_factory::from_string("10.0.0.2");
    auto b = ethernet_address_factory::from_string("10.0.0.1");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a >= *ipv4_b);
    EXPECT_FALSE(*ipv4_b >= *ipv4_a);
}

TEST(factory_ipv4_comparison, first_octet_determines_order)
{
    auto a = ethernet_address_factory::from_string("1.255.255.255");
    auto b = ethernet_address_factory::from_string("2.0.0.0");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a < *ipv4_b);
}

TEST(factory_ipv4_comparison, last_octet_determines_order)
{
    auto a = ethernet_address_factory::from_string("10.0.0.1");
    auto b = ethernet_address_factory::from_string("10.0.0.2");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a < *ipv4_b);
}

TEST(factory_ipv4_comparison, zero_and_broadcast)
{
    auto a = ethernet_address_factory::from_string("0.0.0.0");
    auto b = ethernet_address_factory::from_string("255.255.255.255");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a < *ipv4_b);
    EXPECT_TRUE(*ipv4_b > *ipv4_a);
    EXPECT_FALSE(*ipv4_a == *ipv4_b);
}

TEST(factory_ipv4_comparison, adjacent_addresses)
{
    auto a = ethernet_address_factory::from_string("192.168.0.255");
    auto b = ethernet_address_factory::from_string("192.168.1.0");

    auto *ipv4_a = dynamic_cast<ethernet_ipv4_address *>((*a).get());
    auto *ipv4_b = dynamic_cast<ethernet_ipv4_address *>((*b).get());

    EXPECT_TRUE(*ipv4_a < *ipv4_b);
    EXPECT_TRUE(*ipv4_a != *ipv4_b);
}

/*******************************************************************************
 *
 * Edge case: very long input strings.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, invalid_very_long_input_string)
{
    std::string very_long = std::string(10000, 'a');
    EXPECT_FALSE(ethernet_address_factory::from_string(very_long).has_value());
}

TEST(ethernet_ipv4_address, invalid_very_long_input_with_dots)
{
    std::string very_long = "10.0.0.1" + std::string(10000, '.');
    EXPECT_FALSE(ethernet_address_factory::from_string(very_long).has_value());
}

TEST(ethernet_ipv4_address, invalid_very_long_octet_value)
{
    std::string very_long_number = std::string(1000, '9') + ".0.0.0";
    EXPECT_FALSE(ethernet_address_factory::from_string(very_long_number).has_value());
}

/*******************************************************************************
 *
 * Edge case: null bytes embedded in string.
 *
 *******************************************************************************/
TEST(ethernet_ipv4_address, invalid_null_byte_in_string)
{
    std::string with_null = "192.168.1.1\0extra";
    EXPECT_TRUE(ethernet_address_factory::from_string(with_null).has_value());
}

TEST(ethernet_ipv4_address, invalid_null_byte_at_start)
{
    std::string with_null = std::string(1, '\0') + "192.168.1.1";
    EXPECT_FALSE(ethernet_address_factory::from_string(with_null).has_value());
}

TEST(ethernet_ipv4_address, invalid_null_byte_between_octets)
{
    std::string with_null = "192.168\0.1.1";
    EXPECT_FALSE(ethernet_address_factory::from_string(with_null).has_value());
}

/*******************************************************************************
 *
 * Self-assignment safety for comparison operators.
 *
 *******************************************************************************/
TEST(factory_ipv4_comparison, self_equality_comparison_is_safe)
{
    auto addr = ethernet_address_factory::from_string("10.20.30.40");

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>((*addr).get());
    EXPECT_TRUE(*ipv4 == *ipv4);
    EXPECT_FALSE(*ipv4 != *ipv4);
}

TEST(factory_ipv4_comparison, self_ordering_comparison_is_safe)
{
    auto addr = ethernet_address_factory::from_string("192.168.1.1");

    auto *ipv4 = dynamic_cast<ethernet_ipv4_address *>((*addr).get());
    EXPECT_FALSE(*ipv4 < *ipv4);
    EXPECT_TRUE(*ipv4 <= *ipv4);
    EXPECT_FALSE(*ipv4 > *ipv4);
    EXPECT_TRUE(*ipv4 >= *ipv4);
}

// NOLINTEND
