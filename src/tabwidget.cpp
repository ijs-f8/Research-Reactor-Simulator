/*
    nanogui/tabwidget.h -- A wrapper around the widgets TabHeader and StackedWidget
    which hooks the two classes together.

    The tab widget was contributed by Stefan Ivanov.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/tabwidget.h>
#include <nanogui/tabheader.h>
#include <nanogui/stackedwidget.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/window.h>
#include <nanogui/screen.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/layout.h>
#include <algorithm>

NAMESPACE_BEGIN(nanogui)

TabWidget::TabWidget(Widget* parent)
    : Widget(parent), mHeader(new TabHeader(this)), mContent(new StackedWidget(this)) {
    mHeader->setCallback([this](int i) {
        mContent->setSelectedIndex(i);
        if (mCallback)
            mCallback(i);
    });
}

void TabWidget::setActiveTab(int tabIndex) {
    mHeader->setActiveTab(tabIndex);
    mContent->setSelectedIndex(tabIndex);
}

int TabWidget::activeTab() const {
    assert(mHeader->activeTab() == mContent->selectedIndex());
    return mContent->selectedIndex();
}

int TabWidget::tabCount() const {
    assert(mContent->childCount() == mHeader->tabCount());
    return mHeader->tabCount();
}

Widget* TabWidget::createTab(int index, const std::string &label, bool scrolling) {
	if (scrolling) {
		VScrollPanel* scrollMain = new VScrollPanel(nullptr);
		addTab(index, label, scrollMain);
		RelativeGridLayout* rel2 = new RelativeGridLayout();
		rel2->appendCol(RelativeGridLayout::Size(1.f));
		rel2->appendRow(RelativeGridLayout::Size(1.f));
		scrollMain->setLayout(rel2);
		Widget* mainTabBase = scrollMain->add<Widget>();
		rel2->setAnchor(mainTabBase, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Fill, Alignment::Fill, RelativeGridLayout::FillMode::Always, RelativeGridLayout::FillMode::IfLess));
		return mainTabBase;
	}
	else {
		Widget* mainTabBase = new Widget(nullptr);
		addTab(index, label, mainTabBase);
		return mainTabBase;
	}
	
}

Widget* TabWidget::createTab(const std::string &label, bool scrolling) {
    return createTab(tabCount(), label, scrolling);
}

void TabWidget::addTab(const std::string &name, Widget *tab) {
    addTab(tabCount(), name, tab);
}

void TabWidget::addTab(int index, const std::string &label, Widget *tab) {
    assert(index <= tabCount());
    // It is important to add the content first since the callback
    // of the header will automatically fire when a new tab is added.
    mContent->addChild(index, tab);
    mHeader->addTab(index, label);
    assert(mHeader->tabCount() == mContent->childCount());
}

int TabWidget::tabLabelIndex(const std::string &label) {
    return mHeader->tabIndex(label);
}

int TabWidget::tabIndex(Widget* tab) {
    return mContent->childIndex(tab);
}

void TabWidget::ensureTabVisible(int index) {
    if(!mHeader->isTabVisible(index))
        mHeader->ensureTabVisible(index);
}

const Widget *TabWidget::tab(const std::string &tabName) const {
    int index = mHeader->tabIndex(tabName);
    if (index == mContent->childCount())
        return nullptr;
    return mContent->children()[index];
}

Widget *TabWidget::tab(const std::string &tabName) {
    int index = mHeader->tabIndex(tabName);
    if (index == mContent->childCount())
        return nullptr;
    return mContent->children()[index];
}

bool TabWidget::removeTab(const std::string &tabName) {
    int index = mHeader->removeTab(tabName);
    if (index == -1)
        return false;
    mContent->removeChild(index);
    return true;
}

void TabWidget::removeTab(int index) {
    assert(mContent->childCount() < index);
    mHeader->removeTab(index);
    mContent->removeChild(index);
    if (activeTab() == index)
        setActiveTab(index == (index - 1) ? index - 1 : 0);
}

const std::string &TabWidget::tabLabelAt(int index) const {
    return mHeader->tabLabelAt(index);
}

void TabWidget::performLayout(NVGcontext* ctx) {
    int headerHeight = mHeader->preferredSize(ctx).y();
    int margin = mTheme->mTabInnerMargin;
    mHeader->setPosition({ 0, 0 });
    mHeader->setSize({ mSize.x(), headerHeight });
    mHeader->performLayout(ctx);
    mContent->setPosition({ margin, headerHeight + margin });
    mContent->setSize({ mSize.x() - 2 * margin, mSize.y() - 2*margin - headerHeight });
    mContent->performLayout(ctx);
}

Vector2i TabWidget::preferredSize(NVGcontext* ctx) const {
    auto contentSize = mContent->preferredSize(ctx);
    auto headerSize = mHeader->preferredSize(ctx);
    int margin = mTheme->mTabInnerMargin;
    auto borderSize = Vector2i(2 * margin, 2 * margin);
    Vector2i tabPreferredSize = contentSize + borderSize + Vector2i(0, headerSize.y());
    return tabPreferredSize;
}

void TabWidget::draw(NVGcontext* ctx) {
	Widget::draw(ctx);
    int tabHeight = mHeader->preferredSize(ctx).y();
	std::pair<Vector2i, Vector2i> activeArea = mHeader->activeButtonArea();
	nvgSave(ctx);

	float thisBorder = mTheme->mBorderWidth * 1.8f;

	nvgStrokeWidth(ctx, thisBorder);
	for (int i = 0; i < 2; i++) {
		float offset = i ? 0.f : thisBorder / 2.f;
		nvgStrokeColor(ctx, i ? mTheme->mBorderDark : mTheme->mBorderLight);
		// Up to the button
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, mPos.x(), mPos.y() + tabHeight + offset);
		nvgLineTo(ctx, mPos.x() + activeArea.first.x(), mPos.y() + tabHeight + offset);
		nvgStroke(ctx);

		// From the button to the end
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, mPos.x() + activeArea.second.x(), mPos.y() + tabHeight + offset);
		nvgLineTo(ctx, mPos.x() + mSize.x(), mPos.y() + tabHeight + offset);
		nvgStroke(ctx);
	}

	nvgRestore(ctx);
    
}

NAMESPACE_END(nanogui)
