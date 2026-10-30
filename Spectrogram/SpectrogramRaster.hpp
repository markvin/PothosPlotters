// Copyright (c) 2014-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <Pothos/Config.hpp>
#include <qwt_raster_data.h>
#include <qwt_interval.h>
#include <QList>
#include <valarray>
#include <mutex>

class MySpectrogramRasterData : public QwtRasterData
{
public:
    MySpectrogramRasterData(void):
        _numCols(1),
        _isComplex(true)
    {
        this->setNumRows(1);
    }

    //! Set the range of an axis (Qwt 6.2 removed QwtRasterData::setInterval)
    void setInterval(const Qt::Axis axis, const QwtInterval &interval)
    {
        _intervals[axis] = interval;
    }

    //! The range of an axis
    QwtInterval interval(const Qt::Axis axis) const override
    {
        return _intervals[axis];
    }

    //! translate a plot coordinate into a raster value
    double value(double x, double y) const override
    {
        const auto time = size_t(_yScale*(y-_yOff));
        const auto bin = size_t(_xScale*(x-_xOff));
        return _data[time][bin];
    }

    //! append a new power spectrum bin array
    void appendBins(const std::valarray<float> &bins)
    {
        std::unique_lock<std::mutex> lock(_rasterMutex);
        _data.push_front(bins);
        _data.pop_back();
    }

    //! A raster operation has begun
    void initRaster(const QRectF &, const QSize &raster) override
    {
        _rasterMutex.lock();

        this->setNumRows(raster.height());

        _yOff = this->interval(Qt::YAxis).minValue();
        _yScale = (_data.size()-1)/this->interval(Qt::YAxis).width();
        if (_isComplex)
        {
            _xOff = this->interval(Qt::XAxis).minValue();
            _xScale = (_numCols-1)/this->interval(Qt::XAxis).width();
        }
        else
        {
            _xScale = (_numCols/2-1)/this->interval(Qt::XAxis).width();
            _xOff = this->interval(Qt::XAxis).minValue() - this->interval(Qt::XAxis).width();
        }

    }

    //! A raster operation has ended
    void discardRaster(void) override
    {
        _rasterMutex.unlock();
    }

    //! Change the number of bins per power spectrum
    void setNumColumns(const size_t numCols)
    {
        std::unique_lock<std::mutex> lock(_rasterMutex);
        if (numCols == _numCols) return;
        _numCols = numCols;
        for (auto &row : _data)
        {
            std::valarray<float> newRow(_numCols);
            for (size_t i = 0; i < newRow.size(); i++)
                newRow[i] = row[size_t((double(i)*(row.size()-1))/(newRow.size()-1))];
            row = newRow;
        }
    }

    //! Set the rendering mode for real valued signals
    void setFFTMode(const bool isComplex)
    {
        std::unique_lock<std::mutex> lock(_rasterMutex);
        _isComplex = isComplex;
    }

private:
    void setNumRows(const int num)
    {
        if (_data.isEmpty()) _data.push_front(std::valarray<float>(-1000, _numCols));
        while (_data.size() > num) _data.pop_back();
        while (_data.size() < num) _data.push_front(_data.front());
    }

    //the ranges of the x, y and z axes (Qt::XAxis, YAxis, ZAxis)
    QwtInterval _intervals[3];

    //raster scale+adjustment factors
    float _yOff, _yScale, _xOff, _xScale;

    //raw data for the entire raster
    QList<std::valarray<float>> _data;

    //thread-safe access mutex
    std::mutex _rasterMutex;

    size_t _numCols;
    bool _isComplex;
};
