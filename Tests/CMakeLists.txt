########################################################################
# PlotterTests module: the self-tests under /plotters/tests
########################################################################
POTHOS_MODULE_UTIL(
    TARGET PlotterTests
    SOURCES
        TestWindow.cpp
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
)
    set_property(GLOBAL APPEND PROPERTY POTHOS_PLOTTERS_TESTS /plotters/tests/${test})
endforeach()
