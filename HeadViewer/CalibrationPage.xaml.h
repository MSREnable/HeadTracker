﻿//
// CalibrationPage.xaml.h
// Declaration of the CalibrationPage class
//

#pragma once

#include "CalibrationPage.g.h"
#include "CalibrationEntry.h"
#include "FrameReader.h"

using namespace Platform::Collections;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::FaceAnalysis;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Media::Capture::Frames;

namespace HeadViewer
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class CalibrationPageParams sealed
    {
    public:
        property MediaFrameSourceGroup^ SourceGroup;
        property MediaFrameSourceInfo^ SourceInfo;
        property MediaFrameFormat^ FrameFormat;
    };

	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class CalibrationPage sealed
	{
	public:
		CalibrationPage();

        property IVectorView<CalibrationEntry^>^ CalibrationData
        {
            IVectorView<CalibrationEntry^>^ get() { return m_calibrationData->GetView(); }
        }

    protected:
        virtual void OnNavigatedTo(NavigationEventArgs^ e) override;
        virtual void OnNavigatedFrom(NavigationEventArgs^ e) override;

    private:
        void OnPageLoaded(Platform::Object^ sender, RoutedEventArgs^ e);
        void OnCalibrationTimerTick(Platform::Object^ sender, Platform::Object^ args);
        void OnFrameArrived(MediaFrameReader^ reader, MediaFrameArrivedEventArgs^ args);
        void OnCapturedImageClick(Object^ sender, RoutedEventArgs^ e);

    private:
        void ShowCalibrationResults();
        task<void> StartStreamingAsync(CalibrationPageParams^ params);
        task<void> StopStreamingAsync();
        task<void> ProcessCalibrationImages();

    private:
        CalibrationPageParams^              m_pageParams;
        int m_curRow;
        int m_curCol;
        bool m_startCollecting;
        FrameReader^                        m_frameReader;
        DispatcherTimer^                    m_calibrationTimer;
        Vector<CalibrationEntry^>^          m_calibrationData;

        FaceDetector^                       m_faceDetector;
    };
}
