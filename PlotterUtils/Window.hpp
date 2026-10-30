// Copyright (c) 2026 Pothos contributors
// SPDX-License-Identifier: BSL-1.0

//Window taps for the FFT plotters (internal header, not installed).

#pragma once

#include "PlotUtilsConfig.hpp"
#include <string_view>
#include <vector>

namespace PothosPlotters {

/*!
 * Window taps (spuce design_window).
 * \param type rectangular, hann, hanning, hamming, blackman, bartlett,
 * flattop, kaiser or chebyshev; an unknown type gives rectangular
 * \param numTaps the number of taps
 * \param arg the beta of kaiser, the sidelobe attenuation (dB) of chebyshev
 */
POTHOS_PLOTTER_UTILS_EXPORT std::vector<double> designWindow(std::string_view type, int numTaps, double arg);

} //namespace PothosPlotters
