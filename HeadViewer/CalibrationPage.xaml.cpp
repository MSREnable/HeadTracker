//
// CalibrationPage.xaml.cpp
// Implementation of the CalibrationPage class
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "CalibrationPage.xaml.h"

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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

CalibrationPage::CalibrationPage()
{
	InitializeComponent();
    m_curRow = 0;
    m_curCol = 0;

    m_calibrationTimer = ref new DispatcherTimer();
    TimeSpan ts;
    ts.Duration = 2 * 1000 * 1000 * 10;
    m_calibrationTimer->Interval = ts;
    m_calibrationTimer->Tick += ref new EventHandler<Platform::Object ^>(this, &CalibrationPage::OnCalibrationTimerTick);
}


void CalibrationPage::Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    m_calibrationTimer->Start();
}

void CalibrationPage::OnCalibrationTimerTick(Object ^sender, Object^ args)
{
    if ((m_curCol == 0) && (m_curRow == 0))
    {
        CalibrationDot->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    CalibrationGrid->SetRow(CalibrationDot, m_curRow);
    CalibrationGrid->SetColumn(CalibrationDot, m_curCol);

    if (m_curRow >= 5)
    {
        CalibrationDot->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        m_calibrationTimer->Stop();
        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(MainPage::typeid));
        return;
    }

    m_curCol += 2;

    if (m_curCol >= 5)
    {
        m_curCol = 0;
        m_curRow += 2;
    }
}
