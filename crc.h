#ifndef _CRC_H
#define _CRC_H
#include <cstddef>
#include <cstdint>

uint32_t crc32(uint32_t crc, const void *buf, size_t size);
#endif //_CRC_H
