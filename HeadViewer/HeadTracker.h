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
        void DrawRectangle(BYTE *buffer, int width, int height, int left, int top, int right, int bottom, COLORREF color);
        void DrawPlus(BYTE *buffer, int width, int height, int x, int y, int size, COLORREF color);
        void DrawHorizontalLine(BYTE *buffer, int width, int height, int x1, int x2, int y, COLORREF color);
        void DrawVerticalLine(BYTE *buffer, int width, int height, int y1, int y2, int x, COLORREF color);


    private:
        // dlib stuff
        dlib::shape_predictor m_shapePredictor;
        dlib::frontal_face_detector m_faceDetector;
    };
}