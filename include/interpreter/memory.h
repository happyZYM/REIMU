#pragma once
#include <interpreter/forward.h>
#include <memory>

namespace dark {

struct Memory {
    struct Impl;

    Executable &fetch_executable(target_size_t pc);

    auto create(const Config &) -> std::unique_ptr<Memory>;

    auto load_i8(target_size_t addr)  -> std::int8_t;
    auto load_i16(target_size_t addr) -> std::int16_t;
    auto load_i32(target_size_t addr) -> std::int32_t;
    auto load_u8(target_size_t addr)  -> std::uint8_t;
    auto load_u16(target_size_t addr) -> std::uint16_t;
    auto load_u32(target_size_t addr) -> std::uint32_t;
    auto load_cmd(target_size_t addr) -> std::uint32_t;

    void store_i8(target_size_t addr, std::uint8_t value);
    void store_i16(target_size_t addr, std::uint16_t value);
    void store_i32(target_size_t addr, std::uint32_t value);

    Impl &get_impl();
};

} // namespace dark
