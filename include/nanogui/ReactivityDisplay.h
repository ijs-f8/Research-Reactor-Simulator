#pragma once
#include <nanogui/common.h>
#include <nanogui/widget.h>
#include <Simulator.h>
#include <iostream>
#include <algorithm>    // std::max

using nanogui::Color;
using std::deque;
using std::pair;
using std::exception;

NAMESPACE_BEGIN(nanogui);

class NANOGUI_EXPORT PeriodDisplay : public Widget
{
public:
	PeriodDisplay(Widget* parent);
	~PeriodDisplay();

	virtual Vector2i preferredSize(NVGcontext *ctx) const override;
	virtual void draw(NVGcontext *ctx) override;
	void setPeriod(double period_) { period = period_; }
protected:
	std::string mFontFace = "sans-bold";
	Color mTextColor = Color(200, 255);
	Color mTextDisabledColor = Color(120, 255);
	float textFontSize = 22.f;
	float mSpacing = 20.f;
	float mPadding = 15.f;
	double period;
private:
	float posFromPeriod(float period_);
	char *axesValues[6] = { "-30", "inf", "30", "7", "5", "3" };
	float axesLocations[6] = { 1.17f, 1.f, 0.83f, 0.373f, 0.270f, 0.10f };
};

NAMESPACE_END(nanogui);

