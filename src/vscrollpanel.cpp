/*
    src/vscrollpanel.cpp -- Adds a vertical scrollbar around a widget
    that is too big to fit into a certain area

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/vscrollpanel.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/serializer/core.h>

NAMESPACE_BEGIN(nanogui)

VScrollPanel::VScrollPanel(Widget *parent)
    : Widget(parent), mChildPreferredHeight(0), mScroll(0.0f) { }

void VScrollPanel::performLayout(NVGcontext *ctx) {
    Widget::performLayout(ctx);

    if (mChildren.empty())
        return;
    Widget *child = mChildren[0];
    mChildPreferredHeight = child->preferredSize(ctx).y();
    child->setPosition(Vector2i(0, 0));
	child->setSize(Vector2i(getSizeDefect() ? (mSize.x() - SCROLL_BAR_THICKNESS) : mSize.x(), mChildPreferredHeight));
}

Vector2i VScrollPanel::preferredSize(NVGcontext *ctx) const {
    if (mChildren.empty())
        return Vector2i::Zero();
    return mChildren[0]->preferredSize(ctx) + Vector2i(getSizeDefect() ? SCROLL_BAR_THICKNESS : 0, 0);
}

bool VScrollPanel::mouseDragEvent(const Vector2i &, const Vector2i &rel, int, int) {
    if (mChildren.empty())
        return false;

    float scrollh = height() * std::min(1.0f, height() / (float)mChildPreferredHeight);

    mScroll = std::max(0.f, std::min(1.f, mScroll + rel.y() / (float)(mSize.y() - scrollh)));
    return true;
}

bool VScrollPanel::scrollEvent(const Vector2i &p, const Vector2f &rel) {
	if (!getSizeDefect()) return false;

	float start = mScroll;
	float scrollAmount = rel.y() * (mSize.y() / 20.0f);

	mScroll = std::max(0.f, std::min(1.f, mScroll - (scrollAmount / getSizeDefect())));
	if (start == mScroll) {
		return Widget::scrollEvent(p, rel);
	}
	else {
		return true;
	}
}

bool VScrollPanel::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (mChildren.empty())
        return false;
    int shift = (int) (mScroll*getSizeDefect());
	return Widget::mouseButtonEvent(p + Vector2i(0, shift), button, down, modifiers);
}

bool VScrollPanel::mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) {
    if (mChildren.empty())
        return false;
    int shift = (int) (mScroll*getSizeDefect());
    return Widget::mouseMotionEvent(p + Vector2i(0, shift), rel, button, modifiers);
}

void VScrollPanel::draw(NVGcontext *ctx) {
	if (drawBackground) {
		nvgFillColor(ctx, mBackgroundColor);
		nvgBeginPath(ctx);
		nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
		nvgFill(ctx);
	}
    if (mChildren.empty())
        return;
    Widget *child = mChildren[0];
    mChildPreferredHeight = child->preferredSize(ctx).y();

    nvgSave(ctx);

    nvgTranslate(ctx, mPos.x(), mPos.y());
	nvgIntersectScissor(ctx, 0, 0, getSizeDefect() ? (mSize.x() - SCROLL_BAR_THICKNESS) : mSize.x(), mSize.y());
    nvgTranslate(ctx, 0, -mScroll*getSizeDefect());
	
    if (child->visible()) child->draw(ctx);

    nvgRestore(ctx);
	// Draw scroll thing
	if (getSizeDefect()) {
		NVGpaint paint = nvgBoxGradient(
			ctx, mPos.x() + mSize.x() - SCROLL_BAR_THICKNESS, mPos.y(), SCROLL_BAR_THICKNESS,
			mSize.y(), SCROLL_BAR_ROUND * SCROLL_BAR_THICKNESS / 2.f, 4, Color(0, 32), Color(0, 92));
		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, mPos.x() + mSize.x() - SCROLL_BAR_THICKNESS, mPos.y(), SCROLL_BAR_THICKNESS,
			mSize.y(), SCROLL_BAR_ROUND * SCROLL_BAR_THICKNESS / 2.f);
		nvgFillPaint(ctx, paint);
		nvgFill(ctx);

		float scrollBarHeight = ((float)mSize.y()) * mSize.y() / mChildPreferredHeight;
		paint = nvgBoxGradient(
			ctx, mPos.x() + mSize.x() - SCROLL_BAR_THICKNESS + 1,
			mPos.y() + mScroll * (mSize.y() - scrollBarHeight), SCROLL_BAR_THICKNESS - 2, scrollBarHeight,
			SCROLL_BAR_ROUND * (SCROLL_BAR_THICKNESS - 2) / 2.f, 4, Color(220, 100), Color(128, 100));

		nvgBeginPath(ctx);
		nvgRoundedRect(ctx, mPos.x() + mSize.x() - SCROLL_BAR_THICKNESS + 1,
			mPos.y() + mScroll * (mSize.y() - scrollBarHeight), SCROLL_BAR_THICKNESS - 2, scrollBarHeight,
			SCROLL_BAR_ROUND * (SCROLL_BAR_THICKNESS - 2) / 2.f);
		nvgFillPaint(ctx, paint);
		nvgFill(ctx);
	}
}

void VScrollPanel::save(Serializer &s) const {
    Widget::save(s);
    s.set("childPreferredHeight", mChildPreferredHeight);
    s.set("scroll", mScroll);
}

Vector2i VScrollPanel::absolutePosition() const
{
	return mParent ? mParent->absolutePosition() + Vector2i(mPos.x(), mPos.y() - mScroll*getSizeDefect()) : Vector2i(mPos.x(), mPos.y() - mScroll*getSizeDefect());
}

bool VScrollPanel::load(Serializer &s) {
    if (!Widget::load(s)) return false;
    if (!s.get("childPreferredHeight", mChildPreferredHeight)) return false;
    if (!s.get("scroll", mScroll)) return false;
    return true;
}

int VScrollPanel::getSizeDefect() const
{
	if (mChildPreferredHeight < mSize.y()) {
		return 0;
	}
	else {
		return mChildPreferredHeight - mSize.y();
	}
}

NAMESPACE_END(nanogui)
