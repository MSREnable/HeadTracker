//
// CalibrationPointViewer.xaml.h
// Declaration of the CalibrationPointViewer class
//

#pragma once

#include "CalibrationPointViewer.g.h"
#include "CalibrationEntry.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::Media::MediaProperties;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace HeadViewer
{
	[Windows::Foundation::Metadata::WebHostHidden]
    public ref class CalibrationPointViewer sealed
    {
    public:
        CalibrationPointViewer();

    internal:
        property CalibrationEntry^ CalibEntry
        {
            CalibrationEntry^ get()
            {
                return m_calibrationEntry;
            }
            void set(CalibrationEntry^ value)
            {
                m_calibrationEntry = value;
                m_bitmaps = value->Bitmaps;
                m_curBitmap = 0;
                m_bmpSource->SetBitmapAsync(m_bitmaps->GetAt(m_curBitmap)->Bitmap);
                
                TotalFrames->Text = m_bitmaps->Size.ToString();
                CurFrame->Text = m_curBitmap.ToString();
            }
        }

        property MediaRatio^      FrameRate;
        
    internal:
        void ShowBestImage();
        void ShowFace();
        void ShowNormalizedFace();

    private:   
        void OnTimerTick(Object^ sender, Object^ args);
        void OnPreviousClicked(Object^ sender, RoutedEventArgs^ e);
        void OnPlayClicked(Object^ sender, RoutedEventArgs^ e);
        void OnStopClicked(Object^ sender, RoutedEventArgs^ e);
        void OnNextClicked(Object^ sender, RoutedEventArgs^ e);

    private:
        CalibrationEntry^ m_calibrationEntry;
        IVectorView<SoftwareBitmapWrapper^>^    m_bitmaps;
        int                                     m_curBitmap;
        SoftwareBitmapSource^                   m_bmpSource;
        DispatcherTimer^                        m_timer;
    };
}
