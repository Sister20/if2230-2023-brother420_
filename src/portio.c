#include "lib-header/stdtype.h"
#include "lib-header/portio.h"
#include "lib-header/fat32.h"

/** x86 inb/outb:
* @param dx target port
* @param al input/output byte
*/

void out(uint16_t port, uint8_t data) {
    __asm__ volatile(
    "outb %0, %1"
    : // <Empty output operand>
    : "a"(data), "Nd"(port)
    );
}

void out16(uint16_t port, uint16_t data) {
    __asm__ volatile(
    "outw %0, %1"
    : // <Empty output operand>
    : "a"(data), "Nd"(port)
    );
}

uint8_t in(uint16_t port) {
    uint8_t result;
    __asm__ volatile(
    "inb %1, %0"
    : "=a"(result)
    : "Nd"(port)
    );
    return result;
}

uint16_t in16(uint16_t port) {
    uint16_t result;
    __asm__ volatile(
    "inw %1, %0"
    : "=a"(result)
    : "Nd"(port)
    );
    return result;
}

uint8_t cmos_read(uint8_t reg) {
    __asm__ volatile (
        "outb %0, %1" 
        : 
        : "a"(reg), "Nd"(0x70));
    uint8_t data;
    __asm__ volatile (
        "inb %1, %0" 
        : "=a"(data) 
        : "Nd"(0x71));
    return data;
}

void get_time(struct Time *time, struct Date *date) {
    // Read the current time from the CMOS registers
    uint8_t jam_cmos = cmos_read(0x04);
    uint8_t menit_cmos = cmos_read(0x02);
    uint8_t detik_cmos = cmos_read(0x00);
    uint8_t tanggal_cmos = cmos_read(0x07);
    uint8_t bulan_cmos = cmos_read(0x08);
    uint8_t tahun_cmos = cmos_read(0x09);

    // Convert BCD values to decimal
    time->jam = ((jam_cmos & 0x0F) + ((jam_cmos >> 4) * 10));
    time->menit = ((menit_cmos & 0x0F) + ((menit_cmos >> 4) * 10));
    time->detik = ((detik_cmos & 0x0F) + ((detik_cmos >> 4) * 10));
    date->tanggal = ((tanggal_cmos & 0x0F) + ((tanggal_cmos >> 4) * 10));
    date->bulan = ((bulan_cmos & 0x0F) + ((bulan_cmos >> 4) * 10));
    date->tahun = ((tahun_cmos & 0x0F) + ((tahun_cmos >> 4) * 10));
    
}






