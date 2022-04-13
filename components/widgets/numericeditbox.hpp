#ifndef OPENMW_NUMERIC_EDIT_BOX_H
#define OPENMW_NUMERIC_EDIT_BOX_H

#include <stdexcept>
#include <iomanip>

#include <MyGUI_EditBox.h>

#include "fontwrapper.hpp"

namespace Gui
{

    /**
     * @brief A variant of the EditBox that only allows number inputs
     */
    template <class T>
    class EditBoxBase : public FontWrapper<MyGUI::EditBox>
    {
        MYGUI_RTTI_DERIVED(EditBoxBase)

    public:
        EditBoxBase()
            : mValue(0)
            , mMinValue(std::numeric_limits<T>::lowest())
            , mMaxValue(std::numeric_limits<T>::max())
            , mPrecision(4)
        { }

        void initialiseOverride() override
        {
            Base::initialiseOverride();
            eventEditTextChange += MyGUI::newDelegate(this, &EditBoxBase::onEditTextChange);

            mValue = 0;
            setCaption("0");
        }

        void shutdownOverride() override
        {
            Base::shutdownOverride();
            eventEditTextChange -= MyGUI::newDelegate(this, &EditBoxBase::onEditTextChange);
        }

        typedef MyGUI::delegates::CMultiDelegate1<T> EventHandle_ValueChanged;
        EventHandle_ValueChanged eventValueChanged;

        void setValue (T value, bool notifyUpdate = false)
        {
            if (value != mValue)
            {
                mValue = value;
                setCaptionFromValue();

                if (notifyUpdate)
                    eventValueChanged(mValue);
            }
        }

        T getValue(bool update = false){ return mValue; }

        void setMinValue(T minValue) { mMinValue = minValue; }
        void setMaxValue(T maxValue) { mMaxValue = maxValue; }

        void setPrecision(int precision) { mPrecision = precision; }

    private:
        void onEditTextChange(MyGUI::EditBox* sender);
        void onKeyLostFocus(MyGUI::Widget* _new) override;
        void onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char character) override;

        void setCaptionFromValue();

        T mValue;

        T mMinValue;
        T mMaxValue;

        int mPrecision;
    };

    template <class T>
    void EditBoxBase<T>::setCaptionFromValue()
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(mPrecision) << mValue;
        setCaption(ss.str());
    }

    template <class T>
    void EditBoxBase<T>::onEditTextChange(MyGUI::EditBox *sender)
    {
        std::string newCaption = sender->getCaption();
        if (newCaption.empty())
        {
            return;
        }

        try
        {
            if constexpr (std::is_integral_v<T>)
                mValue = std::stoi(newCaption);
            else
                mValue = std::stof(newCaption);
            T capped = std::clamp(mValue, mMinValue, mMaxValue);
            if (capped != mValue)
            {
                mValue = capped;
                setCaptionFromValue();
            }
        }
        catch (const std::invalid_argument&)
        {
            setCaptionFromValue();
        }
        catch (const std::out_of_range&)
        {
            setCaptionFromValue();
        }

        eventValueChanged(mValue);
    }

    template <class T>
    void EditBoxBase<T>::onKeyLostFocus(MyGUI::Widget* _new)
    {
        Base::onKeyLostFocus(_new);
        setCaptionFromValue();
    }

    template <class T>
    void EditBoxBase<T>::onKeyButtonPressed(MyGUI::KeyCode key, MyGUI::Char character)
    {
        if (key == MyGUI::KeyCode::ArrowUp)
        {
            setValue(std::min(mValue+1, mMaxValue));
            eventValueChanged(mValue);
        }
        else if (key == MyGUI::KeyCode::ArrowDown)
        {
            setValue(std::max(mValue-1, mMinValue));
            eventValueChanged(mValue);
        }
        else if (character == 0 || (character >= '0' && character <= '9'))
            Base::onKeyButtonPressed(key, character);

        if constexpr (!std::is_integral_v<T>)
        {
            if (character == '.')
            {
                auto accept = [](const std::string& search, size_t start, size_t end)
                {
                    for (size_t i = 0; i < search.size(); ++i)
                    {
                        if (search[i] != '.')
                            continue;

                        if (i >= start && i <= end)
                            return true;

                        return false;
                    }
                    return true;
                };

                if (accept(getCaption(), getTextSelectionStart(), getTextSelectionEnd()))
                    Base::onKeyButtonPressed(key, character);
            }
        }
    }
    class NumericEditBox final : public EditBoxBase<int>
    {
        MYGUI_RTTI_DERIVED(NumericEditBox)
    };
    class FloatEditBox final : public EditBoxBase<float>
    {
        MYGUI_RTTI_DERIVED(FloatEditBox)
    };
}

#endif
