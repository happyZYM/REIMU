#include <utility.h>
#include <utility/config.h>
#include <interpreter/interpreter.h>
#include <assembly/assembly.h>
#include <linker/linker.h>
#include <libc/libc.h>

namespace dark {

static auto get_start_end(const Linker::LinkResult::Section &section) {
    return std::make_pair(section.start, section.start + section.storage.size());
}

static void check_no_overlap(const Linker::LinkResult &result) {
    auto [text_start, text_end] = get_start_end(result.text);
    auto [data_start, data_end] = get_start_end(result.data);
    auto [rodata_start, rodata_end] = get_start_end(result.rodata);
    auto [bss_start, bss_end] = get_start_end(result.bss);

    runtime_assert(
        text_end <= data_start &&
        data_end <= rodata_start &&
        rodata_end <= bss_start);
}

static void print_link_result(const Linker::LinkResult &result) {
    auto print_section = [](const std::string &name, const Linker::LinkResult::Section &section) {
        std::cout << std::format("Section {} \t at [{:x}, {:x})\n",
            name, section.start, section.start + section.storage.size());
    };

    std::cout << std::format("{:=^80}\n", " Section details ");

    print_section("text", result.text);
    print_section("data", result.data);
    print_section("rodata", result.rodata);
    print_section("bss", result.bss);

    std::cout << std::format("{:=^80}\n", "");
}

Interpreter::Interpreter(const Config &config) {
    std::vector <Assembler> assemblies;
    assemblies.reserve(config.assembly_files.size());
    for (const auto &file : config.assembly_files)
        assemblies.emplace_back(file);

    auto result = Linker{ assemblies }.get_result();
    panic_if(result.position_table.count("main") == 0, "No main function found");
    check_no_overlap(result);

    if (config.option_table.at("detail")) print_link_result(result);

}


} // namespace dark
