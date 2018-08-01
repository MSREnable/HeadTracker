//
// CalibrationPointViewer.xaml.cpp
// Implementation of the CalibrationPointViewer class
//

#include "pch.h"
#include "CalibrationPointViewer.xaml.h"

using namespace HeadViewer;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

CalibrationPointViewer::CalibrationPointViewer()
{
	InitializeComponent();
    m_curBitmap = 0;
    m_bmpSource = ref new SoftwareBitmapSource();
    CalibrationImage->Source = m_bmpSource;

    m_timer = ref new DispatcherTimer();
    m_timer->Tick += ref new EventHandler<Platform::Object ^>(this, &CalibrationPointViewer::OnTimerTick);
}

void CalibrationPointViewer::OnTimerTick(Object^ sender, Object^ args)
{
    OnNextClicked(sender, nullptr);
}

void CalibrationPointViewer::OnPreviousClicked(Object^ sender, RoutedEventArgs^ e)
{
    int size = m_bitmaps->Size;
    m_curBitmap = (m_curBitmap > 0) ? m_curBitmap - 1 : size - 1;
    m_bmpSource->SetBitmapAsync(m_bitmaps->GetAt(m_curBitmap));
}

void CalibrationPointViewer::OnPlayClicked(Object^ sender, RoutedEventArgs^ e)
{
    TimeSpan ts;
    ts.Duration = FrameRate->Denominator / FrameRate->Numerator * 1000 * 1000 * 10;
    m_timer->Interval = ts;
    m_timer->Start();

}

void CalibrationPointViewer::OnStopClicked(Object^ sender, RoutedEventArgs^ e)
{
    m_timer->Stop();
}

void CalibrationPointViewer::OnNextClicked(Object^ sender, RoutedEventArgs^ e)
{
    int size = m_bitmaps->Size;
    m_curBitmap = (m_curBitmap + 1) % size;
    m_bmpSource->SetBitmapAsync(m_bitmaps->GetAt(m_curBitmap));
}
