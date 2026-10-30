########################################################################
# Spectrogram module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET Spectrogram
    SOURCES
        Spectrogram.cpp
        SpectrogramWork.cpp
        SpectrogramDisplay.cpp
        ColorMapEntry.cpp
        GeneratedColorMaps.cpp
        QwtColorMapMaker.cpp
    DOC_SOURCES Spectrogram.cpp
    LIBRARIES
        Qt6::Widgets
        Qwt::Qwt
        PothosPlotterUtils
    DESTINATION plotters
)
plotters_target(Spectrogram MODULE)
