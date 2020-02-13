#pragma once

/*==================
DEFAULT VALUES
=====================*/
constexpr auto GRAPH_DEFAULT_HEIGHT = 0.47f;	// The relative height of the graph
constexpr auto DISPLAY_TIME_DEFAULT = 30.f;	// Span of the time axis
constexpr auto CURVE_FILL_DEFAULT = false;	// don't fill by default
constexpr auto ROD_REACTIVITY_PLOT_ENABLED_DEFAULT = true;	// show gray graph
constexpr auto LOG_SCALE_DEFAULT = false;	// linear scale
constexpr auto TEMPERATURE_GRAPH_FROM_DEFAULT = 0.f;
constexpr auto TEMPERATURE_GRAPH_TO_DEFAULT = 400.f;
constexpr auto REACTIVITY_HARDCORE_DEFAULT = false;

// Neutron source
constexpr auto NEUTRON_SOURCE_INSERTED_DEFAULT = true;
// Negative effects on reactivity
constexpr auto TEMPERATURE_EFFECTS_ENABLED_DEFAULT = true;
constexpr auto FISSION_POISONS_ENABLED_DEFAULT = false;
// Rod number
constexpr auto NUMBER_OF_CONTROL_RODS = 3;
// Rod names
constexpr auto SAFETY_ROD_NAME_DEFAULT = "Safety";
constexpr auto REGULATORY_ROD_NAME_DEFAULT = "Regulating";
constexpr auto SHIM_ROD_NAME_DEFAULT = "Shim";
// Approximate values based on calibration curves
// available in the operating room in printed form.
// Rod worth (pcm)
constexpr auto SAFETY_ROD_WORTH_DEFAULT = 4000;
constexpr auto REGULATORY_ROD_WORTH_DEFAULT = 4000;
constexpr auto SHIM_ROD_WORTH_DEFAULT = 4000;
// Rod curves in class
// Rod steps
constexpr auto SAFETY_ROD_STEPS_DEFAULT = 1000;
constexpr auto REGULATORY_ROD_STEPS_DEFAULT = 1000;
constexpr auto SHIM_ROD_STEPS_DEFAULT = 1000;
// Rod speeds (steps/second), similar to measured values with a stopwatch at IJS Triga
constexpr auto SAFETY_ROD_SPEED_DEFAULT = 15;
constexpr auto REGULATORY_ROD_SPEED_DEFAULT = 7;
constexpr auto SHIM_ROD_SPEED_DEFAULT = 20;

// Operational limits
constexpr auto PERIOD_SCRAM_DEFAULT = 6;		// seconds
constexpr auto POWER_SCRAM_DEFAULT = 250000;	// watts
constexpr auto FUEL_TEMPERATURE_SCRAM_DEFAULT = 300;		// celsius
constexpr auto WATER_TEMPERATURE_SCRAM_DEFAULT = 80;		// celsius
constexpr auto WATER_LEVEL_SCRAM_DEFAULT = 0.2f;	// meters
constexpr auto ALL_RODS_AT_ONCE_DEFAULT = false;
constexpr auto AUTOMATIC_PULSE_SCRAM_DEFAULT = true;

// Core excess reactivity (pcm with all rods out)
constexpr auto CORE_EXCESS_REACTIVITY = 3000.;
// Core volume (m^3)
// Calculated based on data available on SOURCE
constexpr auto CORE_VOLUME_DEFAULT = 0.02543;
// Reactor vessel radius (m)
constexpr auto VESSEL_RADIUS_DEFAULT = 1.7f;
// Default neutron source activity was chosen
// arbitrarily
// Neutron source activity (beq)
constexpr auto NEUTRON_SOURCE_ACTIVITY_DEFAULT = 1e5;
constexpr auto NEUTRON_SOURCE_MODULATION_INTENSITY_DEFAULT = 5e4;
constexpr auto NEUTRON_SOURCE_MODE_DEFAULT = 0;
// Prompt neutron lifetime (s)
// from Pulstri.pdf
constexpr auto PROMPT_NEUTRON_LIFETIME_DEFAULT = 39e-6;
// Reactor primary water volume (in cubic meters)
constexpr auto WATER_VOLUME_DEFAULT = 20.;
// Primary water(and core) start temperature
constexpr auto WATER_TEMPERATURE_DEFAULT = 22.f;
// Environment temperature
constexpr auto ENVIRONMENT_TEMPERATURE_DEFAULT = 22.;

// Cooling
constexpr auto WATER_COOLING_ENABLED_DEFAULT = false;
constexpr auto WATER_COOLING_POWER_DEFAULT = 2.4e5;

/*
Default values estimated from the graph in TRIGLAV documentation
*/
// Temp. reactivity coeff.
constexpr auto ALPHA_AT_0_DEFAULT = 6.f;
constexpr auto ALPHA_T1_DEFAULT = 240.f;
constexpr auto ALPHA_AT_T1_DEFAULT = 9.f;
constexpr auto ALPHA_K_DEFAULT = - 0.004;

// Delete old data (seconds)
constexpr auto DELETE_OLD_DATA_TIME_DEFAULT = 3600.;
constexpr auto POISON_DATA_DEL_DIVISION = 5000;

// Automatic mode
constexpr auto KEEP_CURRENT_POWER_DEFAULT = true;
constexpr auto KEEP_STEADY_POWER_DEFAULT = 1e5;		// watts
constexpr auto AVOID_PERIOD_SCRAM_DEFAULT = true;
constexpr auto DEVIATION_MARGIN_DEFAULT = 0.02f;	// raw fraction [2%]

// Simulation modes
// Simulation modes period (second)
constexpr auto SIMULATION_MODE_PERIOD_DEFAULT = 5.f;
// Simulation modes amplitude (steps)
constexpr auto SIMULATION_MODE_AMPLITUDE_DEFAULT = 40.f;
// Square wave
constexpr auto SQUARE_WAVE_START_UP_DEFAULT	= 0.f;
constexpr auto SQUARE_WAVE_END_UP_DEFAULT = 0.5f;
constexpr auto SQUARE_WAVE_START_DOWN_DEFAULT = 0.5f;
constexpr auto SQUARE_WAVE_END_DOWN_DEFAULT = 1.f;
// Sine
constexpr auto SINE_MODE_DEFAULT = 0;
// Saw tooth
constexpr auto SAW_TOOTH_UP_START_DEFAULT = 0.f;
constexpr auto SAW_TOOTH_UP_PEAK_DEFAULT = 0.25f;
constexpr auto SAW_TOOTH_UP_END_DEFAULT = 0.5f;
constexpr auto SAW_TOOTH_DOWN_START_DEFAULT = 0.5f;
constexpr auto SAW_TOOTH_DOWN_PEAK_DEFAULT = 0.75f;
constexpr auto SAW_TOOTH_DOWN_END_DEFAULT = 1.f;

// Pulse
constexpr auto PULSE_START_DEFAULT = 0.;
constexpr auto PULSE_END_AFTER_DEFAULT = 0.4;

constexpr auto DEFAULT_DATA_DIVISION = 100;

// IMPORTANT
const auto SETTINGS_NUMBER = 94;
const auto SETTINGS_VERSION = 1.1f;

class Settings {
private:
	const float safetyRodCurveDefault[2] = { 0.f, 1.f };
	const float regulatoryRodCurveDefault[2] = { 0.f, 1.f };
	const float shimRodCurveDefault[2] = { 0.f, 1.f };

	const double delayedGroupBetasDefault[6] = { 0.23097e-3, 1.53278e-3, 1.3718e-3, 2.76451e-3, 0.80489e-3, 0.29396e-3 };
	const double delayedGroupLambdasDefault[6] = { 0.0124, 0.0305, 0.1115, 0.301, 1.138, 3.01 };
	const bool delayedGroupsEnabledDefault[6] = { true, true, true, true, true, true };

public:

	struct ControlRodSettings {
	public:
		size_t rodSteps = 0;
		float rodWorth = 0.f;
		float rodSpeed = 0.f;
		float rodCurve[2] = {0.f, 1.f};

		ControlRodSettings(size_t steps_, float rodWorth_, float rodSpeed_, float rodCurve1, float rodCurve2) {
			rodSteps = steps_;
			rodWorth = rodWorth_;
			rodSpeed = rodSpeed_;
			rodCurve[0] = rodCurve1;
			rodCurve[1] = rodCurve2;
		}
		ControlRodSettings() {}
	};

	struct SimulationSettings {
	public:
		float period;
		float amplitude;

		SimulationSettings(float per, float ampl) { period = per; amplitude = ampl; }
	};
	struct SquareWaveSettings : SimulationSettings {
		float xIndex[4] = { SQUARE_WAVE_START_UP_DEFAULT, SQUARE_WAVE_END_UP_DEFAULT, SQUARE_WAVE_START_DOWN_DEFAULT, SQUARE_WAVE_END_DOWN_DEFAULT };
		SquareWaveSettings(float per, float ampl) : SimulationSettings(per, ampl) {}
	};
	struct SineSettings : SimulationSettings {
		enum SineMode {
			Normal,
			Quadratic,
		};
		SineMode mode = (SineMode)SINE_MODE_DEFAULT;
		SineSettings(float per, float ampl) : SimulationSettings(per, ampl) {}
	};
	struct SawToothSettings : SimulationSettings {
		float xIndex[6] = { SAW_TOOTH_UP_START_DEFAULT, SAW_TOOTH_UP_PEAK_DEFAULT, SAW_TOOTH_UP_END_DEFAULT,
			SAW_TOOTH_DOWN_START_DEFAULT, SAW_TOOTH_DOWN_PEAK_DEFAULT, SAW_TOOTH_DOWN_END_DEFAULT };
		SawToothSettings(float per, float ampl) : SimulationSettings(per, ampl) {}
	};

	bool waterCooling = WATER_COOLING_ENABLED_DEFAULT;				// 19
	bool neutronSourceInserted = NEUTRON_SOURCE_INSERTED_DEFAULT;	// 20
	float graphSize = GRAPH_DEFAULT_HEIGHT;							// 21
	float displayTime = DISPLAY_TIME_DEFAULT;						// 22
	//float reactivityGraphLimits[2] = {CORE_EXCESS_REACTIVITY - SHIM_ROD_WORTH_DEFAULT - REGULATORY_ROD_WORTH_DEFAULT - SAFETY_ROD_WORTH_DEFAULT, CORE_EXCESS_REACTIVITY};	// 23, 24
	float reactivityGraphLimits[2] = {-1000, 1000 };	// 23, 24
	float temperatureGraphLimits[2] = {TEMPERATURE_GRAPH_FROM_DEFAULT, TEMPERATURE_GRAPH_TO_DEFAULT};	// 25, 26
	bool curveFill = CURVE_FILL_DEFAULT;							// 27
	bool rodReactivityPlot = ROD_REACTIVITY_PLOT_ENABLED_DEFAULT;	// 28

	double betas[6];												// 29 - 34
	double lambdas[6];												// 35 - 40
	bool groupsEnabled[6];											// 41 - 46
	double coreVolume = CORE_VOLUME_DEFAULT;						// 47
	double waterVolume = WATER_VOLUME_DEFAULT;						// 48
	double waterCoolingPower = WATER_COOLING_POWER_DEFAULT;			// 49
	double neutronSourceActivity = NEUTRON_SOURCE_ACTIVITY_DEFAULT;	// 50
	double promptNeutronLifetime = PROMPT_NEUTRON_LIFETIME_DEFAULT;	// 51
	bool temperatureEffects = TEMPERATURE_EFFECTS_ENABLED_DEFAULT;	// 52
	bool fissionPoisons = FISSION_POISONS_ENABLED_DEFAULT;			// 53
	float excessReactivity = CORE_EXCESS_REACTIVITY;				// 54
	float vesselRadius = VESSEL_RADIUS_DEFAULT;						// 55

	ControlRodSettings rodSettings[NUMBER_OF_CONTROL_RODS];			// 10 + i
	SquareWaveSettings squareWave = SquareWaveSettings(SIMULATION_MODE_PERIOD_DEFAULT, SIMULATION_MODE_AMPLITUDE_DEFAULT);			// 0
	SineSettings sineMode = SineSettings(SIMULATION_MODE_PERIOD_DEFAULT, SIMULATION_MODE_AMPLITUDE_DEFAULT);							// 1
	SawToothSettings sawToothMode = SawToothSettings(SIMULATION_MODE_PERIOD_DEFAULT, SIMULATION_MODE_AMPLITUDE_DEFAULT);				// 2
	SquareWaveSettings ns_squareWave = SquareWaveSettings(SIMULATION_MODE_PERIOD_DEFAULT, NEUTRON_SOURCE_MODULATION_INTENSITY_DEFAULT);		// 3
	SineSettings ns_sineMode = SineSettings(SIMULATION_MODE_PERIOD_DEFAULT, NEUTRON_SOURCE_MODULATION_INTENSITY_DEFAULT);						// 4
	SawToothSettings ns_sawToothMode = SawToothSettings(SIMULATION_MODE_PERIOD_DEFAULT, NEUTRON_SOURCE_MODULATION_INTENSITY_DEFAULT);			// 5
	char ns_mode = NEUTRON_SOURCE_MODE_DEFAULT;						// 6

	bool steadyCurrentPower = KEEP_CURRENT_POWER_DEFAULT;			// 70
	bool avoidPeriodScram = AVOID_PERIOD_SCRAM_DEFAULT;				// 71
	double steadyGoalPower = KEEP_STEADY_POWER_DEFAULT;				// 72
	float steadyMargin = DEVIATION_MARGIN_DEFAULT;					// 73

	double periodLimit = PERIOD_SCRAM_DEFAULT;						// 74
	bool periodScram = true;										// 75
	double powerLimit = POWER_SCRAM_DEFAULT;						// 76
	bool powerScram = true;											// 77
	float tempLimit = FUEL_TEMPERATURE_SCRAM_DEFAULT;				// 78
	bool tempScram = true;											// 79
	float waterTempLimit = WATER_TEMPERATURE_SCRAM_DEFAULT;			// 80
	bool waterTempScram = true;										// 81
	bool allRodsAtOnce = ALL_RODS_AT_ONCE_DEFAULT;					// 82

	// future use
	float waterLevelLimit = WATER_LEVEL_SCRAM_DEFAULT;				// 83
	bool waterLevelScram = false;									// 84

	double pulseLimits[2] = {PULSE_START_DEFAULT, PULSE_END_AFTER_DEFAULT};	// 85, 86

	float alpha0 = ALPHA_AT_0_DEFAULT;								// 87
	float alphaAtT1 = ALPHA_AT_T1_DEFAULT;							// 88
	float alphaT1 = ALPHA_T1_DEFAULT;								// 89
	double alphaK = ALPHA_K_DEFAULT;								// 90

	bool yAxisLog = LOG_SCALE_DEFAULT;								// 91

	bool automaticPulseScram = AUTOMATIC_PULSE_SCRAM_DEFAULT;		// 92

	bool reactivityHardcore = REACTIVITY_HARDCORE_DEFAULT;			// 93

	bool squareWaveUsesRodSpeed = false;							// 94


	// DO NOT ADD SETTINGS UNDER THIS LINE
	
	Settings() {
		rodSettings[0] = ControlRodSettings(SAFETY_ROD_STEPS_DEFAULT, SAFETY_ROD_WORTH_DEFAULT, SAFETY_ROD_SPEED_DEFAULT, safetyRodCurveDefault[0], safetyRodCurveDefault[1]);
		rodSettings[1] = ControlRodSettings(REGULATORY_ROD_STEPS_DEFAULT, REGULATORY_ROD_WORTH_DEFAULT, REGULATORY_ROD_SPEED_DEFAULT, regulatoryRodCurveDefault[0], regulatoryRodCurveDefault[1]);
		rodSettings[2] = ControlRodSettings(SHIM_ROD_STEPS_DEFAULT, SHIM_ROD_WORTH_DEFAULT, SHIM_ROD_SPEED_DEFAULT, shimRodCurveDefault[0], shimRodCurveDefault[1]);
		memcpy(betas, delayedGroupBetasDefault, 6 * sizeof(double));
		memcpy(lambdas, delayedGroupLambdasDefault, 6 * sizeof(double));
		memcpy(groupsEnabled, delayedGroupsEnabledDefault, 6 * sizeof(bool));
	};

	const void loadSettingsFromData(char* source, const size_t numEntries) {
		size_t shift = 0;
		for (size_t i = 0; i < numEntries; i++) {
			int id;
			std::memcpy(&id, source + shift, sizeof(int));
			shift += sizeof(int);

			size_t dataSize = getPropertySize(id);
			char* buffer = new char[dataSize]();
			std::memcpy(buffer, source + shift, dataSize);

			updateSetting(id, buffer, dataSize);
			delete[] buffer;

			shift += dataSize;
		}
	}

	const void updateSetting(int id, char* data, size_t dataSize) {
		void* ptr = getSettingPointer(id);
		if (ptr) memcpy(ptr, data, dataSize);
	}

	std::pair<char*, size_t> saveSettings(size_t* entries) {
		size_t size = 0;
		const size_t intSize = sizeof(int);
		for (int i = 0; i <= SETTINGS_NUMBER; i++) {
			size += getPropertySize(i) + intSize;

			if (i == 2) i = 9;
			if (i == 9 + NUMBER_OF_CONTROL_RODS) i = 18;
			if (i == 54) i = 69;
		}

		std::pair<char*, size_t> ret = std::pair<char*, size_t>();
		ret.first = new char[size]();
		ret.second = size;

		size_t shift = 0;
		size_t numEntries = 0;
		for (int i = 0; i <= SETTINGS_NUMBER; i++) {
			size_t pSize = getPropertySize(i);
			memcpy(ret.first + shift, &i, intSize);
			memcpy(ret.first + shift + intSize, getSettingPointer(i), pSize);
			shift += intSize + pSize;
			numEntries++;
			if (i == 2) i = 9;
			if (i == 9 + NUMBER_OF_CONTROL_RODS) i = 18;
			if (i == 54) i = 69;
		}

		*entries = numEntries;
		return ret;
	}

private:

	void* getSettingPointer(int id) {
		void* ptr = nullptr;
		if (id >= 10 && id <= 18) {
			int n = id - 10;
			if (n < NUMBER_OF_CONTROL_RODS) { ptr = &rodSettings[n]; }
		}
		else if (id <= 5) {
			switch (id) {
			case 0: ptr = &squareWave;
				break;
			case 1: ptr = &sineMode;
				break;
			case 2: ptr = &sawToothMode;
				break;
			case 3: ptr = &ns_squareWave;
				break;
			case 4: ptr = &ns_sineMode;
				break;
			case 5: ptr = &ns_sawToothMode;
				break;
			default: break;
			}
		}
		else if (id >= 29 && id <= 46) {
			int n = (int)floor((id - 29) / 6.);
			switch (n) {
			case 0:
				ptr = &betas[id - 29]; break;
			case 1:
				ptr = &lambdas[id - 35]; break;
			case 2:
				ptr = &groupsEnabled[id - 41]; break;
			default: break;
			}
		}
		else {
			switch (id) {
			case 19: ptr = &waterCooling; break;
			case 20: ptr = &neutronSourceInserted; break;
			case 21: ptr = &graphSize; break;
			case 22: ptr = &displayTime; break;
			case 23: ptr = &reactivityGraphLimits[0]; break;
			case 24: ptr = &reactivityGraphLimits[1]; break;
			case 25: ptr = &temperatureGraphLimits[0]; break;
			case 26: ptr = &temperatureGraphLimits[1]; break;
			case 27: ptr = &curveFill; break;
			case 28: ptr = &rodReactivityPlot; break;
			case 47: ptr = &coreVolume; break;
			case 48: ptr = &waterVolume; break;
			case 49: ptr = &waterCoolingPower; break;
			case 50: ptr = &neutronSourceActivity; break;
			case 51: ptr = &promptNeutronLifetime; break;
			case 52: ptr = &temperatureEffects; break;
			case 53: ptr = &fissionPoisons; break;
			case 54: ptr = &excessReactivity; break;
			case 55: ptr = &vesselRadius; break;
			case 70: ptr = &steadyCurrentPower; break;
			case 71: ptr = &avoidPeriodScram; break;
			case 72: ptr = &steadyGoalPower; break;
			case 73: ptr = &steadyMargin; break;
			case 74: ptr = &periodLimit; break;
			case 75: ptr = &periodScram; break;
			case 76: ptr = &powerLimit; break;
			case 77: ptr = &powerScram; break;
			case 78: ptr = &tempLimit; break;
			case 79: ptr = &tempScram; break;
			case 80: ptr = &waterTempLimit; break;
			case 81: ptr = &waterTempScram; break;
			case 82: ptr = &allRodsAtOnce; break;
			case 83: ptr = &waterLevelLimit; break;
			case 84: ptr = &waterLevelScram; break;
			case 85: ptr = &pulseLimits[0]; break;
			case 86: ptr = &pulseLimits[1]; break;
			case 87: ptr = &alpha0; break;
			case 88: ptr = &alphaAtT1; break;
			case 89: ptr = &alphaT1; break;
			case 90: ptr = &alphaK; break;
			case 91: ptr = &yAxisLog; break;
			case 92: ptr = &automaticPulseScram; break;
			case 93: ptr = &reactivityHardcore; break;
			case 94: ptr = &squareWaveUsesRodSpeed; break;
			default: break;
			}
		}

		return ptr;
	}

	const static size_t getPropertySize(int id) {
		if (id == 0 || id == 3) return sizeof(SquareWaveSettings);
		if (id == 1 || id == 4) return sizeof(SineSettings);
		if (id == 2 || id == 5) return sizeof(SawToothSettings);

		if (id == 6) return sizeof(char);

		if (id == 19 || id == 20 || id == 27 || id == 28) return sizeof(bool);
		if (id >= 29 && id <= 40) return sizeof(double);
		if (id >= 41 && id <= 46) return sizeof(bool);
		if (id >= 47 && id <= 51) return sizeof(double);
		if (id == 52 || id == 53 || id == 70 || id == 71 || id == 75) return sizeof(bool);
		if (id >= 10 && id <= 18) return sizeof(ControlRodSettings);
		if (id == 72 || id == 74 || id == 76) return sizeof(double);
		if (id == 77 || id == 79 || (id >=91 && id <= 94)) return sizeof(bool);
		if (id == 81 || id == 82 || id == 84) return sizeof(bool);
		if (id == 85 || id == 86 || id == 90) return sizeof(double);

		if (id < SETTINGS_NUMBER) return sizeof(float);

		return 0;
	}

};
