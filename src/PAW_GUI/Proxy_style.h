#include <QProxyStyle>

class JumpSliderStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
        const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override {
        if (hint == SH_Slider_AbsoluteSetButtons)
            return Qt::LeftButton;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};