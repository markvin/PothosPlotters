########################################################################
# Periodogram module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET Periodogram
    SOURCES
        Periodogram.cpp
        PeriodogramWork.cpp
        PeriodogramChannel.cpp
        PeriodogramDisplay.cpp
    DOC_SOURCES Periodogram.cpp
    LIBRARIES
        Qt6::Widgets
        Qwt::Qwt
        PothosPlotterUtils
    DESTINATION plotters
)
plotters_target(Periodogram MODULE)
