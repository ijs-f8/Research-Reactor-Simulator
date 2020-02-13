#include <nanogui/pieChart.h>
#include <nanogui/opengl.h>

# define M_PI           3.14159265358979323846

NAMESPACE_BEGIN(nanogui)

PieChart::PieChart(Widget * parent) : Widget(parent) {
	mBackgroundColor = Color(20, 128);
}

PieChart::~PieChart()
{
}

void PieChart::draw(NVGcontext *ctx) {
	Widget::draw(ctx);
	float centerX = mPos.x() + mSize.x() / 2.f;
	float centerY = mPos.y() + mSize.y() / 2.f;
	float angle = 0.f;
	size_t lastIndex = 0/*mReactor->getCurrentIndex()*/;
	//lastIndex = (mReactor->getDataLength() > averageValues) ? mReactor->shiftIndex(lastIndex, -(int)averageValues) : (size_t)0;
	size_t cur = 0/*mReactor->getCurrentIndex()*/;
	double sumOfAll = 0./*isDrawingRelative ? mReactor->state_vector_[7][cur] : abs(mReactor->state_vector_[7][cur] - mReactor->state_vector_[7][lastIndex])*/;
	if (sumOfAll == 0.) return;
	for (size_t i = 7 - drawGroups; i < 7; i++) {
		nvgBeginPath(ctx);
		nvgMoveTo(ctx, centerX, centerY);
		float thisArcAngle = 2.f * M_PI * (isDrawingRelative ? 0:1/*mReactor->state_vector_[i][cur] : abs(mReactor->state_vector_[i][cur] - mReactor->state_vector_[i][lastIndex])*/) / sumOfAll;
		nvgArc(ctx, centerX, centerY, mSize.x() / 2.f, angle, angle - thisArcAngle, NVG_CCW);
		angle -= thisArcAngle;
		nvgClosePath(ctx);
		nvgFillColor(ctx, Color(dataColors[i][0], dataColors[i][1], dataColors[i][2], 1.f));
		nvgFill(ctx);
	}
	if (borderWidth > 0.f) {
		nvgBeginPath(ctx);
		nvgCircle(ctx, centerX, centerY, mSize.x() / 2.f);
		nvgStrokeColor(ctx, borderColor);
		nvgStrokeWidth(ctx, borderWidth);
		nvgStroke(ctx);
	}

}

NAMESPACE_END(nanogui)


