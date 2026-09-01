#include "resonant/Energy.hpp"

#include <cmath>
#include <iostream>
#include <limits>

int main() {
    resonant::EnergyMonitor monitor;
    monitor.prepare(4, 2.0F);
    monitor.observe(0.2F, 0.3F, 0.25F, 0.1F);
    monitor.observe(0.2F, 0.4F, -0.75F, 0.1F);
    monitor.observe(0.2F, 0.2F, 0.5F, 0.1F);
    monitor.observe(0.2F, 0.2F, 0.1F, 0.1F);

    const auto completed = monitor.diagnostics();
    if (std::abs(completed.peak - 0.75F) > 1.0e-6F ||
        completed.output_rms <= 0.0 || completed.excitation_rms <= 0.0) {
        std::cerr << "completed energy window did not retain metrics\n";
        return 1;
    }

    monitor.reset();
    monitor.observe(0.0F, 0.0F, 2.5F, 2.5F);
    if (!monitor.diagnostics().runaway_detected) {
        std::cerr << "large finite peak did not report runaway\n";
        return 1;
    }

    monitor.reset();
    monitor.observe(0.0F, 0.0F,
                    std::numeric_limits<resonant::Sample>::quiet_NaN(), 0.0F);
    if (!monitor.diagnostics().nan_detected ||
        monitor.diagnostics().state != resonant::EnergyState::NumericalRunaway) {
        std::cerr << "NaN did not report numerical runaway\n";
        return 1;
    }

    if (resonant::EnergyMonitor::contain(
            std::numeric_limits<resonant::Sample>::infinity()) != 0.0F) {
        std::cerr << "emergency containment did not zero infinity\n";
        return 1;
    }

    std::cout << "PASS: energy and stability diagnostics\n";
    return 0;
}
