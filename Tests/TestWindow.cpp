// Copyright (c) 2026 Pothos contributors
// SPDX-License-Identifier: BSL-1.0

//Compare the window code with the reference data from spuce ad08df7.
//The tolerance is relative to the largest finite value of the reference
//array (1 for an array of zeros): 1e-9, as in the tests of PothosComms
//(the same formulas give the same values, except for the order of
//operations).

#include "Window.hpp"
#include "WindowReferenceData.hpp"
#include <Pothos/Testing.hpp>
#include <algorithm>
#include <cmath>
#include <format>
#include <span>
#include <string>

namespace {

bool sameValue(const double out, const double ref, const double tol)
{
    if (std::isnan(ref)) return std::isnan(out);
    if (std::isinf(ref)) return out == ref;
    return std::abs(out - ref) <= tol;
}

double largest(std::span<const double> ref)
{
    double m = 0.0;
    for (const auto r : ref) if (std::isfinite(r)) m = std::max(m, std::abs(r));
    return (m == 0.0) ? 1.0 : m;
}

//an empty text if out agrees with ref, else a description of the first difference
std::string compare(std::span<const double> out, std::span<const double> ref, const double relTol)
{
    if (out.size() != ref.size()) return std::format("size {} != {}", out.size(), ref.size());
    const auto tol = relTol*largest(ref);
    for (std::size_t i = 0; i < ref.size(); i++)
    {
        if (not sameValue(out[i], ref[i], tol)) return std::format("[{}] {} != {}", i, out[i], ref[i]);
    }
    return "";
}

} //namespace

POTHOS_TEST_BLOCK("/plotters/tests", test_window_reference)
{
    for (const auto &c : PothosPlotters::Reference::windowCases)
    {
        const auto label = std::format("{} {} taps arg {}: ", c.type, c.numTaps, c.arg);
        const auto taps = PothosPlotters::designWindow(c.type, c.numTaps, c.arg);
        POTHOS_TEST_EQUAL(label + compare(taps, c.taps, 1e-9), label);
    }
}

//the blocks call the window with arg 0.0 when windowArgs is empty (the
//default []): each window type of the block docs must give finite taps
POTHOS_TEST_BLOCK("/plotters/tests", test_window_default_args)
{
    for (const std::string type : {"rectangular", "hann", "hamming", "blackman",
        "bartlett", "flattop", "kaiser", "chebyshev"})
    {
        const auto taps = PothosPlotters::designWindow(type, 1024, 0.0);
        POTHOS_TEST_EQUAL(taps.size(), 1024u);
        const auto bad = std::find_if(taps.begin(), taps.end(), [](const double t){return not std::isfinite(t);});
        POTHOS_TEST_EQUAL(type + (bad == taps.end() ? "" : std::format(" [{}] {}", bad - taps.begin(), *bad)), type);
    }
}
