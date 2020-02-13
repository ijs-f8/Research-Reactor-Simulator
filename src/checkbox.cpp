/*
    src/checkbox.cpp -- Two-state check box widget

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/checkbox.h>
#include <nanogui/opengl.h>
#include <nanogui/theme.h>
#include <nanogui/entypo.h>
#include <nanogui/serializer/core.h>

NAMESPACE_BEGIN(nanogui)

CheckBox::CheckBox(Widget *parent, const std::string &caption,
                   const std::function<void(bool) > &callback)
    : Widget(parent), mCaption(caption), mPushed(false), mChecked(false),
      mCallback(callback) { }

bool CheckBox::mouseButtonEvent(const Vector2i &p, int button, bool down,
                                int modifiers) {
    Widget::mouseButtonEvent(p, button, down, modifiers);
    if (!mEnabled)
        return false;

    if (button == GLFW_MOUSE_BUTTON_1) {
        if (down) {
            mPushed = true;
        } else if (mPushed) {
            if (contains(p)) {
                mChecked = !mChecked;
                if (mCallback)
                    mCallback(mChecked);
            }
            mPushed = false;
        }
        return true;
    }
    return false;
}

Vector2i CheckBox::preferredSize(NVGcontext *ctx) const {
    if (mFixedSize != Vector2i::Zero())
        return mFixedSize;
    nvgFontSize(ctx, fontSize());
    nvgFontFace(ctx, "sans");
    return Vector2i(
        nvgTextBounds(ctx, 0, 0, mCaption.c_str(), nullptr, nullptr) +
            1.7f * fontSize(),
        fontSize() * 1.3f);
}

void CheckBox::draw(NVGcontext *ctx) {
    Widget::draw(ctx);

    nvgFontSize(ctx, fontSize());
    nvgFontFace(ctx, "sans");
    nvgFillColor(ctx,
                 mEnabled ? mTheme->mTextColor : mTheme->mDisabledTextColor);
    nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgText(ctx, mPos.x() + 1.2f * mSize.y() + 5, mPos.y() + mSize.y() * 0.5f,
            mCaption.c_str(), nullptr);

    NVGpaint bg = nvgBoxGradient(ctx, mPos.x() + 1.5f, mPos.y() + 1.5f,
                                 mSize.y() - 2.0f, mSize.y() - 2.0f, 3, 3,
                                 mPushed ? Color(0, 100) : Color(0, 32),
                                 Color(0, 0, 0, 180));

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, mPos.x() + 1.0f, mPos.y() + 1.0f, mSize.y() - 2.0f,
                   mSize.y() - 2.0f, 3);
    nvgFillPaint(ctx, bg);
    nvgFill(ctx);

    if (mChecked) {
        nvgFontSize(ctx, 1.8 * mSize.y());
        nvgFontFace(ctx, "icons");
        nvgFillColor(ctx, mEnabled ? mTheme->mIconColor
                                   : mTheme->mDisabledTextColor);
        nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(ctx, mPos.x() + mSize.y() * 0.5f + 1,
                mPos.y() + mSize.y() * 0.5f, utf8(ENTYPO_ICON_CHECK).data(),
                nullptr);
    }
}

void CheckBox::save(Serializer &s) const {
    Widget::save(s);
    s.set("caption", mCaption);
    s.set("pushed", mPushed);
    s.set("checked", mChecked);
}

bool CheckBox::load(Serializer &s) {
    if (!Widget::load(s)) return false;
    if (!s.get("caption", mCaption)) return false;
    if (!s.get("pushed", mPushed)) return false;
    if (!s.get("checked", mChecked)) return false;
    return true;
}

SliderCheckBox::SliderCheckBox(Widget *parent, const std::function<void(bool) > &callback)
	: Widget(parent), mPushed(false), mChecked(false), state(0), mouseE(false),
	mCallback(callback) {
	mCursor = Cursor::Hand;
}

bool SliderCheckBox::mouseButtonEvent(const Vector2i &p, int button, bool down,
	int modifiers) {
	Widget::mouseButtonEvent(p, button, down, modifiers);
	if (!mEnabled)
		return false;

	if (button == GLFW_MOUSE_BUTTON_1) {
		if (down) {
			mPushed = true;
		}
		else if (mPushed) {
			if (contains(p)) {
				mChecked = !mChecked;
				if (state == 0) {
					state = mChecked ? 1 : -1;
					timeAtChange = nanogui::get_seconds_since_epoch();
				}else{
					state = -state;
					timeAtChange = 2*nanogui::get_seconds_since_epoch() - timeAtChange - ANIM_TIME;
				}
				if (mCallback)
					mCallback(mChecked);
			}
			mPushed = false;
		}
		return true;
	}
	return false;
}

bool SliderCheckBox::mouseEnterEvent(const Vector2i & /*p*/, bool enter)
{
	mouseE = enter;
	return true;
}

Vector2i SliderCheckBox::preferredSize(NVGcontext* /* ctx */) const {
	if (mFixedSize != Vector2i::Zero())
		return mFixedSize;
	return Vector2i((int)(fontSize() * 2.6f), (fontSize() * 1.3f));
}

void SliderCheckBox::draw(NVGcontext *ctx) {
	Widget::draw(ctx);
	
	float cornerR = height() / 2.f;

	NVGpaint bg = nvgBoxGradient(ctx, mPos.x() + 1.5f, mPos.y() + 1.5f,
		mSize.x() - 3.0f, mSize.y() - 3.0f, cornerR, 3,
		mPushed ? Color(0, 100) : (mouseE ? Color(0,16) : Color(0, 32)),
		Color(0, 0, 0, 180));

	nvgBeginPath(ctx);
	nvgRoundedRect(ctx, mPos.x() + 1.f, mPos.y() + 1.f, mSize.x() - 2.f,
		mSize.y() - 2.f, cornerR);
	nvgFillPaint(ctx, bg);
	nvgFill(ctx);

	float amp0 = width() - 2.f*cornerR;
	if (state != 0) {
		double t = (nanogui::get_seconds_since_epoch() - timeAtChange) / ANIM_TIME;
		if (t >= 1.) {
			t = 1.;
			state = 0;
		}
		float x = (1.f - pow(exp(10*(t-0.5)) + 1, -1))*1.01 - 0.005;
		Color sliderCol;
		if (x < 0.5) {
			sliderCol = Color(mChecked ? 1.f : (x*2), mChecked ? (x*2) : 1.f, 0.f, 1.f);
		}
		else {
			sliderCol = Color(mChecked ? (1.f - x)*2 : 1.f, mChecked ? 1.f : (1.f - x)*2, 0.f, 1.f);
		}
		float loc[2] = { mPos.x() + cornerR + amp0*(mChecked ? x : (1 - x)), mPos.y() + height() / 2.f };
		NVGpaint shineOn = nvgRadialGradient(ctx, loc[0], loc[1], 4.f, 6.5f, sliderCol, Color(sliderCol.r(),sliderCol.g(), sliderCol.b(), 0.f));
		// shine
		nvgFillPaint(ctx, shineOn);
		nvgBeginPath(ctx);
		nvgEllipse(ctx, loc[0], loc[1], 6.5f, 6.5f);
		nvgFill(ctx);
	}
	else {
		float loc[2] = { mPos.x() + (mChecked ? (width() - cornerR) : cornerR), mPos.y() + height() / 2.f };
		Color sliderCol =  mChecked ? Color(0,255,0,255) : Color(255,0,0,255);
		NVGpaint shineOn = nvgRadialGradient(ctx, loc[0], loc[1],
			4.f, 6.5f, sliderCol, Color(sliderCol.r(), sliderCol.g(), sliderCol.b(), 0.f));
		// shine
		nvgFillPaint(ctx, shineOn);
		nvgBeginPath(ctx);
		nvgEllipse(ctx, loc[0], loc[1], 6.5f, 6.5f);
		nvgFill(ctx);
	}

}

void SliderCheckBox::save(Serializer &s) const {
	Widget::save(s);
	s.set("pushed", mPushed);
	s.set("checked", mChecked);
}

bool SliderCheckBox::load(Serializer &s) {
	if (!Widget::load(s)) return false;
	if (!s.get("pushed", mPushed)) return false;
	if (!s.get("checked", mChecked)) return false;
	return true;
}

NAMESPACE_END(nanogui)
