#pragma once
#include <string>
#include <declarations.h>
#include <libc/forward.h>
#include <interpreter/forward.h>

namespace dark {

enum class Error : std::uint8_t {
    LoadMisAligned,
    LoadOutOfBound,

    StoreMisAligned,
    StoreOutOfBound,

    InsMisAligned,
    InsOutOfBound,
    InsUnknown,

    LibcMisAligned, // libc read/write access
    LibcOutOfBound, // libc read/write access
    LibcError,      // libc error

    DivideByZero,

    NotImplemented,
};

struct FailToInterpret {
    static constexpr auto kLibcDummy = static_cast<libc::libc_index_t>(-1);
    Error error;
    libc::libc_index_t libc_which = kLibcDummy;

    union {
        struct {
            union {
                target_size_t address;
            };
            union {
                command_size_t  command;
                target_size_t   alignment;
                target_size_t   size;
            };
        };
        const char *message;
    };

    auto what(RegisterFile &, Memory &, Device &) const -> std::string;
};

} // namespace dark
