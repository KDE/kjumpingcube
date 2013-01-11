
// Settings widget.
#include "ui_settings.h"

class SettingsWidget : public QWidget, public Ui::Settings
{
public:
    SettingsWidget(QWidget *parent)
        : QWidget(parent)
        {
            setupUi(this);
        }
};
