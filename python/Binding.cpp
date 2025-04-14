#include "common/communication/Serial.hpp"
#include "common/logging/Logger.hpp"

#include "hal/imu/MPU6050.hpp"
#include "hal/imu/Imu.hpp"

#include <memory>
#include <chrono>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

namespace py = pybind11;

class PY_IMU
{
private :
    class ImuObserverImpl : public ::common::hal::MPU6050::DefaultObserver
    {
    private :
        std::function<void(const double direction)> _callback;

    public :
        ImuObserverImpl(std::function<void(const double direction)>&& callback) 
            : _callback(std::forward<std::function<void(const double direction)>>(callback))
        {
            
        }
            

    public :
        auto onDirectionUpdate(const double direction) -> void override
        {
            _INFO_("IMU Direction: %f", direction);
            _callback(direction);
        }
    };

private :
    std::shared_ptr<::common::Serial> _serial;
    std::shared_ptr<::common::hal::Imu> _imu;
    std::unordered_map<uint8_t, std::shared_ptr<ImuObserverImpl>> _observers;
    uint8_t _sequence =0;
    std::future<void> _future;

public :
    auto initialize(const char* const port, const uint32_t baudrate, const uint8_t mode) -> bool
    {
        _INFO_("initialize : port=%s baudrate=%d mode=%d", port, baudrate, mode);
        _serial = ::common::Serial::create();
        if(!_serial->open(port, baudrate, mode)) 
        { 
            _ERROR_("initialize : serial open failed");
            return false; 
        }
        _imu = ::common::hal::Imu::create(_serial);
        _future = _imu->run();
        _INFO_("initialize : done");
        return true;
    }

    auto finalize() -> void
    {
        _INFO_("finalize");
        _imu->stop();
        _future.wait();
        if(_serial) 
        { 
            _serial->close();
        }
        _INFO_("finalize : done");
    }

    auto subscribe(std::function<void(const double direction)>&& callback) -> uint8_t
    {
        auto observer = std::make_shared<ImuObserverImpl>(
            std::forward<std::function<void(const double)>>(callback)
        );
        _imu->subscribe_update_imuData(observer);
        ++_sequence;
        _observers[_sequence] = observer;
        _INFO_("subscribe : _sequence=%d", _sequence);
        return _sequence;
    }

    auto unsubscribe(uint8_t sequnece) -> void
    {
        auto itor = _observers.find(sequnece);
        if(itor != _observers.end()) 
        { 
            _imu->unsubscribe_update_imuData(itor->second);
            _observers.erase(itor); 
        }
        _INFO_("unsubscribe : _sequence=%d", _sequence);
    }
};

PYBIND11_MODULE(py_common_lib, m) 
{
    m.doc() = "common-lib for python";

    py::class_<PY_IMU>(m, "Imu")
        .def(py::init())
        .def("initialize", 
             &PY_IMU::initialize,
             "Initialize IMU with Serial port",
             py::arg("port"), py::arg("baudrate"), py::arg("mode"))
        .def("finalize", 
             &PY_IMU::finalize,
             "Close Serial port")
        .def("subscribe", 
             &PY_IMU::subscribe,
             "Subscribe IMU data",
             py::arg("callback"))
        .def("unsubscribe", 
             &PY_IMU::unsubscribe,
             "Ubsubscribe IMU data",
             py::arg("sequence"));
}