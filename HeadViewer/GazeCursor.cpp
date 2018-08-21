#include "pch.h"
#include "GazeCursor.h"

using namespace HeadViewer;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Shapes;

GazeCursor::GazeCursor()
{
    _gazePopup = ref new Popup();
    _gazePopup->IsHitTestVisible = false;

    _gazeCursor = ref new Ellipse();
    _gazeCursor->Fill = ref new SolidColorBrush(Colors::IndianRed);
    _gazeCursor->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Top;
    _gazeCursor->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
    _gazeCursor->Width = 2 * CursorRadius;
    _gazeCursor->Height = 2 * CursorRadius;
    _gazeCursor->IsHitTestVisible = false;

    _gazePopup->Child = _gazeCursor;
}

void GazeCursor::CursorRadius::set(int value)
{
    _cursorRadius = value;
    _gazeCursor->Width = 2 * _cursorRadius;
    _gazeCursor->Height = 2 * _cursorRadius;
}

void GazeCursor::IsCursorVisible::set(bool value)
{
    _isCursorVisible = value;
    SetVisibility();
}

void GazeCursor::IsGazeEntered::set(bool value)
{
    _isGazeEntered = value;
    SetVisibility();
}

void GazeCursor::LoadSettings(ValueSet^ settings)
{
    if (settings->HasKey("GazeCursor.CursorRadius"))
    {
        CursorRadius = (int)(settings->Lookup("GazeCursor.CursorRadius"));
    }
    if (settings->HasKey("GazeCursor.CursorVisibility"))
    {
        IsCursorVisible = (bool)(settings->Lookup("GazeCursor.CursorVisibility"));
    }
}

void GazeCursor::SetVisibility()
{
    auto isOpen = _isCursorVisible && _isGazeEntered;
    if (_gazePopup->IsOpen != isOpen)
    {
        _gazePopup->IsOpen = isOpen;
    }
    else if (isOpen)
    {
        auto topmost = VisualTreeHelper::GetOpenPopups(Window::Current)->First()->Current;
        if (_gazePopup != topmost)
        {
            _gazePopup->IsOpen = false;
            _gazePopup->IsOpen = true;
        }
    }
}