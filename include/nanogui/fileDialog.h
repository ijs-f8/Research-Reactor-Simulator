#pragma once
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/layout.h>
#include <nanogui/theme.h>
#include <vector>
#include <string>
#include <nanogui/entypo.h>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/textbox.h>
#include <nanogui/messagedialog.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#endif
#define TOOLBAR_HEIGHT	40.f
#define LEFT_PANEL_SIZE	120.f

NAMESPACE_BEGIN(nanogui)

class NANOGUI_EXPORT FileDialog : public Window {
	static const bool isTopLevelDirectory(std::string dir) {
		if (dir.empty()) return false;
		if (dir[dir.length() - 1] != mDelimiter) return false;
		int count = 0;
		for (size_t i = 0; i < dir.length(); i++) {
			if (dir[i] == mDelimiter) count++;
		}
		return count == 1;

	}
private:
	const static char mDelimiter =
#ifdef _WIN32
		'\\';
#else
		'/';
#endif

	typedef std::pair<std::string, bool> DirInfo;
	Button* mDirUpButton;
	Button* mOkButton;
	Button* mCancelButton;
	Screen* mScreen;
	TextBox* mUrlBox;
	TextBox* mFileBox;
	std::vector<std::string> mTypes;
	bool mSave;
	bool mFolderGoal;
	std::function<void(bool, std::string)> mCallback;

	size_t numEntries = 0;
	double lastUpdate = 0.;
	
	class ItemList : public Widget {
	private:
		bool mDoubleClick = true;
		float itemHeight = 25.f;
		int lastClickIndex = -1;
		int lastClickFocus = -1;
		double lastClick = 0.;
		int mouseOnIndex = -1;
		std::function<void(std::string)> mClickCallback;
		std::function<void(int)> mSelectedChangedCallback;
		std::vector<DirInfo> displayData = std::vector<DirInfo>();
	public:

		ItemList(Widget* parent) : Widget(parent) {}

		bool mouseButtonEvent(const Vector2i &p, int button, bool down, int /*modifiers*/) override;

		void setClickCallback(std::function<void(std::string)> callback) {
			mClickCallback = callback;
		}

		void setSelectedChangedCallback(std::function<void(int)> callback) {
			mSelectedChangedCallback = callback;
		}

		void draw(NVGcontext* ctx) override;

		void setItemHeight(float value) { itemHeight = value; }

		void clearItems() {
			displayData.clear();
			lastClick = 0.;
			lastClickIndex = -1;
			if(mSelectedChangedCallback) mSelectedChangedCallback(lastClickIndex);
		}

		void addItem(DirInfo itm) {
			displayData.push_back(itm);
		}

		int selectedItem() { return lastClickIndex; }

		bool isFolderSelected() {
			if (lastClickIndex >= 0 && lastClickIndex < displayData.size()) {
				return displayData[lastClickIndex].second;
			}
			else {
				return false;
			}
		}

		DirInfo item(size_t index) {
			return displayData[index];
		}

		size_t itemNumber() { return displayData.size(); }

		void setSelected(int index) {
			lastClickIndex = index;
			if (mSelectedChangedCallback) mSelectedChangedCallback(lastClickIndex);
		}
		void setSelected(std::string item) {
			for (int i = 0; i < displayData.size(); i++) {
				if (displayData[i].first == item) {
					setSelected(i);
					break;
				}
			}
		}

		Vector2i preferredSize(NVGcontext* ctx) const override {
			Vector2i ret = Widget::preferredSize(ctx);
			return Vector2i(ret.x(), displayData.size() * itemHeight);
		}

		void setDoubleClick(bool value) { mDoubleClick = value; }

	};

	std::string prevDirectory;
	ItemList* mScrollArea;
	ItemList* mDriveArea;

	std::vector<DirInfo> openDir(const std::string &directory, bool folderOnly = false) {
		std::vector<DirInfo> ret = std::vector<DirInfo>();
		if (directory.length()) {
#if defined(_WIN32)
			HANDLE dir;
			WIN32_FIND_DATA file_data;

			if ((dir = FindFirstFile((directory + "\\*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
				return ret; /* No files found */
			do {
				if (file_data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) continue;
				if (file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;
				if (file_data.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) continue;
				if (file_data.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) continue;

				const std::string file_name = file_data.cFileName;
				const bool is_directory = ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

				if (file_name[0] == '.' ) continue;
				if (folderOnly && !is_directory) continue;

				ret.push_back(DirInfo(file_name, is_directory));
			} while (FindNextFile(dir, &file_data));

			FindClose(dir);
#else
			DIR *dir;
			class dirent *ent;
			class stat st;

			dir = opendir(directory);
			while ((ent = readdir(dir)) != NULL) {
				const string file_name = ent->d_name;
				const string full_file_name = directory + "/" + file_name;

				if (stat(full_file_name.c_str(), &st) == -1) continue;

				const bool is_directory = (st.st_mode & S_IFDIR) != 0;

				if (folderOnly && !is_directory) continue;

				if (file_name[0] == '.' && !is_directory) continue;

				out.push_back(DirInfo(file_name, true));
			}
			closedir(dir);
#endif
		}
		else {
#if defined(_WIN32)
			DWORD dwSize = MAX_PATH;
			char szLogicalDrives[MAX_PATH] = { 0 };
			DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);

			if (dwResult > 0 && dwResult <= MAX_PATH)
			{
				char* szSingleDrive = szLogicalDrives;
				while (*szSingleDrive)
				{
					ret.push_back(DirInfo(szSingleDrive, true));
					szSingleDrive += strlen(szSingleDrive) + 1;
				}
			}
#else
			return openDir(".", folderOnly);
#endif
		}
		return ret;
	}

	bool canContinue();

	std::string getFileName();

public:
	FileDialog(Screen* parent, std::string title, std::string startDir, std::string filetypes, std::string confirmButtonText, bool isFolderGoal, bool save = false);

	static inline bool fileExists(std::string &name) {
		struct stat buffer;
		return (stat(name.c_str(), &buffer) == 0);
	}

	void openDirectory(std::string directory, bool folders);

	void draw(NVGcontext* ctx) override;

	void refreshDrives(NVGcontext* ctx);

	void setCallback(std::function<void(bool, std::string)> callback_) {
		mCallback = callback_;
		mOkButton->setCallback([this]() {
			if (mCallback && canContinue()) {
				std::string fileN = getFileName();
				if (this->mSave && fileExists(fileN)) {
					MessageDialog* msg = new MessageDialog(this, MessageDialog::Type::Warning, mTitle, "The file you chose already exists. Would you like to overwrite it?", "Yes", "No", true);
					msg->setPosition(Vector2i((this->size().x() - msg->size().x()) / 2, (this->size().y() - msg->size().y()) / 2));
					msg->setCallback([this, fileN](int choice) {
						if (choice == 0) {
							mCallback(true, fileN);
							dispose();
						}
					});
				}
				else {
					mCallback(true, fileN);
					dispose();
				}
			}
		});
		mCancelButton->setCallback([this]() {
			if (mCallback) mCallback(false, "");
			dispose();
		});
	}

	std::function<void(bool, std::string)> callback() { return mCallback; }

};

NAMESPACE_END(nanogui)