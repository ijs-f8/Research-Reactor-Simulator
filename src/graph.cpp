/*
    src/graph.cpp -- Simple graph widget for showing a function plot

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#include <nanogui/graph.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>

NAMESPACE_BEGIN(nanogui)

Graph::Graph(Widget *parent, size_t graphNumber, const std::string &caption)
    : Widget(parent), mCaption(caption) {
    mBackgroundColor = Color(20, 128);
    mTextColor = Color(240, 192);
	mGraphNumber = graphNumber;
	graphs = new GraphElement*[graphNumber];
}

Graph::~Graph()
{
	delete[] graphs;
}

void Graph::draw(NVGcontext *ctx) {
    Widget::draw(ctx);
	nvgSave(ctx);
	nvgFontFace(ctx, "sans");
	nvgIntersectScissor(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
	
	// Save commonly used values
	float graphRangeX = mSize.x() - padding[0] - padding[2];
	float graphRangeY = mSize.y() - padding[1] - padding[3];
	float diffX = graphRangeX / 10.f;
	float diffY = graphRangeY / 10.f;
	float xPos = mPos.x() + padding[0];
	float yPos = mPos.y() + padding[1];
	// Create the plot background
	nvgBeginPath(ctx);
	nvgRect(ctx, mPos.x() + padding[0], mPos.y() + padding[1], graphRangeX, graphRangeY);
	nvgFillColor(ctx, mPlotBackgroundColor);
	nvgFill(ctx);
	// Create the grid
	nvgStrokeColor(ctx, mPlotGridColor);
	nvgStrokeWidth(ctx, mPlotGridWidth);
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, xPos + diffX, yPos);
	for (short x = 1; x < 10; x++) {
		nvgLineTo(ctx, xPos + x*diffX, mPos.y() + mSize.y() - padding[3]);
		nvgMoveTo(ctx, xPos + (x + 1)*diffX, yPos);
	}
	nvgStroke(ctx);
	nvgBeginPath(ctx);
	nvgMoveTo(ctx, xPos, yPos + diffY);
	for (short y = 1; y < 10; y++) {
		nvgLineTo(ctx, mPos.x() + mSize.x() - padding[2], yPos + y*diffY);
		nvgMoveTo(ctx, xPos, yPos + (y + 1)*diffY);
	}
	nvgStroke(ctx);
	
	for (size_t index = 0; index < mActualGraphNumber; index++) {
		GraphElement * currentElement = graphs[index];
		if (currentElement->getEnabled()) {
			// Save values for log drawing (saves CPU time)
			double* limits = currentElement->limits();
			double* limLog = currentElement->logLimits();

			// Save current state
			nvgSave(ctx);
			// Set the bounds
			nvgIntersectScissor(ctx, xPos, yPos, graphRangeX, graphRangeY);

			float relFill = -1.f;
			float halfDraw = currentElement->getStrokeWidth()*.5f;
			if (currentElement->graphType()==GraphElement::GraphType::PlotDeque) {
				Plot* current = (Plot*)currentElement;
				if (current->getPlotRange() > 0) {
					size_t plotStartIndex = current->getPlotStart();
					// Start path
					nvgBeginPath(ctx);
					// Move to the first point
					float y1 = yPos + (1 - current->getYat(plotStartIndex)) * graphRangeY;
					nvgMoveTo(ctx, xPos - halfDraw, y1);
					nvgLineTo(ctx, xPos + graphRangeX * current->getXat(plotStartIndex), y1);

					switch (current->getDrawMode()) {
					case DrawMode::Default:
					{
						double vx, vy;
						for (size_t i = plotStartIndex + 1; i <= (size_t)current->getPlotEnd(); i++) {
							vx = xPos + graphRangeX * current->getXat(i);
							vy = yPos + (1 - current->getYat(i)) * graphRangeY;
							nvgLineTo(ctx, vx, vy);
						}
						break;
					}
					case DrawMode::Smart:
					{
						size_t pixels = (size_t)ceil(graphRangeX);
						size_t cap = current->getPlotEnd();
						size_t ceila, floora, rounda;
						double step = (double)current->getPlotRange() / (pixels * current->getPixelDrawRatio());
						double a = (double)plotStartIndex;
						double vx, vy;
						for (size_t i = 1; i < pixels; i++) {
							a += step;
							if (step >= 1.) {
								rounda = std::min(cap, (size_t)round(a));
								nvgLineTo(ctx, xPos + graphRangeX * current->getXat(rounda), yPos + (1 - current->getYat(rounda)) * graphRangeY);
							}
							else {
								ceila = std::min(cap, (size_t)ceil(a));
								floora = (size_t)floor(a);
								if (ceila == floora) {
									nvgLineTo(ctx, xPos + graphRangeX * current->getXat(floora), yPos + (1 - current->getYat(floora)) * graphRangeY);
								}
								else {
									// Linear interpolation
									vx = current->getXat(floora) * (ceila - a) + current->getXat(ceila) * (a - floora);
									vy = current->getYat(floora) * (ceila - a) + current->getYat(ceila) * (a - floora);

									vx = vx*graphRangeX + xPos;
									vy = (1 - vy)*graphRangeY + yPos;
									nvgLineTo(ctx, vx, vy);
								}
							}
						}
						// Last Y
						float yLast = yPos + (1 - current->getYat(current->getPlotEnd()))*graphRangeY;
						// Draw last line
						nvgLineTo(ctx, xPos + graphRangeX, yLast);
						// Extend the line by w/2 to prevent drawing on te surface
						nvgLineTo(ctx, xPos + graphRangeX + halfDraw, yLast);
						break;
					}
					case DrawMode::SuperSmart:
					{
						double lastValueY = current->getYat(plotStartIndex);
						double lastValueX = current->getXat(plotStartIndex);
						double range = (double)current->getPlotRange();
						double adaptiveStep = range / graphRangeX;
						for (double a = (double)plotStartIndex + 1; a <= (double)current->getPlotEnd(); a += adaptiveStep) {
							// Linear fit
							size_t ceila = (size_t)ceil(a);
							size_t floora = (size_t)floor(a);
							double thisValueY;
							double thisValueX;
							if (ceila == floora) {
								// If the ceiling and floor of "a" are the same, use one of them
								thisValueY = current->getYat(floora);
								thisValueX = current->getXat(floora);
								nvgLineTo(ctx, xPos + graphRangeX * thisValueX, yPos + (1 - thisValueY) * graphRangeY);
							}
							else {
								// If the ceiling and floor of "a" are not equal, linear interpolate
								thisValueX = current->getXat(floora) * (ceila - a) + current->getXat(ceila) * (a - floora);
								thisValueY = current->getYat(floora) * (ceila - a) + current->getYat(ceila) * (a - floora);
								nvgLineTo(ctx, thisValueX*graphRangeX + xPos, (1 - thisValueY)*graphRangeY + yPos);
							}
							// k factor of a line connecting the two points
							double deviation = std::max(abs(thisValueY - lastValueY) / (thisValueX - lastValueX), 0.1);
							// Update last values
							lastValueX = thisValueX;
							lastValueY = thisValueY;
							adaptiveStep = exp(-deviation - 3.)*range / 80. + 1.;
						}
						// Last Y
						float yLast = yPos + (1 - current->getYat(current->getPlotEnd()))*graphRangeY;
						// Draw last line
						nvgLineTo(ctx, xPos + graphRangeX, yLast);
						// Extend the line by w/2 to prevent drawing on te surface
						nvgLineTo(ctx, xPos + graphRangeX + halfDraw, yLast);
						break;
					}

					}
				}
			}
			else if (currentElement->graphType() == GraphElement::GraphType::Bezier) {
				BezierCurve* current = (BezierCurve*)currentElement;
				nvgBeginPath(ctx);
				nvgMoveTo(ctx, xPos, yPos + graphRangeY);
				nvgBezierTo(ctx, xPos + current->getParameter(0) * graphRangeX, yPos + (1 - current->getParameter(1))*graphRangeY,
					xPos + current->getParameter(2) * graphRangeX, yPos + (1 - current->getParameter(3))*graphRangeY, xPos + graphRangeX, yPos);
				nvgLineTo(ctx, xPos + graphRangeX + halfDraw, yPos);
			}
			if (currentElement->getHorizontalPointerShown()) relFill = currentElement->getHorizontalPointerPosition();
			
			// If the space under the curve should be filled, close the path
			if (currentElement->isFill()) {
				nvgSave(ctx);
				// Straight down from the current point (width + s/2, -s/2)
				nvgLineTo(ctx, xPos + graphRangeX + halfDraw, yPos + graphRangeY + halfDraw);
				// To point (-s/2, -s/2)
				nvgLineTo(ctx, xPos - halfDraw, yPos + graphRangeY + halfDraw);
				// To first point on graph
				nvgClosePath(ctx);
				if (relFill >= 0.f) {
					// Save state
					nvgSave(ctx);
					if (currentElement->graphType() == GraphElement::GraphType::Bezier) {
						// Overflow part
						nvgIntersectScissor(ctx, xPos, yPos, relFill * graphRangeX, (1 - currentElement->getPointerPosition())*graphRangeY);
						// Fill
						nvgFillColor(ctx, ((BezierCurve*)currentElement)->overFillColor());
						nvgFill(ctx);
						nvgRestore(ctx);
						nvgSave(ctx);
						// Rod part
						nvgIntersectScissor(ctx, xPos, yPos + (1 - currentElement->getPointerPosition())*graphRangeY, relFill * graphRangeX, currentElement->getPointerPosition()*graphRangeY);
					}
					else {
						// Other plot elements
						nvgIntersectScissor(ctx, xPos, yPos, relFill * graphRangeX, graphRangeY);
					}
					// Fill
					nvgFillColor(ctx, currentElement->getFillColor());
					nvgFill(ctx);
					nvgRestore(ctx);
				}
				else {
					// Fill
					nvgFillColor(ctx, currentElement->getFillColor());
					nvgFill(ctx);
				}
				nvgRestore(ctx);
			}

			// Paint the line
			nvgStrokeWidth(ctx, currentElement->getStrokeWidth());
			nvgStrokeColor(ctx, currentElement->getColor());
			nvgStroke(ctx);

			// Restore previous state
			nvgRestore(ctx);

			/* VERTICAL AXIS */
			float new_xPos = xPos - currentElement->getAxisOffset();
			int direction = 1;
			if (currentElement->getAxisPosition() == GraphElement::AxisLocation::Right) {
				new_xPos = xPos + graphRangeX + currentElement->getAxisOffset();
				direction = -1;
			}
			if (currentElement->getAxisShown()) {
				nvgStrokeColor(ctx, currentElement->getAxisColor());

				// Draw main line if neccessary
				if (currentElement->getMainLineShown()) {
					nvgStrokeWidth(ctx, currentElement->getAxisWidth());
					nvgBeginPath(ctx);
					nvgMoveTo(ctx, new_xPos, yPos);
					nvgLineTo(ctx, new_xPos, yPos + graphRangeY);
					nvgStroke(ctx);
				}

				// Set alignment for numbers
				if (currentElement->getTextShown()) {
					int textAlignment = NVGalign::NVG_ALIGN_MIDDLE;
					if (direction == 1) {
						textAlignment = textAlignment | NVGalign::NVG_ALIGN_RIGHT;
					}
					else {
						textAlignment = textAlignment | NVGalign::NVG_ALIGN_LEFT;
					}
					nvgTextAlign(ctx, textAlignment);
					nvgFillColor(ctx, currentElement->getTextColor());
				}

				// Draw major ticks and text
				nvgStrokeWidth(ctx, currentElement->getMajorTickWidth());
				nvgBeginPath(ctx);
				nvgMoveTo(ctx, new_xPos, yPos);
				nvgLineTo(ctx, new_xPos - direction*currentElement->getMajorTickSize(), yPos);
				if (currentElement->getTextShown()) {
					string content = currentElement->getOverridenLimits()[3].length() ? currentElement->getOverridenLimits()[3] : currentElement->formatNumber(limits[3] * currentElement->getLimitMultiplier());
					nvgFontSize(ctx, currentElement->getMainTickFontSize());
					nvgText(ctx, new_xPos - direction*(currentElement->getMajorTickSize() + 3), yPos,
						content.c_str(), NULL);
				}
				if (currentElement->getYlog()) {
					for (double i = floor(limLog[2]) + 1; i < ceil(limLog[3]); i++) {
						float thisY = yPos + graphRangeY * ( 1. - ( ( i - limLog[2] ) / ( limLog[3] - limLog[2] ) ) );
						nvgMoveTo(ctx, new_xPos, thisY);
						nvgLineTo(ctx, new_xPos - direction*currentElement->getMajorTickSize(), thisY);
						if (currentElement->getTextShown()) {
							nvgFontSize(ctx, currentElement->getMajorTickFontSize());
							nvgText(ctx, new_xPos - direction*(currentElement->getMajorTickSize() + 3), thisY, ("1e" + std::to_string((int)i)).c_str(), NULL);
						}
					}
				}
				else {
					if (currentElement->getMajorTickNumber() != 0) {
						float tickDiff = graphRangeY / (currentElement->getMajorTickNumber() + 1);
						for (size_t i = 1; i <= currentElement->getMajorTickNumber(); i++) {
							float thisY = yPos + i*tickDiff;
							nvgMoveTo(ctx, new_xPos, thisY);
							nvgLineTo(ctx, new_xPos - direction*currentElement->getMajorTickSize(), thisY);
							if (currentElement->getTextShown()) {
								nvgFontSize(ctx, currentElement->getMajorTickFontSize());
								nvgText(ctx, new_xPos - direction*(currentElement->getMajorTickSize() + 3), thisY,
									currentElement->formatNumber((limits[2] + (currentElement->getMajorTickNumber() + 1 - i)*(limits[3] - limits[2]) / (currentElement->getMajorTickNumber() + 1)) * currentElement->getLimitMultiplier()).c_str(), NULL);
							}
						}
					}
				}
				nvgMoveTo(ctx, new_xPos, yPos + graphRangeY);
				nvgLineTo(ctx, new_xPos - direction*currentElement->getMajorTickSize(), yPos + graphRangeY);
				if (currentElement->getTextShown()) {
					string content = currentElement->getOverridenLimits()[2].length() ? currentElement->getOverridenLimits()[2] : currentElement->formatNumber(limits[2] * currentElement->getLimitMultiplier());
					nvgFontSize(ctx, currentElement->getMainTickFontSize());
					nvgText(ctx, new_xPos - direction*(currentElement->getMajorTickSize() + 3), yPos + graphRangeY,
						content.c_str(), NULL);
				}
				nvgStroke(ctx);

				// Draw minor ticks
				if (currentElement->getYlog()) {
					nvgStrokeWidth(ctx, currentElement->getMinorTickWidth());
					nvgBeginPath(ctx);
					double upperLim = ceil(limLog[3]);
					double holder;
					float yHolder;
					for (double i = floor(limLog[2]); i < upperLim; i++) {
						for (int step = 2; step < 10; step++) {
							holder = pow(10., i)*step;
							if (holder > limits[2] && holder < limits[3]) {
								yHolder = yPos + graphRangeY * (1. - ((log10(holder) - limLog[2]) / (limLog[3] - limLog[2])));
								nvgMoveTo(ctx, new_xPos, yHolder);
								nvgLineTo(ctx, new_xPos - direction*currentElement->getMinorTickSize(), yHolder);
							}
							else {
								continue;
							}
						}
					}
					nvgStroke(ctx);
				}
				else {
					if (currentElement->getMinorTickNumber() != 0) {
						float tickDiffMajor = graphRangeY / (currentElement->getMajorTickNumber() + 1);
						float tickDiffMinor = graphRangeY / ((currentElement->getMajorTickNumber() + 1) * (currentElement->getMinorTickNumber() + 1));
						nvgStrokeWidth(ctx, currentElement->getMinorTickWidth());
						nvgBeginPath(ctx);
						for (size_t major = 0; major < currentElement->getMajorTickNumber() + 1; major++) {
							for (size_t i = 1; i <= currentElement->getMinorTickNumber(); i++) {
								float thisY = yPos + major*tickDiffMajor + i*tickDiffMinor;
								nvgMoveTo(ctx, new_xPos, thisY);
								nvgLineTo(ctx, new_xPos - direction*currentElement->getMinorTickSize(), thisY);
							}
						}
						nvgStroke(ctx);
					}
				}

				if (currentElement->getTextShown()) {
					nvgFontSize(ctx, currentElement->getNameFontSize());
					nvgFillColor(ctx, currentElement->getTextColor());
					nvgTextAlign(ctx, NVG_ALIGN_BOTTOM | NVG_ALIGN_CENTER);

					string content = currentElement->getName();
					if (currentElement->getUnits().length() > 0) content += " [" + currentElement->getUnits() + "]";
					float title_x = new_xPos - direction*(currentElement->getMajorTickSize() + currentElement->getTextOffset());
					float title_y = yPos + graphRangeY / 2.f;
					nvgTranslate(ctx, title_x, title_y);
					nvgRotate(ctx, -direction * NVG_PI / 2.f);
					nvgText(ctx, 0, 0, content.c_str(), NULL);
					nvgRotate(ctx, direction * NVG_PI / 2.f);
					nvgTranslate(ctx, -title_x, -title_y);
				}
			}

			/* HORIZONTAL AXIS */
			float new_yPos = yPos + graphRangeY + currentElement->getHorizontalAxisOffset();
			direction = 1;
			if (currentElement->getHorizontalAxisPosition() == GraphElement::HorizontalAxisLocation::Top) {
				new_yPos = yPos - currentElement->getHorizontalAxisOffset();
				direction = -1;
			}
			if (currentElement->getHorizontalAxisShown()) {
				nvgStrokeColor(ctx, currentElement->getAxisColor());

				// Draw main line if neccessary
				if (currentElement->getHorizontalMainLineShown()) {
					nvgStrokeWidth(ctx, currentElement->getHorizontalAxisWidth());
					nvgBeginPath(ctx);
					nvgMoveTo(ctx, xPos, new_yPos);
					nvgLineTo(ctx, xPos + graphRangeX, new_yPos);
					nvgStroke(ctx);
				}

				// Set alignment for numbers
				if (currentElement->getTextShown()) {
					int textAlignment = NVGalign::NVG_ALIGN_CENTER;
					if (direction == 1) {
						textAlignment = textAlignment | NVGalign::NVG_ALIGN_TOP;
					}
					else {
						textAlignment = textAlignment | NVGalign::NVG_ALIGN_BOTTOM;
					}
					nvgTextAlign(ctx, textAlignment);
					nvgFillColor(ctx, currentElement->getTextColor());
				}

				// Draw major ticks and text
				nvgStrokeWidth(ctx, currentElement->getMajorTickWidth());
				nvgBeginPath(ctx);
				nvgMoveTo(ctx, xPos, new_yPos);
				nvgLineTo(ctx, xPos, new_yPos + direction*currentElement->getMajorTickSize());
				if (currentElement->getTextShown()) {
					string content = currentElement->getOverridenLimits()[0].length() ? currentElement->getOverridenLimits()[0] : currentElement->formatNumber(limits[0] * currentElement->getLimitMultiplier());
					nvgFontSize(ctx, currentElement->getMainTickFontSize());
					nvgText(ctx, xPos, new_yPos + direction*(currentElement->getMajorTickSize() + 3),
						content.c_str(), NULL);
				}
				if (currentElement->getHorizontalMajorTickNumber() != 0) {
					float tickDiff = graphRangeX / (currentElement->getHorizontalMajorTickNumber() + 1);
					for (size_t i = 1; i <= currentElement->getHorizontalMajorTickNumber(); i++) {
						float thisX = xPos + i*tickDiff;
						nvgMoveTo(ctx, thisX, new_yPos);
						nvgLineTo(ctx, thisX, new_yPos + direction*currentElement->getMajorTickSize());
						if (currentElement->getTextShown()) {
							nvgFontSize(ctx, currentElement->getMajorTickFontSize());
							nvgText(ctx, thisX, new_yPos + direction*(currentElement->getMajorTickSize() + 3),
								currentElement->formatNumber((currentElement->limits()[0] + i*(limits[1] - limits[0]) / (currentElement->getHorizontalMajorTickNumber() + 1)) * currentElement->getLimitHorizontalMultiplier()).c_str(), NULL);
						}
					}
				}
				nvgMoveTo(ctx, xPos + graphRangeX, new_yPos);
				nvgLineTo(ctx, xPos + graphRangeX, new_yPos + direction*currentElement->getMajorTickSize());
				if (currentElement->getTextShown()) {
					string content = currentElement->getOverridenLimits()[1].length() ? currentElement->getOverridenLimits()[1] : currentElement->formatNumber(limits[1] * currentElement->getLimitHorizontalMultiplier());
					nvgFontSize(ctx, currentElement->getMainTickFontSize());
					nvgText(ctx, xPos + graphRangeX, new_yPos + direction*(currentElement->getMajorTickSize() + 3),
						content.c_str(), NULL);
				}
				nvgStroke(ctx);

				// Draw minor ticks
				if (currentElement->getHorizontalMinorTickNumber() != 0) {
					float tickDiffMajor = graphRangeX / (currentElement->getHorizontalMajorTickNumber() + 1);
					float tickDiffMinor = graphRangeX / ((currentElement->getHorizontalMajorTickNumber() + 1) * (currentElement->getHorizontalMinorTickNumber() + 1));
					nvgStrokeWidth(ctx, currentElement->getMinorTickWidth());
					nvgBeginPath(ctx);
					for (size_t major = 0; major < currentElement->getHorizontalMajorTickNumber() + 1; major++) {
						for (size_t i = 1; i <= currentElement->getHorizontalMinorTickNumber(); i++) {
							float thisX = xPos + major*tickDiffMajor + i*tickDiffMinor;
							nvgMoveTo(ctx, thisX, new_yPos);
							nvgLineTo(ctx, thisX, new_yPos + direction*currentElement->getMinorTickSize());
						}
					}
					nvgStroke(ctx);
				}

				if (currentElement->getTextShown()) {
					nvgFontSize(ctx, currentElement->getNameFontSize());
					nvgFillColor(ctx, currentElement->getTextColor());
					nvgTextAlign(ctx, NVG_ALIGN_BOTTOM | NVG_ALIGN_CENTER);

					string content = currentElement->getHorizontalName();
					if (currentElement->getHorizontalUnits().length() > 0) content += " [" + currentElement->getHorizontalUnits() + "]";
					nvgText(ctx, xPos + graphRangeX/2.f, new_yPos + direction*(currentElement->getMajorTickSize() + currentElement->getHorizontalTextOffset()), 
						content.c_str(), NULL);
				}

			}
		}
	}

	// Draw pointers on a new layer
	for (size_t index = 0; index < mActualGraphNumber; index++) {
		GraphElement * current = graphs[index];
		if (current->getEnabled()) {
			// Draw vertical pointer
			if (current->getPointerShown()) {
				// Sadly, we have to calculate the axis variables again
				float new_xPos = xPos - current->getAxisOffset();
				int direction = 1;
				if (current->getAxisPosition() == GraphElement::AxisLocation::Right) {
					new_xPos = xPos + graphRangeX + current->getAxisOffset();
					direction = -1;
				}
				float finalY;
				if (current->getPointerOverride()) {
					finalY = current->getPointerPosition();
				}
				else {
					if (current->graphType() == 1) {
						finalY = (float)(((Plot*)current)->getYat(((Plot*)current)->getPlotEnd()));
					}
					else {
						finalY = 0.f;
					}
				}

				// Actual draw
				nvgFillColor(ctx, current->getPointerColor());
				nvgBeginPath(ctx);
				float pointerSize = current->getPointerSize();
				float triMove = direction*(float)sqrt(3)*current->getPointerSize() / 2.f;
				if (finalY < 0.f) {
					nvgMoveTo(ctx, new_xPos - direction*pointerSize / 2.f, yPos + graphRangeY + abs(triMove) + current->getMajorTickWidth() / 2.f);
					nvgLineTo(ctx, new_xPos, yPos + graphRangeY + current->getMajorTickWidth() / 2.f);
					nvgLineTo(ctx, new_xPos - direction*pointerSize, yPos + graphRangeY + current->getMajorTickWidth() / 2.f);
				}
				else if (finalY > 1.f) {
					nvgMoveTo(ctx, new_xPos - direction*pointerSize / 2.f, yPos - abs(triMove) - current->getMajorTickWidth() / 2.f);
					nvgLineTo(ctx, new_xPos, yPos - current->getMajorTickWidth() / 2.f);
					nvgLineTo(ctx, new_xPos - direction*pointerSize, yPos - current->getMajorTickWidth() / 2.f);
				}
				else {
					finalY = (1 - finalY)*graphRangeY + yPos;
					nvgMoveTo(ctx, new_xPos, finalY);
					nvgLineTo(ctx, new_xPos - triMove, finalY - pointerSize / 2.f);
					nvgLineTo(ctx, new_xPos - triMove, finalY + pointerSize / 2.f);
				}
				nvgClosePath(ctx);
				nvgFill(ctx);
			}

			// Draw horizontal pointer
			if (current->getHorizontalPointerShown()) {
				// Calculating axis values
				float new_yPos = yPos + graphRangeY + current->getHorizontalAxisOffset();
				int direction = 1;
				if (current->getHorizontalAxisPosition() == GraphElement::HorizontalAxisLocation::Top) {
					new_yPos = yPos - current->getHorizontalAxisOffset();
					direction = -1;
				}
				float finalX = current->getHorizontalPointerPosition();

				nvgFillColor(ctx, current->getHorizontalPointerColor());
				nvgBeginPath(ctx);
				float pointerSize = current->getHorizontalPointerSize();
				float triMove = direction*(float)sqrt(3)*pointerSize / 2.f;
				if (finalX < 0.f) {
					nvgMoveTo(ctx, xPos - current->getMajorTickWidth() / 2.f, new_yPos);
					nvgLineTo(ctx, xPos - abs(triMove) - current->getMajorTickWidth() / 2.f, new_yPos + direction*current->getHorizontalPointerSize() / 2.f);
					nvgLineTo(ctx, xPos - current->getMajorTickWidth() / 2.f, new_yPos + direction*current->getHorizontalPointerSize());
				}
				else if (finalX > 1.f) {
					nvgMoveTo(ctx, xPos + graphRangeX + current->getMajorTickWidth() / 2.f, new_yPos);
					nvgLineTo(ctx, xPos + graphRangeX + abs(triMove) + current->getMajorTickWidth() / 2.f, new_yPos + direction*current->getHorizontalPointerSize() / 2.f);
					nvgLineTo(ctx, xPos + graphRangeX + current->getMajorTickWidth() / 2.f, new_yPos + direction*current->getHorizontalPointerSize());
				}
				else {
					finalX = xPos + finalX*graphRangeX;
					nvgMoveTo(ctx, finalX, new_yPos);
					nvgLineTo(ctx, finalX + direction*pointerSize / 2.f, new_yPos + triMove);
					nvgLineTo(ctx, finalX - direction*pointerSize / 2.f, new_yPos + triMove);
				}
				nvgClosePath(ctx);
				nvgFill(ctx);
			}
		}
		
	}

	//Draw bar graphs
	for (size_t plotIndex = 0; plotIndex < (size_t)barGraphNumber_; plotIndex++) {
		BarGraph* current = barPlots[plotIndex];
		size_t* range = current->getRange();
		float spacing = graphRangeX / (2.f*(range[1] - range[0] + 1) + 1.f);
		// Set outline width
		nvgStrokeWidth(ctx, current->getOutlineWidth());
		current->updateAutoscale();
		for (size_t i = range[0]; i <= range[1]; i++) {
			// Create the rectangle
			nvgBeginPath(ctx);
			nvgRect(ctx, xPos + (2 * (i - range[0]) + 1) * spacing, yPos + (1 - current->getY(i))*graphRangeY, spacing, current->getY(i)*graphRangeY);
			nvgStrokeColor(ctx, current->getLineColor(i));
			nvgFillColor(ctx, current->getFillColor(i));
			nvgFill(ctx);
			nvgStroke(ctx);
		}
	}

	// Create the graph border
	nvgBeginPath(ctx);
	nvgRect(ctx, xPos, yPos, graphRangeX, graphRangeY);
	nvgStrokeColor(ctx, mPlotBorderColor);
	nvgStrokeWidth(ctx, mPlotBorderWidth);
	nvgStroke(ctx);

    if (!mCaption.empty()) {
        nvgFontSize(ctx, 14.0f);
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(ctx, mTextColor);
        nvgText(ctx, xPos + 4, yPos, mCaption.c_str(), NULL);
    }

    if (!mHeader.empty()) {
        nvgFontSize(ctx, 18.0f);
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
        nvgFillColor(ctx, mTextColor);
        nvgText(ctx, mPos.x() + mSize.x() - 3, mPos.y() + 1, mHeader.c_str(), NULL);
    }

    if (!mFooter.empty()) {
        nvgFontSize(ctx, 15.0f);
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        nvgFillColor(ctx, mTextColor);
        nvgText(ctx, mPos.x() + mSize.x() - 3, mPos.y() + mSize.y() - 1, mFooter.c_str(), NULL);
    }
	
	nvgRestore(ctx);
}

void Graph::setPadding(float left, float top, float right, float bottom) {
	padding[0] = left;
	padding[1] = top;
	padding[2] = right;
	padding[3] = bottom;
}

BarGraph * Graph::addBarGraph()
{
	barPlots.push_back(new BarGraph());
	return barPlots[barGraphNumber_++];
}

BarGraph * Graph::getBarGraph(size_t index)
{
	return barPlots[index];
}

NAMESPACE_END(nanogui)
