/*
    src/theme.cpp -- Storage class for basic theme-related properties

    The text box widget was contributed by Christian Schueller.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui_resources.h>

NAMESPACE_BEGIN(nanogui)

Theme::Theme(NVGcontext *ctx) {
	mBorderWidth                      = 1.f;

    mStandardFontSize                 = 16.f;
    mButtonFontSize                   = 20.f;
    mTextBoxFontSize                  = 20.f;
    mWindowCornerRadius               = 2.f;
    mWindowHeaderHeight               = 30.f;
    mWindowDropShadowSize             = 10.f;
    mButtonCornerRadius               = 2.f;
    mTabBorderWidth                   = 0.75f;
    mTabInnerMargin                   = 5.f;
    mTabMinButtonWidth                = 20.f;
    mTabMaxButtonWidth                = 160.f;
    mTabControlWidth                  = 20.f;
    mTabButtonHorizontalPadding       = 10.f;
    mTabButtonVerticalPadding         = 2.f;

    mDropShadow                       = Color(0, 128);
    mTransparent                      = Color(0, 0);
    mBorderDark                       = Color(29, 255);
    mBorderLight                      = Color(92, 255);
    mBorderMedium                     = Color(35, 255);
    mTextColor                        = Color(255, 160);
    mDisabledTextColor                = Color(255, 80);
    mTextColorShadow                  = Color(0, 160);
    mIconColor                        = mTextColor;

    mButtonGradientTopFocused         = Color(64, 255);
    mButtonGradientBotFocused         = Color(48, 255);
    mButtonGradientTopUnfocused       = Color(74, 255);
    mButtonGradientBotUnfocused       = Color(58, 255);
    mButtonGradientTopPushed          = Color(41, 255);
    mButtonGradientBotPushed          = Color(29, 255);

    /* Window-related */
    mWindowFillUnfocused              = Color(43, 230);
    mWindowFillFocused                = Color(45, 230);
    mWindowTitleUnfocused             = Color(220, 160);
    mWindowTitleFocused               = Color(255, 190);

    mWindowHeaderGradientTop          = mButtonGradientTopUnfocused;
    mWindowHeaderGradientBot          = mButtonGradientBotUnfocused;
    mWindowHeaderSepTop               = mBorderLight;
    mWindowHeaderSepBot               = mBorderDark;

    mWindowPopup                      = Color(50, 255);
    mWindowPopupTransparent           = Color(50, 0);

    mFontNormal = nvgCreateFontMem(ctx, "sans", roboto_regular_ttf,
                                   roboto_regular_ttf_size, 0);
    mFontBold = nvgCreateFontMem(ctx, "sans-bold", roboto_bold_ttf,
                                 roboto_bold_ttf_size, 0);
    mFontIcons = nvgCreateFontMem(ctx, "icons", entypo_ttf,
                                  entypo_ttf_size, 0);
	//nvgCreateFontMem(ctx, "opensans", OpenSans_ttf, OpenSans_ttf_size, 0);
    if (mFontNormal == -1 || mFontBold == -1 || mFontIcons == -1)
        throw std::runtime_error("Could not load fonts!");
}

NAMESPACE_END(nanogui)
