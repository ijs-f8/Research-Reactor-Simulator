#include "..\include\nanogui\controlRodDisplay.h"
#include "nanogui/controlRodDisplay.h"
#include <nanogui/theme.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

void ControlRodDisplay::draw(NVGcontext *ctx)
{
	Widget::draw(ctx);

	nvgSave(ctx);

	const float rodSize = mSize.y() - rodBorder;
	const float rodWidth = (mSize.x() - 2 * rodSpacing) / 3.f;

	// Draw backgrounds
	float x[3];
	nvgBeginPath(ctx);
	for (int i = 0; i < 3; i++) {
		x[i] = mPos.x() + (rodWidth + rodSpacing)*i;
		nvgRect(ctx, mPos.x() + (rodWidth + rodSpacing)*i, mPos.y(), rodWidth, mSize.y());
	}
	nvgFillColor(ctx, mRodBackgroundColor);
	nvgFill(ctx);

	// Drawing the rods and magnet pointers
	float relPosRod, relPosMagnet;
	const float h = std::sqrtf(3.f) * pointerSize * 0.5f;
	NVGpaint upper[2];
	const float rodWgrad = rodWidth / 2 - rodBorder;
	for (int i = 0; i < 3; i++) {
		// Store rod positions
		relPosRod = (1.f - *rodActualPos[i] / *rodSteps[i]) * rodSize;
		relPosMagnet = (1.f - *rodExactPos[i] / *rodSteps[i]) * rodSize;

		// Drawing top part
		if (relPosRod > 0.f) {
			for (int m = 0; m < 2; m++) {
				upper[m] = nvgLinearGradient(ctx, x[i] + rodWgrad, 0, // startx, starty
					m ? (x[i] + rodWidth) : x[i], 0, // endx, endy
					rodEnabled[i] ? mRodExtrudedWhiteColor : mRodDisabledWhiteColor, rodEnabled[i] ? mRodExtrudedColor : mRodDisabledColor); // startcol, endcol
				nvgBeginPath(ctx);
				nvgRect(ctx, x[i] + rodBorder + (m ? rodWgrad : 0.f), mPos.y(), (2 - m)*rodWgrad, relPosRod);
				nvgFillPaint(ctx, upper[m]);
				nvgFill(ctx);
			}
		}

		// Drawing lower part
		if (relPosRod < rodSize) {
			nvgBeginPath(ctx);
			nvgRect(ctx, x[i] + rodBorder, mPos.y() + relPosRod, rodWidth - 2 * rodBorder, rodSize - relPosRod);
			nvgFillColor(ctx, mRodInsertedColor);
			nvgFill(ctx);
		}

		// Drawing pointers
		nvgFillColor(ctx, Color(255, 255));
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, x[i], mPos.y() + relPosMagnet);
		nvgLineTo(ctx, x[i] - h, mPos.y() + relPosMagnet - pointerSize / 2.f);
		nvgLineTo(ctx, x[i] - h, mPos.y() + relPosMagnet + pointerSize / 2.f);
		nvgClosePath(ctx);
		nvgMoveTo(ctx, x[i] + rodWidth, mPos.y() + relPosMagnet);
		nvgLineTo(ctx, x[i] + rodWidth + h, mPos.y() + relPosMagnet - pointerSize / 2.f);
		nvgLineTo(ctx, x[i] + rodWidth + h, mPos.y() + relPosMagnet + pointerSize / 2.f);
		nvgClosePath(ctx);
		nvgFill(ctx);
	}

	nvgRestore(ctx);
}

Vector2i ControlRodDisplay::preferredSize(NVGcontext *) const {
	return Vector2i((int)std::ceilf(2 * rodSpacing + 6 * rodBorder), 70);
}

NAMESPACE_END(nanogui)
