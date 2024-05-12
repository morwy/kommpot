#include <cstdlib>

#include "libkommpot.h"

auto main(int argc, char *argv[]) -> int
{
    printf("libkommpot version: %s.\n\n", kommpot::get_version().to_string().c_str());

    const auto &devices = kommpot::get_device_list();
    printf("Found %zu devices:\n", devices.size());

    for (const auto &device : devices)
    {
        printf("  - %s %s (VID_%04X/PID_%04X), serial number %s, at port %s via %s library.\n",
            device->information().manufacturer.empty() ? "NONE"
                                                       : device->information().manufacturer.c_str(),
            device->information().name.empty() ? "NONE" : device->information().name.c_str(),
            device->information().vendor_id, device->information().product_id,
            device->information().serial_number.empty()
                ? "NONE"
                : device->information().serial_number.c_str(),
            device->information().port.empty() ? "NONE" : device->information().port.c_str(),
            kommpot::communication_type_to_string(device->type()).c_str());
    }

    return EXIT_SUCCESS;
}
