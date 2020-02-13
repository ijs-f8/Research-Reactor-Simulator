#pragma once
#include <nanogui/common.h>
#include <nanogui/widget.h>
#include <iostream>

using nanogui::Color;
using std::pair;
using std::exception;

NAMESPACE_BEGIN(nanogui);

class NANOGUI_EXPORT ControlRodDisplay : public Widget
{
public:
	ControlRodDisplay(Widget* parent) : Widget(parent) {};
	~ControlRodDisplay() {};
	void setRod(int i, size_t* steps, float* actualPos, float* exactPos, bool* r_enabled) { rodSteps[i] = steps; rodActualPos[i] = actualPos; rodExactPos[i] = exactPos; rodEnabled[i] = r_enabled; }

	static const float getRodSpacing() { return 20.f; }

	virtual Vector2i preferredSize(NVGcontext *ctx) const override;
	virtual void draw(NVGcontext *ctx) override;
protected:
	Color mRodBackgroundColor = Color(.15f, 1.f);
	Color mRodInsertedColor = Color(80, 220, 255, 255);
	Color mRodExtrudedColor = Color(60, 0, 150, 255);
	Color mRodExtrudedWhiteColor = Color(90, 39, 166, 255);
	Color mRodDisabledColor = Color(150, 0, 0, 255);
	Color mRodDisabledWhiteColor = Color(166, 39, 39, 255);
	Color mTextColor = Color(200, 255);
	Color mTextDisabledColor = Color(120, 255);

	size_t* rodSteps[3];
	float* rodActualPos[3];
	float* rodExactPos[3];
	bool* rodEnabled[3];

	const float rodSpacing = 20.f;
	const float rodBorder = 2.f;
	const float pointerSize = 9.f;
};

NAMESPACE_END(nanogui);
