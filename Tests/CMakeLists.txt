########################################################################
# PlotterTests module: the self-tests under /plotters/tests
########################################################################
POTHOS_MODULE_UTIL(
    TARGET PlotterTests
    SOURCES
        TestWindow.cpp
        TestPlotters.cpp
    LIBRARIES
        Qt6::Widgets
        Qwt::Qwt
        PothosPlotterUtils
    DESTINATION plotters
)
plotters_target(PlotterTests MODULE)

#the self-tests, run one by one by plotters.selftests
foreach(test
    test_window_reference
    test_window_default_args
    test_constellation
    test_logic_analyzer
    test_periodogram
    test_spectrogram
    test_wave_monitor
    test_qwt_slider
    test_qwt_thermo
    test_qwt_knob
    test_qwt_dial
    test_old_paths
    test_data_constellation
    test_data_wave_monitor
    test_data_periodogram
    test_data_logic_analyzer
    test_data_spectrogram
)
    set_property(GLOBAL APPEND PROPERTY POTHOS_PLOTTERS_TESTS /plotters/tests/${test})
endforeach()
