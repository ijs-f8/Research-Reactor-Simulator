#pragma once
#include <cereal/archives/json.hpp>
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

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(rodSteps, rodWorth, rodSpeed, rodCurve);
		}
	};

	struct SimulationSettings {
	public:
		float period;
		float amplitude;

		SimulationSettings(float per, float ampl) { period = per; amplitude = ampl; }

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(period, amplitude);
		}
	};

	struct SquareWaveSettings : SimulationSettings {
	public:
		float xIndex[4] = { SQUARE_WAVE_START_UP_DEFAULT, SQUARE_WAVE_END_UP_DEFAULT, SQUARE_WAVE_START_DOWN_DEFAULT, SQUARE_WAVE_END_DOWN_DEFAULT };
		SquareWaveSettings(float per, float ampl) : SimulationSettings(per, ampl) {}
		
		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::base_class<SimulationSettings>(this), xIndex);
		}
	};

	struct SineSettings : SimulationSettings {
	public:
		enum SineMode {
			Normal,
			Quadratic,
		};
		SineMode mode = (SineMode)SINE_MODE_DEFAULT;
		SineSettings(float per, float ampl) : SimulationSettings(per, ampl) {}

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::base_class<SimulationSettings>(this), mode);
		}
	};

	struct SawToothSettings : SimulationSettings {
	public:
		float xIndex[6] = { SAW_TOOTH_UP_START_DEFAULT, SAW_TOOTH_UP_PEAK_DEFAULT, SAW_TOOTH_UP_END_DEFAULT,
			SAW_TOOTH_DOWN_START_DEFAULT, SAW_TOOTH_DOWN_PEAK_DEFAULT, SAW_TOOTH_DOWN_END_DEFAULT };
		SawToothSettings(float per, float ampl) : SimulationSettings(per, ampl) {}

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::base_class<SimulationSettings>(this), xIndex);
		}
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

	void saveArchive(std::string fileName) {
		std::ofstream os(fileName);
		cereal::JSONOutputArchive archive(os);

		archive(rodSettings,
			squareWave, ns_squareWave,
			sawToothMode, ns_sawToothMode,
			sineMode, ns_sineMode,
			betas,
			lambdas,
			groupsEnabled,
			waterCooling,
			neutronSourceInserted,
			graphSize,
			displayTime,
			reactivityGraphLimits,
			temperatureGraphLimits,
			curveFill,
			rodReactivityPlot,
			coreVolume,
			waterVolume,
			waterCoolingPower,
			neutronSourceActivity,
			promptNeutronLifetime,
			temperatureEffects,
			fissionPoisons,
			excessReactivity,
			vesselRadius,
			steadyCurrentPower,
			avoidPeriodScram,
			steadyGoalPower,
			steadyMargin,
			periodLimit,
			periodScram,
			powerLimit,
			powerScram,
			tempLimit,
			tempScram,
			waterTempLimit,
			waterTempScram,
			allRodsAtOnce,
			waterLevelLimit,
			waterLevelScram,
			pulseLimits,
			alpha0,
			alphaAtT1,
			alphaT1,
			alphaK,
			yAxisLog,
			automaticPulseScram,
			reactivityHardcore,
			squareWaveUsesRodSpeed
		);

	}

	void restoreArchive(std::string fileName) {
		std::ifstream is(fileName);
		cereal::JSONInputArchive iarchive(is);

		iarchive(rodSettings,
			squareWave, ns_squareWave,
			sawToothMode, ns_sawToothMode,
			sineMode, ns_sineMode,
			betas,
			lambdas,
			groupsEnabled,
			waterCooling,
			neutronSourceInserted,
			graphSize,
			displayTime,
			reactivityGraphLimits,
			temperatureGraphLimits,
			curveFill,
			rodReactivityPlot,
			coreVolume,
			waterVolume,
			waterCoolingPower,
			neutronSourceActivity,
			promptNeutronLifetime,
			temperatureEffects,
			fissionPoisons,
			excessReactivity,
			vesselRadius,
			steadyCurrentPower,
			avoidPeriodScram,
			steadyGoalPower,
			steadyMargin,
			periodLimit,
			periodScram,
			powerLimit,
			powerScram,
			tempLimit,
			tempScram,
			waterTempLimit,
			waterTempScram,
			allRodsAtOnce,
			waterLevelLimit,
			waterLevelScram,
			pulseLimits,
			alpha0,
			alphaAtT1,
			alphaT1,
			alphaK,
			yAxisLog,
			automaticPulseScram,
			reactivityHardcore,
			squareWaveUsesRodSpeed
		);

	}
};
