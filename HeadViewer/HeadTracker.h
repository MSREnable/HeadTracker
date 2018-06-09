#pragma once

#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <iostream>


namespace HeadViewer
{
    private ref class HeadTrackerResult sealed
    {
    public:
        property Windows::Foundation::Rect FaceRect;
        property Windows::Foundation::Collections::IVectorView<Point>^ FacePoints;
    };

    private ref class HeadTracker sealed
    {
    public:
        HeadTracker();

    public:
        HeadTrackerResult^ ProcessBitmap(Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap);

    private:
        // dlib stuff
        dlib::shape_predictor m_shapePredictor;
    };
}