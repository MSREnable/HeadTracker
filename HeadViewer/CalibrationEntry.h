#pragma once

#include "SoftwareBitmapWrapper.h"

using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Imaging;

namespace HeadViewer
{
    private ref class CalibrationEntry sealed
    {
    public:
        property double X;
        property double Y;
        property IVectorView<SoftwareBitmapWrapper^>^ Bitmaps
        {
            IVectorView<SoftwareBitmapWrapper^>^ get()
            {
                return m_bitmaps->GetView();
            }
        }
        property int  BestImageIndex;
        property Rect MainFaceRect;
        property Rect NormalizedFaceRect
        {
            Rect get()
            {
                return m_normalizedFaceRect;
            }
            void set(Rect rc)
            {
                // TODO: Assert that normalized rect is greater than the face rect
                auto dx = (rc.Width - MainFaceRect.Width) / 2;
                auto dy = (rc.Height - MainFaceRect.Height) / 2;
                rc = Rect(MainFaceRect.Left - dx, MainFaceRect.Top - dy, rc.Width, rc.Height);
                m_normalizedFaceRect = rc;
            }
        }

        property SoftwareBitmap^ NormalizedFace;

        CalibrationEntry()
        {
            X = 0;
            Y = 0;
            m_bitmaps = ref new Vector<SoftwareBitmapWrapper^>();
        }

        void AppendBitmap(SoftwareBitmapWrapper^ bitmap) { m_bitmaps->Append(bitmap); }
        void SetBitmap(int index, SoftwareBitmapWrapper^ bitmap) { m_bitmaps->SetAt(index, bitmap); }

    internal:


    private:
        Vector<SoftwareBitmapWrapper^>^    m_bitmaps;
        Rect                               m_normalizedFaceRect;
    };
}