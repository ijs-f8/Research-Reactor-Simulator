#pragma once
#include <nanogui/common.h>
#include <iostream>
#include <deque>
#include <cmath>
#include <cstddef>

using nanogui::Color;
using std::deque;
using std::exception;
using std::string;
using std::abs;

enum class DrawMode {
	Default,
	Smart,
	SuperSmart,
};

class GraphElement {
public:
	enum HorizontalAxisLocation {
		Bottom,
		Top
	};
	enum AxisLocation {
		Left,
		Right
	};
	enum FormattingMode {
		Exponential,
		Normal
	};
	enum GraphType {
		None,
		PlotDeque,
		Bezier,
	};
protected:
	Color curveColor = Color(255, 0, 0, 255);
	Color axisColor = Color(0, 255);
	Color fillColor = Color(255, 0, 0, 127);
	Color textColor = Color(0, 255);
	Color pointerColor = Color(255, 0, 0, 255);
	Color horizontalPointerColor = Color(0, 0, 255, 255);
	float strokeWidth = 2.f;
	double mLimits[4] = { 0.,1.,0.,1. };
	double mLogLimits[4] = { -10., 0., -10., 0. };
	double mDiff[4] = {1., 1., 10., 10.};
	bool axisShown = false;
	bool horizontalAxisShown = false;
	bool drawMainAxis = false;
	bool ylog = false;
	bool drawHorizontalMainAxis = false;
	bool enabled = true;
	bool showText = false;
	bool fill = false;
	bool pointerShown = true;
	bool horizonralPointerShown = false;
	bool overridePointer = false;
	bool roundFloating = false;
	// Default pointer positions
	float pointerPosition = 0.f;
	float horizontalPointerPosition = 1.f;
	// pointer sizes
	float pointerSize = 10.f;
	float horizontalPointerSize = 10.f;
	// tick sizes and offsets
	float minorTickWidth = 0.7f;
	float majorTickWidth = 1.2f;
	float majorTickFontSize = 22.f;
	float mainTickFontSize = 26.f;
	float graphNameFontSize = 26.f;
	float axisWidth = 1.2f;
	float horizontalAxisWidth = 1.2f;
	double multiplier = 1.;
	double horizontalMultiplier = 1.;
	FormattingMode numberFormat = FormattingMode::Normal;
	size_t minorTickNumber = 0;
	size_t majorTickNumber = 0;
	FormattingMode horizontalNumberFormat = FormattingMode::Normal;
	size_t minorTickNumberHorizontal = 0;
	size_t majorTickNumberHorizontal = 0;
	AxisLocation axisPosition = AxisLocation::Left;
	HorizontalAxisLocation horizontalAxisPosition = HorizontalAxisLocation::Bottom;
	float axisOffset = 0.f;
	float horizontalAxisOffset = 0.f;
	float minorTickSize = 7.f;
	float majorTickSize = 14.f;
	float axisTextOffset = 50.f;
	float horizontalAxisTextOffset = 50.f;
	float pixelDrawRatio = 1.f;
	string plotUnits = "";
	string horizontalUnits = "";
	string formatted = "";
	std::string name = "Untitled";
	std::string nameHorizontal = "Untitled";
	std::string mOverrideLimitLabels[4] = { "","","","" };
public:
	GraphElement() {};
	~GraphElement() {};

	void setColor(Color value) { curveColor = value; }
	const Color &getColor() { return curveColor; }
	void setStrokeWidth(float value) { strokeWidth = value; }
	const float &getStrokeWidth() { return strokeWidth; }
	void setName(std::string value) { name = value; }
	const std::string &getName() { return name; }
	void setHorizontalName(std::string value) { nameHorizontal = value; }
	const std::string &getHorizontalName() { return nameHorizontal; }
	void setLimitOverride(size_t limitIndex, std::string value) { mOverrideLimitLabels[limitIndex] = value;	}
	const std::string* getOverridenLimits() { return mOverrideLimitLabels; }

	const bool &getEnabled() const { return enabled; }
	void setEnabled(bool value) { enabled = value; }
	const bool &getRoundFloating() const { return roundFloating; }
	void setRoundFloating(bool value) { roundFloating = value; }
	const bool &getAxisShown() const { return axisShown; }
	void setAxisShown(bool value) { axisShown = value; }
	const bool &getYlog() const { return ylog; }
	void setYlog(bool value) { ylog = value; }
	const bool &getHorizontalAxisShown() const { return horizontalAxisShown; }
	void setHorizontalAxisShown(bool value) { horizontalAxisShown = value; }
	const bool &getMainLineShown() const { return drawMainAxis; }
	void setMainLineShown(bool value) { drawMainAxis = value; }
	const bool &getHorizontalMainLineShown() const { return drawHorizontalMainAxis; }
	void setHorizontalMainLineShown(bool value) { drawHorizontalMainAxis = value; }
	const bool &isFill() const { return fill; }
	void setFill(bool value) { fill = value; }
	const bool &getTextShown() const { return showText; }
	void setTextShown(bool value) { showText = value; }
	const bool &getPointerShown() const { return pointerShown; }
	void setPointerShown(bool value) { pointerShown = value; }
	const bool &getHorizontalPointerShown() const { return horizonralPointerShown; }
	void setHorizontalPointerShown(bool value) { horizonralPointerShown = value; }
	const bool &getPointerOverride() const { return overridePointer; }
	void setPointerOverride(bool value) { overridePointer = value; }

	void setPointerPosition(float value) { pointerPosition = value; }
	const float &getPointerPosition() { return pointerPosition; }
	void setHorizontalPointerPosition(float value) { horizontalPointerPosition = value; }
	const float &getHorizontalPointerPosition() { return horizontalPointerPosition; }
	void setMinorTickWidth(float value) { minorTickWidth = value; }
	const float &getMinorTickWidth() { return minorTickWidth; }
	void setMajorTickWidth(float value) { majorTickWidth = value; }
	const float &getMajorTickWidth() { return majorTickWidth; }
	void setAxisWidth(float value) { axisWidth = value; }
	const float &getAxisWidth() { return axisWidth; }
	void setHorizontalAxisWidth(float value) { horizontalAxisWidth = value; }
	const float &getHorizontalAxisWidth() { return horizontalAxisWidth; }

	void setPixelDrawRatio(float ratio) { pixelDrawRatio = ratio; }
	const float &getPixelDrawRatio() { return pixelDrawRatio; }

	void setAxisColor(Color value) { axisColor = value; }
	const Color &getAxisColor() const { return axisColor; }
	void setFillColor(Color value) { fillColor = value; }
	const Color &getFillColor() const { return fillColor; }
	void setPointerColor(Color value) { pointerColor = value; }
	const Color &getPointerColor() const { return pointerColor; }
	void setHorizontalPointerColor(Color value) { horizontalPointerColor = value; }
	const Color &getHorizontalPointerColor() const { return horizontalPointerColor; }
	void setTextColor(Color value) { textColor = value; }
	const Color &getTextColor() const { return textColor; }

	void setPointerSize(float value) { pointerSize = value; }
	const float &getPointerSize() { return pointerSize; }
	void setHorizontalPointerSize(float value) { horizontalPointerSize = value; }
	const float &getHorizontalPointerSize() { return horizontalPointerSize; }
	void setMainTickFontSize(float value) { mainTickFontSize = value; }
	const float &getMainTickFontSize() { return mainTickFontSize; }
	void setMajorTickFontSize(float value) { majorTickFontSize = value; }
	const float &getMajorTickFontSize() { return majorTickFontSize; }
	void setNameFontSize(float value) { graphNameFontSize = value; }
	const float &getNameFontSize() { return graphNameFontSize; }

	void setMinorTickNumber(size_t value) { minorTickNumber = value; }
	const size_t &getMinorTickNumber() const { return minorTickNumber; }
	void setHorizontalMinorTickNumber(size_t value) { minorTickNumberHorizontal = value; }
	const size_t &getHorizontalMinorTickNumber() const { return minorTickNumberHorizontal; }
	void setMinorTickSize(float value) { minorTickSize = value; }
	const float &getMinorTickSize() const { return minorTickSize; }
	void setMajorTickNumber(size_t value) { majorTickNumber = value; }
	const size_t &getMajorTickNumber() const { return majorTickNumber; }
	void setHorizontalMajorTickNumber(size_t value) { majorTickNumberHorizontal = value; }
	const size_t &getHorizontalMajorTickNumber() const { return majorTickNumberHorizontal; }
	void setMajorTickSize(float value) { majorTickSize = value; }
	const float &getMajorTickSize() const { return majorTickSize; }
	void setAxisOffset(float value) { axisOffset = value; }
	const float &getAxisOffset() const { return axisOffset; }
	void setTextOffset(float value) { axisTextOffset = value; }
	const float &getTextOffset() const { return axisTextOffset; }
	void setAxisPosition(AxisLocation value) { axisPosition = value; }
	const AxisLocation &getAxisPosition() const { return axisPosition; }
	void setHoriziontalAxisOffset(float value) { horizontalAxisOffset = value; }
	const float &getHorizontalAxisOffset() const { return horizontalAxisOffset; }
	void setHorizontalTextOffset(float value) { horizontalAxisTextOffset = value; }
	const float &getHorizontalTextOffset() const { return horizontalAxisTextOffset; }
	void setHorizontalAxisPosition(HorizontalAxisLocation value) { horizontalAxisPosition = value; }
	const HorizontalAxisLocation &getHorizontalAxisPosition() const { return horizontalAxisPosition; }


	void setLimitMultiplier(double value) { multiplier = value; }
	const double &getLimitMultiplier() { return multiplier; }
	void setLimitHorizontalMultiplier(double value) { horizontalMultiplier = value; }
	const double &getLimitHorizontalMultiplier() { return horizontalMultiplier; }

	void setNumberFormatMode(FormattingMode value) { numberFormat = value; }
	const FormattingMode &getNumberFormatMode() const { return numberFormat; }
	void setHorizontalNumberFormatMode(FormattingMode value) { horizontalNumberFormat = value; }
	const FormattingMode &getHorizontalNumberFormatMode() const { return horizontalNumberFormat; }

	const string &getUnits() const { return plotUnits; }
	void setUnits(string units_) { plotUnits = units_; }
	const string &getHorizontalUnits() const { return horizontalUnits; }
	void setHorizontalUnits(string units_) { horizontalUnits = units_; }

	void setLimits(double minX, double maxX, double minY, double maxY) {
		mLimits[0] = minX;
		mLimits[1] = maxX;
		mLimits[2] = minY;
		mLimits[3] = maxY;
		mDiff[0] = mLimits[1] - mLimits[0];
		mDiff[1] = mLimits[3] - mLimits[2];
		for (int i = 0; i < 4; i++) mLogLimits[i] = mLimits[i] ? log10(mLimits[i]) : -10.;
		mDiff[2] = mLogLimits[1] - mLogLimits[0];
		mDiff[3] = mLogLimits[3] - mLogLimits[2];
	}
	double * limits() { return mLimits; }
	double * logLimits() { return mLogLimits; }

	const string formatNumber(double number) {		
		string suffix = "";
		if (numberFormat == FormattingMode::Normal) {
			formatted = format2(number);
		}
		else if (numberFormat == FormattingMode::Exponential) {
			if (number == 0) {
				formatted = "0";
			}
			else {
				int low = (int)floor(log10(number));
				formatted = format2(number / pow(10, low));

				if (low > 0) {
					suffix = "e+" + std::to_string(low);
				}
				else if (low < 0) {
					suffix = "e" + std::to_string(low);
				}
			}

		}
		if (formatted.find(".") != string::npos) {
			if (formatted.substr(formatted.length() - 2) == "00") {
				formatted = formatted.substr(0, formatted.length() - 3);
			}
			else if (formatted.substr(formatted.length() - 1) == "0") {
				formatted = formatted.substr(0, formatted.length() - 1);
			}
		}
		if (roundFloating) {
			number = std::round(number);
			formatted = std::to_string((int)number);
		}
		formatted += suffix;
		return formatted;

	}

	virtual GraphType graphType() = 0;

private:
	static string format2(double value) {
		value = std::round(value * 100) / 100.;
		std::stringstream ss;
		ss << std::fixed;
		ss.precision(2);
		ss << value;
		return ss.str();
	}
};

class BezierCurve : public GraphElement {
protected:
	float parameters[4] = { 0.,0.,1.,1. };
	float rodPosition = 0.f;
	Color mOverFillColor = Color(255, 0, 0, 150);
public:
	BezierCurve() {};
	~BezierCurve() { free(parameters); };

	void setParameter(size_t i, float value) { parameters[i] = value; }
	float getParameter(size_t i) { return parameters[i]; }

	void setOverFillColor(Color value) { mOverFillColor = value; }
	const Color &overFillColor() const { return mOverFillColor; }

	void setRodPosition(float value) { rodPosition = value; }
	float getRodPosition() { return rodPosition; }

	virtual GraphType graphType() { return GraphType::Bezier; }
};

class Plot : public GraphElement {
protected:
	char type = 0;
	size_t plotRange[2] = { 0,0 };
	const size_t mArraySize;
	DrawMode draw = DrawMode::Smart;
	std::function<void(double*, const size_t)> mValueComputing;
	bool mRewriting;
public:
	Plot(const size_t arraySize, bool rewriting = false) : mArraySize(arraySize) { mRewriting = rewriting; };

	const size_t arraySize() { return mArraySize; }

	const DrawMode &getDrawMode() const { return draw; }
	void setDrawMode(DrawMode value) { draw = value; }

	void setPlotRange(size_t from, size_t to) { plotRange[0] = from; plotRange[1] = (to < from && mRewriting) ? to + mArraySize : to; }
	
	size_t getPlotStart() { return plotRange[0]; }
	size_t getPlotEnd() { return plotRange[1]; }
	size_t getPlotRange() {
		if (plotRange[1] == 0) { 
			return mArraySize - 1 - plotRange[0];
		}
		else {
			return getPlotEnd() - getPlotStart() + 1;
		}
	}

	virtual GraphType graphType() { return GraphType::PlotDeque; }

	void setValueComputing(std::function<void(double*, const size_t)> computing) { mValueComputing = computing; }
	std::function<void(double*, const size_t)> valueComputing() { return mValueComputing; }

protected:
	double *xValues;
	float *yValues_float;
	double *yValues_dbl;
	long start = -1L;
public:

	// Enter -1 to disable lin space x axis
	void setXdataLin(long indexOffset) { if(indexOffset >= 0) start = indexOffset; }
	void setXdata(double* x_axis) { xValues = x_axis; }
	void setYdata(double* y_axis) { yValues_dbl = y_axis; type = 2; }
	void setYdata(float* y_axis) { yValues_float = y_axis; type = 1; }

	double getXat(size_t i, bool normalize = true) {
		if (start >= 0L) {
			if (normalize) {
				return ((double)i - plotRange[0] - start) / (plotRange[1] - plotRange[0]);	
			}
			else {
				return (double)i - start;
			}
		}
		if (mRewriting) i = i % mArraySize;
		if (normalize) {
			return (xValues[i] - mLimits[0]) * horizontalMultiplier / mDiff[0];
		}
		else {
			return xValues[i] * horizontalMultiplier;
		}
	}

	double getYat(size_t i, bool normalize = true) {
		if (mRewriting) i = i % mArraySize;
		if (i >= mArraySize) throw new exception("Out of bounds - Plot deque");
		double preNorm;
		if (type == 1) {
			preNorm = (double)yValues_float[i];
		}
		else if (type == 2) {
			preNorm = (double)yValues_dbl[i];
		}
		else {
			return 0.;
		}
		if (mValueComputing) mValueComputing(&preNorm, i);
		if (normalize) {
			if (ylog) {
				preNorm = (log10(preNorm) - mLogLimits[2]) / mDiff[3];
			}
			else {
				preNorm = (preNorm - mLimits[2]) / mDiff[1];
			}
		}
		return (double)preNorm;
	}

};
