########################################################################
# Constellation module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET Constellation
    SOURCES
        Constellation.cpp
        ConstellationWork.cpp
        ConstellationDisplay.cpp
    DOC_SOURCES Constellation.cpp
    LIBRARIES
        Qt6::Widgets
        Qwt::Qwt
        PothosPlotterUtils
    DESTINATION plotters
)
plotters_target(Constellation MODULE)
