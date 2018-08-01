#pragma once

using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;

namespace HeadViewer
{
    public ref class CalibrationEntry sealed
    {
    public:
        property double X;
        property double Y;
        property IVectorView<SoftwareBitmap^>^ Bitmaps
        {
            IVectorView<SoftwareBitmap^>^ get()
            {
                return m_bitmaps->GetView();
            }
        }

        CalibrationEntry()
        {
            X = 0;
            Y = 0;
            m_bitmaps = ref new Vector<SoftwareBitmap^>();
        }

        void AppendBitmap(SoftwareBitmap^ bitmap) { m_bitmaps->Append(bitmap); }
        void SetBitmap(int index, SoftwareBitmap^ bitmap) { m_bitmaps->SetAt(index, bitmap); }


    private:
        Vector<SoftwareBitmap^>^    m_bitmaps;
    };


}