/*
    src/messagedialog.cpp -- Simple "OK" or "Yes/No"-style modal dialogs

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/messagedialog.h>
#include <nanogui/layout.h>
#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/label.h>

NAMESPACE_BEGIN(nanogui)

MessageDialog::MessageDialog(Widget *parent, Type type, const std::string &title,
              const std::string &message,
              const std::string &buttonText,
              const std::string &altButtonText, bool altButton) : Window(parent, title) {
    setLayout(new BoxLayout(Orientation::Vertical,
                            Alignment::Fill, 10, 0));
    setModal(true);

    Widget *panel1 = new Widget(this);
    panel1->setLayout(new BoxLayout(Orientation::Horizontal,
                                    Alignment::Middle, 10, 10));
    int icon = 0;
    switch (type) {
        case Type::Information: icon = ENTYPO_ICON_CIRCLED_INFO; break;
        case Type::Question: icon = ENTYPO_ICON_CIRCLED_HELP; break;
        case Type::Warning: icon = ENTYPO_ICON_WARNING; break;
    }
    Label *iconLabel = new Label(panel1, std::string(utf8(icon).data()), "icons");
    iconLabel->setFontSize(50);
    mMessageLabel = new Label(panel1, message);
	mMessageLabel->setTextAlignment(Label::TextAlign::LEFT || Label::TextAlign::VERTICAL_CENTER);
    Widget *panel2 = new Widget(this);
	RelativeGridLayout* relLayout = new RelativeGridLayout();
	relLayout->appendRow(1.f);
	relLayout->appendCol(1.f);
    panel2->setLayout(relLayout);

    if (altButton) {
		relLayout->appendCol(1.f);
        Button *button = new Button(panel2, altButtonText, ENTYPO_ICON_CIRCLED_CROSS);
		relLayout->setAnchor(button, RelativeGridLayout::makeAnchor(1, 0, 1, 1, Alignment::Middle, Alignment::Middle));
        button->setCallback([&] { if (mCallback) mCallback(1); dispose(); });
    }
    Button *button = new Button(panel2, buttonText, ENTYPO_ICON_CHECK);
	relLayout->setAnchor(button, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Middle, Alignment::Middle));
    button->setCallback([&] { if (mCallback) mCallback(0); dispose(); });

    center();
    requestFocus();
}

NAMESPACE_END(nanogui)
