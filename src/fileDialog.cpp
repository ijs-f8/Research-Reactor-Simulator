#include <nanogui/fileDialog.h>
#include <nanogui/opengl.h>
#include <sys/stat.h>

NAMESPACE_BEGIN(nanogui)

bool FileDialog::ItemList::mouseButtonEvent(const Vector2i &p, int button, bool down, int /*modifiers*/) {
	if (button != GLFW_MOUSE_BUTTON_LEFT) return false;
	int itemClicked = (int)std::floor(p.y() / itemHeight);
	if (itemClicked < 0 || (itemClicked > displayData.size() - 1)) return false;
	if (down) {
		lastClickFocus = itemClicked;
	}
	else {
		if (lastClickFocus == itemClicked) {
			if (mDoubleClick) {
				double time_now = get_seconds_since_epoch();
				if (itemClicked == lastClickIndex) {
					if (time_now - lastClick <= 0.4) {
						if (displayData[itemClicked].second) {
							mClickCallback(displayData[itemClicked].first);
							return true;
						}
					}
				}
				lastClick = time_now;
				lastClickIndex = itemClicked;
				if (mSelectedChangedCallback) mSelectedChangedCallback(lastClickIndex);
			}
			else {
				mClickCallback(displayData[itemClicked].first);
				lastClickIndex = itemClicked;
				if (mSelectedChangedCallback) mSelectedChangedCallback(lastClickIndex);
			}
		}
		else {
			lastClickFocus = -1;
			return false;
		}
	}
	return true;
}

void FileDialog::ItemList::draw(NVGcontext* ctx) {
	Vector2i cursorPos = parentScreen()->mousePos();
	cursorPos -= this->absolutePosition();

	Vector4f bounds;

	float scissor[4];
	nvgGetScissor(ctx, scissor);
	if (scissor[2] <= 0.f || scissor[3] <= 0.f) {
		bounds = Vector4f(0.f, 0.f, (float)mSize.x(), (float)mSize.y());
	}
	else {
		bounds = Vector4f(scissor[0], scissor[1], scissor[2], scissor[3]);
	}

	bool rule = true;
	for (int i = 0; i < 2; i++) {
		rule = rule && (cursorPos[i] >= bounds[i]) && (cursorPos[i] < bounds[i] + bounds[2 + i]);
	}

	Widget::draw(ctx);

	mouseOnIndex = -1;
	if (rule) {
		mouseOnIndex = (int)std::floor(cursorPos.y() / itemHeight);
		if (mouseOnIndex >= displayData.size()) mouseOnIndex = -1;
	}

	const float iconWidth = 1.5f * 0.8f * itemHeight;
	nvgSave(ctx);
	nvgIntersectScissor(ctx, 0.f, 0.f, (float)mSize.x(), (float)mSize.y());
	nvgFontSize(ctx, 0.8f * itemHeight);
	for (int i = 0; i < displayData.size(); i++) {
		Color textColor = (lastClickIndex == i) ? Color(0.3f, 0.8f, 1.f, 1.f) : Color(1.f, 1.f);
		Color backColor = (lastClickIndex == i) ? ((mouseOnIndex == i) ? Color(0.f, 0.2f) : Color(0.f, 0.4f)) : ((mouseOnIndex == i) ? Color(1.f, 0.3f) : Color(0.f, 0.f));
		if (backColor.w()) { // only paint background if alpha is not 0
			nvgBeginPath(ctx);
			nvgRect(ctx, 0.f, i*itemHeight, (float)mSize.x(), (float)itemHeight);
			nvgFillColor(ctx, backColor);
			nvgFill(ctx);
		}

		nvgFillColor(ctx, Color(1.f, 1.f)); // white icon
		nvgFontFace(ctx, "icons");
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);
		int icon = displayData[i].second ? ENTYPO_ICON_FOLDER : ENTYPO_ICON_COG;
		if (FileDialog::isTopLevelDirectory(displayData[i].first)) icon = ENTYPO_ICON_DRIVE;
		nvgText(ctx, iconWidth / 2.f, (0.5f + i)*itemHeight, utf8(icon).data(), NULL);

		nvgFillColor(ctx, textColor); // text color
		nvgFontFace(ctx, "sans");
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_MIDDLE | NVGalign::NVG_ALIGN_LEFT);
		nvgText(ctx, iconWidth, (0.5f + i)*itemHeight, displayData[i].first.c_str(), NULL);
	}
	nvgRestore(ctx);
}

FileDialog::FileDialog(Screen* parent, std::string title, std::string startDir, std::string filetypes, std::string confirmButtonText, bool isFolderGoal, bool save) : Window((Widget*)parent, title) {
	mScreen = parent;
	mSave = save;
	mFolderGoal = isFolderGoal;
	
	mTypes = std::vector<std::string>();
	size_t fileTypeLen = filetypes.length();
	if (filetypes[0] != '*') {
		int lasti = 0;
		for (int i = 0; i < fileTypeLen; i++) {
			if (filetypes[i] == ';') {
				mTypes.push_back(filetypes.substr(lasti, i - lasti));
				i++;
				lasti = i;
			}
		}
		if (lasti < fileTypeLen - 1) mTypes.push_back(filetypes.substr(lasti, fileTypeLen - lasti));
	}

	this->setSize(Vector2i(400, 400));
	this->setVisible(true);
	this->setFocused(true);
	this->setBorder(true);
	this->setBorderColor(Color(77, 184, 255, 255));

	// Create window layout
	RelativeGridLayout* baseLayout = new RelativeGridLayout();
	baseLayout->appendCol(RelativeGridLayout::Size(LEFT_PANEL_SIZE, RelativeGridLayout::SizeType::Fixed));
	baseLayout->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));
	baseLayout->appendCol(1.f);
	baseLayout->appendRow(RelativeGridLayout::Size(mTheme->mWindowHeaderHeight, RelativeGridLayout::SizeType::Fixed)); // WINDOW HEADER
	baseLayout->appendRow(RelativeGridLayout::Size(TOOLBAR_HEIGHT, RelativeGridLayout::SizeType::Fixed));
	baseLayout->appendRow(1.f);
	baseLayout->appendRow(RelativeGridLayout::Size(mSave ? TOOLBAR_HEIGHT * 2 : TOOLBAR_HEIGHT, RelativeGridLayout::SizeType::Fixed));
	this->setLayout(baseLayout);

	// Borders
	Widget* borderl = this->add<Widget>();
	borderl->setDrawBackground(true);
	borderl->setBackgroundColor(Color(77, 184, 255, 255));
	baseLayout->setAnchor(borderl, RelativeGridLayout::makeAnchor(1, 1, 1, 2));

	// Create top panel
	Widget* topPanel = this->add<Widget>();
	topPanel->setDrawBackground(true);
	topPanel->setBackgroundColor(Color(0.1f, 1.f));
	baseLayout->setAnchor(topPanel, RelativeGridLayout::makeAnchor(0, 1, 3));
	RelativeGridLayout* topLayout = new RelativeGridLayout();
	topLayout->appendCol(RelativeGridLayout::Size(LEFT_PANEL_SIZE, RelativeGridLayout::SizeType::Fixed));
	topLayout->appendCol(1.f);
	topLayout->appendRow(1.f);
	topPanel->setLayout(topLayout);

	mDirUpButton = topPanel->add<Button>("Move up", ENTYPO_ICON_LEVEL_UP);
	mDirUpButton->setEnabled(!startDir.empty());
	topLayout->setAnchor(mDirUpButton, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Middle, Alignment::Middle));
	mDirUpButton->setCallback([this]() {
		std::string prev = this->prevDirectory;
		if (this->isTopLevelDirectory(prev)) {
			this->openDirectory("", false);
		}
		else {
			if (prev[prev.length() - 1] == this->mDelimiter) prev = prev.substr(0, prev.length() - 1);
			for (size_t i = prev.length() - 1; i > 0; i--) {
				if (prev[i] == this->mDelimiter) {
					if (this->isTopLevelDirectory(prev.substr(0, i + 1))) {
						this->openDirectory(prev.substr(0, i + 1), false);
					}
					else {
						this->openDirectory(prev.substr(0, i), false);
					}
					break;
				}
			}
		}
	});

	mUrlBox = topPanel->add<TextBox>(startDir);
	mUrlBox->setAlignment(TextBox::Alignment::Left);
	RelativeGridLayout::Anchor anchor_url = RelativeGridLayout::makeAnchor(1, 0, 1, 1, Alignment::Fill, Alignment::Middle);
	anchor_url.padding = Vector4i(5, 0, 5, 0);
	topLayout->setAnchor(mUrlBox, anchor_url);

	// Create right scroll panel
	VScrollPanel* mList = this->add<VScrollPanel>();
	mList->setDrawBackground(true);
	mList->setBackgroundColor(Color(0.2f, 1.f));
	RelativeGridLayout* rel2 = new RelativeGridLayout();
	rel2->appendCol(1.f);
	rel2->appendRow(1.f);
	mList->setLayout(rel2);
	mScrollArea = mList->add<ItemList>();
	rel2->setAnchor(mScrollArea, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Fill, Alignment::Fill, RelativeGridLayout::FillMode::Always, RelativeGridLayout::FillMode::IfLess));
	baseLayout->setAnchor(mList, RelativeGridLayout::makeAnchor(2, 2));

	mScrollArea->setClickCallback([this](std::string folder) {
		std::string prev = this->prevDirectory;
		if (prev.empty()) {
			this->openDirectory(folder, false);
		}
		else if (prev[prev.length() - 1] == this->mDelimiter) {
			this->openDirectory(prev + folder, false);
		}
		else {
			this->openDirectory(prev + this->mDelimiter + folder, false);
		}
	});
	if (mSave) {
		mScrollArea->setSelectedChangedCallback([this](int index) {
			if (index < 0) return;
			DirInfo itm = mScrollArea->item(index);
			if (mFolderGoal) {
				if (itm.second) mFileBox->setText(itm.first);
			}
			else {
				if (!itm.second) mFileBox->setText(itm.first);
			}
		});
	}

	// Create left scroll panel
	VScrollPanel* mLeftList = this->add<VScrollPanel>();
	mLeftList->setDrawBackground(true);
	mLeftList->setBackgroundColor(Color(0.3f, 1.f));
	RelativeGridLayout* rel3 = new RelativeGridLayout();
	rel3->appendCol(1.f);
	rel3->appendRow(1.f);
	mLeftList->setLayout(rel3);
	mDriveArea = mLeftList->add<ItemList>();
	mDriveArea->setItemHeight(30.f);
	mDriveArea->setDoubleClick(false);
	rel3->setAnchor(mDriveArea, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Fill, Alignment::Fill, RelativeGridLayout::FillMode::Always, RelativeGridLayout::FillMode::IfLess));
	baseLayout->setAnchor(mLeftList, RelativeGridLayout::makeAnchor(0, 2));

	mDriveArea->setClickCallback([this](std::string drive) {
		this->openDirectory(drive, false);
	});

	// Create bottom panel
	Widget* bottomPanel = this->add<Widget>();
	bottomPanel->setDrawBackground(true);
	bottomPanel->setBackgroundColor(Color(0.1f, 1.f));
	baseLayout->setAnchor(bottomPanel, RelativeGridLayout::makeAnchor(0, 3, 3));
	RelativeGridLayout* bottomLayout = new RelativeGridLayout();
	bottomLayout->appendCol(RelativeGridLayout::Size(LEFT_PANEL_SIZE, RelativeGridLayout::SizeType::Fixed));
	bottomLayout->appendCol(1.f);
	for (int i = 0; i < (mSave ? 2 : 1); i++)bottomLayout->appendRow(1.f);
	bottomPanel->setLayout(bottomLayout);

	if (mSave) {
		Label* fileLabel = bottomPanel->add<Label>("File name:");
		bottomLayout->setAnchor(fileLabel, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Middle, Alignment::Middle));
		mFileBox = bottomPanel->add<TextBox>();
		mFileBox->setEditable(true);
		RelativeGridLayout::Anchor anc_file = RelativeGridLayout::makeAnchor(1, 0, 1, 1, Alignment::Fill, Alignment::Middle);
		anc_file.padding = Vector4i(5, 0, 5, 0);
		bottomLayout->setAnchor(mFileBox, anc_file);
	}

	mOkButton = bottomPanel->add<Button>(confirmButtonText, ENTYPO_ICON_CHECK);
	RelativeGridLayout::Anchor anc1 = RelativeGridLayout::makeAnchor(0, mSave ? 1 : 0, 1, 1, Alignment::Minimum, Alignment::Middle);
	anc1.padding = Vector4i(20, 0, 0, 0);
	bottomLayout->setAnchor(mOkButton, anc1);

	mOkButton->setCallback([this]() { dispose(); });

	mCancelButton = bottomPanel->add<Button>("Cancel", ENTYPO_ICON_CIRCLED_CROSS);
	RelativeGridLayout::Anchor anc2 = RelativeGridLayout::makeAnchor(1, mSave ? 1 : 0, 1, 1, Alignment::Maximum, Alignment::Middle);
	anc2.padding = Vector4i(0, 0, 20, 0);
	bottomLayout->setAnchor(mCancelButton, anc2);

	mCancelButton->setCallback([this]() { dispose(); });

	refreshDrives(mScreen->nvgContext());
	openDirectory(startDir, false);
}

void FileDialog::openDirectory(std::string directory, bool folders) {
	if (isTopLevelDirectory(directory)) {
		mDriveArea->setSelected(directory);
	}
	else if (directory.empty()) {
		mDriveArea->setSelected(-1);
	}
	else {
		for (int i = 0; i < directory.length(); i++) {
			if (directory[i] == mDelimiter) {
				mDriveArea->setSelected(directory.substr(0, i + 1));
				break;
			}
		}
	}

	mScrollArea->clearItems();

	std::vector<DirInfo> find = openDir(directory);
	for (DirInfo a : find) {
		bool shouldAdd = false;
		if (folders) {
			if (a.second) shouldAdd = true;
		}
		else {
			if (a.second) {
				shouldAdd = true;
			}
			else {
				if (mTypes.size()) {
					bool endsWith = false;
					for (std::string typ : mTypes) {
						if (a.first.length() <= typ.length()) continue;
						if (typ == a.first.substr(a.first.length() - typ.length(), typ.length())) {
							endsWith = true;
							break;
						}
					}
					if (endsWith) shouldAdd = true;
				}
				else {
					shouldAdd = true;
				}
			}
		}
		if (shouldAdd) mScrollArea->addItem(a);
	}
	this->prevDirectory = directory;
	mUrlBox->setText(directory);
	mDirUpButton->setEnabled(!directory.empty());
	this->performLayout(mScreen->nvgContext());
}

void FileDialog::draw(NVGcontext * ctx) {
	mOkButton->setEnabled(canContinue());
	Window::draw(ctx);
	double time_now = get_seconds_since_epoch();
	if (lastUpdate) {
		if (time_now - lastUpdate > 2.) { // update every 2 seconds
			lastUpdate = time_now;
			refreshDrives(ctx);
		}
	}
	else {
		lastUpdate = time_now;
	}
}

void FileDialog::refreshDrives(NVGcontext* ctx)
{
	int selectedIndex = mDriveArea->selectedItem();
	std::string selItem = (selectedIndex == -1) ? "" : mDriveArea->item(mDriveArea->selectedItem()).first;
	mDriveArea->clearItems();
	std::vector<DirInfo> drives = openDir("", false);
	int sel = -1;
	for (int i = 0; i < drives.size(); i++) {
		if (drives[i].first == selItem) sel = i;
		mDriveArea->addItem(drives[i]);
	}
	mDriveArea->setSelected(sel);
	this->performLayout(ctx);
}

bool FileDialog::canContinue()
{
	if (mSave) {
		if (mFolderGoal) {
			return !prevDirectory.empty();
		}
		else {
			char range1[2] = { '0','9' };
			char range2[2] = { 'A','Z' };
			char range3[2] = { 'a','z' };
			char accepted[3] = { '.','_','-' };
			bool invalid_found = false;
			std::string name = mFileBox->currentValue();
			if (name.empty() || name.length() > 37) return false; // check if name is too long or no name is selected
			for (int i = 0; i < name.length(); ++i) { // check for invalid characters
				char one = name[i];
				if ((one < range1[0] || one > range1[1]) && (one < range2[0] || one > range2[1]) && (one < range3[0] || one > range3[1])) {
					invalid_found = true;
					for (int j = 0; j < 3; j++) {
						invalid_found = invalid_found && (one != accepted[j]);
					}
					if (invalid_found) break;
				}
			}
			if (strchr("._-", name[0]) != NULL) return false; // check if file name starts with invalid characters
			if (name[name.length() - 1] == '.') return false; // check if file name ends with dot
			return !(invalid_found || prevDirectory.empty());
		}
	}
	else {
		if (mFolderGoal) {
			return !prevDirectory.empty();
		}
		else {
			return !mScrollArea->isFolderSelected() && (mScrollArea->selectedItem() >= 0);
		}
	}
}

std::string FileDialog::getFileName()
{
	std::string ret = prevDirectory;
	if (mFolderGoal) {
		if (mScrollArea->isFolderSelected()) {
			if (ret[ret.length() - 1] != mDelimiter) ret += mDelimiter;
			ret += mScrollArea->item(mScrollArea->selectedItem()).first;
		}
		else {
			if (ret[ret.length() - 1] == mDelimiter) ret = ret.substr(0, ret.length() - 1);
		}
	}
	else {
		if (mSave) {
			if (ret[ret.length() - 1] != mDelimiter) ret += mDelimiter;
			ret += mFileBox->currentValue();
			if (mTypes.size()) {
				std::string fileType = mTypes[0];
				if (ret.length() < fileType.length() + 2) {
					ret += "." + fileType;
				}
				else {
					if (ret.substr(ret.length() - fileType.length() - 1, fileType.length() + 1) != ("." + fileType)) {
						ret += "." + fileType;
					}
				}
			}
		}
		else {
			if (mScrollArea->selectedItem() >= 0) {
				if (ret[ret.length() - 1] != mDelimiter) ret += mDelimiter;
				ret += mScrollArea->item(mScrollArea->selectedItem()).first;
			}
			else {
				if (ret[ret.length() - 1] == mDelimiter) ret = ret.substr(0, ret.length() - 1);
			}
		}
	}
	return ret;
}

NAMESPACE_END(nanogui)
