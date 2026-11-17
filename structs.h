#pragma once
#include <ntifs.h>
#include <cstdint>

enum class opCodes : uint32_t {
  read  = 0x05,
  write = 0x10,
  base  = 0x15,
	hideOverlay = 0x20,
  cr3 = 0x47,
	init = 0x99,
	end = 0x100
};

struct umRequest {
  opCodes  code;
	uint32_t bytesToRW;
	uint64_t address;
};

#pragma pack(push, 1)
struct IDTR
{
    uint16_t limit;
    uint64_t base;
};
#pragma pack(pop)

struct IDT_ENTRY64
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
};

union PML4E {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t reserved_0 : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDPTE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t page_size : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDPTE_1GB {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t page_size : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pat : 1;
        std::uint64_t reserved_0 : 17;
        std::uint64_t pfn : 22;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
  };
};

union PDE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t ignored_0 : 1;
        std::uint64_t page_size : 1;
        std::uint64_t ignored_1 : 3;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_2 : 11;
        std::uint64_t nx : 1;
    };
};

union PDE_2MB {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t page_size : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pat : 1;
        std::uint64_t reserved_0 : 8;
        std::uint64_t pfn : 31;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
    };
};

union PTE {
    std::uint64_t value;
    struct {
        std::uint64_t present : 1;
        std::uint64_t rw : 1;
        std::uint64_t us : 1;
        std::uint64_t pwt : 1;
        std::uint64_t pcd : 1;
        std::uint64_t accessed : 1;
        std::uint64_t dirty : 1;
        std::uint64_t pat : 1;
        std::uint64_t global : 1;
        std::uint64_t ignored_0 : 2;
        std::uint64_t restart : 1;
        std::uint64_t pfn : 40;
        std::uint64_t ignored_1 : 7;
        std::uint64_t mpk : 4;
        std::uint64_t nx : 1;
    };
};
