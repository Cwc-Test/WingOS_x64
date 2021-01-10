#include <arch.h>

#include <device/ahci.h>
#include <device/network/e1000.h>
#include <device/network/rtl8139.h>
#include <device/pci.h>
#include <liballoc.h>
#include <logging.h>
pci_system main_system;
// the function device_code_to_string is taken from DripOS thank you <3
// there is not license so i don't know if i should put some legal thing but yeah
// check drip os it is a very good os ! :3 https://github.com/Menotdan/DripOS/

// update :
// now it has an license so i put it here : (from commit https://github.com/Menotdan/DripOS/commit/9e2ebd201ec5ef49e39220c91e41eef6a68ad4d8#diff-9879d6db96fd29134fc802214163b95a)
/*
you can steal my code because its bad lol
also im not responsible for bad stuff you do with my code etc etc



this is a very real license that should totally exist
*/
// i think this function is not that bad so i take it (maybe i should write my own device code to string ?)

const char *device_code_to_string(uint8_t class_code, uint8_t subclass, uint8_t prog_if)
{
    switch (class_code)
    {
    case 0:
        return "Undefined";
    case 1:
        switch (subclass)
        {
        case 0:
            return "SCSI Bus Controller";
        case 1:
            return "IDE Controller";
        case 2:
            return "Floppy Disk Controller";
        case 3:
            return "IPI Bus Controller";
        case 4:
            return "RAID Controller";
        case 5:
            return "ATA Controller";
        case 6:
            switch (prog_if)
            {
            case 0:
                return "Vendor specific SATA Controller";
            case 1:
                return "AHCI SATA Controller";
            case 2:
                return "Serial Storage Bus SATA Controller";
            }
            break;
        case 7:
            return "Serial Attached SCSI Controller";
        case 8:
            switch (prog_if)
            {
            case 1:
                return "NVMHCI Controller";
            case 2:
                return "NVMe Controller";
            }
            break;
        }
        return "Mass Storage Controller";
    case 2:
        switch (subclass)
        {
        case 0:
            return "Ethernet Controller";
        case 1:
            return "Token Ring Controller";
        case 2:
            return "FDDI Controller";
        case 3:
            return "ATM Controller";
        case 4:
            return "ISDN Controller";
        case 5:
            return "WorldFip Controller";
        case 6:
            return "PICMG 2.14 Controller";
        case 7:
            return "InfiniBand Controller";
        case 8:
            return "Fabric Controller";
        }
        return "Network Controller";
    case 3:
        switch (subclass)
        {
        case 0:
            return "VGA Compatible Controller";
        case 1:
            return "XGA Controller";
        case 2:
            return "3D Controller";
        }
        return "Display Controller";
    case 4:
        return "Multimedia controller";
    case 5:
        return "Memory Controller";
    case 6:
        switch (subclass)
        {
        case 0:
            return "Host Bridge";
        case 1:
            return "ISA Bridge";
        case 2:
            return "EISA Bridge";
        case 3:
            return "MCA Bridge";
        case 4:
            return "PCI-to-PCI Bridge";
        case 5:
            return "PCMCIA Bridge";
        case 6:
            return "NuBus Bridge";
        case 7:
            return "CardBus Bridge";
        case 8:
            return "RACEway Bridge";
        case 9:
            return "Semi-Transparent PCI-to-PCI Bridge";
        case 10:
            return "InfiniBand-to-PCI Host Bridge";
        }
        return "Bridge Device";
    case 8:
        switch (subclass)
        {
        case 0:
            switch (prog_if)
            {
            case 0:
                return "8259-Compatible PIC";
            case 1:
                return "ISA-Compatible PIC";
            case 2:
                return "EISA-Compatible PIC";
            case 0x10:
                return "I/O APIC IRQ Controller";
            case 0x20:
                return "I/O xAPIC IRQ Controller";
            }
            break;
        case 1:
            switch (prog_if)
            {
            case 0:
                return "8237-Compatible DMA Controller";
            case 1:
                return "ISA-Compatible DMA Controller";
            case 2:
                return "EISA-Compatible DMA Controller";
            }
            break;
        case 2:
            switch (prog_if)
            {
            case 0:
                return "8254-Compatible PIT";
            case 1:
                return "ISA-Compatible PIT";
            case 2:
                return "EISA-Compatible PIT";
            case 3:
                return "HPET";
            }
            break;
        case 3:
            return "Real Time Clock";
        case 4:
            return "PCI Hot-Plug Controller";
        case 5:
            return "SDHCI";
        case 6:
            return "IOMMU";
        }
        return "Base System Peripheral";
    case 0xC:
        switch (subclass)
        {
        case 0:
            switch (prog_if)
            {
            case 0:
                return "Generic FireWire (IEEE 1394) Controller";
            case 0x10:
                return "OHCI FireWire (IEEE 1394) Controller";
            }
            break;
        case 1:
            return "ACCESS Bus Controller";
        case 2:
            return "SSA Controller";
        case 3:
            switch (prog_if)
            {
            case 0:
                return "uHCI USB1 Controller";
            case 0x10:
                return "oHCI USB1 Controller";
            case 0x20:
                return "eHCI USB2 Controller";
            case 0x30:
                return "xHCI USB3 Controller";
            case 0xFE:
                return "USB Device";
            }
            break;
        case 4:
            return "Fibre Channel Controller";
        case 5:
            return "SMBus";
        case 6:
            return "InfiniBand Controller";
        case 7:
            return "IPMI Interface Controller";
        }
        return "Serial Bus Controller";
    default:
        return "Unknown";
        break;
    }
}
pci_device::pci_device(uint8_t bus, uint8_t device)
{
    dbus = bus;
    ddev = device;
}

bool pci_device::function_exist(uint8_t function_id)
{
    if ((uint16_t)read_dword(function_id, 0) != 0xffff)
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool pci_device::valid()
{
    return function_exist(0);
}

uint32_t pci_device::read_dword(uint8_t func, uint8_t reg)
{
    // cast everything in int32
    uint32_t bus32 = (uint32_t)dbus;
    uint32_t device32 = (uint32_t)ddev;
    uint32_t function32 = (uint32_t)func;

    uint32_t target = 0x80000000 | (bus32 << 16) | ((device32) << 11) | ((function32) << 8) | (reg & 0xFC);
    outl(0xcf8, target);

    return inl(0xCFC);
}

void pci_device::write_dword(uint8_t func, uint8_t reg, uint32_t value)
{
    // cast everything in int32
    uint32_t bus32 = (uint32_t)dbus;
    uint32_t device32 = (uint32_t)ddev;
    uint32_t function32 = (uint32_t)func;

    uint32_t target = (1 << 31) | (bus32 << 16) | ((device32 & 0b11111) << 11) | ((function32 & 0b111) << 8) | (reg & ~(0b11));
    outl(0xcf8, target);

    outl(0xCFC, value);
}
bool pci_device::is_bridge(uint8_t function)
{
    if (get_header(function) == 0x1)
    {
        if (get_class(function) == 0x6)
        {
            return true;
        }
    }
    return false;
}

uint8_t pci_device::get_header(uint8_t function)
{
    return (uint8_t)(((uint32_t)read_dword(function, 0xc) >> 16) & ~(1 << 7));
}

uint8_t pci_device::get_sub_buss(uint8_t function)
{

    return (uint8_t)(((uint32_t)read_dword(function, 0x18) >> 8));
}

uint8_t pci_device::get_class(uint8_t function)
{
    return (uint8_t)(((uint32_t)read_dword(function, 0x8) >> 24));
}
uint8_t pci_device::get_irq(uint8_t function)
{
    return (uint8_t)(((uint32_t)read_dword(function, 0x3C)));
}

uint8_t pci_device::get_progif(uint8_t function)
{
    return (uint8_t)(((uint32_t)read_dword(function, 0x8) >> 8));
}

uint16_t pci_device::get_vendor(uint8_t function)
{
    return ((uint16_t)(read_dword(function, 0)));
}
uint16_t pci_device::get_dev_id(uint8_t function)
{
    return (uint16_t)(((uint32_t)read_dword(function, 0)) >> 16);
}
uint8_t pci_device::get_subclass(uint8_t function)
{
    return (uint8_t)(((uint32_t)read_dword(function, 0x8) >> 16));
}

bool pci_device::has_multiple_function()
{
    return ((uint8_t)((uint32_t)read_dword(0, 0xC) >> 16)) & (1 << 7);
}

pci_device_raw pci_device::to_raw(uint8_t function)
{
    pci_device_raw r;
    r.bus = dbus;
    r.function = function;
    r.device = ddev;
    return r;
}
void pci_device::enable_mastering(uint8_t func)
{
    write_dword(func, 0x4, read_dword(func, 0x4) | 0x00000004);
}
pci_bar_data pci_device::get_bar(uint64_t id, uint8_t function)
{
    if (get_header(function) == 2)
    {
        log("pci", LOG_INFO) << "snif";
    }
    uint64_t target = ((uint64_t)id * 4) + 0x10;
    pci_bar_data ret;
    uint32_t value = read_dword(function, target);
    uint32_t base = 0;
    //uint32_t type = value & 0b111;

    if ((value & 0b0111) == 0b0110)
    {
        ret.type = pci_bar_type::MM_IO_64;
        base = value & 0xFFFFFFF0;
    }
    else if ((value & 0b0111) == 0b0000)
    {
        ret.type = pci_bar_type::MM_IO_32;
        base = value & 0xFFFFFFF0;
    }
    else if ((value & 0b0111) == 0b0001)
    {
        ret.type = pci_bar_type::P_IO;
        base = value & 0xFFFFFFFC;
    }

    write_dword(function, target, 0xFFFFFFFF);
    uint64_t nvalue = read_dword(function, target);
    write_dword(function, target, value);

    ret.base = base;
    ret.size = ~(nvalue & 0xFFFFFFF0) + 1;

    log("pci", LOG_INFO) << "pci bar " << id << "func " << function;
    log("pci", LOG_INFO) << "start" << ret.base;
    return ret;
}
pci_system::pci_system()
{
}

pci_system *pci_system::the()
{
    return &main_system;
}

void pci_system::scan_dev(uint8_t dev_id, uint8_t bus_id)
{
    pci_device device(bus_id, dev_id);
    if (device.function_exist(0))
    {
        if (device.is_bridge(0))
        {
            log("pci", LOG_INFO) << "device : " << dev_id << "bus : " << bus_id << "is a bridge";
            scan_bus(device.get_sub_buss(0));
        }
        else
        {
            pci_devices[pci_devices_count] = device.to_raw(0);
            pci_devices_count++;
            if (device.has_multiple_function())
            {
                for (int i = 1; i < 8; i++)
                {
                    if (device.function_exist(i))
                    {
                        if (device.is_bridge(i))
                        {
                            log("pci", LOG_INFO) << "device : " << dev_id << "bus : " << bus_id << "is a bridge";

                            scan_bus(device.get_sub_buss(i));
                        }
                        else
                        {

                            pci_devices[pci_devices_count] = device.to_raw(i);
                            pci_devices_count++;
                        }
                    }
                }
            }
        }
    }
}

void pci_system::scan_bus(uint8_t bus_id)
{
    log("pci", LOG_INFO) << "scanning bus : " << bus_id;
    for (int i = 0; i < 32; i++)
    {
        scan_dev(i, bus_id);
    }
}

void pci_system::init_top_bus()
{
    log("pci", LOG_INFO) << "loading top bus";
    scan_bus(0);
}

void pci_system::init_device()
{
    log("pci", LOG_DEBUG) << "loading pci device";
    for (int i = 0; i < pci_devices_count; i++)
    {
        pci_device dev = pci_device(pci_devices[i].bus, pci_devices[i].device);
        uint8_t dev_func = pci_devices[i].function;

        if (dev.get_vendor(dev_func) == 0x8086)
        {
            if (dev.get_class(dev_func) == 2 && dev.get_subclass(dev_func) == 0)
            {

                uint16_t id = dev.get_dev_id(dev_func);

                if (id == 0x100E || id == 0x153A || id == 0x10EA)
                {
                    dev.enable_mastering(dev_func);
                    e1000::the()->init(&dev, dev_func);
                }
            }
            else if (dev.get_class(dev_func) == 1 && dev.get_subclass(dev_func) == 6)
            {
                ahci *ahci_new_dev = new ahci();
                ahci_new_dev->init(&dev, dev_func);
            }
        }
        else if (dev.get_vendor(dev_func) == 0x10EC)
        {
            if (dev.get_dev_id(dev_func) == 0x8139)
            {
                dev.enable_mastering(dev_func);
                rtl8139::the()->init(&dev, dev_func);
            }
        }
    }
}
void pci_system::init()
{
    log("pci", LOG_DEBUG) << "loading pci";
    pci_devices = (pci_device_raw *)malloc(sizeof(pci_device_raw) * 128);
    pci_devices_count = 0;
    init_top_bus();
    log("pci", LOG_INFO) << "pci devices count : " << pci_devices_count;
    for (int i = 0; i < pci_devices_count; i++)
    {
        pci_device dev = pci_device(pci_devices[i].bus, pci_devices[i].device);
        uint8_t dev_func = pci_devices[i].function;

        log("pci", LOG_INFO) << "=========";
        if (dev.get_vendor(dev_func) == 0x8086)
        {
            log("pci", LOG_INFO) << "=intel=";
            if (dev.get_class(dev_func) == 2 && dev.get_subclass(dev_func) == 0)
            {

                uint16_t id = dev.get_dev_id(dev_func);

                if (id == 0x100E || id == 0x153A || id == 0x10EA)
                {
                    log("pci", LOG_INFO) << "==E1000==";
                }
            }
            else if (dev.get_class(dev_func) == 1 && dev.get_subclass(dev_func) == 6)
            {
                log("pci", LOG_INFO) << "AHCI";
            }
        }
        else if (dev.get_vendor(dev_func) == 0x10EC)
        {
            log("pci", LOG_INFO) << "=realteck=";
            uint16_t id = dev.get_dev_id(dev_func);
            if (id == 0x8139)
            {
                log("pci", LOG_INFO) << "==rtl8139==";
            }
        }
        log("pci", LOG_INFO) << "pci device = " << i;
        log("pci", LOG_INFO) << "bus        = " << pci_devices[i].bus;
        log("pci", LOG_INFO) << "device     = " << pci_devices[i].device;
        log("pci", LOG_INFO) << "function   = " << pci_devices[i].function;
        log("pci", LOG_INFO) << "vendor     = " << dev.get_vendor(dev_func);
        log("pci", LOG_INFO) << "dev id     = " << dev.get_dev_id(dev_func);
        log("pci", LOG_INFO) << "class      = " << dev.get_class(dev_func);
        log("pci", LOG_INFO) << "irq        = " << dev.get_irq(dev_func);
        log("pci", LOG_INFO) << "sub clas   = " << dev.get_subclass(dev_func);
        log("pci", LOG_INFO) << "progif     = " << dev.get_progif(dev_func);
        log("pci", LOG_INFO) << "device name= " << device_code_to_string(dev.get_class(dev_func), dev.get_subclass(dev_func), dev.get_progif(dev_func));
    }
    init_device();
}
