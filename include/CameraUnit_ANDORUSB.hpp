/**
 * @file CameraUnit_ANDORUSB.hpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Interface for Andor USB CameraUnit
 * @version 0.1
 * @date 2022-01-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __CAMERAUNIT_ANDORUSB_HPP__
#define __CAMERAUNIT_ANDORUSB_HPP__

#include "CameraUnit.hpp"
#include "atmcd32d.h"
#include <mutex>

class CCameraUnit_ANDORUSB : public CCameraUnit
{
    bool m_initializationOK;
    mutable std::mutex cs_;
    bool cancelCapture_;
    std::string status_;
    int shutterDelayInMs_;
    bool hasshutter;

    int numtempsensors;

    float exposure_;
    bool exposure_updated_;

    bool requestShutterOpen_;
    bool shutter_updated_;

    int binningX_;
    int binningY_;

    int width_;
    int height_;
    int xmin_;
    int xmax_;
    int ymin_;
    int ymax_;

    int roiLeft;
    int roiRight;
    int roiTop;
    int roiBottom;

    bool roi_updated_;

    int CCDWidth_;
    int CCDHeight_;

    char cam_name[100];

    mutable volatile unsigned int lastError_;

public:
    CCameraUnit_ANDORUSB(int shutterDelayInMs = 50, unsigned int readOutSpeed = 3);
    ~CCameraUnit_ANDORUSB();

    CImageData CaptureImage(long int &retryCount);
    void CancelCapture();

    inline bool CameraReady() const { return m_initializationOK; }
    inline const char *CameraName() const { return cam_name; }
    void SetExposure(float exposureInSeconds);
    inline float GetExposure() const;
    void SetShutterIsOpen(bool open);
    void SetReadout(int ReadSpeed);
    void SetTemperature(double temperatureInCelcius);
    double GetTemperature() const;
    void SetBinningAndROI(int x, int y, int x_min = 0, int x_max = 0, int y_min = 0, int y_max = 0);
    inline int GetBinningX() const { return binningX_; }
    inline int GetBinningY() const { return binningY_; }
    const ROI *GetROI() const;
    inline std::string GetStatus() const { return status_; }
    inline int GetCCDWidth() const { return CCDWidth_; }
    inline int GetCCDHeight() const { return CCDHeight_; }

private:
    void SetStatus(std::string newVal)
    {
        std::lock_guard<std::mutex> lock(cs_);
        status_ = newVal;
    }
    bool StatusIsIdle();
    bool HasError(unsigned int error, unsigned int line) const;
    void SetShutter();
};

#endif // __CAMERAUNIT_ANDORUSB_HPP__
