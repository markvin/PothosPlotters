########################################################################
# LogicAnalyzer module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET LogicAnalyzer
    SOURCES
        LogicAnalyzer.cpp
        LogicAnalyzerDisplay.cpp
    DOC_SOURCES LogicAnalyzer.cpp
    LIBRARIES
        Qt6::Widgets
        PothosPlotterUtils
    DESTINATION plotters
)
plotters_target(LogicAnalyzer MODULE)
