#ifndef fd_apollo_h
#define fd_apollo_h

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wsign-conversion"

#if defined(AM_PART_APOLLO2)
#include <apollo2.h>
#elif defined(AM_PART_APOLLO3)
#include <apollo3.h>
#endif

#include <am_mcu_apollo.h>

#pragma GCC diagnostic pop

#endif