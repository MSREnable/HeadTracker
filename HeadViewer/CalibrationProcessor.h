#pragma once

#include <dlib/opencv.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d_c.h>
#include <iostream>

#include "CalibrationEntry.h"

using namespace Windows::Foundation::Collections;

namespace HeadViewer
{
    public ref class CalibrationProcessor sealed
    {
    internal:
        CalibrationProcessor();

        task<void> ProcessCalibrationEntries();
        void Reset();
        Point ComputeHeadGazeCoordinates(SoftwareBitmapWrapper^ bitmap);

    private:
        int GetBestImageIndex(CalibrationEntry^ entry);
        Rect GetMainFaceRect(SoftwareBitmapWrapper^ bmpWrapper);
        SoftwareBitmapWrapper^ GetNormalizedFaceBitmap(CalibrationEntry^ entry);

    internal:
        bool                                IsCalibrationValid;
        Vector<CalibrationEntry^>^          CalibrationData;
        cv::Mat                             CalibrationMatrix;

    private:
        int m_faceWidth;
        int m_faceHeight;

        // dlib stuff
        dlib::shape_predictor m_shapePredictor;
        dlib::frontal_face_detector m_faceDetector;
    };
}