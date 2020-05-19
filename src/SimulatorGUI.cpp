/*
	src/example2.cpp -- C++ version of an example application that shows
	how to use the form helper class. For a Python implementation, see
	'../python/example2.py'.

	NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
	The widget drawing code is based on the NanoVG demo application
	by Mikko Mononen.

	All rights reserved. Use of this source code is governed by a
	BSD-style license that can be found in the LICENSE.txt file.
*/
#define NOMINMAX

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/tabwidget.h>
#include <Simulator.h>
#include <nanogui/DataDisplay.h>
#include <nanogui/pieChart.h>
#include <nanogui/controlRodDisplay.h>
#include <nanogui/ReactivityDisplay.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/glutil.h>
#include <string>
#include <nanogui/nanogui.h>
#include <iostream>
#include <algorithm>
#include <math.h>
#include <SerialClass.h>
#include <Settings.h>
#include <nanogui/fileDialog.h>
#include <Icon.h>

/* Resolution formats supported:
*	HD 720	(1280 x 720)
*	WXGA	(1280 x 768)
*	laptop	(1366 x 768)
*	SXGA	(1280 x 1024)
*	WXGA	(1280 x 800)
*	better	(1200+ x 700+)
*/
#define WINDOW_DEFAULT_WIDTH	1280	// HD 720p width
#define WINDOW_DEFAULT_HEIGHT	720		// HD 720p height

#define WINDOW_ICON_NUM			7		// Number of icon formats

// SIMULATOR VERSION
// major.minor.revision.build
#define VERSION_MAJOR		1
#define VERSION_MINOR		3
#define VERSION_REVISION	2
#define VERSION_BUILD		0		// Setting this to 0 doesn't display the build number

// RECIEVING
#define BOX_ID "IJS_F8_BOX3"
#define ENABLE_SAFETY_BTN 8
#define UP_SAFETY_BTN 16
#define DOWN_SAFETY_BTN 32
#define ENABLE_SHIM_BTN 64
#define UP_SHIM_BTN 128
#define DOWN_SHIM_BTN 256
#define ENABLE_REG_BTN 512
#define UP_REG_BTN 1024
#define DOWN_REG_BTN 2048
#define FIRE_BTN 4096
#define SCRAM_BTN 8192

// SENDING
#define SCRAM_PER 2
#define SCRAM_FT 4
#define SCRAM_WT 8
#define SCRAM_POW 16
#define SCRAM_MAN 32
#define FIRE_LED_B 64
#define ROD_SAFETY_ENBL 128
#define ROD_SAFETY_UP 256
#define ROD_SAFETY_DOWN 512
#define ROD_REG_ENBL 1024
#define ROD_REG_UP 2048
#define ROD_REG_DOWN 4096
#define ROD_SHIM_ENBL 8192
#define ROD_SHIM_UP 16384
#define ROD_SHIM_DOWN 32768

#define RESET_ALARM_KEY 8128

#define SIM_TIME_FACTOR_NUMBER 21

#define SCI_NUMBER_FORMAT	"[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?"
#define SCI_NUMBER_FORMAT_NEG	"[-]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?"

using namespace nanogui;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::max;
using std::to_string;

class SimulatorGUI : public nanogui::Screen {
private:
	const string box_auth = BOX_ID;
	pair<bool, bool> isZero = pair<bool, bool>(false, false);
	double simulationTimes[SIM_TIME_FACTOR_NUMBER] = { 0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.75, 1., 1.5, 2., 4., 5., 7.5, 10., 15., 20., 50., 100., 250., 500.};
	const std::vector<string> modes = { "Manual", "Square wave","Sine wave","Saw tooth","Automatic", "Pulse" };
	const std::vector<string> ns_modes = { "Constant", "Square wave", "Sine wave", "Saw tooth" };
	const std::vector<string> sineModes = { "Normal","Quadratic" };
	string sqw_settingNames[4] = {"Wave up start: ", "Wave up end: ", "Wave down start: ", "Wave down end: "};
	string saw_settingNames[6] = { "Tooth up start: ","Tooth up peak: ","Tooth up end: ","Tooth down start: ","Tooth down peak: ","Tooth down end: " };
	double lastBoxCheck;
	bool layoutStart = false;
	float fpsSum = 0.f;
	char fpsCount = 0;
	const std::string degCelsiusUnit = std::string(utf8(0xBA).data()) + "C";
	const std::string alpha = std::string(utf8(0x3B1).data());
	double alphaX[3] = { 0., 0., 0. };
	float alphaY[3] = { 0.f, 0.f, 0.f };
public:

	Simulator::PulseData lastPulseData;
	bool pulsePerformed = false;
	double viewStart = -1.;
	double timeAtLastChange = 0.;
	string startScript = "";

	Settings* properties;

	const std::string version() {
		std::string ver = to_string(VERSION_MAJOR) + "." + to_string(VERSION_MINOR);
#if VERSION_REVISION
		ver += "." + to_string(VERSION_REVISION);
		return VERSION_BUILD ? (ver += "." + to_string(VERSION_BUILD)) : ver;
#else
		return VERSION_BUILD ? (ver + ".0." + to_string(VERSION_BUILD)) : ver;
#endif
	}

	Serial* theBox;
	Simulator* reactor;
	Graph* canvas;
	Graph* delayedGroupsGraph;
	Graph* pulseGraph;
	Graph* sourceGraph;
	BoxLayout* layout;
	Window* baseWindow;
	RelativeGridLayout* relativeLayout; // layout for the main window
	Label* fpsLabel;
	Plot* reactivityPlot;
	Plot* rodReactivityPlot;
	Plot* powerPlot;
	Plot* temperaturePlot;
	Plot* delayedGroups[6];
	Plot* pulsePlots[4];
	ToolButton* slowDown;
	ToolButton* playPause;
	ToolButton* speedUp;
	size_t selectedTime = 8;
	BezierCurve* rodCurves[NUMBER_OF_CONTROL_RODS];
	Plot* rodDerivatives[NUMBER_OF_CONTROL_RODS];
	TabWidget* tabControl;

	Widget* sourceSettings;
	Plot* neutronSourcePlot;
	Plot* neutronSourceTracker;
	ComboBox* neutronSourceModeBox;
	ComboBox* neutronSourceSINEModeBox;
	FloatBox<float>* neutronSourcePeriodBoxes[3];
	FloatBox<float>* neutronSourceAmplitudeBoxes[3];
	IntBox<int>* neutronSourceSQWBoxes[4];
	IntBox<int>* neutronSourceSAWBoxes[6];

	Widget* displayPanel1;
	Widget* displayPanel2;
	ControlRodDisplay* rodDisplay;
	PeriodDisplay* periodDisplay;
	ComboBox* rodMode;
	IntBox<int>* rodBox[NUMBER_OF_CONTROL_RODS];
	SliderCheckBox* neutronSourceCB;
	SliderCheckBox* cooling;

	IntBox<int>* graphSizeBox;
	SliderCheckBox* curveFillBox;
	SliderCheckBox* rodReactivityBox;
	FloatBox<float>* reactivityLimitBox[2];
	FloatBox<float>* temperatureLimitBox[2];
	FloatBox<float>* displayBox;
	SliderCheckBox* logScaleBox;
	SliderCheckBox* hardcoreBox;
	IntervalSlider* displayTimeSlider;
	SliderCheckBox* timeLockedBox;

	FloatBox<double>* delayedGroupBoxes[12];
	SliderCheckBox* delayedGroupsEnabledBoxes[6];
	FloatBox<double>* coreVolumeBox;
	FloatBox<double>* waterVolumeInput;
	SliderCheckBox* tempEffectsBox;
	SliderCheckBox* fissionProductsBox;
	FloatBox<float>* excessReactivityBox;
	FloatBox<double>* sourceActivityBox;
	FloatBox<double>* coolingPowerBox;
	FloatBox<double>* promptNeutronLifetimeBox;
	Plot* alphaPlot;
	const size_t sigmaPoints = 3;
	FloatBox<float>* alpha0Box;
	FloatBox<float>* alphaPeakBox;
	FloatBox<float>* tempPeakBox;
	FloatBox<float>* alphaSlopeBox;

	IntBox<int>* rodStepsBox[NUMBER_OF_CONTROL_RODS];
	FloatBox<float>* rodWorthBox[NUMBER_OF_CONTROL_RODS];
	FloatBox<float>* rodSpeedBox[NUMBER_OF_CONTROL_RODS];
	Slider* rodCurveSliders[NUMBER_OF_CONTROL_RODS * 2];

	// Rod modes
	FloatBox<float>* periodBoxes[3];
	FloatBox<float>* amplitudeBoxes[3];
	IntBox<int>* squareWaveBoxes[4];
	SliderCheckBox* squareWaveSpeedBox;
	ComboBox* sineModeBox;
	IntBox<int>* sawToothBoxes[6];
	SliderCheckBox* keepCurrentPowerBox;
	FloatBox<double>* steadyPowerBox;
	SliderCheckBox* avoidPeriodScramBox;
	FloatBox<float>* automaticMarginBox;

	// Neutron source simulation
	FloatBox<float>* ns_periodBoxes[3];
	FloatBox<float>* ns_amplitudeBoxes[3];
	IntBox<int>* ns_squareWaveBoxes[4];
	ComboBox* ns_sineModeBox;
	IntBox<int>* ns_sawToothBoxes[6];

	SliderCheckBox* scramEnabledBoxes[5];
	IntBox<float>* periodLimBox;
	FloatBox<double>* powerLimBox;
	FloatBox<float>* fuel_tempLimBox;
	FloatBox<float>* water_tempLimBox;
	FloatBox<float>* water_levelLimBox;
	SliderCheckBox* allRodsBox;
	SliderCheckBox* autoScramBox;

	IntervalSlider* pulseTimer;

	Label* userScram;
	Label* powerScram;
	Label* fuelTemperatureScram;
	Label* waterTemperatureScram;
	Label* waterLevelScram;
	Label* periodScram;

	Label* timeLabel;
	Label* simStatusLabel;
	Label* simFactorLabel;

	Label* pulseLabels[4];
	Label* pulseDisplayLabels[4];
	Label* standInCover;

	DataDisplay<double>* powerShow;
	DataDisplay<float>* reactivityShow;
	DataDisplay<float>* rodReactivityShow;
	DataDisplay<float>* temperatureShow;
	DataDisplay<double>* periodShow;
	DataDisplay<double>* waterTemperatureShow;
	DataDisplay<double>* waterLevelShow;

	Plot* operationModes[3];
	Plot* operationModesTrackers[3];

	Color coolBlue = Color(77, 184, 255, 255);

	BoxLayout* panelsLayout = new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 10);

	uint16_t LEDstatus = 0;
	bool boxConnected = false;

	void setSimulationTime(size_t time) {
		selectedTime = time;
		reactor->setSpeedFactor(simulationTimes[time]);
		updateSimulationIcon();
	}

	void playPauseSimulation(bool play) {
		if (play) {
			setSimulationTime(selectedTime);
			playPause->setIcon(ENTYPO_ICON_PLAY);
		}
		else {
			reactor->setSpeedFactor(0.);
			playPause->setIcon(ENTYPO_ICON_PAUS);
		}
		playPause->setPushed(!play);
		updateSimulationIcon();
	}

	void updateSimulationIcon() {
		double sf = reactor->getSpeedFactor();
		if (sf == 0.) {
			simFactorLabel->setCaption("paused");
			simStatusLabel->setCaption(utf8(ENTYPO_ICON_PAUS).data());
		}
		else if (sf == 1.) {
			simFactorLabel->setCaption("real-time");
			simStatusLabel->setCaption(utf8(ENTYPO_ICON_PLAY).data());
		}
		else if (sf > 1.) {
			simFactorLabel->setCaption(formatDecimals(sf, 2, false) + "x");
			simStatusLabel->setCaption(utf8(ENTYPO_ICON_FF).data());
		}
		else {
			simFactorLabel->setCaption(formatDecimals(sf, 2, false) + "x");
			simStatusLabel->setCaption(utf8(ENTYPO_ICON_FB).data());
		}
	}

	// For cleaner code
	void initializeSimulator() {
		// Create the Simulator object, set initial properties
		reactor = new Simulator(properties);
		reactor->setDebugMode(debugMode);
		reactor->setScramCallback([this](int signal) {
			if (signal > 0) LEDstatus += (uint16_t)1 << 13;
			std::string reason = "";
			if ((signal & Simulator::ScramSignals::Period) != 0) {
				reason = "Period too low | " + std::to_string(*reactor->getReactorPeriod()) + " s | asymptotic | " + std::to_string(*reactor->getReactorAsymPeriod()) + " s";
				periodScram->setGlow(true);
				periodScram->setBackgroundColor(Color(255, 0, 0, 255));
				LEDstatus |= SCRAM_PER;
			}
			if ((signal & Simulator::ScramSignals::FuelTemperature) != 0) {
				reason = "Fuel temperature too high | " + std::to_string(reactor->getCurrentTemperature()) + " C";
				fuelTemperatureScram->setGlow(true);
				fuelTemperatureScram->setBackgroundColor(Color(255, 0, 0, 255));
				LEDstatus |= SCRAM_FT;
			}
			if ((signal & Simulator::ScramSignals::WaterTemperature) != 0) {
				reason = "Water temperature too high | " + std::to_string(reactor->waterTemperature) + " C";
				waterTemperatureScram->setGlow(true);
				waterTemperatureScram->setBackgroundColor(Color(255, 0, 0, 255));
				LEDstatus |= SCRAM_WT;
			}
			if ((signal & Simulator::ScramSignals::WaterLevel) != 0) {
				reason = "Water level too low | " + std::to_string(*reactor->getWaterLevel()) + " m";
				waterLevelScram->setGlow(true);
				waterLevelScram->setBackgroundColor(Color(255, 0, 0, 255));
				// LEDstatus |= ALARM3;
				// NOT SUPPORTED BY THE BOX
			}
			if ((signal & Simulator::ScramSignals::Power) != 0) {
				reason = "Power too high | " + std::to_string(reactor->getCurrentPower()) + " W";
				powerScram->setGlow(true);
				powerScram->setBackgroundColor(Color(255, 0, 0, 255));
				LEDstatus |= SCRAM_POW;
			}
			if ((signal & Simulator::ScramSignals::User) != 0) {
				reason = "Operator";
				userScram->setGlow(true);
				userScram->setBackgroundColor(Color(255, 0, 0, 255));
				LEDstatus |= SCRAM_MAN;
			}
			cout << "The reactor has SCRAMed!" << endl;
			cout << "=======REASON=======" << endl;
			cout << reason << endl;
			cout << "====================" << endl;
		});
		reactor->setResetScramCallback([this] {
			LEDstatus &= RESET_ALARM_KEY;
			userScram->setGlow(false);
			userScram->setBackgroundColor(Color(120, 120));
			powerScram->setGlow(false);
			powerScram->setBackgroundColor(Color(120, 120));
			periodScram->setGlow(false);
			periodScram->setBackgroundColor(Color(120, 120));
			waterTemperatureScram->setGlow(false);
			waterTemperatureScram->setBackgroundColor(Color(120, 120));
			waterLevelScram->setGlow(false);
			waterLevelScram->setBackgroundColor(Color(120, 120));
			fuelTemperatureScram->setGlow(false);
			fuelTemperatureScram->setBackgroundColor(Color(120, 120));
		});
		reactor->setPulseCallback([this](Simulator::PulseData data) {
			// Format pulse graph
			pulsePerformed = true;
			pulseTimer->setEnabled(true);
			standInCover->setVisible(false);

			lastPulseData = data;
			updatePulseTrack(true);
		});
		reactor->setSevereErrorCallback([this](int reason) {
			toggleBaseWindow(false);
			std::string msgTxt;
			switch (reason) {
			case 0:
				msgTxt = "Power exceeded 10GW - an absurd limit. The reactor will SCRAM, since the simulator can't work with infinite numbers (assuming the power is still rising)"; break;
			case 1:
				msgTxt = "Since the Research reactor simulator can't simulate an explosion,\n the reactor will SCRAM. Information: Fuel temperature exceeded 950" + degCelsiusUnit + " (the uranium isotope melted)!"; break;
			default:
				msgTxt = "An unknown error has occured. An automatic SCRAM is mandatory."; break;
			}
			MessageDialog* msg = new MessageDialog(this, MessageDialog::Type::Warning, "Severe error", msgTxt);
			msg->setPosition(Vector2i((this->size().x() - msg->size().x()) / 2, (this->size().y() - msg->size().y()) / 2));
			msg->setCallback([this, msg](int /*choice*/) {
				toggleBaseWindow(true);
				msg->dispose();
			});
		});
	}

	void updatePulseTrack(bool updateData = false) {
		if (!pulsePerformed) return;
		size_t startIdx, endIdx;
		startIdx = reactor->getIndexFromTime(reactor->time_[lastPulseData.pulseStartIndex] + pulseTimer->value(0) * 5);
		endIdx = reactor->getIndexFromTime(reactor->time_[lastPulseData.pulseStartIndex] + pulseTimer->value(1) * 5);

		double timeLimits[2] = { reactor->time_[startIdx], reactor->time_[endIdx] };
		
		for (int i = 0; i < 4; i++) {
			pulsePlots[i]->setPlotRange(startIdx, endIdx);
			if (i == 3) { 
				pair<int, int> orders = recalculatePowerExtremes(timeLimits[0], timeLimits[1]);
				pulsePlots[i]->setLimits(timeLimits[0], timeLimits[1], 0., std::pow(10., orders.second));
				bool maxVisible = (lastPulseData.timeAtMax >= timeLimits[0]) || (lastPulseData.timeAtMax <= timeLimits[1]);
				if (maxVisible) {
					pulsePlots[i]->setHorizontalPointerPosition((float)((lastPulseData.timeAtMax - timeLimits[0]) / (timeLimits[1] - timeLimits[0])));
					pulsePlots[i]->setPointerPosition((float)(lastPulseData.peakPower / pulsePlots[i]->limits()[3]));
				}
				pulsePlots[i]->setHorizontalPointerShown(maxVisible);
				pulsePlots[i]->setPointerShown(maxVisible);
				pulsePlots[i]->setLimitOverride(0, pulseTimer->value(0) ? (to_string((int)(pulseTimer->value(0) * 5e3)) + "ms") : "pulse start");
				pulsePlots[i]->setLimitOverride(1, to_string((int)(pulseTimer->value(1) * 5e3)) + "ms");
			}
			else if (!(i % 2)) {
				pulsePlots[i]->setLimits(timeLimits[0], timeLimits[1], std::floor(reactor->reactivity_[startIdx] / 200.) * 200, std::ceil(reactor->rodReactivity_[endIdx] / 200.) * 200);
			}
			else {
				pulsePlots[i]->setLimits(timeLimits[0], timeLimits[1], 0.f, updateData ? (std::ceil(lastPulseData.maxFuelTemp / 200.) * 200) : pulsePlots[i]->limits()[3]);
			}
		}

		const int per[2] = { (int)roundf(fmodf(pulseTimer->value(0) * 5, 1.f) * 100) , (int)roundf(fmodf(pulseTimer->value(1) * 5, 1.f) * 100) };
		pulseDisplayLabels[0]->setCaption(std::to_string((int)(pulseTimer->value(0) * 5)));
		pulseDisplayLabels[1]->setCaption((per[0] < 10) ? "0" : "" + std::to_string(per[0]));
		pulseDisplayLabels[2]->setCaption(std::to_string((int)(pulseTimer->value(1) * 5)));
		pulseDisplayLabels[3]->setCaption((per[1] < 10) ? "0" : "" + std::to_string(per[1]));

		if (updateData) {
			pulseLabels[0]->setCaption(formatDecimals(lastPulseData.peakPower * 1e-6, 1) + " MW");
			pulseLabels[1]->setCaption(to_string((int)(lastPulseData.FWHM * 1e3)) + " ms");
			pulseLabels[2]->setCaption(formatDecimals(lastPulseData.maxFuelTemp, 1) + " " + degCelsiusUnit);
			pulseLabels[3]->setCaption(formatDecimals(lastPulseData.releasedEnergy * 1e-6, 1) + " MJ");
		}
	}

	void viewingIntervalChanged(bool firstChanged) {
		const double timeElapsed = reactor->getCurrentTime();
		const double range = std::min(timeElapsed, DELETE_OLD_DATA_TIME_DEFAULT);
		if (firstChanged) {
			viewStart = std::max(0., timeElapsed - DELETE_OLD_DATA_TIME_DEFAULT) + std::round(1000 * displayTimeSlider->value(0) * range) * 1e-3;
			timeAtLastChange = timeElapsed;
		}
		{ // update range
			float vals[2];
			for (int i = 0; i < 2; i++) vals[i] = displayTimeSlider->value(i);
			displayBox->setValue((float)(std::round(1000 * range * (vals[1] - vals[0])) * 1e-3));
		}
	}

	// MORE
	void createDataDisplays(Widget* parent, RelativeGridLayout* rLayout) {
		// Create the displays
		displayPanel1 = parent->add<Widget>();
		displayPanel1->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 5));
		rLayout->setAnchor(displayPanel1, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Fill, Alignment::Fill, RelativeGridLayout::FillMode::Always, RelativeGridLayout::FillMode::IfLess));
		displayPanel1->setBackgroundColor(Color(50, 255));
		displayPanel1->setDrawBackground(true);
		displayPanel2 = parent->add<Widget>();
		displayPanel2->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 5));
		rLayout->setAnchor(displayPanel2, RelativeGridLayout::makeAnchor(1, 0, 1, 1, Alignment::Fill, Alignment::Fill, RelativeGridLayout::FillMode::Always, RelativeGridLayout::FillMode::IfLess));
		displayPanel2->setBackgroundColor(Color(50, 255));
		displayPanel2->setDrawBackground(true);
		powerShow = displayPanel1->add<DataDisplay<double>>("Power:");
		powerShow->setTextColor(Color(200, 255));
		powerShow->setData(reactor->getCurrentPower());
		powerShow->setFixedHeight(85);
		powerShow->setDisplayMode(DisplayMode::Scientific);
		powerShow->setUnit("W");
		reactivityShow = displayPanel2->add<DataDisplay<float>>("Reactivity (actual):");
		reactivityShow->setVisible(!properties->reactivityHardcore);
		reactivityShow->setTextColor(Color(200, 255));
		reactivityShow->setPointerColor(Color(0, 0, 255, 255));
		reactivityShow->setData(reactor->getCurrentReactivity());
		reactivityShow->setFixedHeight(85);
		reactivityShow->setDisplayMode(DisplayMode::FixedDecimalPlaces1);
		reactivityShow->setUnit("pcm");
		rodReactivityShow = displayPanel2->add<DataDisplay<float>>("Reactivity (inserted):");
		rodReactivityShow->setVisible(!properties->reactivityHardcore);
		rodReactivityShow->setTextColor(Color(200, 255));
		rodReactivityShow->setPointerColor(Color(180, 255));
		rodReactivityShow->setData(reactor->getCurrentRodReactivity());
		rodReactivityShow->setFixedHeight(85);
		rodReactivityShow->setDisplayMode(DisplayMode::FixedDecimalPlaces1);
		rodReactivityShow->setUnit("pcm");
		periodShow = displayPanel1->add<DataDisplay<double>>("Period:");
		periodShow->setTextColor(Color(200, 255));
		periodShow->setPointerColor(Color(0, 0, 0, 0));
		periodShow->setData(*reactor->getReactorPeriod());
		periodShow->setAbsoluteLimit(100.);
		periodShow->setFixedHeight(85);
		periodShow->setDisplayMode(DisplayMode::FixedDecimalPlaces1);
		periodShow->setUnit("s");
		periodShow->setBackgroundColor(Color(255, 255));
		temperatureShow = displayPanel1->add<DataDisplay<float>>("Fuel temperature:");
		temperatureShow->setTextColor(Color(200, 255));
		temperatureShow->setPointerColor(Color(0, 255, 0, 255));
		temperatureShow->setData(reactor->getCurrentTemperature());
		temperatureShow->setFixedHeight(85);
		temperatureShow->setDisplayMode(DisplayMode::FixedDecimalPlaces2);
		temperatureShow->setUnit(degCelsiusUnit);
		temperatureShow->setBackgroundColor(Color(255, 255));

		waterTemperatureShow = displayPanel2->add<DataDisplay<double>>("Water temperature:");
		waterTemperatureShow->setTextColor(Color(200, 255));
		waterTemperatureShow->setPointerColor(Color(0, 0, 0, 0));
		waterTemperatureShow->setData(*reactor->getWaterTemperature());
		waterTemperatureShow->setFixedHeight(85);
		waterTemperatureShow->setDisplayMode(DisplayMode::FixedDecimalPlaces2);
		waterTemperatureShow->setUnit(degCelsiusUnit);
		waterTemperatureShow->setBackgroundColor(Color(255, 255));

		waterLevelShow = displayPanel1->add<DataDisplay<double>>("Water level change:");
		waterLevelShow->setTextColor(Color(200, 255));
		waterLevelShow->setPointerColor(Color(0, 0, 0, 0));
		waterLevelShow->setData(*reactor->getWaterLevel() * 100.);
		waterLevelShow->setFixedHeight(85);
		waterLevelShow->setDisplayMode(DisplayMode::FixedDecimalPlaces2);
		waterLevelShow->setUnit("cm");
		waterLevelShow->setBackgroundColor(Color(255, 255));

		waterLevelShow->setVisible(false);
	}

	// Anti spaghetti machine
	void initializeGraph() {
		// Create a graph object
		canvas = baseWindow->add<Graph>(4, "Main graph");
		relativeLayout->setAnchor(canvas, RelativeGridLayout::makeAnchor(0, 0));
		canvas->setBackgroundColor(Color(250, 255));
		canvas->setDrawBackground(true);
		canvas->setPadding(90.f, 25.f, properties->reactivityHardcore ? 120.f : 220.f, 50.f);

		// Create and save the plots
		rodReactivityPlot = canvas->addPlot(reactor->getDataLength(), true);
		temperaturePlot = canvas->addPlot(reactor->getDataLength(), true);
		reactivityPlot = canvas->addPlot(reactor->getDataLength(), true);
		powerPlot = canvas->addPlot(reactor->getDataLength(), true);

		// Styling
		canvas->setTextColor(Color(0, 255));
		reactivityPlot->setEnabled(!properties->reactivityHardcore);
		reactivityPlot->setName("Reactivity");
		reactivityPlot->setUnits("pcm");
		reactivityPlot->setColor(Color(0, 0, 255, 255));
		reactivityPlot->setFillColor(Color(0, 0, 255, 50));
		reactivityPlot->setPointerColor(Color(0, 0, 255, 255));
		reactivityPlot->setAxisShown(true);
		reactivityPlot->setMainLineShown(true);
		reactivityPlot->setTextShown(true);
		reactivityPlot->setNumberFormatMode(GraphElement::FormattingMode::Normal);
		reactivityPlot->setRoundFloating(true);
		reactivityPlot->setMajorTickNumber(7);
		reactivityPlot->setMinorTickNumber(2);
		reactivityPlot->setAxisPosition(GraphElement::AxisLocation::Right);
		reactivityPlot->setAxisOffset(110.f);
		reactivityPlot->setTextOffset(60.f);
		reactivityPlot->setFill(properties->curveFill);
		rodReactivityPlot->setEnabled(properties->rodReactivityPlot && !properties->reactivityHardcore);
		rodReactivityPlot->setName("Rod position");
		rodReactivityPlot->setColor(Color(200, 255));
		rodReactivityPlot->setFillColor(Color(0, 0, 255, 10));
		rodReactivityPlot->setPointerColor(Color(180, 255));
		rodReactivityPlot->setAxisPosition(GraphElement::AxisLocation::Right);
		rodReactivityPlot->setAxisOffset(110.f);
		rodReactivityPlot->setFill(properties->curveFill);
		powerPlot->setName("Power");
		powerPlot->setUnits("W");
		powerPlot->setColor(Color(255, 0, 0, 255));
		powerPlot->setFillColor(Color(255, 0, 0, 50));
		powerPlot->setAxisShown(true);
		powerPlot->setYlog(properties->yAxisLog);
		powerPlot->setAxisPosition(GraphElement::AxisLocation::Right);
		powerPlot->setTextOffset(60.f);
		powerPlot->setMajorTickNumber(4);
		powerPlot->setMinorTickNumber(4);
		powerPlot->setTextShown(true);
		powerPlot->setNumberFormatMode(GraphElement::FormattingMode::Exponential);
		powerPlot->setDrawMode(DrawMode::Smart);
		powerPlot->setHorizontalAxisShown(true);
		powerPlot->setHorizontalMinorTickNumber(4);
		powerPlot->setHorizontalName("Time");
		powerPlot->setHorizontalTextOffset(20.f);
		powerPlot->setLimitOverride(0, "30 seconds ago");
		powerPlot->setLimitOverride(1, "now");
		powerPlot->setFill(properties->curveFill);
		temperaturePlot->setName("Temperature");
		temperaturePlot->setUnits(degCelsiusUnit);
		temperaturePlot->setColor(Color(0, 255, 0, 255));
		temperaturePlot->setFillColor(Color(0, 255, 0, 50));
		temperaturePlot->setPointerColor(Color(0, 255, 0, 255));
		temperaturePlot->setAxisShown(true);
		temperaturePlot->setMainLineShown(true);
		temperaturePlot->setTextShown(true);
		temperaturePlot->setNumberFormatMode(GraphElement::FormattingMode::Normal);
		temperaturePlot->setMajorTickNumber(3);
		temperaturePlot->setMinorTickNumber(4);
		temperaturePlot->setFill(properties->curveFill);
		// Link plots to data
		reactivityPlot->setXdata(reactor->time_);
		reactivityPlot->setYdata(reactor->reactivity_);
		rodReactivityPlot->setXdata(reactor->time_);
		rodReactivityPlot->setYdata(reactor->rodReactivity_);
		powerPlot->setXdata(reactor->time_);
		powerPlot->setYdata(reactor->state_vector_[0]);
		powerPlot->setValueComputing([this](double* val, const size_t /*index*/) { *val = reactor->powerFromNeutrons(*val); });
		temperaturePlot->setXdata(reactor->time_);
		temperaturePlot->setYdata(reactor->temperature_);
		// Link plots to display interval
		//for (size_t i = 0; i < canvas->graphNumber(); i++) {
		//	canvas->getPlot(i)->setPlotRange(displayInterval[0], displayInterval[1]);
		//}
	}

	vector<string> comPorts;
	vector<string> lastCOMports;
	void initializeSerial() {
		try {
			comPorts = getCOMports();
			lastCOMports = comPorts;
			lastBoxCheck = nanogui::get_seconds_since_epoch();
			for (size_t i = 0; i < comPorts.size(); i++) {
				if (!boxConnected) tryConnectingTo(comPorts[i]);
			}
		}
		catch (exception e) {
			std::cout << "Exception while reading COM ports!" << std::endl;
		}
	}

	void updateCOMports() {
		if (boxConnected) return;

		double now = nanogui::get_seconds_since_epoch();
		if (now > lastBoxCheck + 5.) {
			try {
				lastCOMports = comPorts;
				comPorts = getCOMports();
				if (comPorts.size() > lastCOMports.size()) {
					string port;
					for (size_t i = 0; i < comPorts.size(); i++) { // handle new ports
						port = comPorts[i];
						for (size_t j = 0; j < lastCOMports.size(); j++) {
							if (port == lastCOMports[j]) {
								port = "";
								break;
							}
						}
						if (port.length() && !boxConnected) tryConnectingTo(port);
					}
				}
			}
			catch (exception e) {
				std::cout << "Exception while reading COM ports!" << std::endl;
			}
			lastBoxCheck = now;
		}
	}

	void tryConnectingTo(string port) {
		cout << "Connecting to " << port << "..." << endl;
		try {
			theBox = new Serial(port.c_str());;
		}
		catch (exception e) {
			std::cout << "Exception while astablishing connection with " << port << "!" << std::endl;
		}
		bool flag = false;
		if (theBox->IsConnected()) {
			if (theBox->availableBytes() >= 11) {
				char buffer[11];
				theBox->ReadData(buffer, 11);
				flag = true;
				for (int i = 0; i < 11; i++) {
					if (buffer[i] != box_auth[i]) {
						flag = false;
						break;
					}
				}
			}
		}
		if (flag) {
			// Successfull authentication
			std::cout << "Box found on " << theBox->GetName() << "!" << std::endl;
		}
		else {
			theBox->~Serial();
		}
		boxConnected = flag;
	}

	void initializePulseGraph() {
		pulseGraph->setBackgroundColor(Color(245, 255));
		pulseGraph->setTextColor(Color(16, 255));
		pulseGraph->setDrawBackground(true);
		pulseGraph->setPadding(90, 25, 220, 50);

		for (int i = 0; i < 4; i++) {
			pulsePlots[i] = pulseGraph->addPlot(reactor->getDataLength(), true);
			pulsePlots[i]->setXdata(reactor->time_);
			pulsePlots[i]->setNumberFormatMode((i < 3) ? GraphElement::FormattingMode::Normal : GraphElement::FormattingMode::Exponential);
			pulsePlots[i]->setDrawMode(DrawMode::Default);
			pulsePlots[i]->setAxisShown(i > 0);
			pulsePlots[i]->setTextShown(i > 0);
			pulsePlots[i]->setPointerShown(i > 0);
		}
		pulsePlots[3]->setName("Power");
		pulsePlots[3]->setUnits("W");
		pulsePlots[3]->setAxisPosition(GraphElement::AxisLocation::Right);
		pulsePlots[3]->setColor(Color(255, 0, 0, 255));
		pulsePlots[3]->setTextOffset(60.f);
		pulsePlots[3]->setMajorTickNumber(4);
		pulsePlots[3]->setMinorTickNumber(4);
		pulsePlots[3]->setPointerOverride(true);
		pulsePlots[3]->setHorizontalPointerColor(Color(64, 255));
		pulsePlots[3]->setHorizontalPointerShown(true);
		pulsePlots[3]->setHorizontalAxisShown(true);
		pulsePlots[3]->setHorizontalMinorTickNumber(4);
		pulsePlots[3]->setHorizontalName("Time");
		pulsePlots[3]->setHorizontalUnits("s");
		pulsePlots[3]->setHorizontalTextOffset(20.f);
		pulsePlots[3]->setYdata(reactor->state_vector_[0]);
		pulsePlots[3]->setValueComputing([this](double* val, const size_t /*index*/) { *val = reactor->powerFromNeutrons(*val); }); // convert neutrons to watts

		pulsePlots[2]->setName("Reactivity");
		pulsePlots[2]->setUnits("pcm");
		pulsePlots[2]->setColor(Color(0, 0, 255, 255));
		pulsePlots[2]->setYdata(reactor->reactivity_);
		pulsePlots[2]->setAxisShown(true);
		pulsePlots[2]->setPointerColor(Color(0, 0, 255, 255));
		pulsePlots[2]->setMainLineShown(true);
		pulsePlots[2]->setRoundFloating(true);
		pulsePlots[2]->setMajorTickNumber(4);
		pulsePlots[2]->setMinorTickNumber(4);
		pulsePlots[2]->setAxisPosition(GraphElement::AxisLocation::Right);
		pulsePlots[2]->setAxisOffset(110.f);
		pulsePlots[2]->setTextOffset(60.f);

		pulsePlots[0]->setColor(Color(200, 255));
		pulsePlots[0]->setYdata(reactor->rodReactivity_);
		pulsePlots[0]->setAxisPosition(GraphElement::AxisLocation::Right);
		pulsePlots[0]->setAxisOffset(110.f);

		pulsePlots[1]->setColor(Color(0, 255, 0, 255));
		pulsePlots[1]->setYdata(reactor->temperature_);
		pulsePlots[1]->setName("Temperature");
		pulsePlots[1]->setUnits("C");
		pulsePlots[1]->setPointerColor(Color(0, 255, 0, 255));
		pulsePlots[1]->setAxisShown(true);
		pulsePlots[1]->setMainLineShown(true);
		pulsePlots[1]->setMajorTickNumber(4);
		pulsePlots[1]->setMinorTickNumber(4);
	}

	SimulatorGUI() : nanogui::Screen(Vector2i(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT), "Research reactor simulator") {
		cout << "======= Research reactor simulator " << version() << " =======" << endl;

		//Load settings
		properties = new Settings();

		//Keyboard
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) lastKeyPressed[i] = false;

		// Theme
		this->mTheme->mTabInnerMargin = 0;
		this->mTheme->mStandardFontSize = 18;
		this->mTheme->mTextColor = Color(0.92f, 1.f);
		this->mTheme->mButtonCornerRadius = 2.f;
		this->mTheme->mTabMaxButtonWidth = 250.f;
		this->mTheme->mTabButtonVerticalPadding = 7.f;
		this->mTheme->mBorderDark = coolBlue;
		this->mTheme->mBorderLight = Color(coolBlue.r(), coolBlue.g(), coolBlue.b(), 0.4f);
		this->mTheme->mBorderWidth = 0.8f;
		this->mTheme->mTextBoxFontSize = 18.f;
		// Set minimum size
		glfwSetWindowSizeLimits(mGLFWWindow, WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);

		// Icon
		GLFWimage icons[WINDOW_ICON_NUM];
		int idx = 0;
		for (int size = 24; size < 12*(2+WINDOW_ICON_NUM); size += 12) {
			icons[idx].width = size;
			icons[idx].height = size;
			icons[idx].pixels = createPixelData(size, size);
			idx++;
		}
		glfwSetWindowIcon(mGLFWWindow, WINDOW_ICON_NUM, icons);
		

		// Initialize the reactor simulator
		initializeSimulator();

		// Initialize THE BOX
		memset(btns, false, 11 * sizeof(bool));
		if (!reactor->scriptCommands.size())
			initializeSerial();
		if (boxConnected) {
			std::cout << "===========The Box Mk. III===========" << std::endl;
		}

		RelativeGridLayout* baseLayout = new RelativeGridLayout();
		baseLayout->appendCol(1.f);
		baseLayout->appendRow(1.f);
		this->setLayout(baseLayout);
		baseWindow = this->add<Window>("");
		baseLayout->setAnchor(baseWindow, RelativeGridLayout::makeAnchor(0, 0));

		// Create a layout for the window
		relativeLayout = new RelativeGridLayout();
		relativeLayout->appendCol(1.f);
		relativeLayout->appendRow(properties->graphSize);													// 0 graph
		relativeLayout->appendRow(1.f - properties->graphSize);												// 1 controls
		relativeLayout->appendRow(RelativeGridLayout::Size(2.f, RelativeGridLayout::SizeType::Fixed));		// 2 border
		relativeLayout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));		// 3 bottom panel
		baseWindow->setLayout(relativeLayout);
		baseWindow->setBackgroundColor(Color(80, 255));
		baseWindow->setDrawBackground(true);

		Widget* bottomBorder = baseWindow->add<Widget>();
		bottomBorder->setBackgroundColor(Color(255, 255));
		bottomBorder->setDrawBackground(true);
		bottomBorder->setBackgroundColor(coolBlue);
		relativeLayout->setAnchor(bottomBorder, RelativeGridLayout::makeAnchor(0, 2));

		// Create the graph element
		initializeGraph();

		// Create the bottom panel
		createBottomPanel();

		// Create a place for all the other stuff
		tabControl = baseWindow->add<TabWidget>();
		tabControl->header()->setStretch(true);
		tabControl->header()->setButtonAlignment(NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_MIDDLE);
		tabControl->header()->setFontSize(20.f);
		relativeLayout->setAnchor(tabControl, RelativeGridLayout::makeAnchor(0, 1));

		// Create main tab
		createMainTab();

		// Create graph screen tab
		createGraphSettingsTab();

		// Create physics settings tab
		createPhysicsSettingsTab();

		// Create rod settings tab
		createRodSettingsTab();

		//Create delayed groups tab
		createDelayedGroupsTab();

		//Create data in/out tab
		//Widget* data_tab = tabControl->createTab("Data tab settings");
		//data_tab->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 10, 10));

		// Create operation modes tab
		createOperationModesTab();

		// Create operational limits and conditions tab
		createOperationalLimitsTab();

		// Create pulse tab
		createPulseTab();

		// Create other tab
		createOtherTab();

		tabControl->setActiveTab(0);

		// Create layout
		performLayout();
	}

	// Bottom panel initialization
	void createBottomPanel() {
		Widget* bottomPanel = baseWindow->add<Widget>();
		bottomPanel->setId("bottom panel");
		bottomPanel->setDrawBackground(true);
		bottomPanel->setBackgroundColor(Color(35, 255));
		relativeLayout->setAnchor(bottomPanel, RelativeGridLayout::makeAnchor(0, 3));
		RelativeGridLayout* bottomLayout = new RelativeGridLayout();
		bottomLayout->appendRow(1.f);
		bottomLayout->appendCol(RelativeGridLayout::Size(120.f, RelativeGridLayout::SizeType::Fixed));	// 0 version label
		bottomLayout->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));	// 1 border
		bottomLayout->appendCol(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed));	// 2 fps label
		bottomLayout->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));	// 3 border
		bottomLayout->appendCol(1.f);																	// 4 speed label
		bottomLayout->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));	// 5 border
		bottomLayout->appendCol(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed));	// 6 buttons
		bottomLayout->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));	// 7 border
		bottomLayout->appendCol(RelativeGridLayout::Size(80.f, RelativeGridLayout::SizeType::Fixed));	// 8 simulation time factor
		bottomLayout->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));	// 9 border
		bottomLayout->appendCol(RelativeGridLayout::Size(150.f, RelativeGridLayout::SizeType::Fixed));	// 10 time label
		bottomPanel->setLayout(bottomLayout);

		for (int i = 1; i < 11; i += 2) {
			Widget* border = bottomPanel->add<Widget>();
			border->setDrawBackground(true);
			border->setBackgroundColor(coolBlue);
			bottomLayout->setAnchor(border, RelativeGridLayout::makeAnchor(i, 0));
		}

		Label* versionLabel = bottomPanel->add<Label>("v " + version());
		versionLabel->setTextAlignment(Label::TextAlign::LEFT | Label::TextAlign::VERTICAL_CENTER);
		versionLabel->setPadding(0, 5.f);
		versionLabel->setFontSize(20.f);
		versionLabel->setColor(Color(255, 255));
		bottomLayout->setAnchor(versionLabel, RelativeGridLayout::makeAnchor(0, 0));

		fpsLabel = bottomPanel->add<Label>("FPS: ");
		fpsLabel->setTextAlignment(Label::TextAlign::LEFT | Label::TextAlign::VERTICAL_CENTER);
		fpsLabel->setFontSize(20.f);
		fpsLabel->setColor(Color(255, 255));
		fpsLabel->setPadding(0, 5.f);
		bottomLayout->setAnchor(fpsLabel, RelativeGridLayout::makeAnchor(2, 0));

		Label* speedText = bottomPanel->add<Label>("Simulation speed:");
		speedText->setPadding(2, 5);
		speedText->setColor(Color(255, 255));
		speedText->setFontSize(20.f);
		speedText->setTextAlignment(Label::TextAlign::RIGHT | Label::TextAlign::VERTICAL_CENTER);
		bottomLayout->setAnchor(speedText, RelativeGridLayout::makeAnchor(4, 0, 1, 1, Alignment::Maximum));

		Widget* speedToolPanel = bottomPanel->add<Widget>();
		bottomLayout->setAnchor(speedToolPanel, RelativeGridLayout::makeAnchor(6, 0, 1, 1, Alignment::Middle));
		speedToolPanel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 5));
		slowDown = speedToolPanel->add<ToolButton>(ENTYPO_ICON_FB);
		slowDown->setFlags(Button::Flags::NormalButton);
		slowDown->setCallback([this]() {
			if (!reactor->isPaused()) {
				this->setSimulationTime(std::max((int)selectedTime - 1, 0));
			}
		});
		playPause = speedToolPanel->add<ToolButton>(ENTYPO_ICON_PAUS);
		playPause->setChangeCallback([this](bool value) {
			playPauseSimulation(!value);
		});
		speedUp = speedToolPanel->add<ToolButton>(ENTYPO_ICON_FF);
		speedUp->setFlags(Button::Flags::NormalButton);
		speedUp->setCallback([this]() {
			if (!this->reactor->isPaused()) {
				this->setSimulationTime(std::min((int)selectedTime + 1, SIM_TIME_FACTOR_NUMBER - 1));
			}
		});

		simFactorLabel = bottomPanel->add<Label>("real-time");
		simFactorLabel->setColor(Color(255, 255));
		simFactorLabel->setFontSize(20.f);
		simFactorLabel->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
		bottomLayout->setAnchor(simFactorLabel, RelativeGridLayout::makeAnchor(8, 0));

		timeLabel = bottomPanel->add<Label>("00:00:00");
		timeLabel->setColor(Color(255, 255));
		timeLabel->setFontSize(20.f);
		timeLabel->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
		bottomLayout->setAnchor(timeLabel, RelativeGridLayout::makeAnchor(10, 0));

		simStatusLabel = bottomPanel->add<Label>(utf8(ENTYPO_ICON_PLAY).data(), "icons");
		simStatusLabel->setColor(Color(255, 255));
		simStatusLabel->setFontSize(40.f);
		simStatusLabel->setFixedWidth(30);
		simStatusLabel->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
		bottomLayout->setAnchor(simStatusLabel, RelativeGridLayout::makeAnchor(10, 0, 1, 1, Alignment::Minimum, Alignment::Fill));
	}

	void createMainTab() {
		Widget* mainTabBase = tabControl->createTab("Main controls");
		mainTabBase->setId("main tab");
		RelativeGridLayout* rel3 = new RelativeGridLayout();
		rel3->appendCol(RelativeGridLayout::Size(250.f, RelativeGridLayout::SizeType::Fixed)); // 0 first column
		rel3->appendCol(RelativeGridLayout::Size(250.f, RelativeGridLayout::SizeType::Fixed)); // 1 second column
		rel3->appendCol(RelativeGridLayout::Size(80.f, RelativeGridLayout::SizeType::Fixed));  // 2 reactivitycolumn
		rel3->appendCol(RelativeGridLayout::Size(7.f, RelativeGridLayout::SizeType::Fixed));   // 3 padding
		rel3->appendCol(RelativeGridLayout::Size(190.f, RelativeGridLayout::SizeType::Fixed)); // 4 control rod display
		rel3->appendCol(RelativeGridLayout::Size(7.f, RelativeGridLayout::SizeType::Fixed));   // 5 padding
		rel3->appendCol(1.f);																   // 6 main content
		rel3->appendCol(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed)); // 7 alarms
		rel3->appendRow(1.f);
		mainTabBase->setLayout(rel3);

		// Create the laft hand side of the main screen
		createDataDisplays(mainTabBase, rel3);

		// Create a panel for the buttons of the main window
		Widget* main_right = mainTabBase->add<Widget>();
		rel3->setAnchor(main_right, RelativeGridLayout::makeAnchor(6, 0));
		main_right->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 10));

		// Create a panel for reactivity controls
		/*Widget* reactivityPanel = main_right->add<Widget>();
		reactivityPanel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));*/

		Widget* controlButtonsPanel = main_right->add<Widget>();
		controlButtonsPanel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));
		Button *scram;
		scram = controlButtonsPanel->add<Button>("SCRAM", ENTYPO_ICON_NEW);
		scram->setBackgroundColor(Color(255, 0, 0, 100));
		scram->setFixedSize(Vector2i(150, 45));
		scram->setTextColor(Color(255, 255));
		scram->setCallback([this] { reactor->scram(Simulator::ScramSignals::User); });
		Button *fire;
		fire = controlButtonsPanel->add<Button>("FIRE");
		fire->setBackgroundColor(Color(255, 0, 0, 100));
		fire->setFixedSize(Vector2i(150, 45));
		fire->setTextColor(Color(255, 255));
		fire->setCallback([this] { reactor->beginPulse(); });


		// Rod mode
		Widget* panel = main_right->add<Widget>();
		panel->setLayout(panelsLayout);
		rodMode = panel->add<ComboBox>(modes);
		rodMode->setFixedWidth(150);
		rodMode->setCallback([this](int change) {
			switch (change) {
			case 0:
				reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);
				break;
			case 1:
				reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Simulation);
				reactor->regulatingRod()->setSimulationMode(SimulationModes::SquareWaveMode);
				break;
			case 2:
				reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Simulation);
				reactor->regulatingRod()->setSimulationMode(SimulationModes::SineMode);
				break;
			case 3:
				reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Simulation);
				reactor->regulatingRod()->setSimulationMode(SimulationModes::SawToothMode);
				break;
			case 4:
				reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Automatic);
				reactor->setPowerHold(reactor->getCurrentPower());
				break;
			case 5:
				reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Pulse);
				break;
			}
		});
		rodMode->setSelectedIndex(0);

		Button *unscram;
		unscram = main_right->add<Button>("RESET SCRAM", ENTYPO_ICON_CCW);
		unscram->setTextColor(Color(255, 255));
		unscram->setFixedWidth(150);
		unscram->setCallback([this] { reactor->scram(Simulator::ScramSignals::None); });

		// Create reactivity display
		periodDisplay = mainTabBase->add<PeriodDisplay>();
		rel3->setAnchor(periodDisplay, RelativeGridLayout::makeAnchor(2, 0));

		// Create rod display
		Widget* controlRodBase = mainTabBase->add<Widget>();
		rel3->setAnchor(controlRodBase, RelativeGridLayout::makeAnchor(4, 0));
		RelativeGridLayout* rodLayout = new RelativeGridLayout();
		rodLayout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));
		rodLayout->appendRow(1.f);
		rodLayout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));
		for(int i = 0; i < 3; i++) rodLayout->appendCol(1.f);
		controlRodBase->setLayout(rodLayout);

		Label* temp;
		Widget* backg = controlRodBase->add<Widget>();
		backg->setDrawBackground(true);
		backg->setBackgroundColor(Color(.15f, 1.f));
		rodLayout->setAnchor(backg, RelativeGridLayout::makeAnchor(0, 0, 3));
		rodDisplay = controlRodBase->add<ControlRodDisplay>();
		for (int i = 0; i < 3; i++) {
			temp = controlRodBase->add<Label>(i ? ((i == 1) ? "R" : "C") : "S", "sans-bold");
			temp->setFontSize(25.f);
			temp->setColor(Color(255, 255));
			if (i != 1) temp->setPadding(2 - i, ControlRodDisplay::getRodSpacing() * 2 / 3);
			temp->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
			rodLayout->setAnchor(temp, RelativeGridLayout::makeAnchor(i, 0));
			rodDisplay->setRod(i, reactor->rods[i]->getRodSteps(), reactor->rods[i]->getActualPosition(), reactor->rods[i]->getExactPosition(), reactor->rods[i]->isEnabled());
		}
		rodLayout->setAnchor(rodDisplay, RelativeGridLayout::makeAnchor(0, 1, 3));

		// Create a panel for alarms
		Widget* alarmPanel = mainTabBase->add<Widget>();
		alarmPanel->setDrawBackground(true);
		alarmPanel->setBackgroundColor(Color(40, 255));
		rel3->setAnchor(alarmPanel, RelativeGridLayout::makeAnchor(7, 0));
		RelativeGridLayout* alarmLayout = new RelativeGridLayout();
		alarmLayout->appendCol(1.f);
		alarmLayout->appendRow(RelativeGridLayout::Size(35.f, RelativeGridLayout::SizeType::Fixed));
		alarmPanel->setLayout(alarmLayout);

		Label* alarmHeader = alarmPanel->add<Label>("SCRAMs");
		alarmHeader->setFontSize(24.f);
		alarmHeader->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
		alarmLayout->setAnchor(alarmHeader, RelativeGridLayout::makeAnchor(0, 0));
		// Alarm labels
		Label* alarmLabels[6];
		std::string text[6] = { "MAN", "PER", "FTEMP", "WTEMP", "POW", "WLEVEL" };
		RelativeGridLayout::Anchor a;
		for (int i = 0; i < 6; i++) {
			if (i < 5) {
				alarmLayout->appendRow(1.f);
				alarmLabels[i] = alarmPanel->add<Label>(text[i], "sans-bold");
				a = RelativeGridLayout::makeAnchor(0, i + 1);
				a.padding = Vector4i(14, 7, 14, 7);
				alarmLayout->setAnchor(alarmLabels[i], a);
			}
			else {
				alarmLabels[i] = new Label(nullptr, text[i], "sans-bold");
			}
			alarmLabels[i]->setColor(Color(255, 255));
			alarmLabels[i]->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
			alarmLabels[i]->setFixedSize(Vector2i(60, 40));
			alarmLabels[i]->setGlow(false);
			alarmLabels[i]->setBackgroundColor(Color(100, 255));
			alarmLabels[i]->setDrawBackground(true);
			alarmLabels[i]->setGlowAmount(20.f);
			alarmLabels[i]->setGlowColor(Color(255, 0, 0, 125));
		}
		userScram = alarmLabels[0];
		periodScram = alarmLabels[1];
		fuelTemperatureScram = alarmLabels[2];
		waterTemperatureScram = alarmLabels[3];
		powerScram = alarmLabels[4];
		waterLevelScram = alarmLabels[5];

		// FOR NOW
		waterLevelScram->setVisible(false);

		// Controls
		{
			for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
				rodBox[i] = makeSettingLabel<IntBox<int>>(main_right, reactor->rods[i]->getRodName() + " rod magnet position: ", 200);
				rodBox[i]->setFixedSize(Vector2i(100, 20));
				rodBox[i]->setUnits("steps");
				rodBox[i]->setValueIncrement(1);
				rodBox[i]->setDefaultValue("0");
				rodBox[i]->setFontSize(16);
				rodBox[i]->setFormat("[0-9]+");
				rodBox[i]->setMinMaxValues(0, (int)*reactor->rods[i]->getRodSteps());
				rodBox[i]->setSpinnable(true);
				rodBox[i]->setCallback([this, i](const int change) {
					try {
						this->reactor->rods[i]->commandMove((size_t)change);
					}
					catch (exception e) {
						return false;
					}
					return true;
				});
			}
			
		}

		// Checkboxes
		{
			Widget* checkBoxCooling = main_right->add<Widget>();
			checkBoxCooling->setLayout(panelsLayout);
			checkBoxCooling->add<Label>("Cooling :", "sans-bold");
			cooling = checkBoxCooling->add<SliderCheckBox>();
			cooling->setFontSize(16);
			cooling->setChecked(false);
			cooling->setCallback([this](bool value) {
				properties->waterCooling = value;
				reactor->setWaterCooling(value);
				if (value) {
					//cooling->setCaption("enabled");
				}
				else {
					//cooling->setCaption("disabled");
				}
			});
			
			Widget* checkBoxPanelNeutronSource = main_right->add<Widget>();
			checkBoxPanelNeutronSource->setLayout(panelsLayout);
			checkBoxPanelNeutronSource->add<Label>("Neutron source :", "sans-bold");
			neutronSourceCB = checkBoxPanelNeutronSource->add<SliderCheckBox>();
			neutronSourceCB->setFontSize(16);
			neutronSourceCB->setChecked(reactor->getNeutronSourceInserted());
			neutronSourceCB->setCallback([this](bool value) {
				reactor->setNeutronSourceInserted(value);
				properties->neutronSourceInserted = value;
			});
		}
	}

	void createGraphSettingsTab() {
		Widget* graph_controls = tabControl->createTab("Graph controls");
		graph_controls->setId("graph controls");
		RelativeGridLayout* graphControlsLayout = new RelativeGridLayout();
		graphControlsLayout->appendRow(RelativeGridLayout::Size(140.f, RelativeGridLayout::SizeType::Fixed));
		graphControlsLayout->appendRow(RelativeGridLayout::Size(10.f, RelativeGridLayout::SizeType::Fixed));
		graphControlsLayout->appendRow(RelativeGridLayout::Size(2.f, RelativeGridLayout::SizeType::Fixed));
		graphControlsLayout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));
		graphControlsLayout->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));
		graphControlsLayout->appendRow(1.f);
		graphControlsLayout->appendCol(1.f);
		graphControlsLayout->appendCol(1.f);
		graph_controls->setLayout(graphControlsLayout);
		Widget* border = graph_controls->add<Widget>();
		border->setBackgroundColor(coolBlue);
		border->setDrawBackground(true);
		graphControlsLayout->setAnchor(border, RelativeGridLayout::makeAnchor(0, 2, 2, 1));

		// Create a panel for graph size
		Widget* generalLeftPanel = new Widget(graph_controls);
		graphControlsLayout->setAnchor(generalLeftPanel, RelativeGridLayout::makeAnchor(0, 0));
		generalLeftPanel->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 10, 10));
		Widget* graphSizePanel = generalLeftPanel->add<Widget>();
		graphSizePanel->setLayout(panelsLayout);

		graphSizePanel->add<Label>("Graph size :", "sans-bold");
		graphSizeBox = graphSizePanel->add<IntBox<int>>((int)(100 * properties->graphSize));
		graphSizeBox->setUnits("%");
		graphSizeBox->setDefaultValue(to_string((int)std::roundf(properties->graphSize * 100)));
		graphSizeBox->setFontSize(16);
		graphSizeBox->setFormat("[0-9]+");
		graphSizeBox->setSpinnable(true);
		graphSizeBox->setMinValue(30);
		graphSizeBox->setMaxValue(70);
		graphSizeBox->setValueIncrement(1);
		graphSizeBox->setCallback([this](int a) {
			if (a > 70) a = 70;
			if (a < 30) a = 30;
			relativeLayout->setRowSize(0, a / 100.f);
			relativeLayout->setRowSize(1, 1.f - a / 100.f);
			performLayout();
			properties->graphSize = a / 100.f;
		});

		// Create a panel for display time
		Widget* timePanel = generalLeftPanel->add<Widget>();
		timePanel->setLayout(panelsLayout);

		// Create a panel for graph limits
		Widget* reactivityLimitsPanel = generalLeftPanel->add<Widget>();
		reactivityLimitsPanel->setLayout(panelsLayout);

		// Create another panel for graph limits
		Widget* temperatureLimitsPanel = generalLeftPanel->add<Widget>();
		temperatureLimitsPanel->setLayout(panelsLayout);

		// Create a panel for curve fill, rod reactivity line visibility, log scale power and hardcore mode
		Widget* sliderPanel = graph_controls->add<Widget>();
		graphControlsLayout->setAnchor(sliderPanel, RelativeGridLayout::makeAnchor(1, 0, 1, 1, Alignment::Maximum, Alignment::Fill));

		RelativeGridLayout* sliderLayout = new RelativeGridLayout();
		for(int i = 0; i < 4; i++) sliderLayout->appendCol((i % 2) ? RelativeGridLayout::Size(10.f, RelativeGridLayout::SizeType::Fixed) : 1.f);
		for (int i = 0; i < 4; i++) sliderLayout->appendRow(1.f);
		sliderPanel->setLayout(sliderLayout);

		sliderLayout->setAnchor(sliderPanel->add<Label>("Rod reactivity plot:", "sans-bold"), RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Minimum, Alignment::Middle));
		rodReactivityBox = sliderPanel->add<SliderCheckBox>();
		sliderLayout->setAnchor(rodReactivityBox, RelativeGridLayout::makeAnchor(2, 0, 1, 1, Alignment::Maximum, Alignment::Middle));
		rodReactivityBox->setFontSize(16);
		rodReactivityBox->setChecked(properties->rodReactivityPlot);
		rodReactivityBox->setCallback([this](bool value) {
			properties->rodReactivityPlot = value;
			rodReactivityPlot->setEnabled(value && !properties->reactivityHardcore);
		});

		sliderLayout->setAnchor(sliderPanel->add<Label>("Curve fill:", "sans-bold"), RelativeGridLayout::makeAnchor(0, 1, 1, 1, Alignment::Minimum, Alignment::Middle));
		curveFillBox = sliderPanel->add<SliderCheckBox>();
		sliderLayout->setAnchor(curveFillBox, RelativeGridLayout::makeAnchor(2, 1, 1, 1, Alignment::Maximum, Alignment::Middle));
		curveFillBox->setFontSize(16);
		curveFillBox->setChecked(properties->curveFill);
		curveFillBox->setCallback([this](bool value) {
			rodReactivityPlot->setFill(value);
			reactivityPlot->setFill(value);
			powerPlot->setFill(value);
			temperaturePlot->setFill(value);
			properties->curveFill = value;
		});

		timePanel->add<Label>("Display time :", "sans-bold");
		displayBox = timePanel->add<FloatBox<float>>(properties->displayTime);
		displayBox->setFixedSize(Vector2i(100, 20));
		displayBox->setUnits("s");
		displayBox->setDefaultValue(formatDecimals((double)properties->displayTime, 1));
		displayBox->setFontSize(16);
		displayBox->setFormat("[0-9]*[.]?[0-9]?");
		displayBox->setSpinnable(true);
		displayBox->setMinMaxValues(0.5f, (float)reactor->getDeleteOldValues());
		displayBox->setValueIncrement(1.f);
		displayBox->setCallback([this](float a) {
			properties->displayTime = std::min((float)reactor->getDeleteOldValues(), std::max(a, 0.5f));
			std::string limit = formatDecimals((double)properties->displayTime, 1) + " seconds ago";
			powerPlot->setLimitOverride(0, limit);
			delayedGroups[0]->setLimitOverride(0, limit);
		});

		{
			Label* temp = reactivityLimitsPanel->add<Label>("Reactivity graph:  from ", "sans-bold");
			temp->setFixedWidth(160);
			reactivityLimitBox[0] = reactivityLimitsPanel->add<FloatBox<float>>(properties->reactivityGraphLimits[0]);
			reactivityLimitBox[0]->setFixedSize(Vector2i(100, 20));
			reactivityLimitBox[0]->setUnits("pcm");
			reactivityLimitBox[0]->setDefaultValue(to_string(properties->reactivityGraphLimits[0]));
			reactivityLimitBox[0]->setFontSize(16);
			reactivityLimitBox[0]->setFormat("[-]?[0-9]*[.]?[0-9]?");
			reactivityLimitBox[0]->setSpinnable(true);
			reactivityLimitBox[0]->setValueIncrement(1.f);
			reactivityLimitBox[0]->setCallback([this](float a) {
				properties->reactivityGraphLimits[0] = a;
			});

			reactivityLimitsPanel->add<Label>(" to ", "sans-bold");
			reactivityLimitBox[1] = reactivityLimitsPanel->add<FloatBox<float>>(properties->reactivityGraphLimits[1]);
			reactivityLimitBox[1]->setFixedSize(Vector2i(100, 20));
			reactivityLimitBox[1]->setUnits("pcm");
			reactivityLimitBox[1]->setDefaultValue(to_string(properties->reactivityGraphLimits[1]));
			reactivityLimitBox[1]->setFontSize(16);
			reactivityLimitBox[1]->setFormat("[-]?[0-9]*[.]?[0-9]?");
			reactivityLimitBox[1]->setSpinnable(true);
			reactivityLimitBox[1]->setValueIncrement(1.f);
			reactivityLimitBox[1]->setCallback([this](float a) {
				properties->reactivityGraphLimits[1] = a;
			}); 
			Button* btn = reactivityLimitsPanel->add<Button>("Reset");
			btn->setCallback([this]() {
				reactivityLimitBox[0]->setValue(reactor->getExcessReactivity() - reactor->getTotalRodWorth());
				reactivityLimitBox[1]->setValue(static_cast<int>(reactor->getExcessReactivity()));
			});
		}

		{
			// Temperature stuff
			Label* temp = temperatureLimitsPanel->add<Label>("Temperature graph:  from ", "sans-bold");
			temp->setFixedWidth(160);
			temperatureLimitBox[0] = temperatureLimitsPanel->add<FloatBox<float>>(properties->temperatureGraphLimits[0]);
			temperatureLimitBox[0]->setFixedSize(Vector2i(100, 20));
			temperatureLimitBox[0]->setUnits(degCelsiusUnit);
			temperatureLimitBox[0]->setDefaultValue(to_string(properties->temperatureGraphLimits[0]));
			temperatureLimitBox[0]->setFontSize(16);
			temperatureLimitBox[0]->setFormat("[-]?[0-9]*[.]?[0-9]?");
			temperatureLimitBox[0]->setMinValue(0.f);
			temperatureLimitBox[0]->setSpinnable(true);
			temperatureLimitBox[0]->setValueIncrement(1.f);
			temperatureLimitBox[0]->setCallback([this](float a) {
				properties->temperatureGraphLimits[0] = a;
			});

			temperatureLimitsPanel->add<Label>(" to ", "sans-bold");
			temperatureLimitBox[1] = temperatureLimitsPanel->add<FloatBox<float>>(properties->temperatureGraphLimits[1]);
			temperatureLimitBox[1]->setFixedSize(Vector2i(100, 20));
			temperatureLimitBox[1]->setUnits(degCelsiusUnit);
			temperatureLimitBox[1]->setDefaultValue(to_string(properties->temperatureGraphLimits[1]));
			temperatureLimitBox[1]->setFontSize(16);
			temperatureLimitBox[1]->setFormat("[-]?[0-9]*[.]?[0-9]?");
			temperatureLimitBox[1]->setMinValue(0.f);
			temperatureLimitBox[1]->setSpinnable(true);
			temperatureLimitBox[1]->setValueIncrement(1.f);
			temperatureLimitBox[1]->setCallback([this](float a) {
				properties->temperatureGraphLimits[1] = a;
			});

			Button* btn = temperatureLimitsPanel->add<Button>("Reset");
			btn->setCallback([this]() {
				temperatureLimitBox[0]->setValue(TEMPERATURE_GRAPH_FROM_DEFAULT);
				temperatureLimitBox[1]->setValue(TEMPERATURE_GRAPH_TO_DEFAULT);
			});
		}

		sliderLayout->setAnchor(sliderPanel->add<Label>("Power log scale:", "sans-bold"), RelativeGridLayout::makeAnchor(0, 2, 1, 1, Alignment::Minimum, Alignment::Middle));
		logScaleBox = sliderPanel->add<SliderCheckBox>();
		sliderLayout->setAnchor(logScaleBox, RelativeGridLayout::makeAnchor(2, 2, 1, 1, Alignment::Maximum, Alignment::Middle));
		logScaleBox->setFontSize(16);
		logScaleBox->setChecked(properties->yAxisLog);
		logScaleBox->setCallback([this](bool value) {
			properties->yAxisLog = value;
			powerPlot->setYlog(value);
		});

		sliderLayout->setAnchor(sliderPanel->add<Label>("Hide reactivity:", "sans-bold"), RelativeGridLayout::makeAnchor(0, 3, 1, 1, Alignment::Minimum, Alignment::Middle));
		hardcoreBox = sliderPanel->add<SliderCheckBox>();
		sliderLayout->setAnchor(hardcoreBox, RelativeGridLayout::makeAnchor(2, 3, 1, 1, Alignment::Maximum, Alignment::Middle));
		hardcoreBox->setFontSize(16);
		hardcoreBox->setChecked(properties->yAxisLog);
		hardcoreBox->setCallback([this](bool value) {
			properties->reactivityHardcore = value;
			hardcoreMode(value);
		});

		Label* timeAdjLabel = graph_controls->add<Label>("Edit display range", "sans-bold");
		timeAdjLabel->setFontSize(25);
		timeAdjLabel->setPadding(0, 15);
		graphControlsLayout->setAnchor(timeAdjLabel, RelativeGridLayout::makeAnchor(0, 3, 1, 1, Alignment::Minimum, Alignment::Minimum));

		Widget* timeLockPanel = graph_controls->add<Widget>();
		graphControlsLayout->setAnchor(timeLockPanel, RelativeGridLayout::makeAnchor(0, 4, 1, 1, Alignment::Minimum, Alignment::Fill));
		RelativeGridLayout* tlLayout = new RelativeGridLayout();
		tlLayout->appendCol(RelativeGridLayout::Size(120.f, RelativeGridLayout::SizeType::Fixed));
		tlLayout->appendCol(1.f);
		tlLayout->appendRow(1.f);
		timeLockPanel->setLayout(tlLayout);
		Label* tmp = timeLockPanel->add<Label>("Lock view:", "sans-bold");
		tlLayout->setAnchor(tmp, RelativeGridLayout::makeAnchor(0, 0));
		tmp->setPadding(0, 25);
		tmp->setPadding(2, 10);
		timeLockedBox = timeLockPanel->add<SliderCheckBox>();
		tlLayout->setAnchor(timeLockedBox, RelativeGridLayout::makeAnchor(1, 0));
		timeLockedBox->setFontSize(16);
		timeLockedBox->setChecked(false);
		timeLockedBox->setCallback([this](bool value) {
			if (value) {
				this->viewingIntervalChanged(true);
			}
			else {
				timeAtLastChange = this->reactor->getCurrentTime();
			}
		});

		displayTimeSlider = new IntervalSlider(graph_controls);
		RelativeGridLayout::Anchor acr = RelativeGridLayout::makeAnchor(0, 5, 2, 1, Alignment::Fill, Alignment::Minimum);
		acr.padding = Vector4i(20, 10, 20, 0);
		graphControlsLayout->setAnchor(displayTimeSlider, acr);
		displayTimeSlider->setHighlightColor(coolBlue);
		displayTimeSlider->setSteps((unsigned int)DELETE_OLD_DATA_TIME_DEFAULT * 1000U);
		displayTimeSlider->setEnabled(true);
		displayTimeSlider->setFixedHeight(25);
		for (int i = 0; i < 2; i++) displayTimeSlider->setCallback(i, [this, i](float /*change*/) {
			this->viewingIntervalChanged(i == 0);
			if (i == 0 && !timeLockedBox->checked()) {
				timeAtLastChange = this->reactor->getCurrentTime();
			}
		});
		Button* displayResetBtn = new Button(graph_controls, "Reset view to newest");
		RelativeGridLayout::Anchor acr2 = RelativeGridLayout::makeAnchor(1, 3, 1, 1, Alignment::Maximum, Alignment::Middle);
		acr2.padding = Vector4i(0, 0, 10, 0);
		graphControlsLayout->setAnchor(displayResetBtn, acr2);
		displayResetBtn->setCallback([this]() { // reset the view to default
			this->viewStart = -1.;
			timeLockedBox->setChecked(false);
			timeLockedBox->callback()(false);
		});
	}

	void createPhysicsSettingsTab() {
		Widget* physics_settings_base = tabControl->createTab("Physics settings", false);
		TabWidget* modeTabs = physics_settings_base->add<TabWidget>();
		modeTabs->header()->setStretch(true);
		modeTabs->header()->setButtonAlignment(NVGalign::NVG_ALIGN_MIDDLE | NVGalign::NVG_ALIGN_CENTER);
		RelativeGridLayout* grid = new RelativeGridLayout();
		grid->appendCol(1.f);
		grid->appendRow(1.f);
		physics_settings_base->setLayout(grid);
		grid->setAnchor(modeTabs, RelativeGridLayout::makeAnchor(0, 0));

		// Create titles, periods and amplitudes
		std::string titles[2] = { "Physics","Neutron source" };
		//RelativeGridLayout* layouts[2];
		//Widget* tabs[2];
		

		Widget* physics_settings = modeTabs->createTab("Physics");
		physics_settings->setId("Physics tab");
		RelativeGridLayout* physicsLayout = new RelativeGridLayout();
		physicsLayout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));
		physicsLayout->appendRow(RelativeGridLayout::Size(4 * 46.f, RelativeGridLayout::SizeType::Fixed));
		physicsLayout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));
		physicsLayout->appendRow(1.f);
		physicsLayout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));
		physicsLayout->appendCol(RelativeGridLayout::Size(10.f, RelativeGridLayout::SizeType::Fixed));
		physicsLayout->appendCol(1.f);
		physicsLayout->appendCol(1.f);
		physicsLayout->appendCol(RelativeGridLayout::Size(10.f + SCROLL_BAR_THICKNESS, RelativeGridLayout::SizeType::Fixed));
		physics_settings->setLayout(physicsLayout);
		Widget* delayedPanel = physics_settings->add<Widget>();
		physicsLayout->setAnchor(delayedPanel, RelativeGridLayout::makeAnchor(1, 1, 2, 1));
		delayedPanel->setDrawBackground(true);
		delayedPanel->setBackgroundColor(Color(40, 40, 40, 255));
		RelativeGridLayout * relPhysics = new RelativeGridLayout();
		for (int i = 1; i < 10; i++)relPhysics->appendRow(RelativeGridLayout::Size((i%2) ? 1.f : 45.f, RelativeGridLayout::SizeType::Fixed));
		relPhysics->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));
		relPhysics->appendCol(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed));
		for (int i = 0; i < 13; i++) relPhysics->appendCol((i%2) ? 1.f : RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));
		delayedPanel->setLayout(relPhysics);

		// Source settings tab
		sourceSettings = modeTabs->createTab("Neutron source");
		sourceSettings->setId("Neutron source tab");
		RelativeGridLayout* sourceLayout = new RelativeGridLayout();
		sourceLayout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));
		for (int j = 0; j < 11; j++) {
			sourceLayout->appendRow(RelativeGridLayout::Size((j==2) ? 1.f : 30.f, RelativeGridLayout::SizeType::Fixed));
		}
		sourceLayout->appendRow(1.f);
		sourceLayout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));
		sourceLayout->appendCol(1.f);
		sourceLayout->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));
		sourceLayout->appendCol(RelativeGridLayout::Size(350.f, RelativeGridLayout::SizeType::Fixed));
		sourceLayout->appendCol(RelativeGridLayout::Size(SCROLL_BAR_THICKNESS, RelativeGridLayout::SizeType::Fixed));
		sourceSettings->setLayout(sourceLayout);
		// source seperator
		Widget* ns_border = sourceSettings->add<Widget>();
		ns_border->setDrawBackground(true);
		ns_border->setBackgroundColor(coolBlue);
		sourceLayout->setAnchor(ns_border, RelativeGridLayout::makeAnchor(1, 3, 2));
		// Source base
		sourceActivityBox = makeSettingLabel<FloatBox<double>>(sourceSettings, "Base intensity: ", 100, properties->neutronSourceActivity);
		sourceLayout->setAnchor(sourceActivityBox->parent(), RelativeGridLayout::makeAnchor(2, 1, 1, 1, Alignment::Minimum, Alignment::Middle));
		sourceActivityBox->setAlignment(TextBox::Alignment::Left);
		sourceActivityBox->setFixedSize(Vector2i(125, 30));
		sourceActivityBox->setSpinnable(true);
		sourceActivityBox->setValueIncrement(5e4);
		sourceActivityBox->setDefaultValue(std::to_string(sourceActivityBox->value()));
		sourceActivityBox->setMinValue(0.);
		sourceActivityBox->setFormat(SCI_NUMBER_FORMAT);
		sourceActivityBox->setUnits("n/s");
		sourceActivityBox->setCallback([this](double change) {
			properties->neutronSourceActivity = change;
			reactor->setNeutronSourceActivity(change);
		});
		neutronSourceModeBox = makeSettingLabel<ComboBox>(sourceSettings, "Source type:", 100, ns_modes);
		sourceLayout->setAnchor(neutronSourceModeBox->parent(), RelativeGridLayout::makeAnchor(2, 2, 1, 1, Alignment::Minimum, Alignment::Middle));
		neutronSourceModeBox->setFixedWidth(125);
		neutronSourceModeBox->setCallback([this](int change) {
			properties->ns_mode = (char)change;
			reactor->setNeutronSourceMode((SimulationModes)change);
			updateNeutronSourceTab();
		});
		neutronSourceModeBox->setSelectedIndex(0);
		// Source graph
		sourceGraph = sourceSettings->add<Graph>(2, "Neutron source simulation");
		sourceGraph->setDrawBackground(true);
		sourceGraph->setBackgroundColor(Color(32, 255));
		sourceLayout->setAnchor(sourceGraph, RelativeGridLayout::makeAnchor(0, 0, 1, 14, Alignment::Fill, Alignment::Fill));
		sourceGraph->setPadding(90.f, 20.f, 10.f, 70.f);
		sourceGraph->setPlotBackgroundColor(Color(60, 255));
		sourceGraph->setPlotGridColor(Color(177, 255));
		sourceGraph->setPlotBorderColor(Color(200, 255));
		PeriodicalMode* per;
		// source general
		for (int i = 0; i < 3; i++) {
			per = reactor->getSourceModeClass((SimulationModes)(i + 1));
			neutronSourcePeriodBoxes[i] = makeSettingLabel<FloatBox<float>>(sourceSettings, "Period: ", 100, per->getPeriod());
			sourceLayout->setAnchor(neutronSourcePeriodBoxes[i]->parent(), RelativeGridLayout::makeAnchor(2, 4, 1, 1, Alignment::Minimum, Alignment::Middle));
			neutronSourcePeriodBoxes[i]->setAlignment(TextBox::Alignment::Left);
			neutronSourcePeriodBoxes[i]->setFixedWidth(100);
			neutronSourcePeriodBoxes[i]->setFormat(SCI_NUMBER_FORMAT);
			neutronSourcePeriodBoxes[i]->setUnits("s");
			neutronSourcePeriodBoxes[i]->setMinValue(0.1f);
			neutronSourcePeriodBoxes[i]->setValueIncrement(0.1f);
			neutronSourcePeriodBoxes[i]->setSpinnable(true);
			neutronSourcePeriodBoxes[i]->setDefaultValue(formatDecimals(per->getPeriod(), 1));
			neutronSourcePeriodBoxes[i]->setCallback([this, i, per](float change) {
				switch (i) {
				case 0: properties->squareWave.period = change; break;
				case 1: properties->sineMode.period = change; break;
				case 2: properties->sawToothMode.period = change; break;
				}
				per->setPeriod(change);
				updateNeutronSourceTab();
			});
			neutronSourceAmplitudeBoxes[i] = makeSettingLabel<FloatBox<float>>(sourceSettings, "Intensity: ", 100, per->getAmplitude());
			sourceLayout->setAnchor(neutronSourceAmplitudeBoxes[i]->parent(), RelativeGridLayout::makeAnchor(2, 5));
			neutronSourceAmplitudeBoxes[i]->setAlignment(TextBox::Alignment::Left);
			neutronSourceAmplitudeBoxes[i]->setFixedWidth(100);
			neutronSourceAmplitudeBoxes[i]->setFormat(SCI_NUMBER_FORMAT);
			neutronSourceAmplitudeBoxes[i]->setUnits("n/s");
			neutronSourceAmplitudeBoxes[i]->setMinValue(0.f);
			neutronSourceAmplitudeBoxes[i]->setValueIncrement(1e4);
			neutronSourceAmplitudeBoxes[i]->setSpinnable(true);
			neutronSourceAmplitudeBoxes[i]->setDefaultValue(to_string(per->getAmplitude()));
			neutronSourceAmplitudeBoxes[i]->setCallback([per, i, this](float change) {
				switch (i) {
				case 0: properties->squareWave.amplitude = change; break;
				case 1: properties->sineMode.amplitude = change; break;
				case 2: properties->sawToothMode.amplitude = change; break;
				}
				per->setAmplitude(change);
				updateNeutronSourceTab();
			});
		}
		// source SQW
		float sqPeriod = properties->ns_squareWave.period;
		for (int sqw = 0; sqw < 4; sqw++) {
			neutronSourceSQWBoxes[sqw] = makeSimulationSetting(sourceSettings, (int)roundf(properties->ns_squareWave.xIndex[sqw] * 100), sqw_settingNames[sqw]);
			sourceLayout->setAnchor(neutronSourceSQWBoxes[sqw]->parent(), RelativeGridLayout::makeAnchor(2, 6 + sqw));
			neutronSourceSQWBoxes[sqw]->setCallback([this, sqw, sqPeriod](int change) {
				float val = change / 100.f;
				reactor->source_sqw->xIndex[sqw] = val;
				properties->squareWave.xIndex[sqw] = val;
				if (sqw != 3) {
					neutronSourceSQWBoxes[sqw + 1]->setMinValue(change);
					if (neutronSourceSQWBoxes[sqw + 1]->value() < change) { squareWaveBoxes[sqw + 1]->setValue(change); }
					else { updateNeutronSourceTab(); }
				}
				else {updateNeutronSourceTab();}
			});
		}
		// source sine
		neutronSourceSINEModeBox = makeSettingLabel<ComboBox>(sourceSettings, "Sine type: ", 100, sineModes);
		sourceLayout->setAnchor(neutronSourceSINEModeBox->parent(), RelativeGridLayout::makeAnchor(2, 6));
		neutronSourceSINEModeBox->setFixedWidth(125);
		neutronSourceSINEModeBox->setCallback([this](int change) {
			properties->ns_sineMode.mode = (Settings::SineSettings::SineMode)change;
			reactor->source_sinMode->mode = (Sine::SineMode)change;
			updateNeutronSourceTab();
		});
		// source saw tooth
		float stPeriod = properties->ns_sawToothMode.period;
		for (int saw = 0; saw < 6; saw++) {
			neutronSourceSAWBoxes[saw] = makeSimulationSetting(sourceSettings, (int)roundf(properties->sawToothMode.xIndex[saw] * 100), saw_settingNames[saw]);
			sourceLayout->setAnchor(neutronSourceSAWBoxes[saw]->parent(), RelativeGridLayout::makeAnchor(2, 6 + saw));
			neutronSourceSAWBoxes[saw]->setCallback([this, stPeriod, saw](int change) {
				float val = change / 100.f;
				properties->ns_sawToothMode.xIndex[saw] = val;
				reactor->source_saw->xIndex[saw] = val;
				if (saw < 5) {
					neutronSourceSAWBoxes[saw + 1]->setMinValue(change);
					if (neutronSourceSAWBoxes[saw + 1]->value() < change) { neutronSourceSAWBoxes[saw + 1]->setValue(change); }
					else { updateNeutronSourceTab(); }
				}
				else { updateNeutronSourceTab(); }
			});
		}
		updateNeutronSourceTab();

		/* PHYSICS */
		// Texts and borders
		for (int i = 0; i < 6; i++) {
			Label* textLabel = delayedPanel->add<Label>("Group " + std::to_string(i + 1) + ((i == 5 || i == 0) ? (i == 5 ? " (fastest)" : " (slowest)") : ""), "sans-bold");
			relPhysics->setAnchor(textLabel, RelativeGridLayout::makeAnchor((i + 1) * 2 + 1, 1, 1, 1, Alignment::Middle, Alignment::Middle));
		}
		Widget* border;
		for (int i = 0; i < 8; i++) {
			border = delayedPanel->add<Widget>();
			border->setDrawBackground(true);
			border->setBackgroundColor(coolBlue);
			relPhysics->setAnchor(border, RelativeGridLayout::makeAnchor(i * 2, 0, 1, 9));
		}
		Label* textLabel;
		std::string rowText[3] = {"Beta(i):","Lambda(i):","Enabled:"};
		for (int i = 0; i < 3; i++) {
			textLabel = delayedPanel->add<Label>(rowText[i], "sans-bold");
			textLabel->setPadding(0, 7.f);
			relPhysics->setAnchor(textLabel, RelativeGridLayout::makeAnchor(0, 2 * (i + 1) + 1, 1, 1, Alignment::Minimum, Alignment::Middle));
		}

		// Data inputs
		for (int row = 0; row < 4; row++) {
			border = delayedPanel->add<Widget>();
			border->setDrawBackground(true);
			border->setBackgroundColor(coolBlue);
			relPhysics->setAnchor(border, RelativeGridLayout::makeAnchor(0, 2 * row, 15));
			switch (row) {
			case 2: {
				for (int i = 0; i < 6; i++) {
					delayedGroupsEnabledBoxes[i] = delayedPanel->add<SliderCheckBox>();
					delayedGroupsEnabledBoxes[i]->setChecked(properties->groupsEnabled[i]);
					relPhysics->setAnchor(delayedGroupsEnabledBoxes[i], RelativeGridLayout::makeAnchor(2 * (i + 1) + 1, 7, 1, 1, Alignment::Middle, Alignment::Middle));
					delayedGroupsEnabledBoxes[i]->setCallback([this, i](bool change) {
						properties->groupsEnabled[i] = change;
						reactor->setDelayedGroupEnabled(i, change);
					});
				}
				break;
			}
			case 3: {
				border = delayedPanel->add<Widget>();
				border->setDrawBackground(true);
				border->setBackgroundColor(coolBlue);
				relPhysics->setAnchor(border, RelativeGridLayout::makeAnchor(0, 2 * (row + 1), 15));
				continue;
				break;
			}
			default:
			{
				for (int i = 0; i < 6; i++) {
					size_t index = 6 * row + i;
					delayedGroupBoxes[index] = delayedPanel->add<FloatBox<double>>(row ? properties->lambdas[i] : properties->betas[i]);
					relPhysics->setAnchor(delayedGroupBoxes[index], RelativeGridLayout::makeAnchor(2 * (i + 1) + 1, 2 * (row + 1) + 1, 1, 1, Alignment::Middle, Alignment::Middle));
					delayedGroupBoxes[index]->setFixedSize(Vector2i(130, 30));
					delayedGroupBoxes[index]->setSpinnable(true);
					delayedGroupBoxes[index]->setValueIncrement(0.00001f);
					delayedGroupBoxes[index]->setDefaultValue(std::to_string(delayedGroupBoxes[index]->value()));
					delayedGroupBoxes[index]->setMinValue(0.0000001f);
					delayedGroupBoxes[index]->setFormat(SCI_NUMBER_FORMAT);
					delayedGroupBoxes[index]->setCallback([this, row, i](double change) {
						if (row) {
							properties->lambdas[i] = change;
							reactor->setDelayedGroupDecay(i, change);
						}
						else {
							properties->betas[i] = change;
							reactor->setDelayedGroupFraction(i, change);
						}
					});
				}
			}
			}
		}

		// Create panel for lower left settings
		Widget* settingsVert = physics_settings->add<Widget>();
		physicsLayout->setAnchor(settingsVert, RelativeGridLayout::makeAnchor(1, 3, 1, 1));
		settingsVert->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 15));

		// Core volume
		Widget* corePanel = settingsVert->add<Widget>();
		
		corePanel->setLayout(panelsLayout);
		corePanel->add<Label>("Core volume: ", "sans-bold");
		coreVolumeBox = corePanel->add<FloatBox<double>>(properties->coreVolume * 1e3);
		coreVolumeBox->setAlignment(TextBox::Alignment::Left);
		coreVolumeBox->setFixedSize(Vector2i(180, 30));
		coreVolumeBox->setSpinnable(true);
		coreVolumeBox->setValueIncrement(1.);
		coreVolumeBox->setDefaultValue(std::to_string(coreVolumeBox->value()));
		coreVolumeBox->setMinValue(1.);
		coreVolumeBox->setFormat(SCI_NUMBER_FORMAT);
		coreVolumeBox->setUnits("L");
		coreVolumeBox->setCallback([this](double change) {
			reactor->setReactorCoreVolume(change * 1e-03);
			properties->coreVolume = change * 1e-03;
		});

		Widget* waterPanel = settingsVert->add<Widget>();
		waterPanel->setLayout(panelsLayout);
		waterPanel->add<Label>("Water volume: ", "sans-bold");
		waterVolumeInput = waterPanel->add<FloatBox<double>>(properties->waterVolume);
		waterVolumeInput->setFixedSize(Vector2i(100, 30));
		waterVolumeInput->setAlignment(TextBox::Alignment::Left);
		waterVolumeInput->setSpinnable(true);
		waterVolumeInput->setValueIncrement(1.);
		waterVolumeInput->setDefaultValue(std::to_string(waterVolumeInput->value()));
		waterVolumeInput->setMinValue(1.);
		waterVolumeInput->setFormat(SCI_NUMBER_FORMAT);
		waterVolumeInput->setUnits("m" + string(utf8(0xB3).data()));
		waterVolumeInput->setCallback([this](double change) {
			reactor->setWaterVolume(change);
			properties->waterVolume = change;
		});

		// Create a panel for temperature effects
		Widget* checkBoxPanelTemperatureEffects = settingsVert->add<Widget>();
		checkBoxPanelTemperatureEffects->setLayout(panelsLayout);
		// Create a panel for Fission poisoning
		Widget* checkBoxPanelFissionPoisoning = settingsVert->add<Widget>();
		checkBoxPanelFissionPoisoning->setLayout(panelsLayout);
		// Create a panel for excess reactivity
		Widget* excessPanel = settingsVert->add<Widget>();
		excessPanel->setLayout(panelsLayout);
		// Create a panel for cooling power
		Widget* waterCoolingPowerPanel = settingsVert->add<Widget>();
		waterCoolingPowerPanel->setLayout(panelsLayout);
		// Create a panel for prompt neutron lifetime
		Widget* promptPanel = settingsVert->add<Widget>();
		promptPanel->setLayout(panelsLayout);

		{
			checkBoxPanelTemperatureEffects->add<Label>("Temperature effects: ", "sans-bold");
			tempEffectsBox = checkBoxPanelTemperatureEffects->add<SliderCheckBox>();
			tempEffectsBox->setFontSize(16);
			tempEffectsBox->setChecked(properties->temperatureEffects);
			tempEffectsBox->setCallback([this](bool value) {
				reactor->setTemperatureEffectsEnabled(value);
				properties->temperatureEffects = value;
			});
		}
		{
			checkBoxPanelFissionPoisoning->add<Label>("Xe poisoning: ", "sans-bold");
			fissionProductsBox = checkBoxPanelFissionPoisoning->add<SliderCheckBox>();
			fissionProductsBox->setFontSize(16);
			fissionProductsBox->setChecked(properties->fissionPoisons);
			fissionProductsBox->setCallback([this](bool value) {
				reactor->setFissionPoisoningEffectsEnabled(value);
				properties->fissionPoisons = value;
			});
		}

		excessPanel->add<Label>("Excess reactivity: ", "sans-bold");
		excessReactivityBox = excessPanel->add<FloatBox<float>>(properties->excessReactivity);
		excessReactivityBox->setFixedSize(Vector2i(125, 30));
		excessReactivityBox->setUnits("pcm");
		excessReactivityBox->setMinValue(0.f);
		excessReactivityBox->setFormat(SCI_NUMBER_FORMAT);
		excessReactivityBox->setSpinnable(true);
		excessReactivityBox->setAlignment(TextBox::Alignment::Left);
		excessReactivityBox->setDefaultValue(std::to_string(excessReactivityBox->value()));
		excessReactivityBox->setValueIncrement(10.);
		excessReactivityBox->setCallback([this](float change) {
			properties->excessReactivity = change;
			reactor->setExcessReactivity(change);
		});
		
		// Water cooling power
		waterCoolingPowerPanel->add<Label>("Water cooling power: ", "sans-bold");
		coolingPowerBox = waterCoolingPowerPanel->add<FloatBox<double>>(properties->waterCoolingPower);
		coolingPowerBox->setAlignment(TextBox::Alignment::Left);
		coolingPowerBox->setFixedSize(Vector2i(150, 30));
		coolingPowerBox->setSpinnable(true);
		coolingPowerBox->setValueIncrement(1.);
		coolingPowerBox->setDefaultValue(std::to_string(coolingPowerBox->value()));
		coolingPowerBox->setMinValue(1.);
		coolingPowerBox->setFormat(SCI_NUMBER_FORMAT);
		coolingPowerBox->setUnits("W");
		coolingPowerBox->setCallback([this](double change) {
			properties->waterCoolingPower = change;
			reactor->setCoolingPower(change);
		});

		// Prompt neutron lifetime
		promptPanel->add<Label>("Prompt neutron lifetime: ", "sans-bold");
		promptNeutronLifetimeBox = promptPanel->add<FloatBox<double>>(properties->promptNeutronLifetime);
		promptNeutronLifetimeBox->setAlignment(TextBox::Alignment::Left);
		promptNeutronLifetimeBox->setFixedSize(Vector2i(150, 30));
		promptNeutronLifetimeBox->setSpinnable(true);
		promptNeutronLifetimeBox->setValueIncrement(1e-6);
		promptNeutronLifetimeBox->setDefaultValue(std::to_string(promptNeutronLifetimeBox->value()));
		promptNeutronLifetimeBox->setMinMaxValues(1e-7, 1.);
		promptNeutronLifetimeBox->setFormat(SCI_NUMBER_FORMAT);
		promptNeutronLifetimeBox->setUnits("s");
		promptNeutronLifetimeBox->setCallback([this](double change) {
			properties->promptNeutronLifetime = change;
			reactor->setPromptNeutronLifetime(change);
		});

		// Alpha panel
		Widget* alphaPanel = physics_settings->add<Widget>();
		physicsLayout->setAnchor(alphaPanel, RelativeGridLayout::makeAnchor(2, 3, 1, 1));
		alphaPanel->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 10));
		// Temperature reactivity coef.
		Graph* alphaGraph = alphaPanel->add<Graph>(1, "Temp. reactivity coef.");
		alphaGraph->setBackgroundColor(Color(60, 255));
		alphaGraph->setDrawBackground(true);
		alphaGraph->setFixedHeight(250);
		alphaGraph->setPadding(80.f, 25.f, 30.f, 65.f);
		alphaGraph->setTextColor(Color(0, 255));

		alphaPlot = alphaGraph->addPlot(sigmaPoints, false);
		alphaPlot->setName("-" + alpha);
		alphaPlot->setHorizontalName("Temperature");
		alphaPlot->setTextColor(Color(250, 255));
		alphaPlot->setTextOffset(30.f);
		alphaPlot->setColor(Color(0, 120, 255, 255));
		alphaPlot->setPointerColor(Color(0, 120, 255, 255));
		alphaPlot->setPointerOverride(true);
		alphaPlot->setFill(true);
		alphaPlot->setFillColor(Color(0, 120, 255, 150));
		alphaPlot->setAxisShown(true);
		alphaPlot->setMainLineShown(true);
		alphaPlot->setAxisColor(Color(250, 255));
		alphaPlot->setMajorTickNumber(2);
		alphaPlot->setMinorTickNumber(4);
		alphaPlot->setTextShown(true);
		alphaPlot->setMainTickFontSize(22.f);
		alphaPlot->setMajorTickFontSize(20.f);
		alphaPlot->setUnits("pcm/" + degCelsiusUnit);
		alphaPlot->setHorizontalPointerColor(Color(120, 0, 255, 255));
		alphaPlot->setHorizontalAxisShown(true);
		alphaPlot->setHorizontalMainLineShown(true);
		alphaPlot->setHorizontalPointerShown(true);
		alphaPlot->setHorizontalMajorTickNumber(3);
		alphaPlot->setHorizontalMinorTickNumber(4);
		alphaPlot->setHorizontalUnits(degCelsiusUnit);
		alphaPlot->setDrawMode(DrawMode::Default);
		alphaPlot->setPlotRange(0, 2);

		alphaPlot->setLimits(0., 1000., 0., 15.);
		updateAlphaGraph();
		alphaPlot->setXdata(alphaX);
		alphaPlot->setYdata(alphaY);

		// Alpha settings
		string alphaText[4] = {"Starting " + alpha + ":","Peak temp.:",alpha + " at peak temp.:","Slope after peak temp.:"};
		float vals[4] = { properties->alpha0, properties->alphaT1, properties->alphaAtT1, (float)properties->alphaK };
		float increments[4] = { .1f, 5.f, .1f, .001f };
		float maxVals[3] = { 20.f, 1000.f, 120.f };
		string units[4] = { "pcm/" + degCelsiusUnit, degCelsiusUnit, "pcm/" + degCelsiusUnit, "pcm/" + degCelsiusUnit + string(utf8(0xB2).data()) };
		for (int i = 0; i < 4; i++) {
			// Create a panel for prompt neutron lifetime
			Widget* alphaSettingPanel = alphaPanel->add<Widget>();
			alphaSettingPanel->setLayout(panelsLayout);
			alphaSettingPanel->add<Label>(alphaText[i] + " ", "sans-bold");
			FloatBox<float>* temp = alphaSettingPanel->add<FloatBox<float>>(vals[i]);
			temp->setAlignment(TextBox::Alignment::Left);
			temp->setFixedSize(Vector2i(150, 30));
			temp->setSpinnable(true);
			temp->setValueIncrement(increments[i]);
			temp->setDefaultValue(std::to_string(temp->value()));
			if (i < 3) { temp->setMinMaxValues((i == 1) ? 0.f : -maxVals[i], maxVals[i]); }
			else { temp->setMinMaxValues(-4.f, 4.f); }
			temp->setFormat((i==1) ? SCI_NUMBER_FORMAT : SCI_NUMBER_FORMAT_NEG);
			temp->setUnits(units[i]);
			switch (i) {
			case 0: alpha0Box = temp; break;
			case 1: tempPeakBox = temp; break;
			case 2: alphaPeakBox = temp; break;
			case 3: alphaSlopeBox = temp; break;
			}
		}

		alpha0Box->setCallback([this](float change) {
			properties->alpha0 = change;
			reactor->setAlpha0(change);
			updateAlphaGraph();
		});
		tempPeakBox->setCallback([this](float change) {
			properties->alphaT1 = change;
			reactor->setAlphaTempPeak(change);
			updateAlphaGraph();
		});
		alphaPeakBox->setCallback([this](float change) {
			properties->alphaAtT1 = change;
			reactor->setAlphaPeak(change);
			updateAlphaGraph();
		});
		alphaSlopeBox->setCallback([this](float change) {
			properties->alphaK = (double)change;
			reactor->setAlphaSlope((double)change);
			updateAlphaGraph();
		});

		modeTabs->setActiveTab(0);
	}

	void createRodSettingsTab() {
		Widget* rod_settings = tabControl->createTab("Control rods");
		rod_settings->setId("Rod settings tab");
		RelativeGridLayout* rod_settings_layout = new RelativeGridLayout();									  /* COLUMNS */
		for (int i = 0; i < 9; i++) {
			if (i == 3 || i == 6) rod_settings_layout->appendCol(RelativeGridLayout::Size(2.f, RelativeGridLayout::SizeType::Fixed));
			rod_settings_layout->appendCol(1.f);
		}
																											  /* ROWS */
		rod_settings_layout->appendRow(RelativeGridLayout::Size(35.f, RelativeGridLayout::SizeType::Fixed));  // 0 title
		rod_settings_layout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));  // 1 empty space
		rod_settings_layout->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));  // 2 setting name
		rod_settings_layout->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));  // 3 setting value
		rod_settings_layout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));  // 4 top slider
		rod_settings_layout->appendRow(RelativeGridLayout::Size(320.f, RelativeGridLayout::SizeType::Fixed)); // 5 graph
		rod_settings_layout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));  // 6 bottom slider
		rod_settings_layout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));  // 7 empty space
		rod_settings_layout->appendRow(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed));	  // 8 border
		rod_settings_layout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));  // 9 empty space
		rod_settings_layout->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));  // 10 label
		rod_settings_layout->appendRow(RelativeGridLayout::Size(320.f, RelativeGridLayout::SizeType::Fixed)); // 11 derivative graph
		rod_settings->setLayout(rod_settings_layout);

		// Create borders
		for (int i = 0; i < 2; i++) {
			Widget* border = rod_settings->add<Widget>();
			rod_settings_layout->setAnchor(border, RelativeGridLayout::makeAnchor(i ? 3 : 7, 0, 1, 13));
			border->setBackgroundColor(coolBlue);
			border->setDrawBackground(true);
		}
		Widget* border = rod_settings->add<Widget>();
		rod_settings_layout->setAnchor(border, RelativeGridLayout::makeAnchor(0, 8, 11, 1));
		border->setBackgroundColor(coolBlue);
		border->setDrawBackground(true);

		Label* tempLabel;
		ControlRod* useRod;
		// for each rod
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
			useRod = reactor->rods[i];

			// Titles
			tempLabel = rod_settings->add<Label>(useRod->getRodName() + " rod", "sans-bold");
			tempLabel->setFontSize(30);
			rod_settings_layout->setAnchor(tempLabel, RelativeGridLayout::makeAnchor(i * 4, 0, 3, 1, Alignment::Middle, Alignment::Maximum));

			// Rod steps setting
			tempLabel = rod_settings->add<Label>("Rod steps:", "sans-bold");
			rod_settings_layout->setAnchor(tempLabel, RelativeGridLayout::makeAnchor(i * 4, 2, 1, 1, Alignment::Middle, Alignment::Middle));
			rodStepsBox[i] = rod_settings->add<IntBox<int>>((int)properties->rodSettings[i].rodSteps);
			rod_settings_layout->setAnchor(rodStepsBox[i], RelativeGridLayout::makeAnchor(i * 4, 3, 1, 1, Alignment::Middle));
			rodStepsBox[i]->setFixedSize(Vector2i(100, 20));
			rodStepsBox[i]->setAlignment(TextBox::Alignment::Left);
			rodStepsBox[i]->setUnits("steps");
			rodStepsBox[i]->setDefaultValue(to_string(properties->rodSettings[i].rodSteps));
			rodStepsBox[i]->setMinMaxValues(1, 100000);
			rodStepsBox[i]->setValueIncrement(10);
			rodStepsBox[i]->setFormat("[0-9]+");
			rodStepsBox[i]->setCallback([useRod, i, this](int change) {
				properties->rodSettings[i].rodSteps = change;
				useRod->setRodSteps((size_t)change);
				rodCurves[i]->setLimitHorizontalMultiplier((double)*useRod->getRodSteps());
			});

			// Rod worth setting
			tempLabel = rod_settings->add<Label>("Rod worth:", "sans-bold");
			rod_settings_layout->setAnchor(tempLabel, RelativeGridLayout::makeAnchor(i * 4 + 1, 2, 1, 1, Alignment::Middle, Alignment::Middle));
			rodWorthBox[i] = rod_settings->add<FloatBox<float>>(properties->rodSettings[i].rodWorth);
			rod_settings_layout->setAnchor(rodWorthBox[i], RelativeGridLayout::makeAnchor(i * 4 + 1, 3, 1, 1, Alignment::Middle));
			rodWorthBox[i]->setFixedSize(Vector2i(100, 20));
			rodWorthBox[i]->setAlignment(TextBox::Alignment::Left);
			rodWorthBox[i]->setUnits("pcm");
			rodWorthBox[i]->setDefaultValue(to_string(properties->rodSettings[i].rodWorth));
			rodWorthBox[i]->setMinValue(1.f);
			rodWorthBox[i]->setValueIncrement(10.f);
			rodWorthBox[i]->setFormat("[0-9]*\\.?[0-9]+");
			rodWorthBox[i]->setCallback([useRod, i, this](float change) {
				properties->rodSettings[i].rodWorth = change;
				useRod->setRodWorth(change);
			});

			// Rod speed setting
			tempLabel = rod_settings->add<Label>("Rod speed:", "sans-bold");
			rod_settings_layout->setAnchor(tempLabel, RelativeGridLayout::makeAnchor(i * 4 + 2, 2, 1, 1, Alignment::Middle, Alignment::Middle));
			rodSpeedBox[i] = rod_settings->add<FloatBox<float>>(properties->rodSettings[i].rodSpeed);
			rod_settings_layout->setAnchor(rodSpeedBox[i], RelativeGridLayout::makeAnchor(i * 4 + 2, 3, 1, 1, Alignment::Middle));
			rodSpeedBox[i]->setFixedSize(Vector2i(100, 20));
			rodSpeedBox[i]->setAlignment(TextBox::Alignment::Left);
			rodSpeedBox[i]->setUnits("steps/s");
			rodSpeedBox[i]->setDefaultValue(to_string(properties->rodSettings[i].rodSpeed));
			rodSpeedBox[i]->setMinValue(0.f);
			rodSpeedBox[i]->setValueIncrement(1.f);
			rodSpeedBox[i]->setFormat("[0-9]*\\.?[0-9]+");
			rodSpeedBox[i]->setCallback([useRod, i, this](float change) {
				properties->rodSettings[i].rodSpeed = change;
				useRod->setRodSpeed(change);
				if(i == 1) reactor->regulatingRod()->sine()->fillXYaxis(operationModesPlots[0][1], operationModesPlots[1][1]); // Update SQW graph
			});

			// Display tool
			Graph* rodControl = rod_settings->add<Graph>(1);
			rod_settings_layout->setAnchor(rodControl, RelativeGridLayout::makeAnchor(i * 4, 5, 3, 1, Alignment::Middle));
			rodControl->setFixedWidth(320);
			rodControl->setBackgroundColor(Color(60, 255));
			rodControl->setDrawBackground(true);
			rodControl->setPadding(80.f, 25.f, 30.f, 70.f);
			rodControl->setTextColor(Color(250, 255));
			rodCurves[i] = rodControl->addBezierCurve();
			rodCurves[i]->setName("Reactivity");
			rodCurves[i]->setHorizontalName("Position");
			rodCurves[i]->setTextColor(Color(250, 255));
			rodCurves[i]->setTextOffset(30.f);
			rodCurves[i]->setColor(Color(0, 120, 255, 255));
			rodCurves[i]->setPointerColor(Color(0, 120, 255, 255));
			rodCurves[i]->setPointerOverride(true);
			rodCurves[i]->setFill(true);
			rodCurves[i]->setFillColor(Color(0, 120, 255, 150));
			rodCurves[i]->setAxisShown(true);
			rodCurves[i]->setMainLineShown(true);
			rodCurves[i]->setAxisColor(Color(250, 255));
			rodCurves[i]->setMajorTickNumber(3);
			rodCurves[i]->setMinorTickNumber(1);
			rodCurves[i]->setTextShown(true);
			rodCurves[i]->setMainTickFontSize(22.f);
			rodCurves[i]->setMajorTickFontSize(20.f);
			rodCurves[i]->setLimitMultiplier(100.);
			rodCurves[i]->setUnits("%");
			rodCurves[i]->setHorizontalPointerColor(Color(120, 0, 255, 255));
			rodCurves[i]->setHorizontalAxisShown(true);
			rodCurves[i]->setHorizontalMainLineShown(true);
			rodCurves[i]->setHorizontalPointerShown(true);
			rodCurves[i]->setHorizontalMajorTickNumber(1);
			rodCurves[i]->setHorizontalMinorTickNumber(1);
			rodCurves[i]->setLimitHorizontalMultiplier((double)*useRod->getRodSteps());
			rodCurves[i]->setHorizontalUnits("steps");

			// Sliders
			for (int j = 0; j < 2; j++) {
				rodCurves[i]->setParameter(j*2, properties->rodSettings[i].rodCurve[j]);
				size_t sliderIndex = i * 2 + j;
				rodCurveSliders[sliderIndex] = rod_settings->add<Slider>();
				rod_settings_layout->setAnchor(rodCurveSliders[sliderIndex], RelativeGridLayout::makeAnchor(i * 4, 4 + j * 2, 3, 1, Alignment::Middle, Alignment::Middle));
				rodCurveSliders[sliderIndex]->setFixedSize(Vector2i(340, 15));
				rodCurveSliders[sliderIndex]->setValue(properties->rodSettings[i].rodCurve[j]);
				rodCurveSliders[sliderIndex]->setCallback([this, i, j](float change) {
					rodCurves[i]->setParameter(j * 2, change);
				});
				rodCurveSliders[sliderIndex]->setFinalCallback([useRod, j, i, this](float stop) {
					rodCurves[i]->setParameter(j * 2, stop);
					properties->rodSettings[i].rodCurve[j] = stop;
					useRod->setParameter(j, stop);
					handleDerivativeChange();
				});
			}

			// Derivative graphs
			tempLabel = rod_settings->add<Label>("Differential\nrod worth curve", "sans-bold", 25);
			rod_settings_layout->setAnchor(tempLabel, RelativeGridLayout::makeAnchor(i * 4, 10, 3, 1, Alignment::Middle, Alignment::Maximum));

			Graph* rodDerDisplay = rod_settings->add<Graph>(1);
			rod_settings_layout->setAnchor(rodDerDisplay, RelativeGridLayout::makeAnchor(i * 4, 11, 3, 1, Alignment::Middle, Alignment::Minimum));
			rodDerDisplay->setFixedSize(Vector2i(320, 320));
			rodDerDisplay->setBackgroundColor(Color(60, 255));
			rodDerDisplay->setDrawBackground(true);
			rodDerDisplay->setPadding(80.f, 25.f, 30.f, 70.f);
			rodDerDisplay->setTextColor(Color(250, 255));
			rodDerivatives[i] = rodDerDisplay->addPlot(ControlRod::dataPoints, false);
			rodDerivatives[i]->setName("Reactivity change");
			rodDerivatives[i]->setHorizontalName("Position");
			rodDerivatives[i]->setTextColor(Color(250, 255));
			rodDerivatives[i]->setTextOffset(30.f);
			rodDerivatives[i]->setColor(Color(0, 120, 255, 255));
			rodDerivatives[i]->setPointerColor(Color(0, 120, 255, 255));
			rodDerivatives[i]->setPointerOverride(true);
			rodDerivatives[i]->setPointerShown(true);
			rodDerivatives[i]->setHorizontalPointerShown(true);
			rodDerivatives[i]->setFill(true);
			rodDerivatives[i]->setFillColor(Color(0, 120, 255, 150));
			rodDerivatives[i]->setAxisShown(true);
			rodDerivatives[i]->setMainLineShown(true);
			rodDerivatives[i]->setAxisColor(Color(250, 255));
			rodDerivatives[i]->setMajorTickNumber(3);
			rodDerivatives[i]->setMinorTickNumber(1);
			rodDerivatives[i]->setTextShown(true);
			rodDerivatives[i]->setMainTickFontSize(22.f);
			rodDerivatives[i]->setMajorTickFontSize(20.f);
			rodDerivatives[i]->setUnits("pcm/step");
			rodDerivatives[i]->setHorizontalPointerColor(Color(120, 0, 255, 255));
			rodDerivatives[i]->setHorizontalAxisShown(true);
			rodDerivatives[i]->setHorizontalMainLineShown(true);
			rodDerivatives[i]->setHorizontalPointerShown(true);
			rodDerivatives[i]->setHorizontalMajorTickNumber(1);
			rodDerivatives[i]->setHorizontalMinorTickNumber(1);
			rodDerivatives[i]->setLimitHorizontalMultiplier((double)*useRod->getRodSteps());
			rodDerivatives[i]->setHorizontalUnits("steps");
			rodDerivatives[i]->setXdataLin(0L);
			rodDerivatives[i]->setPlotRange(0, *useRod->getRodSteps());
			rodDerivatives[i]->setValueComputing([this, i](double* v, size_t /*in*/) {
				*v *= reactor->rods[i]->getRodWorth();
			});
		}
		handleDerivativeChange();
	}

	void createDelayedGroupsTab() {
		Widget* delayed_tab = tabControl->createTab("Delayed neutrons");
		delayed_tab->setId("delayed tab");
		RelativeGridLayout* delayedLayout = new RelativeGridLayout();
		delayedLayout->appendCol(1.f);	// graph area
		delayedLayout->appendRow(1.f);	// graph area
		delayed_tab->setLayout(delayedLayout);

		Widget* topHost = delayed_tab->add<Widget>();
		topHost->setBackgroundColor(Color(240, 255));
		topHost->setDrawBackground(true);
		delayedLayout->setAnchor(topHost, RelativeGridLayout::makeAnchor(0, 0));
		RelativeGridLayout* topLayout = new RelativeGridLayout();
		topLayout->appendCol(1.f); // graph
		topLayout->appendCol(RelativeGridLayout::Size(158.f, RelativeGridLayout::SizeType::Fixed)); // key
		topLayout->appendRow(1.f);
		topHost->setLayout(topLayout);

		Color dataColors[7] = { Color(255,255), Color(34,116,165,255), Color(247, 92, 3,255), Color(241, 196, 15, 255), Color(0, 204, 102, 255), Color(240, 58, 71, 255), Color(153, 0, 153, 255) };

		Widget* keyPanel = topHost->add<Widget>();
		topLayout->setAnchor(keyPanel, RelativeGridLayout::makeAnchor(1, 0, 1, 1, Alignment::Minimum, Alignment::Middle));
		keyPanel->setDrawBackground(true);
		keyPanel->setBackgroundColor(Color(65, 255));
		keyPanel->setBorder(Border::BOTTOM | Border::LEFT | Border::TOP | Border::RIGHT);
		keyPanel->setBorderColor(coolBlue);
		keyPanel->setBorderWidth(1.f);
		RelativeGridLayout* rel = new RelativeGridLayout();
		rel->appendCol(RelativeGridLayout::Size(50.f, RelativeGridLayout::SizeType::Fixed)); // colors
		rel->appendCol(RelativeGridLayout::Size(1.f, RelativeGridLayout::SizeType::Fixed)); // border
		rel->appendCol(RelativeGridLayout::Size(10.f, RelativeGridLayout::SizeType::Fixed)); // local padding
		rel->appendCol(RelativeGridLayout::Size(70.f, RelativeGridLayout::SizeType::Fixed)); // text
		for(int i = 0; i < 7; i++) rel->appendRow(RelativeGridLayout::Size(35.f, RelativeGridLayout::SizeType::Fixed)); // data rows
		rel->appendRow(RelativeGridLayout::Size(5.f, RelativeGridLayout::SizeType::Fixed)); // padding
		keyPanel->setLayout(rel);
		Label* tempLabelKey = keyPanel->add<Label>("Color", "sans-bold");
		rel->setAnchor(tempLabelKey, RelativeGridLayout::makeAnchor(0, 0, 1, 1, Alignment::Middle, Alignment::Middle));
		tempLabelKey = keyPanel->add<Label>("Group No.", "sans-bold");
		rel->setAnchor(tempLabelKey, RelativeGridLayout::makeAnchor(3, 0, 1, 1, Alignment::Minimum, Alignment::Middle));
		Widget* temp = keyPanel->add<Widget>();
		rel->setAnchor(temp, RelativeGridLayout::makeAnchor(1, 0));
		temp->setDrawBackground(true);
		temp->setBackgroundColor(coolBlue);
		for (int i = 1; i < 7; i++) {
			temp = keyPanel->add<Widget>();
			rel->setAnchor(temp, RelativeGridLayout::makeAnchor(0, i, 1, 1, Alignment::Middle, Alignment::Middle));
			temp->setDrawBackground(true);
			temp->setBackgroundColor(dataColors[i]);
			temp->setFixedSize(Vector2i(25, 25));
			Label* tempLabel = keyPanel->add<Label>(to_string(i), "sans-bold");
			rel->setAnchor(tempLabel, RelativeGridLayout::makeAnchor(3, i, 1, 1, Alignment::Minimum, Alignment::Middle));
		}

		delayedGroupsGraph = topHost->add<Graph>(6, "Delayed neutron groups");
		delayedGroupsGraph->setPadding(90.f, 20.f, 60.f, 50.f);
		delayedGroupsGraph->setTextColor(Color(0, 255));
		topLayout->setAnchor(delayedGroupsGraph, RelativeGridLayout::makeAnchor(0, 0));
		//double betaLambda = reactor->prompt_lifetime;
		double betaLambda = 0;
		double sumBeta = 0, sumLambdaC = 0;
		/*
		The delayed group fractions for display are normalized to 100%, where 100% is a concentration
		of each group during stationary conditions at current power.
		*/
		for (int i = 0; i < 6; i++) {
			//betaLambda += reactor->beta_neutrons[i] / reactor->delayed_decay_time[i];
			sumBeta += reactor->beta_neutrons[i];
		}
		for (int i = 0; i < 6; i++) {
			delayedGroups[i] = delayedGroupsGraph->addPlot(reactor->getDataLength(), true);
			delayedGroups[i]->setXdata(reactor->time_);
			delayedGroups[i]->setYdata(reactor->state_vector_[i + 1]);
			delayedGroups[i]->setColor(dataColors[i + 1]);
			delayedGroups[i]->setDrawMode(DrawMode::Smart);
			delayedGroups[i]->setPointerColor(dataColors[i + 1]);
			delayedGroups[i]->setAxisPosition((i < 3) ? GraphElement::AxisLocation::Right : GraphElement::AxisLocation::Left);
			delayedGroups[i]->setPixelDrawRatio(0.3f); // skip 70% of pixels when drawing
			delayedGroups[i]->setValueComputing([this, sumBeta, sumLambdaC, i](double* val, const size_t index) {
				//*val = *val * sumLambdaC * reactor->delayed_decay_time[i] / (reactor->state_vector_[7][index] * reactor->beta_neutrons[i]);
				double sumLambdaC = 0;
				for (int j = 0; j < 6; j++)
					sumLambdaC += reactor->delayed_decay_time[j] * reactor->state_vector_[j + 1][index];
				*val = *val * sumBeta * reactor->delayed_decay_time[i] / (sumLambdaC * reactor->beta_neutrons[i]);
			});
			if (!i) {
				delayedGroups[i]->setAxisShown(true);
				delayedGroups[i]->setAxisPosition(GraphElement::AxisLocation::Left);
				delayedGroups[i]->setHorizontalAxisShown(true);
				delayedGroups[i]->setHorizontalMainLineShown(true);
				delayedGroups[i]->setTextShown(true);
				delayedGroups[i]->setName("Deviation");
				delayedGroups[i]->setUnits("%");
				delayedGroups[i]->setMajorTickNumber(2);
				delayedGroups[i]->setMinorTickNumber(3);
				delayedGroups[i]->setLimitOverride(1, "now");
				delayedGroups[i]->setLimitOverride(0, "30 seconds ago");
				delayedGroups[i]->setLimitMultiplier(100.);
				delayedGroups[i]->setHorizontalName("Time");
				delayedGroups[i]->setHorizontalUnits("s");
				delayedGroups[i]->setHorizontalTextOffset(20.f);
			}
		};
	}

	double* operationModesPlots[2][3];
	const int simModeFields[3] = { 7, 3, 8 };
	void createOperationModesTab()  {
		Widget* modes_base = tabControl->createTab("Operation modes", false);
		TabWidget* modeTabs = modes_base->add<TabWidget>();
		modeTabs->header()->setStretch(true);
		modeTabs->header()->setButtonAlignment(NVGalign::NVG_ALIGN_MIDDLE | NVGalign::NVG_ALIGN_CENTER);
		RelativeGridLayout* grid = new RelativeGridLayout();
		grid->appendCol(1.f);
		grid->appendRow(1.f);
		modes_base->setLayout(grid);
		grid->setAnchor(modeTabs, RelativeGridLayout::makeAnchor(0, 0));

		// Create titles, periods and amplitudes
		std::string titles[3] = { "Square wave","Sine wave","Saw tooth" };
		RelativeGridLayout* layouts[3];
		Widget* tabs[3];
		for (int i = 0; i < 3; i++) {
			tabs[i] = modeTabs->createTab(titles[i]);
			tabs[i]->setId(titles[i] + " tab");
			tabs[i]->setDrawBackground(true);
			tabs[i]->setBackgroundColor(Color(100, 255));

			layouts[i] = new RelativeGridLayout();
			layouts[i]->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// left border
			layouts[i]->appendCol(RelativeGridLayout::Size(250.f, RelativeGridLayout::SizeType::Fixed));	// settings
			layouts[i]->appendCol(1.f);																		// graph
			layouts[i]->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// right border
			layouts[i]->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// top border
			for (int j = 0; j < simModeFields[i]; j++) {
				layouts[i]->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));
			}
			layouts[i]->appendRow(1.f);																		// empty space
			layouts[i]->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));		// bottom border
			tabs[i]->setLayout(layouts[i]);

			PeriodicalMode* use = reactor->regulatingRod()->getMode((SimulationModes)(i + 1));
			size_t arraySize = use->num_points();

			// create graph first just for event binding
			Graph* graph = tabs[i]->add<Graph>(2, titles[i]);
			graph->setDrawBackground(true);
			graph->setBackgroundColor(Color(32, 255));
			layouts[i]->setAnchor(graph, RelativeGridLayout::makeAnchor(2, 0, 2, simModeFields[i] + 3, Alignment::Fill, Alignment::Fill));
			graph->setPadding(90.f, 20.f, 10.f, 70.f);
			graph->setPlotBackgroundColor(Color(60, 255));
			graph->setPlotGridColor(Color(177, 255));
			graph->setPlotBorderColor(Color(200, 255));
			operationModes[i] = graph->addPlot(arraySize);
			operationModes[i]->setPlotRange(0, arraySize - 1);
			operationModes[i]->setLimits(0., use->getPeriod(), -1.5*use->getAmplitude(), 1.5*use->getAmplitude());
			operationModesTrackers[i] = graph->addPlot(2);
			operationModesTrackers[i]->setPlotRange(0, 1);
			operationModesTrackers[i]->setDrawMode(DrawMode::Default);
			operationModesTrackers[i]->setAxisShown(false);
			operationModesTrackers[i]->setColor(Color(1.f, 0.f, 0.f, 1.f));
			operationModesTrackers[i]->setHorizontalAxisShown(false);
			operationModesTrackers[i]->setTextShown(false);
			operationModesTrackers[i]->setPointerShown(false);
			operationModesTrackers[i]->setXdata(use->getTrackerArray());
			operationModesTrackers[i]->setYdata(trackerY);
			operationModesTrackers[i]->setLimits(0., use->getPeriod(), 0., 1.);


			// Make arrays
			for (size_t p = 0; p < 2; p++) operationModesPlots[p][i] = new double[arraySize];
			// Fill arrays
			use->fillXYaxis(operationModesPlots[0][i], operationModesPlots[1][i]);

			// period
			periodBoxes[i] = makeSettingLabel<FloatBox<float>>(tabs[i], "Period: ", 100, use->getPeriod());
			layouts[i]->setAnchor(periodBoxes[i]->parent(), RelativeGridLayout::makeAnchor(1, 1, 1, 1, Alignment::Minimum, Alignment::Middle));
			periodBoxes[i]->setFixedWidth(100);
			periodBoxes[i]->setAlignment(TextBox::Alignment::Left);
			periodBoxes[i]->setFormat(SCI_NUMBER_FORMAT);
			periodBoxes[i]->setUnits("s");
			periodBoxes[i]->setMinValue(0.1f);
			periodBoxes[i]->setValueIncrement(0.1f);
			periodBoxes[i]->setSpinnable(true);
			periodBoxes[i]->setDefaultValue(formatDecimals(use->getPeriod(), 1));
			periodBoxes[i]->setCallback([use, i, this](float change) {
				switch (i) {
				case 0: properties->squareWave.period = change; break;
				case 1: properties->sineMode.period = change; break;
				case 2: properties->sawToothMode.period = change; break;
				}
				use->setPeriod(change);
				use->fillXYaxis(operationModesPlots[0][i], operationModesPlots[1][i]);
				operationModes[i]->setLimits(0., change, -1.5*use->getAmplitude(), 1.5*use->getAmplitude());
				operationModesTrackers[i]->setLimits(0., use->getPeriod(), 0., 1.);
			});

			// amplitude
			amplitudeBoxes[i] = makeSettingLabel<FloatBox<float>>(tabs[i], "Amplitude: ", 100, use->getAmplitude());
			layouts[i]->setAnchor(amplitudeBoxes[i]->parent(), RelativeGridLayout::makeAnchor(1, 2, 1, 1, Alignment::Minimum, Alignment::Middle));
			amplitudeBoxes[i]->setFixedWidth(100);
			amplitudeBoxes[i]->setAlignment(TextBox::Alignment::Left);
			amplitudeBoxes[i]->setFormat(SCI_NUMBER_FORMAT);
			amplitudeBoxes[i]->setUnits("steps");
			amplitudeBoxes[i]->setMinValue(0.f);
			amplitudeBoxes[i]->setValueIncrement(1);
			amplitudeBoxes[i]->setSpinnable(true);
			amplitudeBoxes[i]->setDefaultValue(to_string(use->getAmplitude()));
			amplitudeBoxes[i]->setCallback([use, i, this](float change) {
				switch (i) {
				case 0: properties->squareWave.amplitude = change; break;
				case 1: properties->sineMode.amplitude = change; break;
				case 2: properties->sawToothMode.amplitude = change; break;
				}
				use->setAmplitude(change);
				use->fillXYaxis(operationModesPlots[0][i], operationModesPlots[1][i]);
				operationModes[i]->setLimits(0., use->getPeriod(), -1.5*change, 1.5*change);
			});

			operationModes[i]->setMainTickFontSize(18.f);
			operationModes[i]->setMajorTickFontSize(16.f);
			operationModes[i]->setNameFontSize(24.f);
			operationModes[i]->setPointerShown(false);
			operationModes[i]->setXdata(operationModesPlots[0][i]);
			operationModes[i]->setYdata(operationModesPlots[1][i]);
			operationModes[i]->setColor(coolBlue);
			operationModes[i]->setLimits(0., use->getPeriod(), -1.5*use->getAmplitude(), 1.5*use->getAmplitude());
			operationModes[i]->setDrawMode(DrawMode::Default);
			operationModes[i]->setTextColor(Color(250, 255));
			operationModes[i]->setAxisColor(Color(250, 255));
			operationModes[i]->setTextShown(true);
			operationModes[i]->setAxisShown(true);
			operationModes[i]->setUnits("steps");
			operationModes[i]->setName("Delta rod position");
			operationModes[i]->setTextOffset(40.f);
			operationModes[i]->setMainLineShown(true);
			operationModes[i]->setMajorTickNumber(3);
			operationModes[i]->setMinorTickNumber(1);
			operationModes[i]->setHorizontalAxisShown(true);
			operationModes[i]->setHorizontalUnits("s");
			operationModes[i]->setHorizontalName("Time");
			operationModes[i]->setHorizontalMainLineShown(true);
			operationModes[i]->setHorizontalMajorTickNumber(3);
			operationModes[i]->setHorizontalMinorTickNumber(1);
		}

		/* SQUARE WAVE */
		for (int sqw = 0; sqw < 4; sqw++) {
			squareWaveBoxes[sqw] = makeSimulationSetting(tabs[0], (int)roundf(properties->squareWave.xIndex[sqw] * 100), sqw_settingNames[sqw]);
			layouts[0]->setAnchor(squareWaveBoxes[sqw]->parent(), RelativeGridLayout::makeAnchor(1, 3 + sqw, 1, 1, Alignment::Minimum, Alignment::Middle));
			squareWaveBoxes[sqw]->setCallback([this, sqw](int change) {
				float val = change / 100.f;
				reactor->regulatingRod()->squareWave()->xIndex[sqw] = val;
				properties->squareWave.xIndex[sqw] = val;
				if (sqw != 3) {
					squareWaveBoxes[sqw + 1]->setMinValue(change);
					if (squareWaveBoxes[sqw + 1]->value() < change) squareWaveBoxes[sqw + 1]->setValue(change);
				}
				reactor->regulatingRod()->squareWave()->fillXYaxis(operationModesPlots[0][0], operationModesPlots[1][0]);
			});
		}
		squareWaveSpeedBox = makeSettingLabel<SliderCheckBox>(tabs[0], "Use finite rod speed: ");
		layouts[0]->setAnchor(squareWaveSpeedBox->parent(), RelativeGridLayout::makeAnchor(1, 7, 1, 1, Alignment::Minimum, Alignment::Middle));
		squareWaveSpeedBox->setCallback([this](bool change) {
			properties->squareWaveUsesRodSpeed = change;
			if (change) {
				reactor->regulatingRod()->squareWave()->rodSpeed = reactor->regulatingRod()->getRodSpeed();
			}
			else {
				reactor->regulatingRod()->squareWave()->rodSpeed = 0.f;
			}
			reactor->regulatingRod()->squareWave()->fillXYaxis(operationModesPlots[0][0], operationModesPlots[1][0]);
		});

		/* SINE MODE */
		sineModeBox = makeSettingLabel<ComboBox>(tabs[1], "Sine type: ", 0, sineModes);
		layouts[1]->setAnchor(sineModeBox->parent(), RelativeGridLayout::makeAnchor(1, 3, 1, 1, Alignment::Minimum, Alignment::Middle));
		sineModeBox->setFixedWidth(150);
		sineModeBox->setCallback([this](int change) {
			properties->sineMode.mode = (Settings::SineSettings::SineMode)change;
			reactor->regulatingRod()->sine()->mode = (Sine::SineMode)change;
			reactor->regulatingRod()->sine()->fillXYaxis(operationModesPlots[0][1], operationModesPlots[1][1]);
		});

		/* SAW TOOTH */
		for (int saw = 0; saw < 6; saw++) {
			sawToothBoxes[saw] = makeSimulationSetting(tabs[2], (int)roundf(properties->sawToothMode.xIndex[saw] * 100), saw_settingNames[saw]);
			layouts[2]->setAnchor(sawToothBoxes[saw]->parent(), RelativeGridLayout::makeAnchor(1, 3 + saw, 1, 1, Alignment::Minimum, Alignment::Middle));
			sawToothBoxes[saw]->setCallback([this, saw](int change) {
				float val = change / 100.f;
				reactor->regulatingRod()->sawTooth()->xIndex[saw] = val;
				properties->sawToothMode.xIndex[saw] = val;
				if (saw < 5) {
					sawToothBoxes[saw + 1]->setMinValue(change);
					if (sawToothBoxes[saw + 1]->value() < change) sawToothBoxes[saw + 1]->setValue(change);
				}
				reactor->regulatingRod()->sawTooth()->fillXYaxis(operationModesPlots[0][2], operationModesPlots[1][2]);
			});
		}

		// AUTOMATIC
		Widget* autoTab = modeTabs->createTab("Automatic");
		RelativeGridLayout* autoLayout = new RelativeGridLayout();
		autoLayout->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed)); // left border
		autoLayout->appendCol(1.f); // everything else
		autoLayout->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed)); // top border
		for(int i = 0; i < 4; i++) autoLayout->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed)); // 4 rows for 4 settings
		autoTab->setLayout(autoLayout);

		keepCurrentPowerBox = makeSettingLabel<SliderCheckBox>(autoTab, "Current power stability: ", 0);
		keepCurrentPowerBox->setChecked(properties->steadyCurrentPower);
		autoLayout->setAnchor(keepCurrentPowerBox->parent(), RelativeGridLayout::makeAnchor(1, 1, 1, 1, Alignment::Minimum, Alignment::Middle));

		steadyPowerBox = makeSettingLabel<FloatBox<double>>(autoTab, "Steady power: ", 100, properties->steadyGoalPower);
		steadyPowerBox->setEnabled(!properties->steadyCurrentPower);
		steadyPowerBox->setUnits("W");
		steadyPowerBox->setFormat(SCI_NUMBER_FORMAT);
		steadyPowerBox->setMinValue(0.);
		steadyPowerBox->setAlignment(TextBox::Alignment::Right);
		steadyPowerBox->setValueIncrement(1e3);
		steadyPowerBox->setSpinnable(true);
		autoLayout->setAnchor(steadyPowerBox->parent(), RelativeGridLayout::makeAnchor(1, 2, 1, 1, Alignment::Minimum, Alignment::Middle));

		keepCurrentPowerBox->setCallback([this](bool checked) {
			steadyPowerBox->setEnabled(!checked);
			properties->steadyCurrentPower = checked;
			reactor->setKeepCurrentPower(checked);
		});

		steadyPowerBox->setCallback([this](double newValue) {
			if (newValue > 0.) { 
				properties->steadyGoalPower = newValue;
				reactor->setAutomaticSteadyPower(newValue);
			}
		});

		avoidPeriodScramBox = makeSettingLabel<SliderCheckBox>(autoTab, "Avoid period SCRAM: ", 0);
		avoidPeriodScramBox->setChecked(reactor->getKeepCurrentPower());
		autoLayout->setAnchor(avoidPeriodScramBox->parent(), RelativeGridLayout::makeAnchor(1, 3, 1, 1, Alignment::Minimum, Alignment::Middle));

		avoidPeriodScramBox->setCallback([this](bool checked) {
			properties->avoidPeriodScram = checked;
			reactor->setAutomaticAvoidPeriodScram(checked);
		});

		automaticMarginBox = makeSettingLabel<FloatBox<float>>(autoTab, "Maximum power deviation: ", 0, properties->steadyMargin * 100);
		automaticMarginBox->setUnits("%");
		automaticMarginBox->setFormat(SCI_NUMBER_FORMAT);
		automaticMarginBox->setMinMaxValues(0.1f, 10.f);
		automaticMarginBox->setAlignment(TextBox::Alignment::Right);
		automaticMarginBox->setValueIncrement(0.5f);
		automaticMarginBox->setSpinnable(true);
		autoLayout->setAnchor(automaticMarginBox->parent(), RelativeGridLayout::makeAnchor(1, 4, 1, 1, Alignment::Minimum, Alignment::Middle));

		automaticMarginBox->setCallback([this](float change) {
			properties->steadyMargin = change / 100;
			reactor->setAutomaticDeviation(change / 100);
		});

		modeTabs->setActiveTab(0);
	}

	const std::string labels[5] = { "Period","Power","Fuel temperature","Water temperature","Water level" };
	const Simulator::ScramSignals reasons[5] = { Simulator::ScramSignals::Period, Simulator::ScramSignals::Power, Simulator::ScramSignals::FuelTemperature, Simulator::ScramSignals::WaterTemperature, Simulator::ScramSignals::WaterLevel };
	void createOperationalLimitsTab() {
		Widget* limits_tab = tabControl->createTab("Operational limits");
		limits_tab->setId("op. limits tab");
		RelativeGridLayout* rel = new RelativeGridLayout();
		rel->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// left border
		rel->appendCol(RelativeGridLayout::Size(200.f, RelativeGridLayout::SizeType::Fixed));		// text
		rel->appendCol(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed));		// data
		rel->appendCol(1.f);																		// check boxes
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// top border
		limits_tab->setLayout(rel);

		// Skips water level
		for (int i = 0; i < 4; i++) {
			rel->appendRow(RelativeGridLayout::Size(27.5f, RelativeGridLayout::SizeType::Fixed));
			Label* temp = limits_tab->add<Label>(labels[i] + " SCRAM :", "sans-bold");
			rel->setAnchor(temp, RelativeGridLayout::makeAnchor(1, 1 + i, 1, 1, Alignment::Minimum, Alignment::Middle));
			scramEnabledBoxes[i] = limits_tab->add<SliderCheckBox>();
			RelativeGridLayout::Anchor a = RelativeGridLayout::makeAnchor(3, 1 + i, 1, 1, Alignment::Minimum, Alignment::Middle);
			a.padding[0] = 8;
			rel->setAnchor(scramEnabledBoxes[i], a);
			scramEnabledBoxes[i]->setChecked(reactor->getScramEnabled(reasons[i]));
			scramEnabledBoxes[i]->setCallback([this, i](bool checked) {
				reactor->setScramEnabled(reasons[i], checked);
				switch (i) {
				case 0: properties->periodScram = checked; break;
				case 1: properties->powerScram = checked; break;
				case 2: properties->tempScram = checked; break;
				case 3: properties->waterTempScram = checked; break;
				case 4: properties->waterLevelScram = checked; break;
				}
			});
		}

		// Create the period limit 
		periodLimBox = limits_tab->add<IntBox<float>>((float)properties->periodLimit);
		rel->setAnchor(periodLimBox, RelativeGridLayout::makeAnchor(2, 1, 1, 1, Alignment::Fill, Alignment::Middle));
		periodLimBox->setFixedSize(Vector2i(100, 20));
		periodLimBox->setUnits("s");
		periodLimBox->setDefaultValue(formatDecimals((float)properties->periodLimit, 1));
		periodLimBox->setFontSize(16);
		periodLimBox->setFormat(SCI_NUMBER_FORMAT);
		periodLimBox->setMinMaxValues(0.f, 3600.f);
		periodLimBox->setValueIncrement(0.1f);
		periodLimBox->setCallback([this](float a) {
			properties->periodLimit = a;
			reactor->setPeriodLimit(a);
		});

		// Create the power limit
		powerLimBox = limits_tab->add<FloatBox<double>>(properties->powerLimit / 1000.);
		rel->setAnchor(powerLimBox, RelativeGridLayout::makeAnchor(2, 2, 1, 1, Alignment::Fill, Alignment::Middle));
		powerLimBox->setFixedSize(Vector2i(100, 20));
		powerLimBox->setUnits("kW");
		powerLimBox->setDefaultValue(to_string(powerLimBox->value()));
		powerLimBox->setFontSize(16);
		powerLimBox->setFormat(SCI_NUMBER_FORMAT);
		powerLimBox->setMinMaxValues(0., 1e12);
		powerLimBox->setValueIncrement(1e2);
		powerLimBox->setCallback([this](double a) {
			properties->powerLimit = a*1e3;
			reactor->setPowerLimit(a * 1e3);
		});

		// Create the fuel temperature limit
		fuel_tempLimBox = limits_tab->add<FloatBox<float>>(properties->tempLimit);
		rel->setAnchor(fuel_tempLimBox, RelativeGridLayout::makeAnchor(2, 3, 1, 1, Alignment::Fill, Alignment::Middle));
		fuel_tempLimBox->setFixedSize(Vector2i(100, 20));
		fuel_tempLimBox->setUnits(degCelsiusUnit);
		fuel_tempLimBox->setDefaultValue(to_string(fuel_tempLimBox->value()));
		fuel_tempLimBox->setFontSize(16);
		fuel_tempLimBox->setFormat(SCI_NUMBER_FORMAT);
		fuel_tempLimBox->setMinValue((int)ENVIRONMENT_TEMPERATURE_DEFAULT);
		fuel_tempLimBox->setValueIncrement(10);
		fuel_tempLimBox->setCallback([this](float a) {
			properties->tempLimit = a;
			reactor->setFuelTemperatureLimit(a);
		});

		// Create the water temperature limit
		water_tempLimBox = limits_tab->add<FloatBox<float>>(properties->waterTempLimit);
		rel->setAnchor(water_tempLimBox, RelativeGridLayout::makeAnchor(2, 4, 1, 1, Alignment::Fill, Alignment::Middle));
		water_tempLimBox->setFixedSize(Vector2i(100, 20));
		water_tempLimBox->setUnits(degCelsiusUnit);
		water_tempLimBox->setDefaultValue(to_string(water_tempLimBox->value()));
		water_tempLimBox->setFontSize(16);
		water_tempLimBox->setFormat(SCI_NUMBER_FORMAT);
		water_tempLimBox->setMinMaxValues(0, 100);
		water_tempLimBox->setValueIncrement(10);
		water_tempLimBox->setCallback([this](float a) {
			properties->waterTempLimit = a;
			reactor->setWaterTemperatureLimit(a);
		});

		// Create the water level limit
		/*water_levelLimBox = limits_tab->add<FloatBox<float>>(properties->waterLevelLimit);
		rel->setAnchor(water_levelLimBox, RelativeGridLayout::makeAnchor(2, 5, 1, 1, Alignment::Fill, Alignment::Middle));
		water_levelLimBox->setFixedSize(Vector2i(100, 20));
		water_levelLimBox->setUnits("m");
		water_levelLimBox->setDefaultValue(to_string(water_levelLimBox->value()));
		water_levelLimBox->setFontSize(16);
		water_levelLimBox->setFormat("[0-9]*\\.?[0-9]+");
		water_levelLimBox->setMinValue(0.f);
		water_levelLimBox->setValueIncrement(0.1f);
		water_levelLimBox->setCallback([this](float a) {
			properties->waterLevelLimit = a;
			reactor->setWaterLevelLimit(a);
		});
		water_levelLimBox->setEnabled(!DEMO_VERSION);
		water_levelLimBox->setEditable(!DEMO_VERSION);*/

		// Create a panel for rod reactivity plot visibility
		rel->appendRow(RelativeGridLayout::Size(27.5f, RelativeGridLayout::SizeType::Fixed));
		Widget* checkBoxPanelRods = limits_tab->add<Widget>();
		rel->setAnchor(checkBoxPanelRods, RelativeGridLayout::makeAnchor(1, 5));
		checkBoxPanelRods->setLayout(panelsLayout);
		checkBoxPanelRods->add<Label>("All control rods at once: ", "sans-bold");
		allRodsBox = checkBoxPanelRods->add<SliderCheckBox>();
		allRodsBox->setFontSize(16);
		allRodsBox->setChecked(properties->allRodsAtOnce);
		allRodsBox->setCallback([this](bool value) {
			properties->allRodsAtOnce = value;
		});

		// Create a panel for rod reactivity plot visibility
		rel->appendRow(RelativeGridLayout::Size(27.5f, RelativeGridLayout::SizeType::Fixed));
		Widget* checkBoxPanelAutoScram = limits_tab->add<Widget>();
		rel->setAnchor(checkBoxPanelAutoScram, RelativeGridLayout::makeAnchor(1, 6));
		checkBoxPanelAutoScram->setLayout(panelsLayout);
		checkBoxPanelAutoScram->add<Label>("SCRAM after pulse: ", "sans-bold");
		autoScramBox = checkBoxPanelAutoScram->add<SliderCheckBox>();
		autoScramBox->setFontSize(16);
		autoScramBox->setChecked(properties->automaticPulseScram);
		autoScramBox->setCallback([this](bool value) {
			properties->automaticPulseScram = value;
			reactor->setAutoScram(value);
		});
	}

	void createPulseTab() {
		Widget* pulse_tab = tabControl->createTab("Pulse");
		pulse_tab->setId("pulse tab");
		RelativeGridLayout* rel = new RelativeGridLayout();
		rel->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 0 left border
		rel->appendCol(2.f);																		// 1 pulse graph
		rel->appendCol(1.f);																		// 2 info panel
		rel->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 3 right border
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 0 top border
		rel->appendRow(1.f);																		// 1 content
		pulse_tab->setLayout(rel);

		pulseGraph = pulse_tab->add<Graph>(4, "Last pulse");
		rel->setAnchor(pulseGraph, RelativeGridLayout::makeAnchor(0, 0, 2, 2));
		initializePulseGraph();

		standInCover = pulse_tab->add<Label>("Perform a pulse experiment to view data", "sans-bold", 35);
		rel->setAnchor(standInCover, RelativeGridLayout::makeAnchor(0, 0, 2, 2));
		standInCover->setTextAlignment(Label::TextAlign::HORIZONTAL_CENTER | Label::TextAlign::VERTICAL_CENTER);
		standInCover->setDrawBackground(true);
		standInCover->setBackgroundColor(Color(60, 255));
		standInCover->setColor(Color(255, 255));
		standInCover->setVisible(true);

		Widget* dataSheet = pulse_tab->add<Widget>();
		rel->setAnchor(dataSheet, RelativeGridLayout::makeAnchor(2, 1));

		RelativeGridLayout* rel2 = new RelativeGridLayout();
		rel2->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 0 left border
		rel2->appendCol(RelativeGridLayout::Size(5.f, RelativeGridLayout::SizeType::Fixed));		// 1 color border
		rel2->appendCol(1.f);																		// 2 content
		rel2->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 3 seperation border
		rel2->appendCol(RelativeGridLayout::Size(5.f, RelativeGridLayout::SizeType::Fixed));		// 4 color border
		rel2->appendCol(1.f);																		// 5 content
		rel2->appendRow(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed));		// 0 content
		rel2->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 1 seperation border
		rel2->appendRow(RelativeGridLayout::Size(100.f, RelativeGridLayout::SizeType::Fixed));		// 2 content
		rel2->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 3 bottom border
		rel2->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));		// 4 time interval
		rel2->appendRow(RelativeGridLayout::Size(20.f, RelativeGridLayout::SizeType::Fixed));		// 5 time labels
		rel2->appendRow(1.f);																		// 6 display label
		dataSheet->setLayout(rel2);

		Color colors[4] = { Color(255,0,0,255), Color(0,255), Color(0,255,0,255), Color(255,0,255,255) };
		std::string text[4] = {"Peak power","FWHM","Peak fuel temp.","Pulse energy"};
		for (int i = 0; i < 4; i++) {
			Widget* marker = dataSheet->add<Widget>();
			rel2->setAnchor(marker, RelativeGridLayout::makeAnchor((i % 2) * 3 + 1, (i / 2) * 2));
			marker->setDrawBackground(true);
			marker->setBackgroundColor(colors[i]);

			Label* baseLabel = dataSheet->add<Label>(text[i], "sans-bold");
			rel2->setAnchor(baseLabel, RelativeGridLayout::makeAnchor((i % 2) * 3 + 2, (i / 2) * 2));
			baseLabel->setPadding(0, 7.f);
			baseLabel->setPadding(1, 5.f);
			baseLabel->setBackgroundColor(Color(32, 255));
			baseLabel->setDrawBackground(true);
			baseLabel->setTextAlignment(Label::TextAlign::TOP | Label::TextAlign::LEFT);
			baseLabel->setFontSize(25.f);
			baseLabel->setColor(Color(170, 255));

			RelativeGridLayout* rel3 = new RelativeGridLayout();
			rel3->appendCol(1.f);
			rel3->appendRow(1.f);
			baseLabel->setLayout(rel3);

			pulseLabels[i] = baseLabel->add<Label>("", "sans-bold");
			pulseLabels[i]->setColor(Color(255, 255));
			pulseLabels[i]->setPadding(0, 7.f);
			pulseLabels[i]->setFontSize(44.f);
			pulseLabels[i]->setTextAlignment(Label::TextAlign::BOTTOM | Label::TextAlign::LEFT);
			rel3->setAnchor(pulseLabels[i], RelativeGridLayout::makeAnchor(0, 0));
		}

		pulseTimer = dataSheet->add<IntervalSlider>();
		rel2->setAnchor(pulseTimer, RelativeGridLayout::makeAnchor(1, 4, 5, 1));
		pulseTimer->setEnabled(false);
		pulseTimer->setHighlightColor(coolBlue);
		pulseTimer->setSteps(50U);
		for (int i = 0; i < 2; i++) pulseTimer->setCallback(i, [this](float /*change*/) {
			updatePulseTrack();
		});
		Label* infoLbl;
		for (int i = 0; i < 2; i++) {
			infoLbl = dataSheet->add<Label>(i ? "5s later" : "pulse start", "sans-bold");
			infoLbl->setColor(Color(255, 255));
			infoLbl->setFontSize(17.f);
			infoLbl->setPadding(1, 4);
			rel2->setAnchor(infoLbl, RelativeGridLayout::makeAnchor(1, 5, 5, 1, i ? Alignment::Maximum : Alignment::Minimum, Alignment::Minimum));
		}

		Widget* displayPanel = dataSheet->add<Widget>();
		rel2->setAnchor(displayPanel, RelativeGridLayout::makeAnchor(1, 6, 5, 1, Alignment::Middle, Alignment::Middle));
		displayPanel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill));

		Label* temp;
		temp = displayPanel->add<Label>("From", "sans-bold", 25);
		temp->setPadding(2, 10);
		pulseDisplayLabels[0] = displayPanel->add<Label>("0", "sans", 30);
		pulseDisplayLabels[1] = displayPanel->add<Label>("00", "sans", 20);
		temp = displayPanel->add<Label>("to", "sans-bold", 25);
		temp->setPadding(0, 10);
		temp->setPadding(2, 10);
		pulseDisplayLabels[2] = displayPanel->add<Label>("5", "sans", 30);
		pulseDisplayLabels[3] = displayPanel->add<Label>("00", "sans", 20);

		for (int i = 0; i < 4; i++) {
			pulseDisplayLabels[i]->setColor(Color(255, 255));
			pulseDisplayLabels[i]->setTextAlignment((i % 2) ? (Label::TextAlign::TOP | Label::TextAlign::LEFT) : (Label::TextAlign::VERTICAL_CENTER | Label::TextAlign::HORIZONTAL_CENTER));
		}
	}

	void createOtherTab() {
		Widget* other_tab = tabControl->createTab("Other");
		other_tab->setId("other tab");
		RelativeGridLayout* rel = new RelativeGridLayout();
		rel->appendCol(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 0 left border
		rel->appendCol(RelativeGridLayout::Size(120.f, RelativeGridLayout::SizeType::Fixed));		// 1 save button
		rel->appendCol(RelativeGridLayout::Size(10.f, RelativeGridLayout::SizeType::Fixed));		// 2 border
		rel->appendCol(RelativeGridLayout::Size(120.f, RelativeGridLayout::SizeType::Fixed));		// 3 load button
		rel->appendCol(RelativeGridLayout::Size(10.f, RelativeGridLayout::SizeType::Fixed));		// 2 border
		rel->appendCol(RelativeGridLayout::Size(120.f, RelativeGridLayout::SizeType::Fixed));		// 5 division thing
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 0 top border
		rel->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));		// 1 Load and save settings
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 2 Seperating space
		rel->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));		// 3 Load and save log
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 4 Seperating space
		rel->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));		// 5 Rod Curves
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 6 Seperating space
		rel->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));		// 7 Reset simulator
		rel->appendRow(RelativeGridLayout::Size(15.f, RelativeGridLayout::SizeType::Fixed));		// 8 Seperating space
		rel->appendRow(RelativeGridLayout::Size(30.f, RelativeGridLayout::SizeType::Fixed));		// 9 Load script

		other_tab->setLayout(rel);

		Button* saveBtn = other_tab->add<Button>("Save settings");
		rel->setAnchor(saveBtn, RelativeGridLayout::makeAnchor(1, 1));
		saveBtn->setCallback([this]() {
			toggleBaseWindow(false);
			std::string saveFileName = file_dialog({ { "rrs", "Simulator settings file" } }, true);
			if (saveFileName.substr((size_t)std::max(0, (int)saveFileName.length() - 4), std::min((size_t)4, saveFileName.length())) != ".rrs") {
				saveFileName = saveFileName.append(".rrs");
			}
			//saveSettings(saveFileName);
			saveArchive(saveFileName);
		});

		Button* loadBtn = other_tab->add<Button>("Load settings");
		rel->setAnchor(loadBtn, RelativeGridLayout::makeAnchor(3, 1));
		loadBtn->setCallback([this]() {
			toggleBaseWindow(false);
			std::string loadFileName = file_dialog({ { "rrs", "Simulator settings file" } }, false);
			// loadSettingsFromFile(loadFileName);
			loadArchive(loadFileName);
			std::cout << "Graph size after GUI: " << properties->graphSize << std::endl;
			updateSettings();
		});

		Button* saveLogBtn = other_tab->add<Button>("Save data");
		rel->setAnchor(saveLogBtn, RelativeGridLayout::makeAnchor(1, 3));
		saveLogBtn->setCallback([this]() {
			std::string logFileName = file_dialog(
			{ { "dat", "Data file" },{ "txt", "Text file" } }, true);
			reactor->dataToFile(logFileName);
		});

		Label* divisionLabel = other_tab->add<Label>("Save each steps:");
		rel->setAnchor(divisionLabel, RelativeGridLayout::makeAnchor(3, 3));

		IntBox<int>* divisionBox = other_tab->add<IntBox<int>>(reactor->data_division);
		rel->setAnchor(divisionBox, RelativeGridLayout::makeAnchor(5, 3));
		//divisionBox->setUnits("%");
		divisionBox->setDefaultValue(to_string(reactor->data_division));
		divisionBox->setFontSize(16);
		divisionBox->setFormat("[0-9]+");
		divisionBox->setSpinnable(true);
		divisionBox->setMinValue(1);
		divisionBox->setMaxValue(100);
		divisionBox->setValueIncrement(1);
		divisionBox->setCallback([this](int a) {
			reactor->data_division = a;
			});

		Button* saveRodCurves = other_tab->add<Button>("Rod curves");
		rel->setAnchor(saveRodCurves, RelativeGridLayout::makeAnchor(1, 5));
		saveRodCurves->setCallback([this]() {
			std::string logFileName = file_dialog(
			{ { "dat", "Data file" },{ "txt", "Text file" } }, true);
			reactor->rodsToFile(logFileName);
		});

		Button* loadScriptBtn = other_tab->add<Button>("Load script");
		rel->setAnchor(loadScriptBtn, RelativeGridLayout::makeAnchor(1, 9));
		loadScriptBtn->setCallback([this]() {
			toggleBaseWindow(false);
			std::string loadFileName = file_dialog({ { "rrs", "Simulator Script file" } }, false);
			loadScriptFromFile(loadFileName);
			});

		Button* resetBtn = other_tab->add<Button>("Reset simulator");
		rel->setAnchor(resetBtn, RelativeGridLayout::makeAnchor(1, 7));
		resetBtn->setCallback([this]() {
			this->resetSimToStart();
		});
	}

	double trackerY[2] = { 0.,1. };

	void resetSimToStart() {
		reactor->resetSimulator();
		properties = new Settings();
		updateSettings(false);
		setSimulationTime(8); // 1x speed
		playPauseSimulation(true);
	}

	void handleDerivativeChange() {
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
			rodDerivatives[i]->setYdata(reactor->rods[i]->derivativeArray());
			double avg = reactor->rods[i]->getRodWorth() / *reactor->rods[i]->getRodSteps();
			double lim = std::round(std::max(std::ceil(reactor->rods[i]->getRodWorth() * reactor->rods[i]->derivativeArray()[reactor->rods[i]->maxDerivative()] / avg)*avg, 2 * avg));
			rodDerivatives[i]->setLimits(0., 1., 0., lim);
		}
	}

	void hardcoreMode(bool value) {
		reactivityPlot->setEnabled(!value);
		rodReactivityPlot->setEnabled(properties->rodReactivityPlot && !value);

		reactivityShow->setVisible(!value);
		rodReactivityShow->setVisible(!value);

		canvas->setPadding(90.f, 25.f, value ? 120.f : 220.f, 50.f);

		performLayout();
	}
	
	template<typename WidgetClass, typename... Args>
	WidgetClass* makeSettingLabel(Widget* parent, std::string text, int fixedWidth = 0, const Args&... args) {
		Widget* panel = parent->add<Widget>();
		panel->setLayout(panelsLayout);
		Label* temp = panel->add<Label>(text, "sans-bold");
		if (fixedWidth) temp->setFixedWidth(fixedWidth);
		return panel->add<WidgetClass>(args...);
	}

	IntBox<int>* makeSimulationSetting(Widget* parent, int initialValue, std::string text) {
		IntBox<int>* tempBox = makeSettingLabel<IntBox<int>>(parent, text, 100, initialValue);
		tempBox->setFixedWidth(100);
		tempBox->setFormat("[0-9]+");
		tempBox->setUnits("%");
		tempBox->setMinMaxValues(0, 100);
		tempBox->setValueIncrement(1);
		tempBox->setSpinnable(true);
		tempBox->setDefaultValue(to_string(initialValue));
		return tempBox;
	}

#if defined(_WIN32)
	void handleDebugChanged() {
		reactor->setDebugMode(debugMode);
		if (debugMode) {
			ShowWindow(GetConsoleWindow(), SW_SHOW);
		}
		else {
			ShowWindow(GetConsoleWindow(), SW_HIDE);
		}
	}
#endif

	~SimulatorGUI() {
		delete reactor;
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 3; j++) {
				delete[] operationModesPlots[i][j];
			}
		}
		
		if (boxConnected) delete theBox;
	}

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
		if (Screen::keyboardEvent(key, scancode, action, modifiers))
			return true;
		if (!baseWindow->enabled()) return false;

		if (action == GLFW_PRESS) {
			if (last10keys.size() == 10) { last10keys.pop_front(); }
			last10keys.push_back(key);
			size_t keyN = last10keys.size();
			bool isGodMode = false;
			bool isDebug = false;
			bool isReset = false;
			if (keyN >= 7) {
				isGodMode = true;
				for (size_t i = keyN - 7; i < keyN; i++) {
					isGodMode = isGodMode && (last10keys[i] == cheat1[i - (keyN - 7)]);
				}
			}
			if (keyN >= 5) {
				isDebug = true;
				isReset = true;
				for (size_t i = keyN - 5; i < keyN; i++) {
					isDebug = isDebug && (last10keys[i] == cheat2[i - (keyN - 5)]);
					isReset = isReset && (last10keys[i] == cheat3[i - (keyN - 5)]);
				}
			}

			if (isGodMode) {
				reactor->godMode = !reactor->godMode;
				cout << "God mode: " << reactor->godMode << endl;
				return true;
			}
			if (isDebug) {
				debugMode = !debugMode;
				handleDebugChanged();
			}
			if (isReset) {
				resetSimToStart();
			}
		}

		// Safety rod
		if (key == safetyRodControl) {
			if (action == GLFW_RELEASE) {
				lastKeyPressed[0] = false;
				reactor->safetyRod()->clearCommands();
			}
			else {
				if (properties->allRodsAtOnce || !(lastKeyPressed[1] || lastKeyPressed[2])) lastKeyPressed[0] = true;
			}
		}
		else if (key == enableSafetyCommand && action == GLFW_PRESS) {
			reactor->safetyRod()->setEnabled(!reactor->safetyRod()->isEnabled());
		} // Regulation rod
		else if (key == regulatoryRodControl) {
			if (action == GLFW_RELEASE) {
				lastKeyPressed[1] = false;
				reactor->regulatingRod()->clearCommands();
			}
			else {
				if (properties->allRodsAtOnce || !(lastKeyPressed[0] || lastKeyPressed[2])) lastKeyPressed[1] = true;
			}
		}
		else if (key == enableRegCommand && action == GLFW_PRESS) {
			reactor->regulatingRod()->setEnabled(!reactor->regulatingRod()->isEnabled());
		} // Shim rod
		else if (key == shimRodControl) {
			if (action == GLFW_RELEASE) {
				lastKeyPressed[2] = false;
				reactor->shimRod()->clearCommands();
			}
			else {
				if (properties->allRodsAtOnce || !(lastKeyPressed[0] || lastKeyPressed[1])) lastKeyPressed[2] = true;
			}
		}
		else if (key == enableShimCommand && action == GLFW_PRESS) {
			reactor->shimRod()->setEnabled(!reactor->shimRod()->isEnabled());
		} // Move rod up
		else if (key == rodUpCommand && action != GLFW_REPEAT) {
			if (action == GLFW_RELEASE) {
				reactor->safetyRod()->clearCommands(ControlRod::CommandType::Top);
				reactor->regulatingRod()->clearCommands(ControlRod::CommandType::Top);
				reactor->shimRod()->clearCommands(ControlRod::CommandType::Top);
			}
			else {
				for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
					if (lastKeyPressed[i]) reactor->rods[i]->commandToTop();
				}
			}
		} // Move rod down
		else if (key == rodDownCommand && action != GLFW_REPEAT) {
			if (action == GLFW_RELEASE) {
				reactor->safetyRod()->clearCommands(ControlRod::CommandType::Bottom);
				reactor->regulatingRod()->clearCommands(ControlRod::CommandType::Bottom);
				reactor->shimRod()->clearCommands(ControlRod::CommandType::Bottom);
			}
			else {
				for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
					if (lastKeyPressed[i]) reactor->rods[i]->commandToBottom();
				}
			}
		} // SCRAM
		else if (key == scramCommand && action == GLFW_PRESS) {
			reactor->scram(Simulator::ScramSignals::User);
		} // reset scram
		else if (key == resetScramCommand && action == GLFW_PRESS) {
			reactor->scram(Simulator::ScramSignals::None);
		} // pause
		else if (key == pauseCommand && action == GLFW_PRESS) {
			playPauseSimulation(reactor->isPaused());
		} // fast forward
		else if (key == fasterCommand && action != GLFW_RELEASE) {
			if (!reactor->isPaused()) {
				setSimulationTime(std::min((int)selectedTime + 1, SIM_TIME_FACTOR_NUMBER - 1));
			}
		} // slow down
		else if (key == slowerCommand && action != GLFW_RELEASE) {
			if (!reactor->isPaused()) {
				setSimulationTime(std::max((int)selectedTime - 1, 0));
			}
		} // exit
		else if (key == GLFW_KEY_ESCAPE) {
			setVisible(false);
		} // change active tab
		else if (key == tabChangeCommand && action == GLFW_PRESS) {
			if (modifiers & GLFW_MOD_SHIFT) {
				if (tabControl->activeTab() == 0) {
					tabControl->setActiveTab(tabControl->tabCount() - 1);
				}
				else {
					tabControl->setActiveTab((tabControl->activeTab() - 1) % tabControl->tabCount());
				}
			}
			else {
				tabControl->setActiveTab((tabControl->activeTab() + 1) % tabControl->tabCount());
			}
		} // fire
		else if (key == firePulseCommand && action == GLFW_PRESS){
			reactor->beginPulse();
		} // toogle neutron source
		else if (key == sourceToggleCommand && action == GLFW_PRESS) {
			reactor->setNeutronSourceInserted(!reactor->getNeutronSourceInserted());
			neutronSourceCB->setChecked(reactor->getNeutronSourceInserted());
		}
		else if (action == GLFW_PRESS && key == demoModeCommand && modifiers & GLFW_MOD_CONTROL) {
			reactor->setDemoMode();
			reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);
			rodMode->setSelectedIndex(0);
		}
		else if (action == GLFW_PRESS && key == demoModeHighPowerCommand && modifiers & GLFW_MOD_CONTROL) {
			reactor->setHighPowerDemoMode();
			reactor->regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);
			rodMode->setSelectedIndex(0);
		}
		else {
			return false;
		}
		// If code execution got to here, it means that one of the if's must have been executed, so return true(handled)
		return true;
	}

	virtual bool resizeEvent(const Eigen::Vector2i &size) {
		if (Screen::resizeEvent(size)) return true;
		if (layoutStart) {
			performLayout();
		}
		else {
			layoutStart = true;
		}
		return true;
	}

private:
	bool lastKeyPressed[NUMBER_OF_CONTROL_RODS];
	int safetyRodControl = GLFW_KEY_S;
	int regulatoryRodControl = GLFW_KEY_R;
	int shimRodControl = GLFW_KEY_C;
	int rodUpCommand = GLFW_KEY_UP;
	int rodDownCommand = GLFW_KEY_DOWN;
	int scramCommand = GLFW_KEY_X;
	int resetScramCommand = GLFW_KEY_0;
	int enableSafetyCommand = GLFW_KEY_1;
	int enableRegCommand = GLFW_KEY_2;
	int enableShimCommand = GLFW_KEY_3;
	int pauseCommand = GLFW_KEY_SPACE;
	int fasterCommand = GLFW_KEY_RIGHT;
	int slowerCommand = GLFW_KEY_LEFT;
	int tabChangeCommand = GLFW_KEY_TAB;
	int firePulseCommand = GLFW_KEY_P;
	int sourceToggleCommand = GLFW_KEY_N;
	int demoModeCommand = GLFW_KEY_D;
	int demoModeHighPowerCommand = GLFW_KEY_F;
	int cheat1[7] = { GLFW_KEY_G, GLFW_KEY_O, GLFW_KEY_D, GLFW_KEY_M, GLFW_KEY_O, GLFW_KEY_D, GLFW_KEY_E };
	int cheat2[5] = { GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_B, GLFW_KEY_U, GLFW_KEY_G };
	int cheat3[5] = { GLFW_KEY_R, GLFW_KEY_E, GLFW_KEY_S, GLFW_KEY_E, GLFW_KEY_T };
	bool debugMode = false;
	deque<int> last10keys = deque<int>();
	size_t displayInterval[2] = { 0,0 };
	bool btns[11];
	int lastModeState = 0;

	void updateAlphaGraph() {
		// Point 1
		alphaX[0] = 0.;
		alphaY[0] = properties->alpha0;

		// Point 2
		alphaX[1] = (double)properties->alphaT1;
		alphaY[1] = properties->alphaAtT1;

		// Point 3
		alphaX[2] = 1000.;
		alphaY[2] = properties->alphaAtT1 + (float)(properties->alphaK * (1000.f - properties->alphaT1));

		// Autoscale
		alphaPlot->setLimits(0., 1000., (std::ceil(std::min(std::min(properties->alpha0, properties->alphaAtT1), alphaY[2]) / 5.) - 1.) * 5., (std::floor(std::max(std::max(properties->alpha0, properties->alphaAtT1), alphaY[2]) / 5) + 1.)*5.);
		alphaPlot->setMajorTickNumber((size_t) std::max((alphaPlot->limits()[3] - alphaPlot->limits()[2] - 5.) / 5., 0.));
	}

	std::string getTimeSinceStart() {
		size_t time[3];
		double t = reactor->getCurrentTime();
		time[2] = (size_t)floor(fmod(t, 60.));
		time[1] = (size_t)floor(fmod(t, 3600.) / 60.);
		time[0] = (size_t)floor(t / 3600.);
		std::string ret[3];
		for (int i = 0; i < 3; i++) {
			ret[i] = ((time[i] < 10) ? ("0" + to_string(time[i])) : (to_string(time[i])));
		}
		return ret[0] + ":" + ret[1] + ":" + ret[2];
	}

public:
	double lastTime = nanogui::get_seconds_since_epoch();

	virtual void draw(NVGcontext *ctx) {
		double reactorElapsed = reactor->getCurrentTime();
		if (startScript.size()) {
			loadScriptFromFile(startScript);
			startScript = "";
		}
			
		// Run new calculation
		reactor->runLoop();


		// Get from which index to which index the data will be drawn and update view slider
		const double sliderRange = std::min(DELETE_OLD_DATA_TIME_DEFAULT, reactorElapsed);
		double sliderStart = displayTimeSlider->value(0) * sliderRange;
		if (viewStart >= 0.) {
			if (!timeLockedBox->checked()) {
				double diff = reactorElapsed - timeAtLastChange;
				sliderStart = viewStart + diff - max(0., reactorElapsed - DELETE_OLD_DATA_TIME_DEFAULT);
				reculculateDisplayInterval(max(viewStart + diff, 0.), viewStart + diff + properties->displayTime);
			}
			else {
				sliderStart = viewStart - max(0., reactorElapsed - DELETE_OLD_DATA_TIME_DEFAULT);
				reculculateDisplayInterval(max(viewStart, 0.), viewStart + properties->displayTime);
			}
		}
		else {
			sliderStart = max(reactorElapsed - properties->displayTime, 0.);
			reculculateDisplayInterval(sliderStart, reactorElapsed);
		}
		displayTimeSlider->setValue(0, (float)(sliderStart / sliderRange));
		displayTimeSlider->setValue(1, (float)min(1., (sliderStart + properties->displayTime)/sliderRange));

		// Link plots to display interval
		reactivityPlot->setPlotRange(displayInterval[0], displayInterval[1]);
		rodReactivityPlot->setPlotRange(displayInterval[0], displayInterval[1]);
		temperaturePlot->setPlotRange(displayInterval[0], displayInterval[1]);
		powerPlot->setPlotRange(displayInterval[0], displayInterval[1]);
		for (size_t i = 0; i < 6; i++) {
			delayedGroups[i]->setPlotRange(displayInterval[0], displayInterval[1]);
		}

		try {
			// Save times for better performance
			double timeStart = reactor->time_[displayInterval[0]];
			double timeEnd = reactor->time_[displayInterval[1]];
			// Set reactivity scaling
			reactivityPlot->setLimits(timeStart, timeEnd, properties->reactivityGraphLimits[0], properties->reactivityGraphLimits[1]);
			rodReactivityPlot->setLimits(timeStart, timeEnd, properties->reactivityGraphLimits[0], properties->reactivityGraphLimits[1]);
			// Set power plot scaling
			pair<int, int> newExtremes = recalculatePowerExtremes();
			if (isZero.first || isZero.second) {
				if (isZero.first && isZero.second) {
					powerPlot->setLimits(timeStart, timeEnd,
						0., 1.);
				}
				else {
					powerPlot->setLimits(timeStart, timeEnd,
						0., pow(10., std::max(newExtremes.first, newExtremes.second)));
				}
			}
			else {
				powerPlot->setLimits(timeStart, timeEnd,
					/*(newExtremes.first < -3) ? 0. : pow(10., newExtremes.first)*/ powerPlot->getYlog() ? pow(10., newExtremes.first) : 0., pow(10., newExtremes.second));
			}
			// Set temperature scaling
			temperaturePlot->setLimits(timeStart, timeEnd, properties->temperatureGraphLimits[0], properties->temperatureGraphLimits[1]);

			// Set stacked graph scaling
			if (tabControl->activeTab() == 4) {
				for (int i = 0; i < 6; i++) {
					delayedGroups[i]->setLimits(timeStart, timeEnd, 0., 3.);
				}
			}
		}
		catch (exception e) {
			cerr << "Index out of bounds: SimulatorGUI.draw" << "\n" << e.what() << endl;
		}

		// re-draw control rods
		if (tabControl->activeTab() == 3) {
			float pointPos;
			for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
				rodCurves[i]->setPointerPosition(reactor->rods[i]->getCurrentPCM() / reactor->rods[i]->getRodWorth());
				rodCurves[i]->setRodPosition(*reactor->rods[i]->getExactPosition() / *reactor->rods[i]->getRodSteps());
				rodCurves[i]->setHorizontalPointerPosition(*reactor->rods[i]->getExactPosition() / *reactor->rods[i]->getRodSteps());

				pointPos = *reactor->rods[i]->getExactPosition();
				pointPos = (float)(reactor->rods[i]->derivativeArray()[(int)std::floor(pointPos)] * (std::ceil(pointPos) - pointPos) + (pointPos - std::floor(pointPos))*reactor->rods[i]->derivativeArray()[(int)std::ceil(pointPos)]);
				rodDerivatives[i]->setPointerPosition((float)(reactor->rods[i]->getRodWorth() * pointPos / rodDerivatives[i]->limits()[3]));
				rodDerivatives[i]->setHorizontalPointerPosition(*reactor->rods[i]->getExactPosition() / *reactor->rods[i]->getRodSteps());
			}
		}

		// Show data
		powerShow->setData(reactor->getCurrentPower());
		size_t curIndx = reactor->getCurrentIndex();
		reactivityShow->setData(reactor->reactivity_[curIndx]);
		rodReactivityShow->setData(reactor->rodReactivity_[curIndx]);
		temperatureShow->setData(reactor->temperature_[curIndx]);
		waterTemperatureShow->setData(*reactor->getWaterTemperature());
		//waterLevelShow->setData(*reactor->getWaterLevel() * 100.);
		periodShow->setData(*reactor->getReactorPeriod());

		//Data for graphical reactor period display
		periodDisplay->setPeriod(*reactor->getReactorPeriod());

		double newTime = nanogui::get_seconds_since_epoch();
		float thisFps = powf((float)(newTime - lastTime), -1.f);
		fpsSum += thisFps;
		if (fpsCount == 0 || fpsCount == 20) {
			fpsLabel->setCaption("FPS: " + to_string((int)roundf(fpsCount ? (fpsSum / fpsCount) : thisFps)));
			fpsSum = thisFps;
			fpsCount %= 20;
		}
		fpsCount++;
		lastTime = newTime;

		// Update alpha plot
		float tempNow = reactor->getCurrentTemperature();
		alphaPlot->setHorizontalPointerPosition(tempNow / 1000.f);
		alphaPlot->setPointerPosition((float)((reactor->getReactivityCoefficient(tempNow) - alphaPlot->limits()[2]) / (alphaPlot->limits()[3] -  alphaPlot->limits()[2])));

		// Update the text
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) rodBox[i]->setText((int)std::ceilf(*reactor->rods[i]->getExactPosition()));

		// Update time
		timeLabel->setCaption(getTimeSinceStart());

		if (!reactor->getScramStatus()) {
			if ((*reactor->getReactorPeriod() < 1.1 * properties->periodLimit) && (*reactor->getReactorPeriod() > 0.)) {
				periodScram->setBackgroundColor(Color(175, 100, 0, 255));
			}
			else {
				periodScram->setBackgroundColor(Color(120, 120));
			}
			if (tempNow > 0.9 * properties->tempLimit) {
				fuelTemperatureScram->setBackgroundColor(Color(175, 100, 0, 255));
			}
			else {
				fuelTemperatureScram->setBackgroundColor(Color(120, 120));
			}
			if (*reactor->getWaterTemperature() > 0.9 * properties->waterTempLimit) {
				waterTemperatureScram->setBackgroundColor(Color(175, 100, 0, 255));
			}
			else {
				waterTemperatureScram->setBackgroundColor(Color(120, 120));
			}
			if (reactor->getCurrentPower() > 0.9 * properties->powerLimit) {
				powerScram->setBackgroundColor(Color(175, 100, 0, 255));
			}
			else {
				powerScram->setBackgroundColor(Color(120, 120));
			}
		}

		if (shouldUpdateNeutronSource) {
			sourceSettings->performLayout(ctx);
			shouldUpdateNeutronSource = false;
		}
		
		/* Draw the user interface */
		Screen::draw(ctx);
		
		// Send dickbut PNG bits over serial
		if (boxConnected) {
			if (theBox->IsConnected()) handleBox();
		}
		else {
			updateCOMports();
		}
	}
	
	double lastData = 0.;
	void handleBox() {
		LEDstatus = (uint16_t)0;
		// Write LED status
		int scramS = reactor->getScramStatus();
		if(Simulator::ScramSignals::Period & scramS) LEDstatus |= SCRAM_PER;
		if(Simulator::ScramSignals::FuelTemperature & scramS) LEDstatus |= SCRAM_FT;
		if(Simulator::ScramSignals::WaterTemperature & scramS) LEDstatus |= SCRAM_WT;
		if(Simulator::ScramSignals::Power & scramS) LEDstatus |= SCRAM_POW;
		if(Simulator::ScramSignals::User & scramS) LEDstatus |= SCRAM_MAN;

		if (reactor->safetyRod()->isEnabled()) { LEDstatus |= ROD_SAFETY_ENBL; }
		if (reactor->regulatingRod()->isEnabled()) { LEDstatus |= ROD_REG_ENBL; }
		if (reactor->shimRod()->isEnabled()) { LEDstatus |= ROD_SHIM_ENBL; }
		
		if (reactor->safetyRod()->getCommandType() == ControlRod::CommandType::Top || *reactor->safetyRod()->getExactPosition() == (float)*reactor->safetyRod()->getRodSteps()) {
			LEDstatus |= ROD_SAFETY_UP;
		}
		if (reactor->regulatingRod()->getCommandType() == ControlRod::CommandType::Top || *reactor->regulatingRod()->getExactPosition() == (float)*reactor->regulatingRod()->getRodSteps()) {
			LEDstatus |= ROD_REG_UP;
		}
		if (reactor->shimRod()->getCommandType() == ControlRod::CommandType::Top || *reactor->shimRod()->getExactPosition() == (float)*reactor->shimRod()->getRodSteps()) {
			LEDstatus |= ROD_SHIM_UP;
		}
		if (reactor->safetyRod()->getCommandType() == ControlRod::CommandType::Bottom || *reactor->safetyRod()->getExactPosition() == 0.f) {
			LEDstatus |= ROD_SAFETY_DOWN;
		}
		if (reactor->regulatingRod()->getCommandType() == ControlRod::CommandType::Bottom || *reactor->regulatingRod()->getExactPosition() == 0.f) {
			LEDstatus |= ROD_REG_DOWN;
		}
		if (reactor->shimRod()->getCommandType() == ControlRod::CommandType::Bottom || *reactor->shimRod()->getExactPosition() == 0.f) {
			LEDstatus |= ROD_SHIM_DOWN;
		}
		if (reactor->regulatingRod()->getOperationMode() == ControlRod::OperationModes::Pulse && reactor->getScramStatus() == 0) {
			LEDstatus |= FIRE_LED_B;
		}

		// Convert LED status to two bytes
		char sendByte[3];
		sendByte[0] = 77;
		sendByte[1] = LEDstatus >> 8;
		sendByte[2] = LEDstatus & 0x00ff;

		// Write LED data
		theBox->WriteData(sendByte, 3);

		// Reset sounds
		// LEDstatus &= (1 << 13) - 1;

		// Read data
		uint16_t box_data = 0;
		bool gotData = false;
		char buffer[2];
		double time_now = nanogui::get_seconds_since_epoch();
		while (theBox->availableBytes() >= 2) {
			theBox->ReadData(buffer, 2);
			box_data = (unsigned char)buffer[1];
			box_data += ((uint16_t)buffer[0]) << 8;
			gotData = true;
			handleBoxData(box_data, time_now);
		}
		if (!gotData) { // Disconnect box if no data is recieved in 1 second
			if (lastData == 0.) {
				lastData = time_now;
			}
			else if (time_now > lastData + 1.) {
				boxConnected = false;
				theBox->~Serial();
				std::cout << "Box disconnected! (timeout)" << std::endl;
				lastData = 0.;
			}
			return;
		}
		
	}

	bool shouldUpdateNeutronSource = false;
	void updateNeutronSourceTab() {
		int v = (int)reactor->getNeutronSourceMode() - 1;
		bool tempB;
		for (int i = 0; i < 3; i++) {
			tempB = (v == i);
			neutronSourcePeriodBoxes[i]->parent()->setVisible(tempB);
			neutronSourcePeriodBoxes[i]->setEditable(tempB);
			neutronSourceAmplitudeBoxes[i]->parent()->setVisible(tempB);
			neutronSourceAmplitudeBoxes[i]->setEditable(tempB);
		}
		tempB = (v == 0);
		for (int i = 0; i < 4; i++) {
			neutronSourceSQWBoxes[i]->parent()->setVisible(tempB);
			neutronSourceSQWBoxes[i]->setEditable(tempB);
		}
		tempB = (v == 1);
		neutronSourceSINEModeBox->parent()->setVisible(tempB);
		neutronSourceSINEModeBox->setEnabled(tempB);
		tempB = (v == 2);
		for (int i = 0; i < 6; i++) {
			neutronSourceSAWBoxes[i]->parent()->setVisible(tempB);
			neutronSourceSAWBoxes[i]->setEditable(tempB);
		}
		for (int i = (int)sourceGraph->actualGraphNumber() - 1; i >= 0; i--) {
			sourceGraph->removeGraphElement(i);
		}
		PeriodicalMode* ns_mode = reactor->getSourceModeClass(reactor->getNeutronSourceMode());
		size_t dataP = ns_mode->num_points();
		neutronSourcePlot = sourceGraph->addPlot(dataP);
		neutronSourcePlot->setPlotRange(0, dataP - 1);
		neutronSourcePlot->setLimits(0., ns_mode->getPeriod(), std::min(-1.5*ns_mode->getAmplitude(), -1.), std::max(1.5*ns_mode->getAmplitude(), 1.));
		neutronSourcePlot->setMainTickFontSize(18.f);
		neutronSourcePlot->setMajorTickFontSize(16.f);
		neutronSourcePlot->setNameFontSize(24.f);
		neutronSourcePlot->setPointerShown(false);
		neutronSourcePlot->setColor(coolBlue);
		neutronSourcePlot->setDrawMode(DrawMode::Default);
		neutronSourcePlot->setTextColor(Color(250, 255));
		neutronSourcePlot->setAxisColor(Color(250, 255));
		neutronSourcePlot->setTextShown(true);
		neutronSourcePlot->setAxisShown(true);
		neutronSourcePlot->setUnits("n/s");
		neutronSourcePlot->setName("Delta source activity");
		neutronSourcePlot->setTextOffset(40.f);
		neutronSourcePlot->setMainLineShown(true);
		neutronSourcePlot->setMajorTickNumber(3);
		neutronSourcePlot->setMinorTickNumber(1);
		neutronSourcePlot->setHorizontalAxisShown(true);
		neutronSourcePlot->setHorizontalUnits("s");
		neutronSourcePlot->setHorizontalName("Time");
		neutronSourcePlot->setHorizontalMainLineShown(true);
		neutronSourcePlot->setHorizontalMajorTickNumber(3);
		neutronSourcePlot->setHorizontalMinorTickNumber(1);
		if (v >= 0) {
			neutronSourceTracker = sourceGraph->addPlot(2);
			neutronSourceTracker->setPlotRange(0, 1);
			neutronSourceTracker->setDrawMode(DrawMode::Default);
			neutronSourceTracker->setAxisShown(false);
			neutronSourceTracker->setColor(Color(1.f, 0.f, 0.f, 1.f));
			neutronSourceTracker->setHorizontalAxisShown(false);
			neutronSourceTracker->setTextShown(false);
			neutronSourceTracker->setPointerShown(false);
			neutronSourceTracker->setXdata(ns_mode->getTrackerArray());
			neutronSourceTracker->setYdata(trackerY);
			neutronSourceTracker->setLimits(0., ns_mode->getPeriod(), 0., 1.);
		}
		double * xAxis = new double[dataP];
		double * yAxis = new double[dataP];
		ns_mode->fillXYaxis(xAxis, yAxis);

		neutronSourcePlot->setXdata(xAxis);
		neutronSourcePlot->setYdata(yAxis);
		shouldUpdateNeutronSource = true;
	}

	void handleBoxData(uint16_t box_data, double now) {
		lastData = now;
		bool rodsMoving[NUMBER_OF_CONTROL_RODS];
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) rodsMoving[i] = (reactor->rods[i]->getCommandType() == ControlRod::CommandType::None);
		if (box_data & SCRAM_BTN) {
			if (!btns[0]) reactor->scram(Simulator::ScramSignals::User);
		}
		if (box_data & FIRE_BTN) {
			if (!btns[1]) {
				if (reactor->getScramStatus() == 0)reactor->beginPulse();
			}
		}
		if (box_data & ENABLE_SAFETY_BTN) {
			if (!btns[2]) {
				if (reactor->getScramStatus() == 0) reactor->safetyRod()->setEnabled(!reactor->safetyRod()->isEnabled());
			}
		}
		if (box_data & UP_SAFETY_BTN) {
			if (!btns[3] && ((rodsMoving[1] && rodsMoving[2]) || properties->allRodsAtOnce)) reactor->safetyRod()->commandToTop();
		}
		else {
			if (btns[3]) reactor->safetyRod()->clearCommands(ControlRod::CommandType::Top);
		}
		if (box_data & DOWN_SAFETY_BTN) {
			if (!btns[4] && ((rodsMoving[1] && rodsMoving[2]) || properties->allRodsAtOnce)) reactor->safetyRod()->commandToBottom();
		}
		else {
			if (btns[4]) reactor->safetyRod()->clearCommands(ControlRod::CommandType::Bottom);
		}
		if (box_data & ENABLE_REG_BTN) {
			if (!btns[5]) {
				if (reactor->getScramStatus() == 0) reactor->regulatingRod()->setEnabled(!reactor->regulatingRod()->isEnabled());
			}
		}
		if (box_data & UP_REG_BTN) {
			if (!btns[6] && ((rodsMoving[0] && rodsMoving[2]) || properties->allRodsAtOnce)) reactor->regulatingRod()->commandToTop();
		}
		else {
			if (btns[6]) reactor->regulatingRod()->clearCommands(ControlRod::CommandType::Top);
		}
		if (box_data & DOWN_REG_BTN) {
			if (!btns[7] && ((rodsMoving[0] && rodsMoving[2]) || properties->allRodsAtOnce)) reactor->regulatingRod()->commandToBottom();
		}
		else {
			if (btns[7]) reactor->regulatingRod()->clearCommands(ControlRod::CommandType::Bottom);
		}
		if (box_data & ENABLE_SHIM_BTN) {
			if (!btns[8]) {
				if (reactor->getScramStatus() == 0) reactor->shimRod()->setEnabled(!reactor->shimRod()->isEnabled());
			}
		}
		if (box_data & UP_SHIM_BTN) {
			if (!btns[9] && ((rodsMoving[0] && rodsMoving[1]) || properties->allRodsAtOnce)) reactor->shimRod()->commandToTop();
		}
		else {
			if (btns[9]) reactor->shimRod()->clearCommands(ControlRod::CommandType::Top);
		}
		if (box_data & DOWN_SHIM_BTN) {
			if (!btns[10] && ((rodsMoving[0] && rodsMoving[1]) || properties->allRodsAtOnce)) reactor->shimRod()->commandToBottom();
		}
		else {
			if (btns[10]) reactor->shimRod()->clearCommands(ControlRod::CommandType::Bottom);
		}
		btns[0] = (box_data & SCRAM_BTN) != 0;
		btns[1] = (box_data & FIRE_BTN) != 0;
		btns[2] = (box_data & ENABLE_SAFETY_BTN) != 0;
		btns[3] = (box_data & UP_SAFETY_BTN) != 0;
		btns[4] = (box_data & DOWN_SAFETY_BTN) != 0;
		btns[5] = (box_data & ENABLE_REG_BTN) != 0;
		btns[6] = (box_data & UP_REG_BTN) != 0;
		btns[7] = (box_data & DOWN_REG_BTN) != 0;
		btns[8] = (box_data & ENABLE_SHIM_BTN) != 0;
		btns[9] = (box_data & UP_SHIM_BTN) != 0;
		btns[10] = (box_data & DOWN_SHIM_BTN) != 0;

		int mode = box_data & 7;
		if (mode != lastModeState) {
			rodMode->setSelectedIndex(mode);
			lastModeState = mode;
		}
	}

	static string formatDecimals(const double x, const int decDigits, bool removeTrailingZeros = true) {
		stringstream ss;
		ss << fixed;
		ss.precision(decDigits);
		ss << x;
		std::string res = ss.str();
		if (removeTrailingZeros) {
			while (res.back() == '0') res = res.substr(0, res.length() - 1);
			if (res.back() == '.') res = res.substr(0, res.length() - 1);
		}
		return res;
	}

	void reculculateDisplayInterval(double fromTime, double toTime) {
		fromTime = std::max(fromTime, 0.);
		toTime = std::min(toTime, reactor->getCurrentTime());
		displayInterval[0] = reactor->getIndexFromTime(fromTime);
		displayInterval[1] = reactor->getIndexFromTime(toTime);
	}

	// Method for calculating autoscale factors
	pair<int, int> recalculatePowerExtremes(double fromTime = 0., double toTime = 0.) {
		int err = 0;
		if (fromTime + toTime == 0.) {
			fromTime = reactor->time_[displayInterval[0]];
			toTime = reactor->time_[displayInterval[1]];
		}
		try {
			// Find the last change since the graph begin time
			size_t startIndex = 0;
			for (; startIndex < reactor->getOrderChanges(); startIndex++) {
				if (reactor->getExtremeAt(startIndex).when >= fromTime) break;
			}
			Simulator::PowerExtreme* firstExtreme;
			if (startIndex) {
				startIndex--;
				firstExtreme = &reactor->getExtremeAt(startIndex);
			}
			else {
				firstExtreme = &reactor->trailingExtreme;
			}
			err = 1;
			// Find the last change before the graph end time
			size_t endIndex = startIndex;
			for (; endIndex < reactor->getOrderChanges(); endIndex++) {
				if (reactor->getExtremeAt(endIndex).when > toTime) break;
			}
			if (endIndex != 0) endIndex--;
			err = 2;
			// If there is only one order change, return the order
			if (startIndex == endIndex) {
				isZero.first = firstExtreme->isZero;
				isZero.second = firstExtreme->isZero;
				return pair<int, int>(firstExtreme->order, firstExtreme->order + 1);
			}
			else {
				// Iterate through the order changes and find the smallest and largest order
				int minOrder = firstExtreme->order;
				int maxOrder = minOrder;
				isZero.first = firstExtreme->isZero;
				isZero.second = firstExtreme->isZero;
				for (size_t i = startIndex + 1; i <= endIndex; i++) {
					Simulator::PowerExtreme current = reactor->getExtremeAt(i);
					if (current.isZero) {
						isZero.first = true;
					}
					else {
						if (isZero.second) {
							maxOrder = current.order;
							isZero.second = false;
						}
						else {
							maxOrder = max(maxOrder, current.order);
						}
						minOrder = min(minOrder, current.order);
					}
				}
				isZero.first = isZero.first || minOrder < -7;
				return pair<int, int>(minOrder, ++maxOrder);
			}
		}
		catch (exception e) {
			cerr << "recalculatePowerExtremes(): error code " << err << endl;
			isZero.first = true;
			return pair<int, int>(0, 10);
		}

	}

	std::vector<string> getCOMports() {
#ifdef _WIN32
		TCHAR* ptr = new TCHAR[65535];
		TCHAR *temp_ptr;
		unsigned long dwChars = QueryDosDevice(NULL, ptr, 65535);
		std::vector<string> comPorts_ = std::vector<string>();
		while (dwChars)
		{
			int port;
			if (sscanf(ptr, "COM%d", &port) == 1)
			{
				comPorts_.push_back("COM" + std::to_string(port));
			}
			temp_ptr = strchr(ptr, 0);
			dwChars -= (DWORD)((temp_ptr - ptr) / sizeof(TCHAR) + 1);
			ptr = temp_ptr + 1;
		}
		return comPorts_;
#else
		return std::vector<string>();
#endif
	}

	void saveArchive(std::string path) {
		properties->saveArchive(path);
		toggleBaseWindow(true);
	}

	void loadArchive(std::string path) {
		properties->restoreArchive(path);
		toggleBaseWindow(true);
	}

	void loadScriptFromFile(std::string path) {
		double time0 = reactor->getCurrentTime();
		std::ifstream ifs;
		if (path.length()) {
			ifs.open(path);

			if (!ifs)
			{
				std::cerr << "Error opening input file: " << path << std::endl;
				return;
			}
			std::istream& is = static_cast<std::istream&>(ifs);

			Command cmd;

			while (is >> cmd) {
				cmd.timed += time0;
				cout << cmd;
				reactor->scriptCommands.push_back(cmd);
			}


			MessageDialog* msg = new MessageDialog(this, MessageDialog::Type::Warning, "Load script", "Loaded.");
			msg->setPosition(Vector2i((this->size().x() - msg->size().x()) / 2, (this->size().y() - msg->size().y()) / 2));
			msg->setCallback([this](int /*choice*/) {
				toggleBaseWindow(true);
				});
		} else {
			MessageDialog* msg = new MessageDialog(this, MessageDialog::Type::Warning, "Load script", "Bad file name.");
			msg->setPosition(Vector2i((this->size().x() - msg->size().x()) / 2, (this->size().y() - msg->size().y()) / 2));
			msg->setCallback([this](int /*choice*/) {
				toggleBaseWindow(true);
				});
		}
	}

	void updateSettings(bool updateReactor = true) {
		curveFillBox->setChecked(properties->curveFill);
		curveFillBox->callback()(properties->curveFill);
		avoidPeriodScramBox->setChecked(properties->avoidPeriodScram);
		avoidPeriodScramBox->callback()(properties->avoidPeriodScram);
		for (int i = 0; i < 6; i++) {
			delayedGroupBoxes[i]->setValue(properties->betas[i]);
			delayedGroupBoxes[6 + i]->setValue(properties->lambdas[i]);
			delayedGroupsEnabledBoxes[i]->setChecked(properties->groupsEnabled[i]);
			delayedGroupsEnabledBoxes[i]->callback()(properties->groupsEnabled[i]);
		}
		coreVolumeBox->setValue(properties->coreVolume * 1e3); // Convert from m3 to L
		alpha0Box->setValue(properties->alpha0);
		alphaPeakBox->setValue(properties->alphaAtT1);
		alphaSlopeBox->setValue((float)properties->alphaK);
		tempPeakBox->setValue(properties->alphaT1);
		displayBox->setValue(properties->displayTime);
		excessReactivityBox->setValue(properties->excessReactivity);
		fissionProductsBox->setChecked(properties->fissionPoisons);
		fissionProductsBox->callback()(properties->fissionPoisons);
		graphSizeBox->setValue((int)(properties->graphSize * 100));
		sourceActivityBox->setValue(properties->neutronSourceActivity);
		neutronSourceCB->setChecked(properties->neutronSourceInserted);
		neutronSourceCB->callback()(properties->neutronSourceInserted);
		neutronSourceModeBox->setSelectedIndex(properties->ns_mode);
		neutronSourcePeriodBoxes[0]->setValue(properties->ns_squareWave.period);
		neutronSourcePeriodBoxes[1]->setValue(properties->ns_sineMode.period);
		neutronSourcePeriodBoxes[2]->setValue(properties->ns_sawToothMode.period);
		neutronSourceAmplitudeBoxes[0]->setValue(properties->ns_squareWave.amplitude);
		neutronSourceAmplitudeBoxes[1]->setValue(properties->ns_sineMode.amplitude);
		neutronSourceAmplitudeBoxes[2]->setValue(properties->ns_sawToothMode.amplitude);
		for (int i = 0; i < 4; i++)	neutronSourceSQWBoxes[i]->setValue((int)(properties->ns_squareWave.xIndex[i] * 100));
		neutronSourceSINEModeBox->setSelectedIndex((int)properties->ns_sineMode.mode);
		for (int i = 0; i < 6; i++)	neutronSourceSAWBoxes[i]->setValue((int)(properties->ns_sawToothMode.xIndex[i] * 100));
		periodLimBox->setValue((float)properties->periodLimit);
		powerLimBox->setValue(properties->powerLimit * 1e-03);
		fuel_tempLimBox->setValue(properties->tempLimit);
		water_tempLimBox->setValue(properties->waterTempLimit);
		//water_levelLimBox->setValue(properties->waterLevelLimit);
		for (int i = 0; i < 4; i++) {
			bool value = false;
			switch (i) {
			case 0: value = properties->periodScram; break;
			case 1: value = properties->powerScram; break;
			case 2: value = properties->tempScram; break;
			case 3: value = properties->waterTempScram; break;
			case 4: value = properties->waterLevelScram; break;
			}
			scramEnabledBoxes[i]->setChecked(value);
			scramEnabledBoxes[i]->callback()(value);
		}

		promptNeutronLifetimeBox->setValue(properties->promptNeutronLifetime);
		for (int i = 0; i < 2; i++) {
			reactivityLimitBox[i]->setValue(properties->reactivityGraphLimits[i]);
			temperatureLimitBox[i]->setValue(properties->temperatureGraphLimits[i]);
		}
		rodReactivityBox->setChecked(properties->rodReactivityPlot);
		rodReactivityBox->callback()(properties->rodReactivityPlot);
		for (int i = 0; i < 3; i++) {
			rodStepsBox[i]->setValue((int)properties->rodSettings[i].rodSteps);
			rodWorthBox[i]->setValue(properties->rodSettings[i].rodWorth);
			rodSpeedBox[i]->setValue(properties->rodSettings[i].rodSpeed);
			for (int j = 0; j < 2; j++) {
				rodCurveSliders[i * 2 + j]->setValue(properties->rodSettings[i].rodCurve[j]);
				rodCurveSliders[i * 2 + j]->finalCallback()(properties->rodSettings[i].rodCurve[j]);
				rodCurves[i]->setParameter(j * 2, properties->rodSettings[i].rodCurve[j]);
			}
		}
		periodBoxes[0]->setValue(properties->squareWave.period);
		amplitudeBoxes[0]->setValue(properties->squareWave.amplitude);
		for(int i = 0; i < 4; i++)	squareWaveBoxes[i]->setValue((int)(properties->squareWave.xIndex[i] * 100));
		periodBoxes[1]->setValue(properties->sineMode.period);
		amplitudeBoxes[1]->setValue(properties->sineMode.amplitude);
		sineModeBox->setSelectedIndex(properties->sineMode.mode);
		periodBoxes[2]->setValue(properties->sawToothMode.period);
		amplitudeBoxes[2]->setValue(properties->sawToothMode.amplitude);
		for(int i = 0; i < 6; i++)	sawToothBoxes[i]->setValue((int)(properties->sawToothMode.xIndex[i] * 100));
		keepCurrentPowerBox->setChecked(properties->steadyCurrentPower);
		keepCurrentPowerBox->callback()(properties->steadyCurrentPower);
		steadyPowerBox->setValue(properties->steadyGoalPower);
		automaticMarginBox->setValue(properties->steadyMargin * 100);
		tempEffectsBox->setChecked(properties->temperatureEffects);
		tempEffectsBox->callback()(properties->temperatureEffects);
		cooling->setChecked(properties->waterCooling);
		cooling->callback()(properties->waterCooling);
		coolingPowerBox->setValue(properties->waterCoolingPower);
		waterVolumeInput->setValue(properties->waterVolume);
		allRodsBox->setChecked(properties->allRodsAtOnce);
		allRodsBox->callback()(properties->allRodsAtOnce);
		logScaleBox->setChecked(properties->yAxisLog);
		logScaleBox->callback()(properties->yAxisLog);
		autoScramBox->setChecked(properties->automaticPulseScram);
		autoScramBox->callback()(properties->automaticPulseScram);
		hardcoreBox->setChecked(properties->reactivityHardcore);
		hardcoreBox->callback()(properties->reactivityHardcore);
		squareWaveSpeedBox->setChecked(properties->squareWaveUsesRodSpeed);
		squareWaveSpeedBox->callback()(properties->squareWaveUsesRodSpeed);

		if (updateReactor) reactor->setProperties(properties);
	}

	bool prevToggle;
	void toggleBaseWindow(bool value) {
		if (!value && baseWindow->enabled()) prevToggle = (reactor->getSpeedFactor() != 0.);
		baseWindow->setEnabled(value);
		baseWindow->setFocused(value);
		playPauseSimulation(value ? prevToggle : value);
	}

};

int main(int argc, char **  argv ) {
	try {
#if defined(_WIN32)
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
		nanogui::init();
		{
			nanogui::ref<SimulatorGUI> app = new SimulatorGUI();
			app->handleDebugChanged();
			app->drawAll();
			app->setVisible(true);
			if (argc == 2)
				app->startScript = argv[1];
			nanogui::mainloop(10);
		}

		nanogui::shutdown();
	}
	catch (const std::runtime_error &e) {
		std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
		MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
		std::cerr << error_msg << endl;
#endif
		return -1;
	}

	return 0;
}
