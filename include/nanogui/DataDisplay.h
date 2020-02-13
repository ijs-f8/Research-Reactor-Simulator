#pragma once

#include <nanogui/widget.h>

NAMESPACE_BEGIN(nanogui)

enum class NANOGUI_EXPORT DisplayMode {
	Integer,
	Double,
	FixedDecimalPlaces1,
	FixedDecimalPlaces2,
	FixedDecimalPlaces3,
	FixedDecimalPlaces4,
	Scientific
};

template <typename DisplayType>
class DataDisplay : public Widget
{
protected:
	float textFontSize = 22.f;
	float numberFontSize = 45.f;
	float pointerSize = 20.f;
	std::string mCaption;
	Color mTextColor;
	Color mNumberColor;
	Color pointerColor;
	float padding[4] = { 25,10,0,10 };
	const std::string units_old[13] = { "a", "f", "p", "n", "micro", "m", "", "k", "M", "G", "T", "P", "E" };
	const int units[13] = { 0x61,0x66,0x70,0x6E,0xB5,0x6D,0,0x6B,0x4D,0x47,0x54,0x50,0x45 };
	std::string unit;
	DisplayMode numberMode = DisplayMode::Integer;
	DisplayType absoluteLimit = 0.;
	std::string mFontFace = "sans-bold";
public:

	const float &getTextFontSize() { return textFontSize; }
	void setTextFontSize(float size) { textFontSize = size; }
	const float &getNumberFontSize() { return numberFontSize; }
	void setNumberFontSize(float size) { numberFontSize = size; }
	const float &getPointerSize() { return pointerSize; }
	void setPointerSize(float size) { pointerSize = size; }
	const double &getAbsoluteLimit() { return absoluteLimit; }
	void setAbsoluteLimit(DisplayType value) { absoluteLimit = value; }

	const bool &getPointerShown() { return pointerShown; }
	void setPointerShown(bool shown) { pointerShown = shown; }

	void setPadding(float left, float top, float right, float bottom)
	{
		padding[0] = left;
		padding[1] = top;
		padding[2] = right;
		padding[3] = bottom;
	}

	const Color &textColor() const { return mTextColor; }
	void setTextColor(const Color &textColor) { mTextColor = textColor; }

	const Color &numberColor() const { return mNumberColor; }
	void setNumberColor(const Color &numberColor) { mNumberColor = numberColor; }

	const Color &getPointerColor() const { return pointerColor; }
	void setPointerColor(const Color &pointer_color) { pointerColor = pointer_color; }

	const DisplayMode &getDisplayMode() const { return numberMode; }
	void setDisplayMode(const DisplayMode &mode) { numberMode = mode; }

	const std::string &getUnit() const { return unit; }
	void setUnit(const std::string &unit_) { unit = unit_; }

	const std::string &getFontFace() const { return mFontFace; }
	void setFontFace(const std::string &font) { mFontFace = font; }

	void setData(DisplayType data) { currentData = data;}

protected:
	DisplayType currentData;
	bool pointerShown = true;
public:

	virtual void draw(NVGcontext *ctx) override {
		Widget::draw(ctx);
		float xNow = mPos.x() + padding[0];
		float yNow = mPos.y() + padding[1];
		float yRange = mSize.y() - padding[1] - padding[3];
		nvgFontFace(ctx, mFontFace.c_str());
		nvgFontSize(ctx, textFontSize);
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_TOP | NVGalign::NVG_ALIGN_LEFT);
		nvgFillColor(ctx, mTextColor);
		nvgText(ctx, xNow, yNow, mCaption.c_str(), NULL);
		nvgFillColor(ctx, mNumberColor);
		nvgFontSize(ctx, numberFontSize);
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_BOTTOM | NVGalign::NVG_ALIGN_LEFT);
		if (abs(currentData) > absoluteLimit && absoluteLimit > 0.) {
			nvgText(ctx, xNow, yNow + yRange, "Inf", NULL);
		}
		else
		{
			nvgText(ctx, xNow, yNow + yRange, formatNumber(currentData).c_str(), NULL);
		}

		if (pointerShown) {
			nvgFillColor(ctx, pointerColor);
			nvgBeginPath(ctx);
			nvgMoveTo(ctx, (float)mPos.x(), yNow + yRange - (numberFontSize + pointerSize) / 2.f);
			nvgLineTo(ctx, (float)mPos.x(), yNow + yRange - (numberFontSize - pointerSize) / 2.f);
			nvgLineTo(ctx, mPos.x() + (float)sqrt(3)*pointerSize / 2.f, yNow + yRange - numberFontSize / 2.f);
			nvgClosePath(ctx);
			nvgFill(ctx);
		}
	}

	DataDisplay(Widget *parent, const std::string &caption = "Untitled")
		: Widget(parent), mCaption(caption) {
		mBackgroundColor = Color(20, 128);
		mTextColor = Color(255, 255);
		mNumberColor = Color(255, 255);
		pointerColor = Color(255, 0, 0, 255);
		unit = "";
	}
private:
	std::string formatNumber(double value) {
		std::string ret = "";
		switch (numberMode) {
		case DisplayMode::Integer:
			ret = std::to_string((int)value);
			break;
		case DisplayMode::Double:
			ret = std::to_string(value);
			break;
		case DisplayMode::FixedDecimalPlaces1:
			ret = formatDecimals(value, 1);
			break;
		case DisplayMode::FixedDecimalPlaces2:
			ret = formatDecimals(value, 2);
			break;
		case DisplayMode::FixedDecimalPlaces3:
			ret = formatDecimals(value, 3);
			break;
		case DisplayMode::FixedDecimalPlaces4:
			ret = formatDecimals(value, 4);
			break;
		case DisplayMode::Scientific:
			int order;
			if (value != 0.) {
				order = (int)(floor(log10(value) / 3.f));
			}
			else {
				order = 0;
			}
			double newValue = value / pow(10, order * 3);
			if (newValue >= 999.5) {
				newValue = 1.;
				order++;
				ret = "1";
			} else if (newValue >= 100.) {
				ret = std::to_string((int)round(newValue));
			} else {
				ret = formatDecimals(newValue, 2 - (int)floor(log10(newValue)));
			}
			ret += " " + ((order > -7 && order < 13 && order != 0) ? string(utf8(units[order + 6]).data()) : "") + unit;
			return ret;
		}
		ret += " " + unit;
		return ret;
	}
};

NAMESPACE_END(nanogui)
