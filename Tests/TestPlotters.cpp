// Copyright (c) 2026 Pothos contributors
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Framework.hpp>
#include <Pothos/Proxy.hpp>
#include <Pothos/Testing.hpp>
#include <QApplication>
#include <QCoreApplication>
#include <QImage>
#include <QString>
#include <QTableWidget>
#include <QWidget>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <numbers>
#include <string>
#include <thread>
#include <vector>

/***********************************************************************
 * Smoke tests of the plotter blocks: make each block, call its setters,
 * and read the values back. Data tests: send samples through each plotter
 * and look at the plot. The tests need no display: when no application
 * is present, the test makes one with the offscreen platform.
 **********************************************************************/

//The QApplication of a test: use the application of the host (for example
//PothosFlow) when it has one, else make one for the time of the test.
//Delete it at the end of the test, not when the core unloads the module:
//~QCoreApplication waits for the threads of the Qt thread pool (the
//spectrogram renders with QtConcurrent). In the unload of a DLL (Windows
//loader lock) that wait hangs, and after the unload of Qwt and of the
//plotter modules (macOS) it crashes. Make it first in a test, so that the
//blocks and their widgets go before it.
class TestApplication
{
public:
    TestApplication(void)
    {
        if (QApplication::instance() != nullptr) return;
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char arg0[] = "PothosPlottersTest";
        static char *argv[] = {arg0, nullptr};
        _app = std::make_unique<QApplication>(argc, argv);
    }

private:
    std::unique_ptr<QApplication> _app;
};

//run the queued calls of the setters; a queued slot can queue a
//new call, thus do more rounds
static void processEvents(void)
{
    for (int i = 0; i < 10; i++)
    {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::processEvents();
    }
}

//record the Qt warnings of failed calls while a test runs: a queued call
//to a missing slot, or with an argument type that Qt does not know, gives
//only a warning. Other Qt warnings tell about the platform (for example a
//missing font), not about the plotters: they do not count.
class QtWarningCheck
{
public:
    QtWarningCheck(void)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _warnings.clear();
        _previous = qInstallMessageHandler(&QtWarningCheck::handler);
    }

    ~QtWarningCheck(void)
    {
        qInstallMessageHandler(_previous);
    }

    //call after processEvents(): fail when Qt gave a warning
    void check(void)
    {
        std::vector<std::string> warnings;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            warnings.swap(_warnings);
        }
        for (const auto &warning : warnings) std::cerr << "Qt warning: " << warning << std::endl;
        POTHOS_TEST_TRUE(warnings.empty());
    }

private:
    static bool isCallFailure(const QString &msg)
    {
        return msg.startsWith("QMetaObject::invokeMethod") or
            msg.startsWith("QMetaMethod::invoke") or
            msg.contains("Cannot queue arguments");
    }

    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if ((type == QtWarningMsg or type == QtCriticalMsg) and isCallFailure(msg))
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _warnings.push_back(msg.toStdString());
        }
        if (_previous != nullptr) _previous(type, context, msg);
    }

    static inline std::mutex _mutex;
    static inline std::vector<std::string> _warnings;
    static inline QtMessageHandler _previous = nullptr;
};

//a block with a widget; a plotter factory takes the proxy environment
//of the topology (|factory ...(remoteEnv) in the block docs), the Qwt
//control widgets take no argument
static Pothos::Proxy makeWidgetBlock(const std::string &path)
{
    POTHOS_TEST_TRUE(QApplication::instance() != nullptr);
    auto block = Pothos::BlockRegistry::make(path);
    POTHOS_TEST_TRUE(block.call<QWidget *>("widget") != nullptr);
    return block;
}

static Pothos::Proxy makePlotter(const std::string &path)
{
    POTHOS_TEST_TRUE(QApplication::instance() != nullptr);
    auto block = Pothos::BlockRegistry::make(path, Pothos::ProxyEnvironment::make("managed"));
    POTHOS_TEST_TRUE(block.call<QWidget *>("widget") != nullptr);
    return block;
}

/***********************************************************************
 * Smoke tests
 **********************************************************************/
POTHOS_TEST_BLOCK("/plotters/tests", test_constellation)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto block = makePlotter("/plotters/constellation");
    block.call("setTitle", QString("Constellation"));
    block.call("setAutoScale", false);
    block.call("setXRange", std::vector<double>{-2.0, 2.0});
    block.call("setYRange", std::vector<double>{-2.0, 2.0});
    block.call("enableXAxis", true);
    block.call("enableYAxis", true);
    block.call("setCurveStyle", std::string("DOTS"));
    block.call("setCurveColor", QString("red"));
    block.call("setDisplayRate", 10.0);
    block.call("setNumPoints", size_t(512));
    processEvents();
    POTHOS_TEST_TRUE(block.call<QString>("title") == "Constellation");
    POTHOS_TEST_TRUE(not block.call<bool>("autoScale"));
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/plotters/tests", test_logic_analyzer)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto block = makePlotter("/plotters/logic_analyzer");
    block.call("setNumInputs", size_t(2));
    block.call("setSampleRate", 1e6);
    block.call("setChannelLabel", size_t(1), QString("Data"));
    block.call("setChannelBase", size_t(1), size_t(16));
    block.call("setXAxisMode", std::string("TIME"));
    block.call("setDisplayRate", 10.0);
    block.call("setNumPoints", size_t(16));
    processEvents();
    POTHOS_TEST_EQUAL(block.call<size_t>("numInputs"), 2u);
    POTHOS_TEST_EQUAL(block.call<double>("sampleRate"), 1e6);
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/plotters/tests", test_periodogram)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto block = makePlotter("/plotters/periodogram");
    block.call("setTitle", QString("Periodogram"));
    block.call("setSampleRate", 2e6);
    block.call("setCenterFrequency", 100e6);
    block.call("setNumFFTBins", size_t(512));
    block.call("setWindowType", std::string("kaiser"), std::vector<double>{5.0});
    block.call("setFullScale", 1.0);
    block.call("setFFTMode", std::string("COMPLEX"));
    block.call("setReferenceLevel", -10.0);
    block.call("setDynamicRange", 80.0);
    block.call("setAutoScale", false);
    block.call("setAverageFactor", 0.5);
    block.call("setYAxisTitle", QString("dB"));
    block.call("setDisplayRate", 10.0);
    processEvents();
    POTHOS_TEST_TRUE(block.call<QString>("title") == "Periodogram");
    POTHOS_TEST_EQUAL(block.call<double>("sampleRate"), 2e6);
    POTHOS_TEST_EQUAL(block.call<double>("centerFrequency"), 100e6);
    POTHOS_TEST_EQUAL(block.call<size_t>("numFFTBins"), 512u);
    POTHOS_TEST_EQUAL(block.call<double>("referenceLevel"), -10.0);
    POTHOS_TEST_EQUAL(block.call<double>("dynamicRange"), 80.0);
    POTHOS_TEST_TRUE(not block.call<bool>("autoScale"));
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/plotters/tests", test_spectrogram)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto block = makePlotter("/plotters/spectrogram");
    block.call("setTitle", QString("Spectrogram"));
    block.call("setDisplayRate", 20.0);
    block.call("setSampleRate", 2e6);
    block.call("setCenterFrequency", 100e6);
    block.call("setNumFFTBins", size_t(512));
    block.call("setWindowType", std::string("chebyshev"), std::vector<double>{60.0});
    block.call("setTimeSpan", 5.0);
    block.call("setReferenceLevel", -10.0);
    block.call("setDynamicRange", 80.0);
    block.call("setColorMap", std::string("jet"));
    processEvents();
    POTHOS_TEST_TRUE(block.call<QString>("title") == "Spectrogram");
    POTHOS_TEST_EQUAL(block.call<double>("displayRate"), 20.0);
    POTHOS_TEST_EQUAL(block.call<double>("sampleRate"), 2e6);
    POTHOS_TEST_EQUAL(block.call<double>("centerFrequency"), 100e6);
    POTHOS_TEST_EQUAL(block.call<size_t>("numFFTBins"), 512u);
    POTHOS_TEST_EQUAL(block.call<double>("timeSpan"), 5.0);
    POTHOS_TEST_EQUAL(block.call<double>("referenceLevel"), -10.0);
    POTHOS_TEST_EQUAL(block.call<double>("dynamicRange"), 80.0);
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/plotters/tests", test_wave_monitor)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto block = makePlotter("/plotters/wave_monitor");
    block.call("setNumInputs", size_t(2));
    block.call("setTitle", QString("Wave Monitor"));
    block.call("setSampleRate", 1e6);
    block.call("setNumPoints", size_t(256));
    block.call("setAutoScale", false);
    block.call("setYRange", std::vector<double>{-1.5, 1.5});
    block.call("setYAxisTitle", QString("Amplitude"));
    block.call("setChannelLabel", size_t(0), QString("I/Q"));
    block.call("setChannelStyle", size_t(0), std::string("LINE"));
    block.call("setDisplayRate", 10.0);
    processEvents();
    //setNumInputs sets the input ports of the topology; the display has
    //one input (the trigger sends all channels in one packet)
    //(the port names of a topology have the slots too: count the numbers)
    const auto names = block.call<std::vector<std::string>>("inputPortNames");
    POTHOS_TEST_EQUAL(std::count_if(names.begin(), names.end(), [](const std::string &name)
        {return not name.empty() and std::all_of(name.begin(), name.end(), ::isdigit);}), 2);
    POTHOS_TEST_EQUAL(block.call<size_t>("numInputs"), 1u);
    POTHOS_TEST_TRUE(block.call<QString>("title") == "Wave Monitor");
    POTHOS_TEST_EQUAL(block.call<double>("sampleRate"), 1e6);
    POTHOS_TEST_EQUAL(block.call<size_t>("numPoints"), 256u);
    qtWarnings.check();
}

//the 4 Qwt control widgets have the same bounds and value calls
static void testQwtWidget(const std::string &path)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto block = makeWidgetBlock(path);
    block.call("setTitle", QString("Qwt"));
    block.call("setLowerBound", -10.0);
    block.call("setUpperBound", 10.0);
    block.call("setStepSize", 0.5);
    block.call("setValue", 2.5);
    processEvents();
    POTHOS_TEST_EQUAL(block.call<double>("value"), 2.5);
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/plotters/tests", test_qwt_slider)
{
    testQwtWidget("/widgets/qwt_slider");
}

POTHOS_TEST_BLOCK("/plotters/tests", test_qwt_thermo)
{
    testQwtWidget("/widgets/qwt_thermo");
}

POTHOS_TEST_BLOCK("/plotters/tests", test_qwt_knob)
{
    testQwtWidget("/widgets/qwt_knob");
}

POTHOS_TEST_BLOCK("/plotters/tests", test_qwt_dial)
{
    testQwtWidget("/widgets/qwt_dial");
}

//the plotters keep their old paths under /widgets/
POTHOS_TEST_BLOCK("/plotters/tests", test_old_paths)
{
    TestApplication app;
    for (const std::string name : {"constellation", "logic_analyzer",
        "periodogram", "spectrogram", "wave_monitor"})
    {
        std::cout << "/widgets/" << name << std::endl;
        makePlotter("/widgets/" + name);
    }
}

/***********************************************************************
 * Data tests: feeder -> plotter
 **********************************************************************/
//a tone of numElems samples of the type dtype (complex_float32 or int32)
static Pothos::BufferChunk makeTone(const std::string &dtype, const size_t numElems)
{
    Pothos::BufferChunk buff(Pothos::DType(dtype), numElems);
    if (dtype == "int32")
    {
        auto p = buff.as<std::int32_t *>();
        for (size_t i = 0; i < numElems; i++) p[i] = std::int32_t(i % 2);
        return buff;
    }
    auto p = buff.as<std::complex<float> *>();
    for (size_t i = 0; i < numElems; i++)
    {
        p[i] = std::polar(1.0f, float(2*std::numbers::pi*0.1*double(i)));
    }
    return buff;
}

//true when a curve of the plot in the widget has points
static bool curveHasData(QWidget *widget)
{
    for (auto plot : widget->findChildren<QwtPlot *>())
    {
        for (auto item : plot->itemList(QwtPlotItem::Rtti_PlotCurve))
        {
            if (static_cast<QwtPlotCurve *>(item)->dataSize() > 0) return true;
        }
    }
    return false;
}

//true when a cell of the table in the widget has a text
static bool tableHasData(QWidget *widget)
{
    for (auto table : widget->findChildren<QTableWidget *>())
    {
        for (int r = 0; r < table->rowCount(); r++)
        {
            for (int c = 0; c < table->columnCount(); c++)
            {
                const auto item = table->item(r, c);
                if (item != nullptr and not item->text().isEmpty()) return true;
            }
        }
    }
    return false;
}

//commit feeder -> plotter, then process the Qt events until hasData is
//true or 10 s pass. Before the commit, hasData must be false: else the
//check does not see the data of the test.
static void testData(const std::string &path, const std::string &dtype,
    const std::function<void(Pothos::Proxy &)> &setup,
    const std::function<bool(QWidget *)> &hasData)
{
    TestApplication app;
    QtWarningCheck qtWarnings;
    auto plotter = makePlotter(path);
    auto widget = plotter.call<QWidget *>("widget");
    widget->resize(400, 300);
    setup(plotter);
    processEvents();
    POTHOS_TEST_TRUE(not hasData(widget));

    //the trigger in the plotter sends a window of samples only at its
    //events (the display rate), and it drops the samples between the
    //events: feed a new buffer in each round until the plot has data
    auto feeder = Pothos::BlockRegistry::make("/blocks/feeder_source", dtype);
    const auto tone = makeTone(dtype, 4096);

    bool ok = false;
    {
        Pothos::Topology topology;
        topology.connect(feeder, 0, plotter, 0);
        topology.commit();
        const auto exitTime = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (not ok and std::chrono::steady_clock::now() < exitTime)
        {
            feeder.call("feedBuffer", tone);
            processEvents();
            ok = hasData(widget);
            if (not ok) std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        topology.disconnectAll();
        topology.commit();
    }
    processEvents();
    POTHOS_TEST_TRUE(ok);
    qtWarnings.check();
}

POTHOS_TEST_BLOCK("/plotters/tests", test_data_constellation)
{
    testData("/plotters/constellation", "complex_float32", [](Pothos::Proxy &p)
    {
        p.call("setNumPoints", size_t(1024));
        p.call("setDisplayRate", 100.0);
    }, &curveHasData);
}

POTHOS_TEST_BLOCK("/plotters/tests", test_data_wave_monitor)
{
    testData("/plotters/wave_monitor", "complex_float32", [](Pothos::Proxy &p)
    {
        p.call("setNumInputs", size_t(1)); //the initializer of the block
        p.call("setNumPoints", size_t(1024));
        p.call("setDisplayRate", 100.0);
    }, &curveHasData);
}

POTHOS_TEST_BLOCK("/plotters/tests", test_data_periodogram)
{
    testData("/plotters/periodogram", "complex_float32", [](Pothos::Proxy &p)
    {
        p.call("setNumInputs", size_t(1)); //the initializer of the block
        p.call("setNumFFTBins", size_t(1024));
        p.call("setDisplayRate", 100.0);
    }, &curveHasData);
}

POTHOS_TEST_BLOCK("/plotters/tests", test_data_logic_analyzer)
{
    testData("/plotters/logic_analyzer", "int32", [](Pothos::Proxy &p)
    {
        p.call("setNumInputs", size_t(1)); //the initializer of the block
        p.call("setNumPoints", size_t(16));
        p.call("setDisplayRate", 100.0);
    }, &tableHasData);
}

//the spectrogram has no curve: compare the picture of the plot with the
//picture before the data
POTHOS_TEST_BLOCK("/plotters/tests", test_data_spectrogram)
{
    QImage before;
    testData("/plotters/spectrogram", "complex_float32", [](Pothos::Proxy &p)
    {
        p.call("setNumFFTBins", size_t(1024));
        p.call("setDisplayRate", 100.0);
        p.call("setTimeSpan", 1.0);
        p.call("setColorMap", std::string("rainbow")); //the default of the block
    }, [&before](QWidget *widget)
    {
        const auto image = widget->grab().toImage();
        if (before.isNull())
        {
            before = image;
            return false;
        }
        return image != before;
    });
}
