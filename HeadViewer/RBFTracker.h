#pragma once

#include <MemoryBuffer.h>

#include <dlib/image_io.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/opencv.h>
#include <opencv2/opencv.hpp>

namespace HeadViewer
{
    private ref class RBFTracker sealed
    {
    public:
        RBFTracker();

    public:
        void StartCalibration() {   _inCalibrationMode = true; }
        void StopCalibration() { _inCalibrationMode = false; }

        void ProcessFrame(Windows::Graphics::Imaging::SoftwareBitmap^ frameBitmap);

    private:
        Microsoft::WRL::ComPtr<Windows::Foundation::IMemoryBufferByteAccess>
            GetBitmapBuffer(Windows::Graphics::Imaging::SoftwareBitmap^ frameBitmap,
            BYTE **bmpData,
            UINT32 *capacity);

    private:
        bool _inCalibrationMode;
    
        dlib::shape_predictor m_shapePredictor;
        dlib::frontal_face_detector m_faceDetector;
    };
}