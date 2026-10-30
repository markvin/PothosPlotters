########################################################################
# WaveMonitor module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET WaveMonitor
    SOURCES
        WaveMonitor.cpp
        WaveMonitorWork.cpp
        WaveMonitorDisplay.cpp
    DOC_SOURCES WaveMonitor.cpp
    LIBRARIES
        Qt6::Widgets
        Qwt::Qwt
        PothosPlotterUtils
    DESTINATION plotters
)
plotters_target(WaveMonitor MODULE)
