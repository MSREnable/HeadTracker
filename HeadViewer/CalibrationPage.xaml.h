//
// CalibrationPage.xaml.h
// Declaration of the CalibrationPage class
//

#pragma once

#include "CalibrationPage.g.h"

namespace HeadViewer
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class CalibrationPage sealed
	{
	public:
		CalibrationPage();

    private:
        void Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnCalibrationTimerTick(Platform::Object^ sender, Platform::Object^ args);

    private:
        Windows::UI::Xaml::DispatcherTimer^ m_calibrationTimer;
        int m_curRow;
        int m_curCol;
    };
}
