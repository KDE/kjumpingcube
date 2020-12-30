/*
    SPDX-FileCopyrightText: 2013 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

// Settings widget.
#include "ui_settings.h"

class SettingsWidget : public QWidget, public Ui::Settings
{
public:
    explicit SettingsWidget(QWidget *parent)
        : QWidget(parent)
        {
            setupUi(this);
        }
};

#endif

