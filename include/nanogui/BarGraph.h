#pragma once
#include <nanogui/common.h>
#include <iostream>
#include <deque>

using nanogui::Color;
using std::deque;
using std::pair;
using std::exception;

class BarGraph {
protected:
	Color lineColor[7];
	Color fillColor[7];
	bool fill = true;
	bool enabled = true;
	float outlineWidth = 2.;
	deque<std::array<double, 7>>* data;
	size_t range[2] = { 1,6 };
	size_t autoscaleRange[2] = { 1,6 };
public:
	void setLineColor(size_t i, Color value) { lineColor[i] = value; }
	const Color &getLineColor(size_t i) const { return lineColor[i]; }
	void setFillColor(size_t i, Color value) { fillColor[i] = value; }
	const Color &getFillColor(size_t i) const { return fillColor[i]; }
	void setOutineWidth(float value) { outlineWidth = value; }
	const float &getOutlineWidth() { return outlineWidth; }

	BarGraph() {};

	void setData(deque<std::array<double, 7>>* data_) { data = data_; }
	void setRange(size_t fromIndex, size_t toIndex) { range[0] = fromIndex; range[1] = toIndex; }
	void setAutoscaleRange(size_t fromIndex, size_t toIndex) { autoscaleRange[0] = fromIndex; autoscaleRange[1] = toIndex; }
	size_t* getRange() { return range; }

	double nSum = 0;
	void updateAutoscale() {
		nSum = 0;
		for (size_t i = autoscaleRange[0]; i <= autoscaleRange[1]; i++) { nSum += data->back()[i]; }
	}

	double getY(size_t i, bool normalize = true) {
		if (normalize) {
			return data->back()[i] / nSum;
		}
		else {
			return data->back()[i];
		}
	}

};