/*
nanogui/graph.h -- Simple graph widget for showing a function plot

NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
The widget drawing code is based on the NanoVG demo application
by Mikko Mononen.

All rights reserved. Use of this source code is governed by a
BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <nanogui/widget.h>
#include <nanogui/Plot.h>
#include <nanogui/BarGraph.h>
#include <vector>

NAMESPACE_BEGIN(nanogui)

class NANOGUI_EXPORT Graph : public Widget {
public:
	Graph(Widget *parent, size_t graphNumber, const std::string &caption = "Untitled");
	~Graph();

	const std::string &caption() const { return mCaption; }
	void setCaption(const std::string &caption) { mCaption = caption; }

	const std::string &header() const { return mHeader; }
	void setHeader(const std::string &header) { mHeader = header; }

	const std::string &footer() const { return mFooter; }
	void setFooter(const std::string &footer) { mFooter = footer; }

	const Color &textColor() const { return mTextColor; }
	void setTextColor(const Color &textColor) { mTextColor = textColor; }

	const float* getPadding() const { return padding; }
	void setPadding(float left, float top, float right, float bottom);

	size_t graphNumber() const { return mGraphNumber; }
	size_t actualGraphNumber() { return mActualGraphNumber; }

	GraphElement * addGraph(GraphElement * graph) {
		if (mActualGraphNumber < mGraphNumber) {
			graphs[mActualGraphNumber] = graph;
			mActualGraphNumber++;
			return graph;
		}
		else {
			return nullptr;
		}
	}

	void removeGraphElement(size_t i) {
		if (i >= mActualGraphNumber) return;

		delete graphs[i];
		for (size_t j = i; j < (mActualGraphNumber - 1); j++) {
			graphs[j] = graphs[j + 1];
		}
		mActualGraphNumber--;
	}

	Plot * addPlot(size_t dataPoints, bool rewriting = false) { return (Plot*)addGraph(new Plot(dataPoints, rewriting)); }
	Plot * getPlot(size_t i) {
		if (i < mGraphNumber) {
			return (Plot*)graphs[i];
		}
		else {
			return nullptr;
		}
	}

	BarGraph * addBarGraph();

	BarGraph * getBarGraph(size_t index);

	BezierCurve * addBezierCurve() { return (BezierCurve*)addGraph(new BezierCurve()); }
	BezierCurve * getBezier(size_t i) {
		if (i < mGraphNumber) {
			return (BezierCurve*)graphs[i];
		}
		else {
			return nullptr;
		}
	}

	virtual void draw(NVGcontext *ctx) override;

	void setPlotBackgroundColor(Color value) { mPlotBackgroundColor = value; }

	void setPlotGridColor(Color value) { mPlotGridColor = value; }

	void setPlotGridWidth(float value) { mPlotGridWidth = value; }

	void setPlotBorderColor(Color value) { mPlotBorderColor = value; }

protected:
	std::string mCaption, mHeader, mFooter;
	Color mTextColor;
	GraphElement** graphs;
	size_t mGraphNumber;
	size_t mActualGraphNumber = 0;
	std::vector<BarGraph*> barPlots = std::vector<BarGraph*>();
	int barGraphNumber_ = 0;
	float padding[4] = { 50.f,25.f,50.f,50.f };
	Color mPlotBackgroundColor = Color(255, 255);
	float mPlotGridWidth = .5f;
	Color mPlotGridColor = Color(180, 255);
	Color mPlotBorderColor = Color(0, 255);
	float mPlotBorderWidth = 1.f;
	bool needsUpdate = true;
	bool saved = false;
};

NAMESPACE_END(nanogui)
