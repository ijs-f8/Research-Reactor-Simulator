#pragma once

#include <nanogui/widget.h>

NAMESPACE_BEGIN(nanogui)

class NANOGUI_EXPORT PieChart : public Widget
{
protected:
	bool isDrawingRelative = true;
	int drawGroups = 6;
	float borderWidth = 0.f;
	Color borderColor = Color(255, 255);
	Color dataColors[7] = { Color(255,255), Color(34,116,165,255), Color(247, 92, 3,255), Color(241, 196, 15, 255), Color(0, 204, 102, 255), Color(240, 58, 71, 255), Color(153, 0, 153, 255) };
	const size_t averageValues = 500;
public:
	
	PieChart(Widget *parent);
	~PieChart();

	float getBorderWidth() { return borderWidth; }
	Color* getColors() { return dataColors; }
	void setBorderWidth(float width) { borderWidth = width; }
	void setBorderColor(Color value) { borderColor = value; }
	void setDrawRelative(bool value) { isDrawingRelative = value; }
	/*void setData(Simulator* reactor) {
		mReactor = reactor;
	}*/

	virtual void draw(NVGcontext *ctx) override;
};

NAMESPACE_END(nanogui)
