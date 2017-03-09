// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef msr_airlib_AirSimRosFlightBoard_hpp
#define msr_airlib_AirSimRosFlightBoard_hpp

#include <exception>
#include "firmware/board.hpp"
#include "common/Common.hpp"
#include "vehicles/MultiRotorParams.hpp"
#include "sensors/SensorCollection.hpp"

//sensors
#include "sensors/barometer/BarometerSimple.hpp"
#include "sensors/imu/ImuSimple.hpp"
#include "sensors/gps/GpsSimple.hpp"
#include "sensors/magnetometer/MagnetometerSimple.hpp"

namespace msr { namespace airlib {

class AirSimRosFlightBoard : public rosflight::Board {
public: //type
    typedef MultiRotorParams::EnabledSensors EnabledSensors;

public:
    AirSimRosFlightBoard(const EnabledSensors* enabled_sensors, const SensorCollection* sensors)
        : enabled_sensors_(enabled_sensors), sensors_(sensors)
    {
    }

    //interface for simulator --------------------------------------------------------------------------------
    real_T getMotorControlSignal(uint index)
    {
        //convert PWM to scalled 0 to 1 control signal
        return (motors_pwm_[index] - 1000) / 1000.0f;
    }

    void setInputChannel(uint channel, real_T val)
    {
        input_channels_[channel] = val;
    }

    //Board interface implementation --------------------------------------------------------------------------
    virtual void init() override 
    {
        imu_ = static_cast<const ImuBase*>(sensors_->getByType(SensorCollection::SensorType::Imu));
        baro_ = static_cast<const BarometerBase*>(sensors_->getByType(SensorCollection::SensorType::Barometer));
        mag_ = static_cast<const MagnetometerBase*>(sensors_->getByType(SensorCollection::SensorType::Magnetometer));
    }

    virtual uint64_t micros() override 
    {
        return static_cast<uint64_t>(Utils::getTimeSinceEpoch() * 1E6);
    }

    virtual uint32_t millis() override 
    {
        return static_cast<uint64_t>(Utils::getTimeSinceEpoch() * 1E3);
    }

    virtual void init_sensors(uint16_t& acc1G, float& gyro_scale, int boardVersion, const std::function<void(void)>& imu_updated_callback) override 
    {
        //ignored for now
    }

    virtual bool is_sensor_present(SensorType type) override 
    {
        switch (type) {
        case SensorType::Baro: return enabled_sensors_->barometer;
        case SensorType::Gps: return enabled_sensors_->gps;
        case SensorType::Imu: return enabled_sensors_->imu;
        case SensorType::Mag: return enabled_sensors_->magnetometer;
        default:
            return false;
        }
    }

    virtual void pwmInit(bool useCPPM, bool usePwmFilter, bool fastPWM, uint32_t motorPwmRate, uint16_t idlePulseUsec) override 
    {
        for (uint i = 0; i < OutputMotorCount; ++i)
            motors_pwm_[i] = 1000;
        for (uint i = 0; i < InputChannelCount; ++i)
            motors_pwm_[i] = 1000;
    }

    virtual uint16_t pwmRead(int16_t channel) override 
    {
        //convert range -1 to 1 input signal to 1000 to 2000 PWM
        return static_cast<uint16_t>(input_channels_[channel] * 500.0f + 1500.0f);
    }

    virtual void pwmWriteMotor(uint8_t index, uint16_t value) override 
    {
        if (index < OutputMotorCount)
            motors_pwm_[index] = value;
        else
            throw std::exception("cannot write motor output for index > motor count");
    }

    virtual void set_led(uint8_t index, bool is_on) override 
    {
        //ignored for now
    }

    virtual void toggle_led(uint8_t index) override 
    {
        //ignored for now
    }

    virtual void init_params() override 
    {
        //ignored for now
    }

    virtual bool read_params() override 
    {
        //ignored for now
    }

    virtual bool write_params(bool blink_led) override 
    {
        //ignored for now
    }

    virtual void init_imu(uint16_t& acc1G, float& gyroScale, int boardVersion) override 
    {
        //ignored for now
    }

    virtual void read_accel(int16_t accel_adc[3]) override 
    {
        const auto& output = imu_->getOutput();
        //convert from SI units in NED to ADC output
        accel_adc[0] = accel_to_adc(output.linear_acceleration.x());
        accel_adc[1] = -accel_to_adc(output.linear_acceleration.y());
        accel_adc[2] = -accel_to_adc(output.linear_acceleration.z());
    }

    virtual void read_gyro(int16_t gyro_adc[3]) override 
    {
        const auto& output = imu_->getOutput();
        //convert from SI units in NED to ADC output
        gyro_adc[0] = angular_vel_to_adc(output.linear_acceleration.x());
        gyro_adc[1] = -angular_vel_to_adc(output.linear_acceleration.y());
        gyro_adc[2] = -angular_vel_to_adc(output.linear_acceleration.z());
    }

    virtual void read_temperature(int16_t& temp) override
    {
        //TODO: add separate temperature sensor in sim?
        temp = temperature_to_adc(25.0f);
    }

    virtual void read_baro(float& altitude, float& pressure, float& temperature) override 
    {
        const auto& output = baro_->getOutput();
        altitude = output.altitude;
        pressure = output.pressure;
        //TODO: barometer should output temperature as well?
        temperature = 25.0f;
    }

    virtual void read_diff_pressure(float& differential_pressure, float& temp, float& velocity) override 
    {
        throw std::exception("Diff pressure sensor is not available");
    }

    virtual float read_sonar() override 
    {
        throw std::exception("Sonar sensor is not available");
    }

    virtual void read_mag(int16_t mag_adc[3]) override 
    {
        const auto& output = mag_->getOutput();
        //TODO: do we expect adc in Gauss? Do we need NED conversion?
        mag_adc[0] = static_cast<int16_t>(output.magnetic_field_body.x());
        mag_adc[1] = static_cast<int16_t>(output.magnetic_field_body.y());
        mag_adc[2] = static_cast<int16_t>(output.magnetic_field_body.z());
    }

    virtual void delay_micros(uint32_t us) override 
    {
        sleep(us * 1E3f);
    }

    virtual void delay_millis(uint32_t ms) override 
    {
        sleep(ms);
    }

    virtual void system_reset(bool toBootloader) override 
    {
        //no internal state to reset
    }

private:
    uint16_t accel_to_adc(float accel)
    {
        //for MPU6050
        return static_cast<uint16_t>(accel * kAccelAdcBits / (kAccelG * kAccelScale));
    }
    uint16_t angular_vel_to_adc(float angular_vel)
    {
        //for MPU6050
        return static_cast<uint16_t>(angular_vel / kGyroScale);
    }
    int16_t temperature_to_adc(float temperature)
    {
        //for MPU6050
        return static_cast<int16_t>((temperature - 36.53f) * 340.0f);
    }
    void sleep(float msec)
    {
        if (msec <= 0)
            return;

        //if duration is too small, use spin wait otherwise use spin wait
        if (msec >= 5) {
            static constexpr duration<double> MinSleepDuration(0);
            clock::time_point start = clock::now();
            double dt = msec * 1000;
            //spin wait
            while (duration<double>(clock::now() - start).count() < dt) {
                std::this_thread::sleep_for(MinSleepDuration);
            }
        }
        else {
            std::this_thread::sleep_for(duration<double>(msec * 1000));
        }
    }


private: //types and consts
    typedef std::chrono::high_resolution_clock clock;
    template <typename T>
    using duration = std::chrono::duration<T>;

    const MultiRotorParams::EnabledSensors* enabled_sensors_;
    const SensorCollection* sensors_;
    const ImuBase* imu_;
    const BarometerBase* baro_;
    const MagnetometerBase* mag_;

    const uint16_t kAccelAdcBits = 512 * 8; //for mpu6050 as per breezystm32/drv_mpu6050.c
    const float kAccelScale = 1.0; //as set in PARAM_ACCEL_SCALE in ROSFlight
    const float kAccelG = 9.80665f; //as set in ROSFlight sensors.c init_sensors() function

    // 16.4 dps/lsb scalefactor for all Invensense devices
    const float kGyroScale = (1.0f / 16.4f) * (M_PIf / 180.0f);

    static constexpr uint OutputMotorCount = 16;
    static constexpr uint InputChannelCount = 16;
    
private:
    //motor outputs
    uint16_t motors_pwm_[OutputMotorCount];
    real_T input_channels_[InputChannelCount];
};

}} //namespace
#endif
