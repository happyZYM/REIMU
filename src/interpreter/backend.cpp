#include "declarations.h"
#include "interpreter/hint.h"
#include "simulation/debug.h"
#include "utility/error.h"
#include <interpreter/interpreter.h>
#include <interpreter/memory.h>
#include <interpreter/device.h>
#include <interpreter/register.h>
#include <interpreter/exception.h>
#include <interpreter/executable.h>
#include <simulation/icache.h>
#include <linker/layout.h>
#include <config/config.h>
#include <libc/libc.h>
#include <cstddef>

namespace dark {

static void simulate_normal(RegisterFile &, Memory &, Device &, std::size_t);
static void simulate_debug(RegisterFile &, Memory &, Device &, std::size_t, MemoryLayout &);

void Interpreter::simulate() {
    auto &layout = this->memory_layout.get <MemoryLayout &>();

    auto device_ptr = Device::create(config);
    auto memory_ptr = Memory::create(config, layout);

    auto &device = *device_ptr;
    auto &memory = *memory_ptr;

    auto regfile = RegisterFile { layout.position_table.at("main"), config };

    libc::libc_init(regfile, memory, device);

    if (config.has_option("debug")) {
        // Avoid inlining those cold functions.
        [[unlikely]]
        simulate_debug(regfile, memory, device, config.get_timeout(), layout);
    } else {
        simulate_normal(regfile, memory, device, config.get_timeout());
    }

    console::profile << '\n';

    bool enable_detail = config.has_option("detail");
    regfile.print_details(enable_detail);
    memory.print_details(enable_detail);
    device.print_details(enable_detail);
}

static void simulate_normal
    (RegisterFile &rf, Memory &mem, Device &dev, std::size_t timeout)  {
    ICache icache { mem };
    try {
        Hint hint {};
        while (rf.advance() && timeout --> 0) {
            auto &exe = icache.ifetch(rf.get_pc(), hint);
            hint = exe(rf, mem, dev);
        }
        panic_if(timeout + 1 == 0, "Time Limit Exceeded");
    } catch (FailToInterpret &e) {
        panic("{}", e.what(rf, mem, dev));
    } catch (std::exception &e) {
        unreachable(std::format("std::exception caught: {}\n", e.what()));
    }
}

static void simulate_debug
    (RegisterFile &rf, Memory &mem, Device &dev, std::size_t timeout, MemoryLayout &layout) {
    ICache icache { mem };
    DebugManager manager { rf, mem, layout };

    try {
        Hint hint {};
        while (rf.advance() && timeout --> 0) {
            manager.test();
            auto &exe = icache.ifetch(rf.get_pc(), hint);
            hint = exe(rf, mem, dev);
        }
        panic_if(timeout + 1 == 0, "Time Limit Exceeded");
    } catch (FailToInterpret &e) {
        panic("{}", e.what(rf, mem, dev));
    } catch (std::exception &e) {
        unreachable(std::format("std::exception caught: {}\n", e.what()));
    }
}

} // namespace dark
