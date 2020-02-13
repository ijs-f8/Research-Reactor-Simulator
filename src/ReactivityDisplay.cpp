#include <nanogui/ReactivityDisplay.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

void PeriodDisplay::draw(NVGcontext *ctx)
{
	Widget::draw(ctx);
	float displayWidth = 0.5*(mSize.x() - 2 * mPadding);
	float axesWidth = 0.5*(mSize.x() - 2 * mPadding);
	if (displayWidth <= 0.f) return;

	nvgSave(ctx);

	nvgIntersectScissor(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());

	nvgFontFace(ctx, mFontFace.c_str());
	nvgFontSize(ctx, textFontSize);
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_TOP | NVGalign::NVG_ALIGN_LEFT);

	const float verticalMargins = 25.f;
	const float pointerSize = 8.f;
	const float rangeY = mSize.y() - 2 * verticalMargins;
	const float rangeYM = (mSize.y() - 2 * verticalMargins) / 1.17;
	float centerLocation = 0.17 * rangeY / 1.17;
	float displayVal;

	if (period > 0) {
		displayVal = std::max(-(1.f - posFromPeriod(period)) * (rangeYM), -rangeYM);
		displayVal = std::min(displayVal, 0.f);
	}
	else {
		displayVal = std::min((1.f - posFromPeriod(-period)) * rangeYM, centerLocation);
		displayVal = std::max(displayVal, 0.f);
	}

	// Draw backgrounds
	nvgBeginPath(ctx);

	nvgRect(ctx, mPos.x() + mPadding + axesWidth, mPos.y() + verticalMargins*0.98f, displayWidth, rangeY*1.01f);
	nvgFillColor(ctx, Color(200, 255));
	nvgFill(ctx);

	// Drawing the rods and magnet pointers
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);

	nvgBeginPath(ctx);
	nvgRect(ctx, mPos.x() + mPadding + axesWidth, mPos.y() + verticalMargins + rangeYM, displayWidth, displayVal);
	nvgFillColor(ctx, Color(0, 255, 0, 255));
	nvgFill(ctx);

	//Main line for axes
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, mPos.x() + mPadding + 0.8 * axesWidth, mPos.y() + verticalMargins);
	nvgLineTo(ctx, mPos.x() + mPadding + 0.8 * axesWidth, mPos.y() + verticalMargins + rangeY);
	nvgClosePath(ctx);
	nvgStrokeColor(ctx, Color(255, 255));
	//nvgFill(ctx);

	//Draw tics and text
	for (int i = 1; i < 7; i++) {
		float posY = mPos.y() + verticalMargins + axesLocations[6 - i] * (rangeYM);
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, mPos.x() + mPadding + 0.5 * axesWidth, posY);
		nvgLineTo(ctx, mPos.x() + mPadding + 0.8 * axesWidth, posY);
		//nvgClosePath(ctx);
		nvgStroke(ctx);
		// nvgFill(ctx);
	}

	nvgFillColor(ctx, Color(255, 255));
	for (int i = 1; i < 7; i++) {
		float posY = mPos.y() + verticalMargins + axesLocations[6 - i] * (rangeYM);
		nvgText(ctx, mPos.x() + mPadding, posY, axesValues[6 - i], NULL);
	}

	/*
		// Drawing pointers
		const float h = std::sqrtf(3.f) * pointerSize * 0.5f;
		nvgFillColor(ctx, Color(255, 255));
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, mPos.x() + mPadding + i*(rodWidth + mSpacing) + 0.5f, mPos.y() + magnetPos[i] + verticalMargins);
		nvgLineTo(ctx, mPos.x() + mPadding + i*(rodWidth + mSpacing) + 0.5f - h, mPos.y() + magnetPos[i] + verticalMargins - pointerSize / 2.f);
		nvgLineTo(ctx, mPos.x() + mPadding + i*(rodWidth + mSpacing) + 0.5f - h, mPos.y() + magnetPos[i] + verticalMargins + pointerSize / 2.f);
		nvgClosePath(ctx);
		nvgFill(ctx);
	*/
	nvgRestore(ctx);
}

float PeriodDisplay::posFromPeriod(float period_)
{
	return log(0.46874*period_)*0.316009;
}

Vector2i PeriodDisplay::preferredSize(NVGcontext *) const {
	return Vector2i(15, 70);
}

PeriodDisplay::PeriodDisplay(Widget * parent) : Widget(parent)
{

}

PeriodDisplay::~PeriodDisplay()
{

}

NAMESPACE_END(nanogui)
