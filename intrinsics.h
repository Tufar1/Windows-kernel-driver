#pragma once
#include <cstdint>
#include "structs.h"

inline IDTR read_idtr()
{
    IDTR idtr{};
    asm volatile("sidt %0" : "=m"(idtr));
    return idtr;
}

inline std::uint64_t read_cr0() {
    std::uint64_t val;
    asm volatile("mov %%cr0, %0" : "=r"(val));
    return val;
}

inline std::uint64_t read_cr2() {
    std::uint64_t val;
    asm volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

inline std::uint64_t read_cr3() {
    std::uint64_t val;
    asm volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

inline std::uint64_t read_cr4() {
    std::uint64_t val;
    asm volatile("mov %%cr4, %0" : "=r"(val));
    return val;
}

inline std::uint64_t read_cr8() {
    std::uint64_t val;
    asm volatile("mov %%cr8, %0" : "=r"(val));
    return val;
}

inline std::uint64_t read_dr(int reg) {
    std::uint64_t val;
    switch (reg) {
        case 0: asm volatile("mov %%dr0, %0" : "=r"(val)); break;
        case 1: asm volatile("mov %%dr1, %0" : "=r"(val)); break;
        case 2: asm volatile("mov %%dr2, %0" : "=r"(val)); break;
        case 3: asm volatile("mov %%dr3, %0" : "=r"(val)); break;
        case 6: asm volatile("mov %%dr6, %0" : "=r"(val)); break;
        case 7: asm volatile("mov %%dr7, %0" : "=r"(val)); break;
        default: val = 0;
    }
    return val;
}

inline void invlpg(void* addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}
