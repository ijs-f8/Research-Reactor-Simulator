#pragma once

#include <memory>
#include <cmath>
#include <tuple>
#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include <functional>
#include <ControlRod.h>
#include <nanogui/DataDisplay.h>
#include <Settings.h>
#include <ScriptCommand.h>

// Delta time
constexpr auto DT_STEP = 0.001;

constexpr auto AVOGADRO_NUM = 6.0221409e+23;
constexpr auto XENON_MOLAR_MASS = 134.907;
constexpr auto IODINE_MOLAR_MASS = 135.;

// Source? not used at the moment
constexpr auto WATER_ABSORBTION = -0.3635;		  // fluks(h) = phi0 * e^(this * h) [cm^-1]
// Measured at IJS triga in 2016
constexpr auto WATER_HEIGHT = 5.;			  // height of water level above (point)reactor [m]
constexpr auto TANK_RADIUS = 1.;			  // radius of the reactor tank [m]

using namespace std;

class Simulator
{
private:
	// Thermal neutron speed at 0.025 eV, Duderstradt Hamilton page 383
	const double t_neutron_speed = 2200.; //m/s
	// Energy released by signle fission: 200 MeV, Duderstradt Hamilton page 10
	// 1 MeV: 1.6e-19 W s
	const double fissions_per_wattSecond = 3.12E10;

	// The fraction of delayed neutrons.
	double beta_ = 0.;

	bool temperature_effects = true;
	bool fissionPoisoning_effects = false;
	bool debug_mode = false;

	bool power_scram_enabled = true;
	bool fuel_temp_scram_enabled = true;
	bool water_temp_scram_enabled = true;
	bool period_scram_enabled = true;
	bool water_level_scram_enabled = true;

	void recalculatePoisonConcentrations(double dt);
	double Xe_conc = 0;
	double I_conc = 0;

	double startTime = -1.;
	double actualTime = 0;
	double simulatorTime = 0;
	double lastTime = 0;

	// Microscopic cross section for absorbtion
	// Knief, Nuclear engineering, page 173
	const double sigma_Xe_a = 2.6e-22; // m^2
	// macroscopic cross section for fission - recalculate
	double Sigma_f = 0.0056 * 100; // m^-1

	// Cp taken from simnad1981, where Cp(T) = (2.04 + 4.17e-3 T) J/(K cm^3)
	double cp_const_a = 2.04;
	double cp_const_b = 4.17e-3;

	// Termična vrednost MT456, ENDF VIII
	const double nu_bar = 2.43;

	//Unused, uncomment and use if you want to calculate thermal cross sections
	// Total microscopic cross section U235 at 0.0253eV, ENDFVIII in barns
	//double sigma_U235_total = 700.185;
	// Fission microscopic cross section U235 at 0.0253eV, ENDFVIII in barns
	double sigma_U235_fission = 586.691;
	// Capture (102) microscopic cross section U235 at 0.0253eV, ENDFVIII in barns
	double sigma_U235_capture = 99.3843;

	double fuel_density = 6; // g/cm3, from http://www.rcp.ijs.si/ric/description-s.html
	// g/cm3, from http://www.rcp.ijs.si/ric/description-s.html
	// 20% enrichment, 12% uranium content
	double u235_fuel_density = fuel_density * 1e6 * 0.2 * 0.12;
	double u235_atomic_dens = AVOGADRO_NUM * u235_fuel_density / 235.;

	//double Sigma_f = u235_atomic_dens * sigma_U235_fission;
	// From wims in 1 group homogenization, core 
	//double Sigma_f = 0.01926 * 100;
	/*
	Fission product yields and decay constants are for U235,
	taken from page 570 in Duderstadt-Hamilton - Nuclear
	Reactor Analysis
	*/
	const double gamma_I = 0.064;
	const double gamma_X = 0.0023;
	const double lambda_I = 0.1035 / 3600.; // per hour -> per second
	const double lambda_X = 0.0753 / 3600.; // per hour -> per second


	//Con be counted by looking at them
	const int no_fuel_elements = 59;

	double groupStability[6];
	double totalDelayed = 0;

	// Main arrays length
	size_t dataPoints = 0;

	double lambda_eff = 0;

	// IAEA, THERMOPHYSICAL PROPERTIES OF MATERIALS FOR NUCLEAR ENGINEERING, 
	// A TUTORIAL AND COLLECTION OF DATA, 2008
	const double water_specific_heat = 4185.5; // J/kgK

	double waterLevel_delta = 0.;
	double reactor_vessel_radius = VESSEL_RADIUS_DEFAULT;
	//  https://en.wikipedia.org/wiki/Enthalpy_of_vaporization
	const double water_vaporization_ethalpy = 2.257 * 1e6; // J/kg

	bool keepCurrentPower = KEEP_CURRENT_POWER_DEFAULT;
	double keepSteadyPowerAt = KEEP_STEADY_POWER_DEFAULT;
	bool avoidPeriodScram = AVOID_PERIOD_SCRAM_DEFAULT;
	double steadyDeviation = DEVIATION_MARGIN_DEFAULT;

	// alpha(T), default values taken from TRIGLAV documentation
	double alpha0 = ALPHA_AT_0_DEFAULT;
	double alphaAtT1 = ALPHA_AT_T1_DEFAULT;
	double alphaT1 = ALPHA_T1_DEFAULT;
	double alphaK = ALPHA_K_DEFAULT;

	// The coefficient was determined from experimental data
	// From the "new power method", we multiply the k*S/C =0.022 kWh/K by C=19.6 kWh/K
	//and use our own heat capacity later
	const double cooling_coefficient = 0.022 * 19.6 / (60. * 60.);


	// From TRIGLAV documentation
	const double tempModelCoeff[3] = { 67.18e-03, -8.381e-06, 0.3843e-09 };

public:

	// Classes for use in Simulator.cpp
	struct PowerExtreme {
		// The new order this extreme got to
		int order = 0;
		// The index at which this happened
		double when = 0.;
		// Value to hold if the order is negative infinity
		bool isZero = false;

		PowerExtreme(int order, double time) {
			this->order = order;
			this->when = time;
		}
		PowerExtreme() {
			isZero = true;
		}
	};

	struct PulseData {
		double peakPower = 0.;
		double FWHM = 0.;
		double releasedEnergy = 0.;
		double maxFuelTemp = 0.f;
		double powerBeforeSCRAM = 0.;
		size_t pulseStartIndex = 0;
		double timeAtMax = 0.;
	};

	// In case of SCRAM, this enum tells you the reason for SCRAM
	enum ScramSignals : std::uint8_t {
		None = 0,
		Period = 1,
		FuelTemperature = 2,
		WaterTemperature = 4,
		Power = 8,
		WaterLevel = 16,
		User = 32
	};

	enum TemperatureMode : std::uint8_t {
		Asymptotic = 0,
		FH = 1
	};

	void dataToFile(std::string fileName);

	void rodsToFile(std::string fileName);

	void setDemoMode();
	void setHighPowerDemoMode();

	void setScramEnabled(ScramSignals scramType, bool value);
	bool getScramEnabled(ScramSignals scramType);

	// Pleaceholder to calculate delayed group fractions outside the GUI app in the future
	//void calculateDelayedFractions();
	//double delayed_fractions[6] = { 0 };

	// Resets all variables to their defaults
	void setAllToDefaults();

	// Reset the entire simulation
	void reset(Settings* properties);

	// Toggles debug mode
	void setDebugMode(bool& value) {
		debug_mode = value;
	}

	// Toggles the source (inserted/withdrawn)
	const bool& getNeutronSourceInserted() const;
	void setNeutronSourceInserted(const bool& value);
	bool source_inserted = NEUTRON_SOURCE_INSERTED_DEFAULT;
	int data_division = DEFAULT_DATA_DIVISION;
	// Toggles the temperature effects (enabled/disabled)
	const bool& getTemperatureEffectsEnabled() const;
	void setTemperatureEffectsEnabled(const bool& value);
	const bool& getFissionPoisoningEffectsEnabled() { return fissionPoisoning_effects; }
	void setFissionPoisoningEffectsEnabled(const bool& value);

	const size_t getCurrentIndex() const {
		if (iterations_total > 0) {
			return (iterations_total - 1) % dataPoints;
		}
		else {
			return iterations_total;
		}
	}
	const size_t getNextIndex() const {
		return iterations_total % dataPoints;
	}
	const size_t shiftIndex(size_t index, long shift) const {
		if (shift + (long)index >= (long)dataPoints) {
			return (shift + index) % dataPoints;
		}
		else if (shift + (long)index < 0L) {
			return dataPoints + shift + index;
		}
		else {
			return shift + index;
		}
	}

	const size_t getOldestIndex() const {
		return (iterations_total > dataPoints) ? getNextIndex() : (size_t)0;
	}

	const size_t translateIndex(size_t iteration) const {
		return shiftIndex(getNextIndex(), (int)iteration % dataPoints);
	}

	const size_t getIteration(size_t index) const {
		if (index >= getOldestIndex()) {
			return iterations_total - dataPoints + index - getOldestIndex();
		}
		else {
			return iterations_total - getOldestIndex() + index;
		}
	}

	/* Gets or sets the macroscopic cross section cross section(in 1/m) for a fission reaction to occour.
	Default is 2,81111/m.*/
	const double& getMaxFissionCrossSection() const;
	void setMaxFissionCrossSection(const double& value);

	/* Gets or sets the volume of the reactor core(in cm?).
	Default is a clynder with a radius of 21,9cm and a height of 58cm.*/
	const double& getReactorCoreVolume() const;
	void setReactorCoreVolume(const double& value);
	double core_volume = CORE_VOLUME_DEFAULT;

	/* Gets or sets the fraction of total neutrons that are part of the delayed groups(6 groups).
	Defaults are: 2.09999998E-04, 1.30600005E-03, 1.18999998E-03,
	2.57600006E-03, 9.48000001E-04, 2.32000006E-04.*/
	const double* getDelayedGroupFractions() const;
	double getBetaEffective() { return beta_; }
	void setDelayedGroupFraction(size_t group, double value);
	double beta_neutrons[6] = { 0 };


	// Returns flux in m^-1
	double getCurrentFlux() { 
		return state_vector_[0][getCurrentIndex()] * t_neutron_speed / (getReactorCoreVolume()); 
	}

	/* Gets or sets the decay rates of delayed neutrons groups(6 groups).
	Defaults are: 1.27999997E-02, 3.17000002E-02, 0.120499998,
	0.321700007, 1.40300000, 3.89249992.*/
	const double* getDelayedGroupDecays() const;
	void setDelayedGroupDecay(size_t group, double value);
	double delayed_decay_time[6] = { 0 };

	/* Toggles computing of individual delayed groups.
	Defaults is true for all.*/
	const bool* getDelayedGroupEnabled() const;
	void setDelayedGroupEnabled(size_t group, bool value);
	bool delayed_enabled[6] = { 0 };

	/* Gets or sets the prompt neutron lifetime.
	Default is 4E-05. SOURCE?*/
	const double& getPromptNeutronLifetime() const;
	void setPromptNeutronLifetime(const double& value);
	double prompt_lifetime = PROMPT_NEUTRON_LIFETIME_DEFAULT;

	/* Gets or sets the reactivity of the reactor core with no control rods inserted.
	Default is 2000 pcm.*/
	const double& getExcessReactivity() const;
	void setExcessReactivity(const double& value);
	double core_excess_reactivity = CORE_EXCESS_REACTIVITY;

	/* Gets or sets the amplitude of the neutron source simulation modes(in neutrons per second).
	Default is 10^5 neutrons per second.*/
	const double& getNeutronSourceModulation() const;
	void setNeutronSourceModulation(const double& value);
	double ns_modulation = NEUTRON_SOURCE_MODULATION_INTENSITY_DEFAULT;

	/* Gets or sets the activity[amplitude] of the source(in neutrons per second).
	Default is 10^5 neutrons per second.*/
	const double& getNeutronSourceActivity() const;
	void setNeutronSourceActivity(const double& value);
	double ns_base_activity = NEUTRON_SOURCE_ACTIVITY_DEFAULT;

	/* Gets or sets the mode of modulation of the neutron source.
	Default is None (const value).*/
	const SimulationModes& getNeutronSourceMode() const;
	void setNeutronSourceMode(const SimulationModes & value);
	SimulationModes source_mode = SimulationModes::None;

	// Get periodical mode pointer for neutron source
	PeriodicalMode* getSourceModeClass(SimulationModes sim) {
		switch (sim) {
		case SimulationModes::SquareWaveMode:
			return source_sqw;
			break;
		case SimulationModes::SineMode:
			return source_sinMode;
			break;
		case SimulationModes::SawToothMode:
			return source_saw;
			break;
		default:
			return source_none;
		}
	}
	SquareWave* source_sqw;
	Sine* source_sinMode;
	SawTooth* source_saw;
	PeriodicalMode* source_none;

	double getCurrentSourceActivity();

	/* Gets or sets the time in seconds after which values older than x seconds will be deleted.
	Setting this to 0 disables the deleting of old data values.
	Default is 0 seconds.*/
	const double& getDeleteOldValues() const;
	void setDeleteOldValues(const double& value);
	double delete_old_data_time = DELETE_OLD_DATA_TIME_DEFAULT;

	// Returns the number of data samples in the last <see cref="LoopFinished"/> event.
	const size_t& getLatestSampleNumber() const;
	size_t last_sample_number = 0;

	// Get and Set HeatCapacityConstants
	std::pair <double, double> getHeatCpConstants();
	void setHeatCpConstants(std::pair<double, double> &values);

	//The 5 get and set functions for operational limits
	double &getPowerLimit();
	void setPowerLimit(double limit);

	double &getPeriodLimit();
	void setPeriodLimit(double limit);

	double &getFuelTemperatureLimit();
	void setFuelTemperatureLimit(double limit);

	double &getWaterTemperatureLimit();
	void setWaterTemperatureLimit(double limit);

	double &getWaterLevelLimit();
	void setWaterLevelLimit(double limit);

	bool godMode = false;

	// I think this is quite self explanatory
	void scram(ScramSignals reason);

	int getScramStatus() { return status; }

	// Control rods
	ControlRod* rods[NUMBER_OF_CONTROL_RODS];
	ControlRod* safetyRod() { return rods[0]; }
	ControlRod* regulatingRod() { return rods[1]; }
	ControlRod* shimRod() { return rods[2]; }

	// Returns a pointer to an array containing delayed neutron concentrations
	void getCurrentStateVector(double* result, bool copyLast = true) const;
	// Returns the entire array of a specific neutron group
	const double *getNeutronGroup(size_t i = 0);
	double* state_vector_[8];

	// Functions for returning poison concentrations
	double* getXenonConcentration() { return &Xe_conc; }
	double* getIodineConcentration() { return &I_conc; }
	float* xenon_;
	float* iodine_;

	// Returns the current reactivity(in pcm) in the reactor.
	double getCurrentReactivity() const;
	// Returns the entire data array for reactivity.
	const float *getReactivity() const;
	float* reactivity_;

	// Returns reactor period
	double *getReactorPeriod();

	// Returns reactor asymptotic
	double *getReactorAsymPeriod();

	// Returns the current reactivity(in pcm) in the reactor.
	float getCurrentRodReactivity() const;
	// Returns the entire data array for reactivity.
	const float *getRodReactivity() const;
	float* rodReactivity_;

	// Returns the current temperature(in kelvin) in the reactor.
	float getCurrentTemperature() const;
	// Returns the entire data array for temperature.
	const float *getTemperature() const;
	float* temperature_;

	// Water temperature(in celsius).
	double *getWaterTemperature();
	void waterHeatingCycle(double dt);
	double waterTemperature = WATER_TEMPERATURE_DEFAULT;

	// Water level difference from regular water level (in meters)
	double *getWaterLevel();

	// Water cooling
	bool *getWaterCooling() { return &w_cooling; }
	void setWaterCooling(bool value) { w_cooling = value; }
	bool w_cooling = WATER_COOLING_ENABLED_DEFAULT;

	// Cooling power
	double *getCoolingPower() { return &cooling_p; }
	void setCoolingPower(double value) { cooling_p = value; }
	double cooling_p = WATER_COOLING_POWER_DEFAULT;

	double &getWaterVolume() { return waterVolume; }
	void setWaterVolume(double value) { waterVolume = value; }
	double waterVolume =  WATER_VOLUME_DEFAULT;

	// Returns the dose rate above the reactor tank (in mSv/h)
	double *getDoseRate() { return &doseRate; }
	void recalculateDoseRate(double flux);

	// Returns the current power output of the reactor.
	const double getCurrentPower();

	// Return temperature dependent fuel heat capacity
	double getFuelCp(double T);

	// Reverse stable temperature polynomial
	double getCoolingFromTemperature(double T);

	// Returns the speed factor deque.
	void setSpeedFactor(double value);
	// Returns the entire data array for power.
	double &getSpeedFactor();
	double speedFactor = 0;;

	const size_t getIndexFromTime(double time) const;

	double getStableTemperature(double P);

	// Returns the current time(in seconds since start).
	double getCurrentTime() const;
	// Returns the entire data array for time.
	const double *getTime() const;
	double* time_;

	// Returns the number of calculations performed since the start of the simulation.
	const size_t &getCalculationsPerformed() const;
	size_t calc_performed = 0;

	// Returns the number of order changes since the start of the simulation.
	size_t getOrderChanges() const;

	// Returns the power extreme at the desired location
	PowerExtreme &getExtremeAt(size_t i);
	PowerExtreme trailingExtreme = PowerExtreme();

	// Returns a boolean value if the simulation is paused or not
	bool isPaused() const;

	// Returns the power at which the automatic mode should keep the reactor stable
	double &getAutomaticSteadyPower() { return keepSteadyPowerAt; }
	// Sets the stable power in automatic mode
	void setAutomaticSteadyPower(double power) { keepSteadyPowerAt = power; }

	// Returns true if automatic mode holds the current power level when enabled
	bool &getKeepCurrentPower() { return keepCurrentPower; }
	// If true, the automatic mode will keep the reactor steady at the current power
	void setKeepCurrentPower(bool value) { keepCurrentPower = value; }

	// Returns true if automatic mode avoids triggering a period SCRAM when moving the control rod
	bool &getAutomaticAvoidPeriodScram() { return avoidPeriodScram; }
	// If true, the automatic mode will avoid triggering a period SCRAM when moving the control rod
	void setAutomaticAvoidPeriodScram(bool value) { avoidPeriodScram = value; }

	// Returns the raw fraction (power-steadyPower)/steadyPower at which the control rod is automatically moved
	double getAutomaticDeviation() { return steadyDeviation; }
	// Sets the raw fraction (power-steadyPower)/steadyPower at which the control rod is automatically moved 
	void setAutomaticDeviation(float value) { steadyDeviation = value; }

	// Set the scram callback
	void setScramCallback(const std::function<void(int)> &callback);
	void setResetScramCallback(const std::function<void()> &callback);
	void setSevereErrorCallback(const std::function<void(int)> &callback);
	
	// Set the pulse callback
	void setPulseCallback(const std::function<void(PulseData)> &callback);

	// Sets or gets the automatic hold power
	double powerHold;
	double getPowerHold() { return powerHold; }
	void setPowerHold(double hold) { powerHold = hold; }

	// By default uses TRIGA's parameters
	Simulator(Settings* properties = nullptr);
	~Simulator();

	// Self-explanatory
	float getTotalRodReactivity();
	float getTotalRodWorth();

	double getAlpha0() { return alpha0; }
	void setAlpha0(double value) { alpha0 = value; }

	double getAlphaPeak() { return alphaAtT1; }
	void setAlphaPeak(double value) { alphaAtT1 = value; }

	double getAlphaTempPeak() { return alphaT1; }
	void setAlphaTempPeak(double value) { alphaT1 = value; }

	double getAlphaSlope() { return alphaK; }
	void setAlphaSlope(double value) { alphaK = value; }

	void runLoop();

	/*
	Should recieve a pointer to a double array of size 7
	Keep in mind this does not push values to any other deques than the state vector
	*/
	void pushNewState(double* states, size_t index);

	double powerFromNeutrons(double n);

	double getReactivityCoefficient(double temp);

	void pushStableState(double power);

	const size_t getDataLength() const { return dataPoints; }

	void setProperties(Settings* nodes);

	void resetSimulator();

	void beginPulse();

	void setAutoScram(bool value) { autoScramAfterPulse = value; }

	void doScriptCommands();
	std::vector<Command> scriptCommands;

private:
	bool pulsing = false;
	double pulse_maxP = 0;
	double pulse_energy = 0;
	double pulse_FWHM = 0;
	float pulse_maxT = 0;
	size_t pulse_start = 0;
	double time_at_peak = 0;
	double pulse_startP = 0;
	bool autoScramAfterPulse = AUTOMATIC_PULSE_SCRAM_DEFAULT;
	
	double ns_activity_temp = NEUTRON_SOURCE_ACTIVITY_DEFAULT;
	double doseRate = 0;

	// Variables controlling execution of the script
	double scriptStart = 0.;
	
	double scriptTimer = 0;

	// Increment neutron source simtulation time
	void advanceSourceTime(double dt) { if(source_mode != SimulationModes::None) getSourceModeClass(source_mode)->handleAddTime((float)dt); };

	// Variables for neutron changes
	double temp_state[7] = { 0 }, delayed = 0, fissionRate = 0;

	// Per frame calculations
	void solvePerFrame();
	
	// Initialization method
	void init();

	// A function that checks if operational limits have been crossed
	void checkOperationalLimits();

	// The main calculation loop.
	void mainLoop(size_t iterations);

	// Recalculate effective beta and lambda after change of "groups enabled"
	void recalculateLambdaBetaEffective();

	// Method for derivatives
	void neutronChange(double* new_state, double* prev_state, double rho);
	static void multiplyArray(double* in_array, double scalar);
	static void arraySum(double* sum_array1, double* sum_array2, bool posOnly = false);
	size_t iterations_total = 0;
	size_t frames_total = 0;

	deque<PowerExtreme>* powerExtremes = nullptr;

	void addPowerExtremes();

	void checkPulsingStatus();

	double reactorPeriod = 0;
	double reactorAsymPeriod = 0;

	/*Operational limits, each has a get 
	and a set function, so they can be set from the GUI*/
	double periodLimit = PERIOD_SCRAM_DEFAULT;
	double powerLimit = POWER_SCRAM_DEFAULT;
	double fuelTemperatureLimit = FUEL_TEMPERATURE_SCRAM_DEFAULT;
	double waterTemperatureLimit = WATER_TEMPERATURE_SCRAM_DEFAULT;
	double waterLevelLimit = WATER_LEVEL_SCRAM_DEFAULT;

	/*The scram on period will happen only if the period 
	raises over the limit for more than a certain period, so 
	we need a way to know how long has it passed since the period 
	has risen too high*/
	double periodTimer = 0;

	// The status of the reactor
	int status = ScramSignals::None;
	int tempMode = TemperatureMode::Asymptotic;

	size_t resetAverage = 0;

	std::function<void(int)> scramCallback;
	std::function<void()> scramResetCallback;
	std::function<void(PulseData)> pulseCallback;
	std::function<void(int)> severeErrorCallback;

	static std::string formatTime(double t) {
		size_t time[4];
		time[3] = (size_t)floor(fmod(t, 1.) * 1000);
		time[2] = (size_t)floor(fmod(t, 60.));
		time[1] = (size_t)floor(fmod(t, 3600.) / 60.);
		time[0] = (size_t)floor(t / 3600.);
		std::string ret[4];
		for (int i = 0; i < 4; i++) {
			if (i == 3) {
				ret[i] = to_string(time[i]);
				int max = 3 - (int)ret[i].length();
				for (int m = 0; m < max; m++) ret[i] = "0" + ret[i];
			}
			else { ret[i] = ((time[i] < 10) ? ("0" + to_string(time[i])) : (to_string(time[i]))); }
		}
		return ret[0] + ":" + ret[1] + ":" + ret[2] + ":" + ret[3];
	}
};

