########################################################################
# QwtWidgets module
########################################################################
POTHOS_MODULE_UTIL(
    TARGET QwtWidgets
    SOURCES
        QwtSlider.cpp
        QwtThermo.cpp
        QwtKnob.cpp
        QwtDial.cpp
    ENABLE_DOCS
    LIBRARIES
        Qt6::Widgets
        Qwt::Qwt
    DESTINATION plotters
)
plotters_target(QwtWidgets MODULE)
