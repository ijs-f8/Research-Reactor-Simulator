/*
    nanogui/layout.h -- A collection of useful layout managers

    The grid layout was contributed by Christian Schueller.

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <nanogui/compat.h>
#include <nanogui/object.h>
#include <unordered_map>

NAMESPACE_BEGIN(nanogui)

enum class Alignment : uint8_t {
    Minimum = 0,
    Middle,
    Maximum,
    Fill
};

enum class Orientation {
    Horizontal = 0,
    Vertical
};

/// Basic interface of a layout engine
class NANOGUI_EXPORT Layout : public Object {
public:
    virtual void performLayout(NVGcontext *ctx, Widget *widget) const = 0;
    virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const = 0;
protected:
    virtual ~Layout() { }
};

/**
 * \brief Simple horizontal/vertical box layout
 *
 * This widget stacks up a bunch of widgets horizontally or vertically. It adds
 * margins around the entire container and a custom spacing between adjacent
 * widgets
 */
class NANOGUI_EXPORT BoxLayout : public Layout {
public:
    /**
     * \brief Construct a box layout which packs widgets in the given \c orientation
     * \param alignment
     *    Widget alignment perpendicular to the chosen orientation.
     * \param margin
     *    Margin around the layout container
     * \param spacing
     *    Extra spacing placed between widgets
     */
    BoxLayout(Orientation orientation, Alignment alignment = Alignment::Middle,
              int margin = 0, int spacing = 0);

    Orientation orientation() const { return mOrientation; }
    void setOrientation(Orientation orientation) { mOrientation = orientation; }

    Alignment alignment() const { return mAlignment; }
    void setAlignment(Alignment alignment) { mAlignment = alignment; }

    int margin() const { return mMargin; }
    void setMargin(int margin) { mMargin = margin; }

    int spacing() const { return mSpacing; }
    void setSpacing(int spacing) { mSpacing = spacing; }

    /* Implementation of the layout interface */
    virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const override;
    virtual void performLayout(NVGcontext *ctx, Widget *widget) const override;

protected:
    Orientation mOrientation;
    Alignment mAlignment;
    int mMargin;
    int mSpacing;
};

/**
 * \brief Special layout for widgets grouped by labels
 *
 * This widget resembles a box layout in that it arranges a set of widgets
 * vertically. All widgets are indented on the horizontal axis except for
 * \ref Label widgets, which are not indented.
 *
 * This creates a pleasing layout where a number of widgets are grouped
 * under some high-level heading.
 */
class NANOGUI_EXPORT GroupLayout : public Layout {
public:
    GroupLayout(int margin = 15, int spacing = 6, int groupSpacing = 14,
                int groupIndent = 20)
        : mMargin(margin), mSpacing(spacing), mGroupSpacing(groupSpacing),
          mGroupIndent(groupIndent) {}

    int margin() const { return mMargin; }
    void setMargin(int margin) { mMargin = margin; }

    int spacing() const { return mSpacing; }
    void setSpacing(int spacing) { mSpacing = spacing; }

    int groupIndent() const { return mGroupIndent; }
    void setGroupIndent(int groupIndent) { mGroupIndent = groupIndent; }

    int groupSpacing() const { return mGroupSpacing; }
    void setGroupSpacing(int groupSpacing) { mGroupSpacing = groupSpacing; }

    /* Implementation of the layout interface */
    virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const override;
    virtual void performLayout(NVGcontext *ctx, Widget *widget) const override;

protected:
    int mMargin;
    int mSpacing;
    int mGroupSpacing;
    int mGroupIndent;
};

/**
 * \brief Grid layout
 *
 * Widgets are arranged in a grid that has a fixed grid resolution \c resolution
 * along one of the axes. The layout orientation indicates the fixed dimension;
 * widgets are also appended on this axis. The spacing between items can be
 * specified per axis. The horizontal/vertical alignment can be specified per
 * row and column.
 */
class NANOGUI_EXPORT GridLayout : public Layout {
public:
    /// Create a 2-column grid layout by default
    GridLayout(Orientation orientation = Orientation::Horizontal, int resolution = 2,
               Alignment alignment = Alignment::Middle,
               int margin = 0, int spacing = 0)
        : mOrientation(orientation), mResolution(resolution), mMargin(margin) {
        mDefaultAlignment[0] = mDefaultAlignment[1] = alignment;
        mSpacing = Vector2i::Constant(spacing);
    }

    Orientation orientation() const { return mOrientation; }
    void setOrientation(Orientation orientation) {
        mOrientation = orientation;
    }

    int resolution() const { return mResolution; }
    void setResolution(int resolution) { mResolution = resolution; }

    int spacing(int axis) const { return mSpacing[axis]; }
    void setSpacing(int axis, int spacing) { mSpacing[axis] = spacing; }
    void setSpacing(int spacing) { mSpacing[0] = mSpacing[1] = spacing; }

    int margin() const { return mMargin; }
    void setMargin(int margin) { mMargin = margin; }

    Alignment alignment(int axis, int item) const {
        if (item < (int) mAlignment[axis].size())
            return mAlignment[axis][item];
        else
            return mDefaultAlignment[axis];
    }
    void setColAlignment(Alignment value) { mDefaultAlignment[0] = value; }
    void setRowAlignment(Alignment value) { mDefaultAlignment[1] = value; }
    void setColAlignment(const std::vector<Alignment> &value) { mAlignment[0] = value; }
    void setRowAlignment(const std::vector<Alignment> &value) { mAlignment[1] = value; }

    /* Implementation of the layout interface */
    virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const override;
    virtual void performLayout(NVGcontext *ctx, Widget *widget) const override;

protected:
    // Compute the maximum row and column sizes
    void computeLayout(NVGcontext *ctx, const Widget *widget,
                       std::vector<int> *grid) const;

protected:
    Orientation mOrientation;
    Alignment mDefaultAlignment[2];
    std::vector<Alignment> mAlignment[2];
    int mResolution;
    Vector2i mSpacing;
    int mMargin;
};

/**
 * \brief Advanced Grid layout
 *
 * The is a fancier grid layout with support for items that span multiple rows
 * or columns, and per-widget alignment flags. Each row and column additionally
 * stores a stretch factor that controls how additional space is redistributed.
 * The downside of this flexibility is that a layout anchor data structure must
 * be provided for each widget.
 *
 * An example:
 *
 * <pre>
 *   using AdvancedGridLayout::Anchor;
 *   Label *label = new Label(window, "A label");
 *   // Add a centered label at grid position (1, 5), which spans two horizontal cells
 *   layout->setAnchor(label, Anchor(1, 5, 2, 1, Alignment::Middle, Alignment::Middle));
 * </pre>
 *
 * The grid is initialized with user-specified column and row size vectors
 * (which can be expanded later on if desired). If a size value of zero is
 * specified for a column or row, the size is set to the maximum preferred size
 * of any widgets contained in the same row or column. Any remaining space is
 * redistributed according to the row and column stretch factors.
 *
 * The high level usage somewhat resembles the classic HIG layout:
 * https://web.archive.org/web/20070813221705/http://www.autel.cz/dmi/tutorial.html
 * https://github.com/jaapgeurts/higlayout
 */
class NANOGUI_EXPORT AdvancedGridLayout : public Layout {
public:
    struct Anchor {
        uint8_t pos[2];
        uint8_t size[2];
        Alignment align[2];

        Anchor() { }

        Anchor(int x, int y, Alignment horiz = Alignment::Fill,
              Alignment vert = Alignment::Fill) {
            pos[0] = (uint8_t) x; pos[1] = (uint8_t) y;
            size[0] = size[1] = 1;
            align[0] = horiz; align[1] = vert;
        }

        Anchor(int x, int y, int w, int h,
              Alignment horiz = Alignment::Fill,
              Alignment vert = Alignment::Fill) {
            pos[0] = (uint8_t) x; pos[1] = (uint8_t) y;
            size[0] = (uint8_t) w; size[1] = (uint8_t) h;
            align[0] = horiz; align[1] = vert;
        }

        operator std::string() const {
            char buf[50];
			buf[0] = '\0';
            NANOGUI_SNPRINTF(buf, 50, "Format[pos=(%i, %i), size=(%i, %i), align=(%i, %i)]",
                pos[0], pos[1], size[0], size[1], (int) align[0], (int) align[1]);
            return buf;
        }
    };

    AdvancedGridLayout(const std::vector<int> &cols = {}, const std::vector<int> &rows = {});

    int margin() const { return mMargin; }
    void setMargin(int margin) { mMargin = margin; }

    /// Return the number of cols
    int colCount() const { return (int) mCols.size(); }

    /// Return the number of rows
    int rowCount() const { return (int) mRows.size(); }

    /// Append a row of the given size (and stretch factor)
    void appendRow(int size, float stretch = 0.f) { mRows.push_back(size); mRowStretch.push_back(stretch); };

    /// Append a column of the given size (and stretch factor)
    void appendCol(int size, float stretch = 0.f) { mCols.push_back(size); mColStretch.push_back(stretch); };

    /// Set the stretch factor of a given row
    void setRowStretch(int index, float stretch) { mRowStretch.at(index) = stretch; }

    /// Set the stretch factor of a given column
    void setColStretch(int index, float stretch) { mColStretch.at(index) = stretch; }

    /// Specify the anchor data structure for a given widget
    void setAnchor(const Widget *widget, const Anchor &anchor) { mAnchor[widget] = anchor; }

    /// Retrieve the anchor data structure for a given widget
    Anchor anchor(const Widget *widget) const {
        auto it = mAnchor.find(widget);
        if (it == mAnchor.end())
            throw std::runtime_error("Widget was not registered with the grid layout!");
        return it->second;
    }

    /* Implementation of the layout interface */
    virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const override;
    virtual void performLayout(NVGcontext *ctx, Widget *widget) const override;

protected:
    void computeLayout(NVGcontext *ctx, const Widget *widget,
                       std::vector<int> *grid) const;

protected:
    std::vector<int> mCols, mRows;
    std::vector<float> mColStretch, mRowStretch;
    std::unordered_map<const Widget *, Anchor> mAnchor;
    int mMargin;
};

class NANOGUI_EXPORT RelativeGridLayout : public Layout {
public:
	enum FillMode {
		Always,
		IfLess,
		IfMore
	};
	enum SizeType {
		Fixed,
		Relative
	};
	struct Anchor {
		int pos[2] = { 0, 0 };
		int span[2] = { 1, 1 };
		Vector4i padding;
		FillMode expansion[2] = { FillMode::Always, FillMode::Always };
		Alignment alignment[2] = { Alignment::Fill, Alignment::Fill };

		Anchor() {
			padding = Vector4i(0, 0, 0, 0);
		}

		Anchor(int x, int y) : Anchor(){
			pos[0] = x;
			pos[1] = y;
		}

		Anchor(int x, int y, int w, int h) : Anchor(x, y) {
			span[0] = w;
			span[1] = h;
		}

		Anchor(int x, int y, int w, int h, Vector4i padding_) : Anchor(x, y, w, h) {
			padding = padding_;
		}

		void setAlignment(Alignment alignX, Alignment alignY) {
			alignment[0] = alignX;
			alignment[1] = alignY;
		}

		void setFillMode(FillMode modeX, FillMode modeY) {
			expansion[0] = modeX;
			expansion[1] = modeY;
		}

	};
	struct Size {
	protected:
		float value_;
		SizeType type_;
	public:
		Size(float value = 1.f, SizeType type = SizeType::Relative) { value_ = value; type_ = type; }
		const float &value() const { return value_; }
		const SizeType &type() const { return type_; }
	};

	RelativeGridLayout() { mTable[0] = std::vector<Size>(); mTable[1] = std::vector<Size>(); };

	int margin() const { return mMargin; }
	void setMargin(int margin) { mMargin = margin; }

	/// Return the number of cols
	int colCount() const { return (int)mTable[0].size(); }

	/// Return the number of rows
	int rowCount() const { return (int)mTable[1].size(); }

	size_t appendRow(float size) { return appendRow(Size(size, SizeType::Relative)); }
	size_t appendCol(float size) { return appendCol(Size(size, SizeType::Relative)); }

	/// Append a row of the given size (and stretch factor)
	size_t appendRow(Size rowSize) {
		mTable[1].push_back(rowSize);
		if (rowSize.type() == SizeType::Relative) { relativeSum[1] += rowSize.value(); } else { fixedSum[1] += rowSize.value(); };
		return mTable[1].size() - 1;
	}

	/// Append a column of the given size (and stretch factor)
	size_t appendCol(Size columnSize) { 
		mTable[0].push_back(columnSize);
		if (columnSize.type() == SizeType::Relative) { relativeSum[0] += columnSize.value(); } else { fixedSum[0] += columnSize.value(); };
		return mTable[0].size() - 1;
	}

	/// Set the stretch factor of a given row
	void setRowSize(int index, Size rowSize) {
		if (mTable[1].at(index).type() == SizeType::Relative) { relativeSum[1] -= mTable[1].at(index).value(); }
		else { fixedSum[1] -= mTable[0].at(index).value(); }
		if (rowSize.type() == SizeType::Relative) { relativeSum[1] += rowSize.value(); }
		else { fixedSum[1] += rowSize.value(); };
		mTable[1].at(index) = rowSize;
	}

	/// Set the stretch factor of a given column
	void setColSize(int index, Size columnSize) {
		if (mTable[0].at(index).type() == SizeType::Relative) { relativeSum[0] -= mTable[0].at(index).value(); }
		else { fixedSum[0] -= mTable[0].at(index).value(); }
		if (columnSize.type() == SizeType::Relative) { relativeSum[0] += columnSize.value(); }
		else { fixedSum[0] += columnSize.value(); };
		mTable[0].at(index) = columnSize;
	}

	/// Specify the anchor data structure for a given widget
	void setAnchor(const Widget *widget, const Anchor &anchor) { mAnchor[widget] = anchor; }

	/// Retrieve the anchor data structure for a given widget
	Anchor anchor(const Widget *widget) const {
		auto it = mAnchor.find(widget);
		if (it == mAnchor.end()) {
			Anchor a = Anchor(0, 0);
			a.setAlignment(Alignment::Minimum, Alignment::Minimum);
			return a;
		}
		else {
			return it->second;
		}
	}

	/* Implementation of the layout interface */
	virtual Vector2i preferredSize(NVGcontext *ctx, const Widget *widget) const override;
	virtual void performLayout(NVGcontext *ctx, Widget *widget) const override;

	static Anchor makeAnchor(int x, int y, int w = 1, int h = 1, Alignment alignX = Alignment::Fill, 
		Alignment alignY = Alignment::Fill, FillMode modeX = FillMode::Always, FillMode modeY = FillMode::Always);

protected:
	Vector2f positionOf(const Widget * widget, size_t x, size_t y) const;
	Vector4i boundsOf(const Widget * widget, size_t x, size_t y, size_t w, size_t h) const;

	std::vector<Size> mTable[2];
	float relativeSum[2] = { 0.f, 0.f };
	float fixedSum[2] = { 0.f, 0.f };
	std::unordered_map<const Widget *, Anchor> mAnchor;
	int mMargin = 0;
};

NAMESPACE_END(nanogui)
