/*
	src/label.cpp -- Text label with an arbitrary font, color, and size

	NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
	The widget drawing code is based on the NanoVG demo application
	by Mikko Mononen.

	All rights reserved. Use of this source code is governed by a
	BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/label.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/serializer/core.h>

NAMESPACE_BEGIN(nanogui)

Label::Label(Widget *parent, const std::string &caption, const std::string &font, int fontSize)
	: Widget(parent), mCaption(caption), mFont(font) {
	if (mTheme) {
		mFontSize = mTheme->mStandardFontSize;
		mColor = mTheme->mTextColor;
	}
	if (fontSize >= 0) mFontSize = fontSize;
}

void Label::setTheme(Theme *theme) {
	Widget::setTheme(theme);
	if (mTheme) {
		mFontSize = mTheme->mStandardFontSize;
		mColor = mTheme->mTextColor;
	}
}

Vector2i Label::preferredSize(NVGcontext *ctx) const {
	if (mCaption == "")
		return Vector2i::Zero();
	nvgFontFace(ctx, mFont.c_str());
	nvgFontSize(ctx, fontSize());
	float bounds[4];
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_TOP | NVGalign::NVG_ALIGN_LEFT);
	nvgTextBounds(ctx, 0, 0, mCaption.c_str(), nullptr, bounds);
	Vector2i retSize = Vector2i();
	for (int i = 0; i < 2; i++) {
		retSize[i] = mFixedSize[i] ? mFixedSize[i] : bounds[2 + i] - bounds[i] + mPadding[i] + mPadding[i + 2];
	}
	return retSize;
}

void Label::draw(NVGcontext *ctx) {
	if (mGlow) {
		nvgBeginPath(ctx);
		NVGpaint grad = nvgBoxGradient(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y(), mRoundedEdges, mFeather, mGlowColor, Color(mGlowColor.r(), mGlowColor.g(), mGlowColor.b(), 0.f));
		nvgFillPaint(ctx, grad);
		nvgRect(ctx, mPos.x() - mFeather, mPos.y() - mFeather, mSize.x() + 2.f*mFeather, mSize.y() + 2.f*mFeather);
		nvgFill(ctx);
	}
	Widget::draw(ctx);
	nvgFontFace(ctx, mFont.c_str());
	nvgFontSize(ctx, mFontSize);
	nvgFillColor(ctx, mColor);
	float bounds[4];
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_TOP | NVGalign::NVG_ALIGN_LEFT);
	nvgTextBoxBounds(ctx, 0, 0, mSize.x(), mCaption.c_str(), nullptr, bounds);
	bounds[2] -= bounds[0];
	bounds[3] -= bounds[1];
	float offset[2] = { mPadding[0],mPadding[1] };
	Vector2i useSize = Vector2i(mSize.x() - mPadding[0] - mPadding[2], mSize.y() - mPadding[1] - mPadding[3]);
	if (mTextAlignment & TextAlign::HORIZONTAL_CENTER) {
		offset[0] = (useSize.x() - bounds[2]) / 2 + mPadding[0];
	}
	else if (mTextAlignment & TextAlign::RIGHT) {
		offset[0] = useSize.x() - bounds[2] - mPadding[2];
	}
	if (mTextAlignment & TextAlign::VERTICAL_CENTER) {
		offset[1] = (useSize.y() - bounds[3]) / 2 + mPadding[1];
	}
	else if(mTextAlignment & TextAlign::BOTTOM) {
		offset[1] = useSize.y() - bounds[3] - mPadding[3];
	}
	else if (mTextAlignment & TextAlign::BASELINE) {
		offset[1] = useSize.y() - mFontSize - mPadding[3];
	}
	
	nvgText(ctx, mPos.x() + offset[0], mPos.y() + offset[1], mCaption.c_str(), nullptr);
}

void Label::save(Serializer &s) const {
	Widget::save(s);
	s.set("caption", mCaption);
	s.set("font", mFont);
	s.set("color", mColor);
}

bool Label::load(Serializer &s) {
	if (!Widget::load(s)) return false;
	if (!s.get("caption", mCaption)) return false;
	if (!s.get("font", mFont)) return false;
	if (!s.get("color", mColor)) return false;
	return true;
}

NAMESPACE_END(nanogui)
