/*
    src/combobox.cpp -- simple combo box widget based on a popup button

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/combobox.h>
#include <nanogui/layout.h>
#include <nanogui/serializer/core.h>
#include <cassert>
#include <iostream>

NAMESPACE_BEGIN(nanogui)

ComboBox::ComboBox(Widget *parent) : PopupButton(parent), mSelectedIndex(0) {
}

ComboBox::ComboBox(Widget *parent, const std::vector<std::string> &items)
    : PopupButton(parent), mSelectedIndex(0) {
    setItems(items);
}

ComboBox::ComboBox(Widget *parent, const std::vector<std::string> &items, const std::vector<std::string> &itemsShort)
    : PopupButton(parent), mSelectedIndex(0) {
    setItems(items, itemsShort);
}

void ComboBox::setSelectedIndex(size_t idx) {
    if (mItemsShort.empty())
        return;
    const std::vector<Widget *> &children = popup()->childAt(0)->childAt(0)->children();
	for (size_t i = 0U; i < children.size(); i++) {
		((Button *)children[i])->setPushed(i == idx);

	}
	if (mSelectedIndex != (int)idx) {
		mSelectedIndex = (int)idx;
		mCallback((int)idx);
	}
    setCaption(mItemsShort[idx]);
}

void ComboBox::setItems(const std::vector<std::string> &items, const std::vector<std::string> &itemsShort) {
    assert(items.size() == itemsShort.size());
    mItems = items;
    mItemsShort = itemsShort;
    if (mSelectedIndex < 0 || mSelectedIndex >= (int) items.size())
        mSelectedIndex = 0;
    while (mPopup->childCount() > 0)
        mPopup->removeChild(0);
	panel = new VScrollPanel(mPopup);
	Widget* w = new Widget(panel);
	w->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 8, 5));
	for (size_t i = 0; i < items.size(); i++) {
		Button *button = new Button(w, mItems[i]);
		button->setFlags(Button::RadioButton);
		button->setFontSize(20.f);
		button->setFixedHeight(30);
		button->setChangeCallback([this, button, i] (bool change)
		{
			if (change) {
				setCaption(mItems[i]);
				setPushed(false);
				popup()->setVisible(false);
				setSelectedIndex(i);
			}
		});
	}
    setSelectedIndex(mSelectedIndex);
	setMoveWindow(true);
}

bool ComboBox::scrollEvent(const Vector2i &p, const Vector2f &rel) {
    if (rel.y() < 0) {
        setSelectedIndex(std::min(mSelectedIndex+1, (int)(items().size()-1)));
        if (mCallback)
            mCallback(mSelectedIndex);
        return true;
    } else if (rel.y() > 0) {
        setSelectedIndex(std::max(mSelectedIndex-1, 0));
        if (mCallback)
            mCallback(mSelectedIndex);
        return true;
    }
    return Widget::scrollEvent(p, rel);
}

bool ComboBox::focusEvent(bool focused)
{
	Widget::focusEvent(focused);
	if (!focused && mPopup) {
		setPushed(false);
		mPopup->setVisible(false);
		return true;
	}
	return false;
}

void ComboBox::save(Serializer &s) const {
    Widget::save(s);
    s.set("items", mItems);
    s.set("itemsShort", mItemsShort);
    s.set("selectedIndex", mSelectedIndex);
}

bool ComboBox::load(Serializer &s) {
    if (!Widget::load(s)) return false;
    if (!s.get("items", mItems)) return false;
    if (!s.get("itemsShort", mItemsShort)) return false;
    if (!s.get("selectedIndex", mSelectedIndex)) return false;
    return true;
}

NAMESPACE_END(nanogui)
