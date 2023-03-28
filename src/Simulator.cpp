#include <Simulator.h>
#include <limits>
#include <cmath>
#include <ctime>
#include <iterator>
#include <iomanip>

void Simulator::dataToFile(std::string fileName)
{
	ofstream logFile;
	logFile.open(fileName + ".dat");
	logFile << "###############################################################################################################\n";
	time_t now = time(0);
	struct tm *p = localtime(&now);
	char s[100];
	strftime(s, 100, "%c", p);
	logFile << "#                  Research reactor simulator log " << s << "                   #\n";
	logFile << "#Time[h:m:s:ms] Reactivity[pcm] Inserted-reactivity[pcm] Power[W] Temp[C] Xenon-conc.[g/m3] Iodine-conc.[g/m3]#\n";
	logFile << "###############################################################################################################\n";
	size_t start = getOldestIndex();
	long len = (long)std::min(iterations_total, getDataLength());
	size_t idx, poisonIdx;
	for (long shift = 0L; shift < len/data_division; shift++) {
		idx = shiftIndex(start, shift * data_division);
		poisonIdx = idx / POISON_DATA_DEL_DIVISION;
		logFile << formatTime(time_[idx]) << "\t" << reactivity_[idx] << "\t" << rodReactivity_[idx] << "\t"
			<< powerFromNeutrons(state_vector_[0][idx]) << "\t" << temperature_[idx] << "\t" << xenon_[poisonIdx] << "\t" << iodine_[poisonIdx] << std::endl;
	}
	logFile.close();
}

void Simulator::rodsToFile(std::string fileName)
{
	ofstream rodFile;
	rodFile.open(fileName);
	rodFile << "#####################################################################\n";
	rodFile << "# Pos(0-1) | Safety rod[pcm] | Pulse rod[pcm] | Regulating rod[pcm] #\n";
	rodFile << "#####################################################################\n";

	size_t max_steps = 0;

	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
		max_steps = std::max(max_steps, *rods[i]->getRodSteps());
	}

	for (size_t i = 0; i <= max_steps; i++) {
		rodFile << std::setprecision(2) << fixed << (float)i << "\t";
		for (size_t j = 0; j < NUMBER_OF_CONTROL_RODS; j++) {
			if (i <= *rods[j]->getRodSteps()) {
				rodFile << std::setprecision(2) << fixed << rods[j]->getPCMat((float)i) << "\t";
				if (i <= *rods[j]->getRodSteps() - 1) {
					rodFile << std::setprecision(2) << fixed << rods[j]->getPCMat((float)(i + 1)) - rods[j]->getPCMat((float)i) << "\t";
				}
				else {
					rodFile << std::setprecision(2) << fixed << rods[j]->getPCMat((float)(i)) - rods[j]->getPCMat((float)(i-1)) << "\t";
				}
			}
			else {
				rodFile << "Nan" << "\t";
			}
		}
		rodFile << "\n";
	}
	rodFile.close();
}

void Simulator::setDemoMode()
{
	std::cout << "Setting to demo mode..." << std::endl;
	pushStableState(0.5); // 500mW
	regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);
	
	scram(ScramSignals::None);
	simulatorTime += DT_STEP;
	last_sample_number = 1;
}

void Simulator::setHighPowerDemoMode()
{
	std::cout << "Setting to high power demo mode..." << std::endl;
	pushStableState(1.2e4); // 12kW
	regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);

	scram(ScramSignals::None);
	simulatorTime += DT_STEP;
	last_sample_number = 1;
}

void Simulator::setScramEnabled(ScramSignals scramType, bool value)
{
	switch (scramType) {
		case Period:
			period_scram_enabled = value;
			break;
		case FuelTemperature:
			fuel_temp_scram_enabled = value;
			break;
		case WaterTemperature:
			water_temp_scram_enabled = value;
			break;
		case Power:
			power_scram_enabled = value;
			break;
		case WaterLevel:
			water_level_scram_enabled = value;
			break;
	}
}

bool Simulator::getScramEnabled(ScramSignals scramType)
{
	switch (scramType) {
		case Period:
			return period_scram_enabled;
			break;
		case FuelTemperature:
			return fuel_temp_scram_enabled;
			break;
		case WaterTemperature:
			return water_temp_scram_enabled;
			break;
		case Power:
			return power_scram_enabled;
			break;
		case WaterLevel:
			return water_level_scram_enabled;
			break;
		case User:
			return true;
			break;
		default:
			return false;
	}
}

Simulator::Simulator(Settings* properties)
{
	dataPoints = (size_t)std::round(DELETE_OLD_DATA_TIME_DEFAULT / DT_STEP) + 1;
	time_ = new double[dataPoints];
	reactivity_ = new float[dataPoints];
	rodReactivity_ = new float[dataPoints];

	// Initialize the state vector
	for (int i = 0; i < 8; i++)
		state_vector_[i] = new double[dataPoints];

	xenon_ = new float[(size_t)(2*DELETE_OLD_DATA_TIME_DEFAULT + 1)];
	iodine_ = new float[(size_t)(2*DELETE_OLD_DATA_TIME_DEFAULT + 1)];
	temperature_ = new float[dataPoints];

	// Create control rods
	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++)
		rods[i] = new ControlRod(false);

	source_sqw = new SquareWave(SIMULATION_MODE_PERIOD_DEFAULT, 1.f);
	source_sinMode = new Sine(SIMULATION_MODE_PERIOD_DEFAULT, 1.f);
	source_saw = new SawTooth(SIMULATION_MODE_PERIOD_DEFAULT, 1.f);
	source_none = new PeriodicalMode(1.f, 0.f);

	// Initialize simulator
	reset(properties);
	
}

void Simulator::init() {
	// Reset rods
	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
		rods[i]->resetRod();
	}

	// Initial values
	time_[0] = 0.;
	time_[0] = 0.;
	reactivity_[0] = getTotalRodReactivity() + core_excess_reactivity - getTotalRodWorth();
	rodReactivity_[0] = reactivity_[0];
	state_vector_[0][0] = -1e5 * getCurrentSourceActivity() * prompt_lifetime / rodReactivity_[0];
	state_vector_[7][0] = state_vector_[0][0];
	for (int i = 1; i < 7; i++) {
		state_vector_[i][0] = state_vector_[0][0] * groupStability[i - 1];
		state_vector_[7][0] += state_vector_[i][0];
	}
	xenon_[0] = 0.f;
	iodine_[0] = 0.f;
	temperature_[0] = WATER_TEMPERATURE_DEFAULT;
	waterTemperature = WATER_TEMPERATURE_DEFAULT;
	Xe_conc = 0.;
	I_conc = 0.;
	startTime = -1.;
	actualTime = 0.;
	simulatorTime = 0.;
	lastTime = 0.;
	waterLevel_delta = 0.;
	powerHold = 0.;
	pulse_maxP = 0.;
	pulse_energy = 0.;
	pulse_FWHM = 0.;
	pulse_maxT = 0.f;
	pulse_start = 0;
	time_at_peak = 0.;
	last_sample_number = 0;
	speedFactor = 1.;
	calc_performed = 0;
	iterations_total = 0;
	frames_total = 0;
	periodTimer = -1.;
	resetAverage = 0;
	doseRate = 0.;

	// Adding power extremes
	if (powerExtremes) delete powerExtremes;
	powerExtremes = new deque<PowerExtreme>;
	powerExtremes->push_back(PowerExtreme());

	recalculateLambdaBetaEffective();

	iterations_total++;
}

Simulator::~Simulator() {
	delete time_;
	delete reactivity_;
	for(int i = 0; i < 8; i++)
		delete state_vector_[i];
	delete xenon_;
	delete iodine_;
	delete temperature_;
	delete rodReactivity_;
	delete powerExtremes;
	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) delete rods[i];
}

void Simulator::setAllToDefaults()
{
	Settings* default_settings = new Settings();
	setProperties(default_settings);
	delete default_settings;
}

void Simulator::reset(Settings * properties)
{
	if (properties) { setProperties(properties); }
	else { setAllToDefaults(); }
	init();
}

const bool &Simulator::getNeutronSourceInserted() const
{
	return source_inserted;
}

void Simulator::setNeutronSourceInserted(const bool &value)
{
	source_inserted = value;
}

const bool &Simulator::getTemperatureEffectsEnabled() const {
	return temperature_effects;
}

void Simulator::setTemperatureEffectsEnabled(const bool &value) {
	temperature_effects = value;
}

void Simulator::setFissionPoisoningEffectsEnabled(const bool &value) {
	fissionPoisoning_effects = value;
}

const double &Simulator::getMaxFissionCrossSection() const
{
	return Sigma_f;
}

void Simulator::setMaxFissionCrossSection(const double &value)
{
	Sigma_f = value;
}

const double &Simulator::getReactorCoreVolume() const
{
	return core_volume;
}

void Simulator::setReactorCoreVolume(const double &value)
{
	core_volume = value;
}

const double* Simulator::getDelayedGroupFractions() const
{
	return beta_neutrons;
}

void Simulator::setDelayedGroupFraction(size_t group, double value)
{
	beta_ -= beta_neutrons[group];
	beta_neutrons[group] = value;
	beta_ += value;
	recalculateLambdaBetaEffective();
}

const double* Simulator::getDelayedGroupDecays() const
{
	return delayed_decay_time;
}

void Simulator::setDelayedGroupDecay(size_t group, double value)
{
	delayed_decay_time[group] = value;
	recalculateLambdaBetaEffective();
}

const bool * Simulator::getDelayedGroupEnabled() const
{
	return delayed_enabled;
}

void Simulator::setDelayedGroupEnabled(size_t group, bool value)
{
	delayed_enabled[group] = value;
	recalculateLambdaBetaEffective();
}

const double &Simulator::getPromptNeutronLifetime() const
{
	return prompt_lifetime;
}

void Simulator::setPromptNeutronLifetime(const double &value)
{
	prompt_lifetime = value;
}

const double & Simulator::getExcessReactivity() const
{
	return core_excess_reactivity;
}

void Simulator::setExcessReactivity(const double &value)
{
	core_excess_reactivity = value;
}

const double & Simulator::getNeutronSourceModulation() const
{
	return ns_modulation;
}

void Simulator::setNeutronSourceModulation(const double & value)
{
	ns_modulation = value;
}

const double &Simulator::getNeutronSourceActivity() const
{
	return ns_base_activity;
}

void Simulator::setNeutronSourceActivity(const double &value)
{
	ns_base_activity = value;
}

const SimulationModes & Simulator::getNeutronSourceMode() const
{
	return source_mode;
}

void Simulator::setNeutronSourceMode(const SimulationModes & value)
{
	source_mode = value;
	if (source_mode != SimulationModes::None) getSourceModeClass(source_mode)->reset();
}

double Simulator::getCurrentSourceActivity()
{
	if (source_mode == SimulationModes::None) {
		return ns_base_activity;
	}
	else {
		return std::max(ns_base_activity + getSourceModeClass(source_mode)->getCurrentOffset(), 0.);
	}
}

const double &Simulator::getDeleteOldValues() const
{
	return delete_old_data_time;
}

void Simulator::setDeleteOldValues(const double &value)
{
	delete_old_data_time = value;
}

const size_t &Simulator::getLatestSampleNumber() const
{
	return last_sample_number;
}

std::pair<double, double> Simulator::getHeatCpConstants()
{
	return std::pair<double, double>(cp_const_a, cp_const_b);
}

void Simulator::setHeatCpConstants(std::pair<double, double> &values)
{
	cp_const_a = values.first;
	cp_const_b = values.second;
}

void Simulator::getCurrentStateVector(double* result, bool copyLast) const
{
	size_t cur_index = getCurrentIndex();
	for (int i = 0; i < (copyLast ? 8 : 7); i++) {
		result[i] = state_vector_[i][cur_index];
	}
}

const double* Simulator::getNeutronGroup(size_t i)
{
	return state_vector_[i];
}

double Simulator::getCurrentReactivity() const
{
	return reactivity_[getCurrentIndex()];
}

const float *Simulator::getReactivity() const
{
	return reactivity_;
}

float Simulator::getCurrentRodReactivity() const
{
	return rodReactivity_[getCurrentIndex()];
}

const float* Simulator::getRodReactivity() const
{
	return rodReactivity_;
}

float Simulator::getCurrentTemperature() const
{
	return temperature_[getCurrentIndex()];
}

const float *Simulator::getTemperature() const
{
	return temperature_;
}

double *Simulator::getWaterTemperature()
{
	return &waterTemperature;
}

void Simulator::waterHeatingCycle(double dt)
{
	double waterCapacity = waterVolume * 1e3 * water_specific_heat; // C_vode = V * rho * c_v
	// Create derivatives
	double T_fuel = getCurrentPower() / waterCapacity; // dT = P*dt/C_vode
	// From Žerovnik power calibration
	double Q_air = 13.6 * pow(ENVIRONMENT_TEMPERATURE_DEFAULT - waterTemperature, 4 / 3);
	double Q_concrete = 250 * (ENVIRONMENT_TEMPERATURE_DEFAULT - WaterTemperature);
	double T_passive_cooling = (Q_air + Q_concrete) / waterCapacity;
	// dT = (T-air_temp) * const. * dt
	// double T_passive_cooling = (waterTemperature - ENVIRONMENT_TEMPERATURE_DEFAULT) * (cooling_coefficient / waterCapacity);
	double T_active_cooling = w_cooling ? (cooling_p / waterCapacity) : 0.; // dT = P*dt / C_vode
	double wt_temp = waterTemperature + dt * (T_fuel - T_passive_cooling - T_active_cooling);
	if (wt_temp < ENVIRONMENT_TEMPERATURE_DEFAULT) {
		waterTemperature = ENVIRONMENT_TEMPERATURE_DEFAULT;
	}
	else if (wt_temp > 100.) {
		waterTemperature = 100.;
		double Q = (wt_temp - 100.) * waterCapacity;
		double delta_water_level = (Q / water_vaporization_ethalpy) / (M_PI * std::pow(reactor_vessel_radius, 2.));
		if (!status) waterLevel_delta -= std::max(delta_water_level, -6.); // Limit water level to -6 meters
	}
	else {
		waterTemperature = wt_temp;
	}
}

double * Simulator::getWaterLevel()
{
	return &waterLevel_delta;
}

void Simulator::recalculateDoseRate(double flux)
{
	double theta = atan(TANK_RADIUS / WATER_HEIGHT); // maximum angle an outward trajectory can have with the vertical axis
	flux = flux * pow(theta / M_PI, 2); // phi = phi0 * theta^2 ; probability that neutron is on outward trajectory multiplied with the gross flux
	double avgPath = WATER_HEIGHT * sqrt(tan(theta) / theta); // average path length of neutron to surface
	flux = flux * exp(WATER_ABSORBTION * avgPath); // exponential decrease of flux intensity through water
	doseRate = flux;
}

const double Simulator::getCurrentPower()
{
	return powerFromNeutrons(state_vector_[0][getCurrentIndex()]);
}

double Simulator::getFuelCp(double T)
{
	// Cp taken from simnad1981, where Cp(T) = (2.04 + 4.17e-3 T) J/(K cm^3)
	// Volume of a signle fuel element  ((0.5 * 3.7338cm)**2 - (0.5 * 0.635cm)**2)*3.14*38.1cm
	// 3.556 cm is outer radius
	// 0.635 cm is inner radius
	// 38.1  cm is fuel element len.
	// Geometry data from http://www.rcp.ijs.si/ric/description-s.html
	const double fuel_volume = (pow(0.5 * 3.556, 2) - pow(0.5 * 0.635, 2)) * M_PI * 38.1 * (float)no_fuel_elements;
	return fuel_volume * (cp_const_a + T * cp_const_b) * 0.858;
}

/*
	Use a general formula to find the real root of the first order
	polynomial from the TRIGLAV code that describes the dependence
	of stationary temperature in relation to the reactor power.
*/
double Simulator::getCoolingFromTemperature(double T)
{
	double a = tempModelCoeff[2];
	double b = tempModelCoeff[1];
	double c = tempModelCoeff[0];
	double d = waterTemperature - T;
	double d0 = pow(b, 2) - 3 * a * c;
	double d1 = 2. * pow(b, 3) - 9. * a * b * c + 27. * pow(a, 2) * d;
	double C = std::cbrt((d1 + std::sqrt(pow(d1, 2) - 4. * pow(d0, 3))) / 2.);
	return -no_fuel_elements * (1. / (3. * a)) * (b + C + (d0 / C));
}

/*
	Return stable temperature according to the third degree polynomial
*/
double Simulator::getStableTemperature(double P)
{
	P /= no_fuel_elements;
	return waterTemperature + tempModelCoeff[0] * P + tempModelCoeff[1] * pow(P, 2), +tempModelCoeff[2] * pow(P, 3);
}

double Simulator::getCurrentTime() const
{
	return time_[getCurrentIndex()];
}

const double *Simulator::getTime() const
{
	return time_;
}

const size_t &Simulator::getCalculationsPerformed() const
{
	return calc_performed;
}

size_t Simulator::getOrderChanges() const
{
	return powerExtremes->size();
}

Simulator::PowerExtreme &Simulator::getExtremeAt(size_t i)
{
	return powerExtremes->at(i);
}

bool Simulator::isPaused() const
{
	return speedFactor == 0.;
}

void Simulator::setScramCallback(const std::function<void(int)>& callback)
{
	scramCallback = callback;
}

void Simulator::setResetScramCallback(const std::function<void()>& callback)
{
	scramResetCallback = callback;
}

void Simulator::setSevereErrorCallback(const std::function<void(int)>& callback)
{
	severeErrorCallback = callback;
}

void Simulator::setPulseCallback(const std::function<void(PulseData)>& callback)
{
	pulseCallback = callback;
}

void Simulator::setSpeedFactor(double value)
{
		this->speedFactor = value;
}

double &Simulator::getSpeedFactor()
{
	return speedFactor;
}

const size_t Simulator::getIndexFromTime(double time) const
{
	size_t ret = getOldestIndex();
	double prevTime = time_[ret];
	if (time >= prevTime) {
		return shiftIndex(ret, (int)std::round((time - prevTime) * 1e3));
	}
	else {
		return ret;
	}
}

double &Simulator::getPowerLimit()
{
	return powerLimit;
}

void Simulator::setPowerLimit(double limit)
{
	powerLimit = limit;
}

double &Simulator::getPeriodLimit()
{
	return periodLimit;
}

void Simulator::setPeriodLimit(double limit)
{
	periodLimit = limit;
}

double &Simulator::getFuelTemperatureLimit()
{
	return fuelTemperatureLimit;
}

void Simulator::setFuelTemperatureLimit(double limit)
{
	fuelTemperatureLimit = limit;
}

double &Simulator::getWaterTemperatureLimit()
{
	return waterTemperatureLimit;
}

void Simulator::setWaterTemperatureLimit(double limit)
{
	waterTemperatureLimit = limit;
}

double& Simulator::getWaterLevelLimit()
{
	return waterLevelLimit;
}

void Simulator::setWaterLevelLimit(double limit)
{
	waterLevelLimit = limit;
}

void Simulator::pushNewState(double * states, size_t index)
{
	for (int i = 0; i < 8; i++) {
		state_vector_[i][index] = states[i];
	}
}

void Simulator::checkOperationalLimits()
{
	if (status) return;
	if (getCurrentPower() > 1e13) { // 10GW
		if (severeErrorCallback) severeErrorCallback(0);
		scram(ScramSignals::Power);
	}
	else if (getCurrentTemperature() > 950.f) {
		if (severeErrorCallback) severeErrorCallback(1);
		scram(ScramSignals::FuelTemperature);
	}
	else {
		if (godMode) {
			if (getCurrentPower() > std::numeric_limits<double>::max()*0.1)
			{
				scram(ScramSignals::Power);
			}

			if (getCurrentTemperature() > std::numeric_limits<float>::max()*0.001) {
				scram(ScramSignals::FuelTemperature);
			}
		}
		else {
			if ((getCurrentPower() > powerLimit) && !pulsing)
			{
				if (power_scram_enabled) scram(ScramSignals::Power);
			}

			if (getCurrentTemperature() > fuelTemperatureLimit) {
				if (fuel_temp_scram_enabled) scram(ScramSignals::FuelTemperature);
			}

			if (waterTemperature > waterTemperatureLimit) {
				if (water_temp_scram_enabled) scram(ScramSignals::WaterTemperature);
			}

			/*if (std::abs(waterLevel_delta) > waterLevelLimit) {
			if (water_level_scram_enabled) scram(ScramSignals::WaterLevel);
			}*/

			if (periodTimer == -1) // If period hasn't gone past the limit yet
			{
				if (reactorPeriod < periodLimit && reactorPeriod > 0)
				{
					periodTimer = getCurrentTime(); // Set period time to current time
				}
			}
			else
			{
				if (reactorPeriod > periodLimit || reactorPeriod < 0)
				{
					periodTimer = -1; // Reset period timer
				}
				else
				{
					if (periodTimer + 0.5 < getCurrentTime()) // If it's been there for more than half a second, SCRAM!
					{
						if (period_scram_enabled && !(regulatingRod()->getOperationMode() == ControlRod::OperationModes::Simulation)) scram(ScramSignals::Period);
					}
				}
			}
		}
	}
}

void Simulator::scram(ScramSignals reason)
{
	if (reason) {
		if ((status | reason) == status) return;
		status |= reason;
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
			rods[i]->scramRod();
		}
		waterLevel_delta = 0.;
		if (scramCallback) scramCallback(status);
	}
	else {
		status = ScramSignals::None;
		for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
			rods[i]->setEnabled(true);
			rods[i]->clearCommands();
		}
		if (scramResetCallback) scramResetCallback();
	}
}

double *Simulator::getReactorPeriod()
{
	return &reactorPeriod;
}

double *Simulator::getReactorAsymPeriod()
{
	return &reactorAsymPeriod;
}

float Simulator::getTotalRodReactivity()
{
	float sum = 0.f;
	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) sum += rods[i]->getCurrentPCM();
	return sum;
}

float Simulator::getTotalRodWorth()
{
	float sum = 0.f;
	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) sum += rods[i]->getRodWorth();
	return sum;
}

void Simulator::runLoop()
{
	double time = nanogui::get_seconds_since_epoch();
	size_t srt_iterations;
	if (startTime < 0.) {
		startTime = nanogui::get_seconds_since_epoch();
		srt_iterations = 1;
	}
	else {
		double processTime = (time - lastTime) * speedFactor; // the amount of time we need to process
															  // itterations from the time we need to process plus the difference between the actual simulator time and the latest time we simulated
		srt_iterations = (size_t)floor((processTime + simulatorTime - time_[getCurrentIndex()]) / DT_STEP);
		// Increment actual simulator time
		simulatorTime += processTime;
	}

	mainLoop(srt_iterations);
	last_sample_number = srt_iterations;
	solvePerFrame();
	frames_total++;
	lastTime = time;
	actualTime = time - startTime; // Maybe we will use this some time in the future, doesn't hurt fps so why not
}

const float rodAutoMove = 0.001f; // how much can the control rod move at a time (raw fraction of rodSteps)[0.1%]
void Simulator::mainLoop(size_t iterations)
{
	// Optimizations:
	size_t currentIndex, nextIndex;
	double newPower, tempPow, negative_reactivity, rho, lastState[7], kf[4][7], finalState[8];
	double stationary_temperature, new_temperature;
	for (size_t i = 0; i < iterations; i++)
	{
		// Check pulse status
		checkPulsingStatus();

		currentIndex = getCurrentIndex();
		nextIndex = getNextIndex();
		// Increment time
		time_[nextIndex] = time_[currentIndex] + DT_STEP;

		newPower = getCurrentPower();

		// Calculate stationary temperature
		tempPow = std::min(newPower, 1e6) / (float)no_fuel_elements;
		stationary_temperature = (float)waterTemperature;
		for (int order = 0; order < 3; order++) 
			stationary_temperature += (float)(tempModelCoeff[order] * pow(tempPow, order + 1));
		new_temperature = temperature_[currentIndex];


		double power_losses = getCoolingFromTemperature(new_temperature);
		new_temperature += (newPower - power_losses) * DT_STEP / getFuelCp(new_temperature);
		new_temperature = std::max(new_temperature, 22.);
		// The cooling step, performed in both FH model and asymptotic model, commented out due to temperature model refractoring

		temperature_[nextIndex] = static_cast<float>(new_temperature);

		// Move rods
		// In the automatic mode, the rods are moved to reach or maintain a constant power
		if (regulatingRod()->getOperationMode() == ControlRod::OperationModes::Automatic) {
			double powerToKeep = keepCurrentPower ? powerHold : keepSteadyPowerAt;
			if (std::abs(powerToKeep - newPower) / powerToKeep > steadyDeviation) {
				float move = rodAutoMove * *regulatingRod()->getRodSteps();
				float newPos = *regulatingRod()->getExactPosition();
				if (powerToKeep < newPower) {
					newPos -= move;
					newPos = std::max(0.f, newPos);
				}
				else {
					newPos += move;
					newPos = std::min((float)*regulatingRod()->getRodSteps(), newPos);
				}
				if ((avoidPeriodScram && (reactorPeriod > periodLimit * 1.1 || reactorPeriod < 0.)) || !avoidPeriodScram || (powerToKeep < newPower)) {
					regulatingRod()->commandMove(newPos);
				}
				else {
					regulatingRod()->clearCommands();
				}
			}
		}

		// Rod positions are updated by the ControlRod class
		for (int r = 0; r < NUMBER_OF_CONTROL_RODS; r++) 
			rods[r]->refreshRod(DT_STEP);

		rodReactivity_[nextIndex] = -getTotalRodWorth() + getTotalRodReactivity() + core_excess_reactivity;


		// The fission poison concentrations are changing slowly, so they do not need to be
		// calculated as often as the point kinetics. Default is each 125 steps
		if (nextIndex % 125 == 0) 
			recalculatePoisonConcentrations(125 * DT_STEP);
		// Save values everyPOISON_DATA_DEL_DIVISION steps and convert to g/m3
		if (nextIndex % POISON_DATA_DEL_DIVISION == 0) {
			size_t poi_idx = nextIndex / POISON_DATA_DEL_DIVISION;
			xenon_[poi_idx] = (float)(Xe_conc / AVOGADRO_NUM * XENON_MOLAR_MASS);
			iodine_[poi_idx] = (float)(I_conc / AVOGADRO_NUM * IODINE_MOLAR_MASS);
		}

		/*This adds negative temperature and fission poisoning effects on
		reactivity if enabled.*/
		negative_reactivity = 0.;
		if (temperature_effects) {
			negative_reactivity += getReactivityCoefficient(new_temperature) * (new_temperature - ENVIRONMENT_TEMPERATURE_DEFAULT);	
		}
		if (fissionPoisoning_effects) {
			negative_reactivity += Xe_conc * 1e5 * sigma_Xe_a / (nu_bar * Sigma_f);
			/*
			if (nextIndex % 5000 == 0) {
				std::cout << "Xenon: " << Xe_conc << " Xenon negative reactivity ";
				std::cout << Xe_conc * 1e5 * sigma_Xe_a / (nu_bar * Sigma_f) << " pcm conv ";
				std::cout <<	sigma_Xe_a / (nu_bar * Sigma_f) << " flux " << getCurrentFlux() << std::endl;
			}
			*/
		}

		// Substract total negative reactivity from insrted reactivity
		reactivity_[nextIndex] = rodReactivity_[nextIndex] - (float)(negative_reactivity);

		// Get neutron source activity
		ns_activity_temp = getCurrentSourceActivity();
		advanceSourceTime(DT_STEP);

		// RK4 koeficients
		// arraySum(result, firstArray, secondArray)
		// multiplyArray(result, firstArray, scalar)
		getCurrentStateVector(lastState, false); // get neutron populations
		rho = (reactivity_[nextIndex]) * 1e-5; // set variable for reactivity
		neutronChange(kf[0], lastState, rho); // f(t0) -- change of population at t0
		std::memcpy(kf[1], kf[0], 7 * sizeof(double));
		multiplyArray(kf[1], 0.5 * DT_STEP);
		arraySum(kf[1], lastState, true);
		neutronChange(kf[1], kf[1], rho); // f(t0 + h/2) -- change of population at t0 + 1/2*h
		std::memcpy(kf[2], kf[1], 7 * sizeof(double));
		multiplyArray(kf[2], 0.5 * DT_STEP);
		arraySum(kf[2], lastState, true);
		neutronChange(kf[2], kf[2], rho); // f(t0 + 3h/4) -- change of population at t0 + 3/4*h
		std::memcpy(kf[3], kf[2], 7 * sizeof(double));
		multiplyArray(kf[3], DT_STEP);
		arraySum(kf[3], lastState, true);
		neutronChange(kf[3], kf[3], rho); // f(t0 + h) -- change of population at t0 + h

		// RK4 final sum
		// stateVector += (dt/6)*( (k1+k4) + 2*(k2+k3) )
		arraySum(kf[0], kf[3]);
		arraySum(kf[1], kf[2]);
		multiplyArray(kf[1], 2.);
		arraySum(kf[0], kf[1]);
		multiplyArray(kf[0], DT_STEP / 6.);
		arraySum(kf[0], lastState, true); // No negative values

		// Check for overshooting and make neutron sum
		std::memcpy(finalState, kf[0], 7 * sizeof(double));
		finalState[0] = std::max(finalState[0], 10.);
		finalState[7] = 0.;
		for (int f = 0; f < 7; f++)
			finalState[7] += finalState[f];

		if ((finalState[0] - lastState[0]) * (lastState[0] - state_vector_[0][shiftIndex(currentIndex, -1)]) < 0.) 
			resetAverage = iterations_total;

		// Calculate power, temperature and reactivity extremes during pulsing
		if (pulsing) {
			if (finalState[0] > pulse_maxP) {
				time_at_peak = time_[nextIndex];
				pulse_maxP = finalState[0];
			}
			pulse_energy += newPower * DT_STEP;
			pulse_maxT = std::max(pulse_maxT, temperature_[nextIndex]);
		}

		// Push new neutron concentrations
		pushNewState(finalState, nextIndex);

		waterHeatingCycle(DT_STEP);

		// Increase number of iterations
		iterations_total++;

		checkOperationalLimits();
	}
}

void Simulator::recalculateLambdaBetaEffective()
{
	beta_ = 0.;
	lambda_eff = 0.;
	for (int i = 0; i < 6; i++) {
		if (delayed_enabled[i]) {
			beta_ += beta_neutrons[i];
			lambda_eff += beta_neutrons[i] / delayed_decay_time[i];
		}
	}

	double leftOver = 0.;
	for (int i = 0; i < 6; i++) {
		groupStability[i] = beta_neutrons[i] / (delayed_decay_time[i] * prompt_lifetime);
		if (!delayed_enabled[i]) leftOver += groupStability[i];
	}
}

const double periodK = 0.95;
void Simulator::solvePerFrame() {
	const size_t currentIdx = getCurrentIndex();
	//if ()
	doScriptCommands();
	
	const auto NO_AVERAGES_REACTIVITY = 700;
	int averageValues = (int)std::min(iterations_total - resetAverage - 1, (size_t)NO_AVERAGES_REACTIVITY);
	if (averageValues > 1) {
		const double periodSum = (1 - std::pow(periodK, averageValues - 2)) / (1 - periodK);
		double* vals = new double[averageValues];
		double sum = 0.;
		for (int i = 0; i < averageValues; i++) 
			vals[i] = state_vector_[0][shiftIndex(currentIdx, i - averageValues + 1)];
		for (int i = 0; i < averageValues - 1; i++) {
			sum += std::pow(periodK, averageValues - i - 2) / std::log(vals[i + 1] / vals[i]);
		}
		delete[] vals;
		sum *= DT_STEP / periodSum;
		reactorPeriod = sum;
	}
	else {
		reactorPeriod = 3600.;
	}
	
	if (iterations_total > 1) {
		double rho = reactivity_[currentIdx] * 1e-5;
		// 1.15 is conversion from beta to beta effective
		double l_e = prompt_lifetime + (beta_*1.15 - rho) / lambda_eff;
		reactorAsymPeriod = l_e / rho;
	}
	else {
		reactorAsymPeriod = -6000.;
	}

	// Self explanatory
	addPowerExtremes();

	// Delete old data
	if (this->delete_old_data_time > 0)
	{
		size_t deleteIndex = (dataPoints > iterations_total) ? (size_t)0 : (iterations_total - dataPoints);
		if (deleteIndex > 0) {
			// Delete power_extremes order changes that are old
			if (powerExtremes->size()) {
				while (powerExtremes->front().when < time_[getOldestIndex()]) {
					trailingExtreme = powerExtremes->front();
					powerExtremes->pop_front();
					if (powerExtremes->size() == 0) break;
				}
			}
		}
	}
}

// Physically, this is the derivative of all neutron concentration groups
// or the right side of the point kinetics equations
void Simulator::neutronChange(double* new_state, double* prevState, double rho)
{
	fissionRate = prevState[0] / prompt_lifetime;
	temp_state[0] = (rho - beta_/**1.15*/) * fissionRate; // New fission neutrons per second

	if (source_inserted)
		temp_state[0] += ns_activity_temp; // Neutron source neutrons per second

	for (int i = 0; i < 6; i++)
	{
		delayed = delayed_decay_time[i] * prevState[i + 1]; // Delayed neutrons per second
		if (delayed_enabled[i]) 
			temp_state[0] += delayed;
		temp_state[i + 1] = (beta_neutrons[i] * fissionRate) - delayed;
	}

	for (int i = 0; i < 7; i++) 
		new_state[i] = temp_state[i];
}

void Simulator::multiplyArray(double* in_array, double scalar)
{
	for (int i = 0; i < 7; i++)
	{
		in_array[i] *= scalar;
	}
}

void Simulator::arraySum(double* sum_array1, double* sum_array2, bool posOnly)
{
	for (int i = 0; i < 7; i++)
	{
		sum_array1[i] += sum_array2[i];
		if (posOnly) sum_array1[i] = std::max(0., sum_array1[i]);
	}
}

// This is a very slow process, an Euler scheme is used for time 
// propagation
void Simulator::recalculatePoisonConcentrations(double dt) {
	// normirano na 10E13 1 / (cm * *2 s) fluksa pri 250kW
	//double fluks = getCurrentPower() * (double)1e17 / 2.5e5;
	double fluks = getCurrentFlux();
	double I_temp = I_conc; //Save the old value for the iodine DE
	I_conc += dt * (gamma_I * Sigma_f * fluks - lambda_I * I_conc);
	Xe_conc += dt * (gamma_X * Sigma_f * fluks + lambda_I * I_temp
		- lambda_X * Xe_conc - sigma_Xe_a * fluks * Xe_conc);
}

double Simulator::powerFromNeutrons(double n) {
	double fissionEnergy = 3.20435e-11; // 200 MeV in joules
	return n * Sigma_f * t_neutron_speed * fissionEnergy;
}

// The coefficients are defined using GUI, see manual
double Simulator::getReactivityCoefficient(double temp)
{
	if (temp < alphaT1) {
		return alpha0 + temp*(alphaAtT1 - alpha0) / alphaT1;
	}
	else {
		return alphaAtT1 + (float)(alphaK * (temp - alphaT1));
	}
}

void Simulator::pushStableState(double power)
{
	size_t newIndex = getNextIndex();
	size_t currentIndex = getCurrentIndex();

	double dt = DT_STEP;
	// Calculate stable water temp
	// Power = k * (T_w - T_e), express T_w 
	double waterCapacity = waterVolume * 1e3 * water_specific_heat; // C_vode = V * rho * c_v
	float stableWaterTemp = (float)(power / (cooling_coefficient / waterCapacity)) + WATER_TEMPERATURE_DEFAULT;
	//float stableWaterTemp = (float)(power / (waterVolume * 41.855 * 1.3)) + WATER_TEMPERATURE_DEFAULT;
	stableWaterTemp = max(stableWaterTemp, WATER_TEMPERATURE_DEFAULT);
	// me is the power normalized per fuel element
	const double me = power / (float)no_fuel_elements;
	stableWaterTemp = min(stableWaterTemp, (float)getWaterTemperatureLimit() / 2);
	waterTemperature = stableWaterTemp;
	float stableFuelTemp = (float)(stableWaterTemp + tempModelCoeff[0] * me 
		+ tempModelCoeff[1] * pow(me, 2) + tempModelCoeff[2] * pow(me, 3));
	// Moving rods to create desired reactivity
	float reactivityToInsert = getTotalRodWorth() - core_excess_reactivity;
	if (source_inserted) reactivityToInsert -= (float)(1e5 * ns_base_activity * prompt_lifetime * t_neutron_speed * Sigma_f / (power * fissions_per_wattSecond));
	if (temperature_effects) reactivityToInsert += getReactivityCoefficient(stableFuelTemp)*(stableFuelTemp - (float)ENVIRONMENT_TEMPERATURE_DEFAULT);
	float rodPos[NUMBER_OF_CONTROL_RODS] = { safetyRod()->getPosFromPcm(reactivityToInsert) , 0.f, 0.f };
	reactivityToInsert -= safetyRod()->getPCMat(rodPos[0]);
	if (reactivityToInsert > 0.f) {
		rodPos[2] = shimRod()->getPosFromPcm(reactivityToInsert);
		reactivityToInsert -= shimRod()->getPCMat(rodPos[2]);
		if (reactivityToInsert > 0.f) {
			rodPos[1] = regulatingRod()->getPosFromPcm(reactivityToInsert);
			reactivityToInsert -= regulatingRod()->getPCMat(rodPos[1]);
		}
	}
	cout << "Set stable mode at " << power << "W" << endl << "Reactivity overkill: " << reactivityToInsert << "pcm" << endl;

	for (int i = 0; i < NUMBER_OF_CONTROL_RODS; i++) {
		rods[i]->moveRodToStep(rodPos[i], true);
		rods[i]->setEnabled(true, true);
		rods[i]->clearCommands();
		rods[i]->refreshRod(dt);
	}
	
	time_[newIndex] = time_[currentIndex] + DT_STEP;
	rodReactivity_[newIndex] = getTotalRodReactivity() + core_excess_reactivity - getTotalRodWorth();
	reactivity_[newIndex] = 0.f;
	temperature_[newIndex] = stableFuelTemp;
	

	// Calculating neutron populations
	double neuts[8];
	
	neuts[0] = power * fissions_per_wattSecond / (Sigma_f * t_neutron_speed);
	neuts[7] = neuts[0];
	for (int i = 1; i < 7; i++) {
		neuts[i] = groupStability[i - 1] * neuts[0];
		neuts[7] += neuts[i];
	}
	pushNewState(neuts, newIndex);

	resetAverage = iterations_total;
	iterations_total++;
}

void Simulator::setProperties(Settings * nodes)
{
	source_inserted = nodes->neutronSourceInserted;

	safetyRod()->setRodName(SAFETY_ROD_NAME_DEFAULT);
	regulatingRod()->setRodName(REGULATORY_ROD_NAME_DEFAULT);
	shimRod()->setRodName(SHIM_ROD_NAME_DEFAULT);
	for (size_t rodIndex = 0; rodIndex < NUMBER_OF_CONTROL_RODS; rodIndex++) {
		rods[rodIndex]->setRodSteps(nodes->rodSettings[rodIndex].rodSteps, false);
		rods[rodIndex]->setRodSpeed(nodes->rodSettings[rodIndex].rodSpeed);
		rods[rodIndex]->setRodWorth(nodes->rodSettings[rodIndex].rodWorth);
		for (size_t i = 0; i < 2; i++) {
			rods[rodIndex]->setParameter(i, nodes->rodSettings[rodIndex].rodCurve[i], false);
		}
		rods[rodIndex]->recalculateStepData();
	}
	rods[1]->squareWave()->setPeriod(nodes->squareWave.period);
	rods[1]->squareWave()->setAmplitude(nodes->squareWave.amplitude);
	for(int sqw = 0; sqw < 4; sqw++) rods[1]->squareWave()->xIndex[sqw] = nodes->squareWave.xIndex[sqw];
	if (nodes->squareWaveUsesRodSpeed)rods[1]->squareWave()->rodSpeed = rods[1]->getRodSpeed();

	rods[1]->sine()->setPeriod(nodes->sineMode.period);
	rods[1]->sine()->setAmplitude(nodes->sineMode.amplitude);
	rods[1]->sine()->mode = (Sine::SineMode)nodes->sineMode.mode;

	rods[1]->sawTooth()->setPeriod(nodes->sawToothMode.period);
	rods[1]->sawTooth()->setAmplitude(nodes->sawToothMode.amplitude);
	for(int saw = 0; saw < 6; saw++) rods[1]->sawTooth()->xIndex[saw] = nodes->sawToothMode.xIndex[saw];

	periodLimit = nodes->periodLimit;
	period_scram_enabled = nodes->periodScram;
	powerLimit = nodes->powerLimit;
	power_scram_enabled = nodes->powerScram;
	fuelTemperatureLimit = nodes->tempLimit;
	fuel_temp_scram_enabled = nodes->tempScram;
	waterTemperatureLimit = nodes->waterTempLimit;
	water_temp_scram_enabled = nodes->waterTempScram;
	waterLevelLimit = nodes->waterLevelLimit;
	water_level_scram_enabled = nodes->waterLevelScram;

	temperature_effects = nodes->temperatureEffects;

	core_excess_reactivity = nodes->excessReactivity;
	core_volume = nodes->coreVolume;
	reactor_vessel_radius = nodes->vesselRadius;
	prompt_lifetime = nodes->promptNeutronLifetime;
	double sumBeta = 0.;
	for (size_t i = 0; i < 6; i++) {
		beta_neutrons[i] = nodes->betas[i];
		if(nodes->groupsEnabled[i]) sumBeta += beta_neutrons[i];
		setDelayedGroupDecay(i, nodes->lambdas[i]);
		delayed_enabled[i] = nodes->groupsEnabled[i];
	}
	beta_ = sumBeta;
	waterVolume = nodes->waterVolume;

	w_cooling = nodes->waterCooling;
	cooling_p = nodes->waterCoolingPower;

	ns_base_activity = nodes->neutronSourceActivity;
	source_inserted = nodes->neutronSourceInserted;
	source_mode = (SimulationModes)((int)nodes->ns_mode);

	source_sqw->setPeriod(nodes->ns_squareWave.period);
	source_sqw->setAmplitude(nodes->ns_squareWave.amplitude);
	for(int sqw = 0; sqw < 4; sqw++) source_sqw->xIndex[sqw] = nodes->ns_squareWave.xIndex[sqw];

	source_sinMode->setPeriod(nodes->ns_sineMode.period);
	source_sinMode->setAmplitude(nodes->ns_sineMode.amplitude);
	source_sinMode->mode = (Sine::SineMode)nodes->ns_sineMode.mode;

	source_saw->setPeriod(nodes->ns_sawToothMode.period);
	source_saw->setAmplitude(nodes->ns_sawToothMode.amplitude);
	for(int saw = 0; saw < 6; saw++) source_saw->xIndex[saw] = nodes->ns_sawToothMode.xIndex[saw];

	keepCurrentPower = nodes->steadyCurrentPower;
	keepSteadyPowerAt = nodes->steadyGoalPower;
	avoidPeriodScram = nodes->avoidPeriodScram;
	steadyDeviation = nodes->steadyMargin;

	alpha0 = nodes->alpha0;
	alphaAtT1 = nodes->alphaAtT1;
	alphaT1 = nodes->alphaT1;
	alphaK = nodes->alphaK;

	autoScramAfterPulse = nodes->automaticPulseScram;
}

void Simulator::resetSimulator()
{
	reset(nullptr);
}

void Simulator::beginPulse()
{
	if (getScramStatus()) return; // Only fire if reactor isn't scrammed
	// Fire regulating rod
	regulatingRod()->fire(true);
	// Reset pulse variables
	pulse_maxP = 0.;
	pulse_energy = 0.;
	pulse_maxT = 0.f;
	pulse_FWHM = 0.;
	pulsing = true;
	pulse_start = getCurrentIndex();
	pulse_startP = getCurrentPower();
	//tempMode = TemperatureMode::FH;
}
void Simulator::doScriptCommands()
{
	if (!scriptCommands.empty()) {
		std::pair<double, double> coefficients;
		scriptTimer += DT_STEP;
		for (auto i = scriptCommands.begin(); i != scriptCommands.end(); ) {
			if (i->timed <= getCurrentTime()) {
				switch (i->command) {
					size_t dest;
					size_t position;
					//auto coefficients = getHeatCpConstants();
					cout << "Running action number" << i->command << " name " << i->strCommand << endl;
				case setRegulatingRod:
					cout << "Pushing regulating rod to position" << i->value << endl;
					if (sscanf(i->value.c_str(), "%zu", &dest))
						regulatingRod()->commandMove(dest);
					break;
				case setRegulatingSteps:
					cout << i->strCommand << " " << i->value << endl;
					if (sscanf(i->value.c_str(), "%zu", &dest))
						regulatingRod()->moveRodToStep(dest);
					break;
				case moveRegulatingRod:
					position = regulatingRod()->getPosition();
					if (sscanf(i->value.c_str(), "%zu", &dest)) {
						position += dest;
						cout << "Pushing regulating rod to position " << position << endl;
						regulatingRod()->commandMove(position);
					}
					break;
				case setShimRod:
					cout << "Pushing shim rod to position " << i->value << endl;
					if (sscanf(i->value.c_str(), "%zu", &dest))
						shimRod()->commandMove(dest);
					break;
				case setSafetyRod:
					cout << "Pushing safety rod to position " << i->value << endl;
					if (sscanf(i->value.c_str(), "%zu", &dest))
						safetyRod()->commandMove(dest);
					break;
				case commands::setAlpha0:
					cout << i->strCommand << " " << i->value << endl;
					setAlpha0(stod(i->value));
					break;
				case commands::setAlphaAtT1:
					cout << i->strCommand << " " << i->value << endl;
					setAlphaPeak(stod(i->value));
					break;
				case commands::setAlphaT1:
					cout << i->strCommand << " " << i->value << endl;
					setAlphaTempPeak(stod(i->value));
					break;
				case commands::setAlphaK:
					cout << i->strCommand << " " << i->value << endl;
					setAlphaSlope(stod(i->value));
					break;
				case setStablePower:
					cout << "Pushing stable state ..." << i->value << endl;
					pushStableState(stod(i->value));
					regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);
					scram(ScramSignals::None);
					simulatorTime += DT_STEP;
					last_sample_number = 1;
					break;
				case setSimulationSpeed:
					cout << "Setting simulation speed to: " << i->value << endl;
					setSpeedFactor(std::stod(i->value));
					break;
				case setSimulationMode:
					cout << "Setting simulation mode to: " << i->value << endl;
					if (i->value == "Manual")
						regulatingRod()->setOperationMode(ControlRod::OperationModes::Manual);
					if (i->value == "Automatic")
						regulatingRod()->setOperationMode(ControlRod::OperationModes::Automatic);
					if (i->value == "Pulse")
						regulatingRod()->setOperationMode(ControlRod::OperationModes::Pulse);
					if (i->value == "Simulation")
						regulatingRod()->setOperationMode(ControlRod::OperationModes::Simulation);
					break;
				case holdPower:
					cout << "Holding power at: " << i->value << endl;
					regulatingRod()->setOperationMode(ControlRod::OperationModes::Automatic);
					setPowerHold(stod(i->value));
					break;
				case saveToFile:
					std::cout << "Saving data to file: " << i->value << endl;
					dataToFile(i->value);
					break;
				case exitSimulator:
					std::cout << "Exiting simulator" << endl;
					std::exit(0);
					break;
				case firePulse:
					std::cout << "Fireing pulse rod" << endl;
					beginPulse();
					break;
				case setDataLogDivider:
					cout << i->strCommand << " " << i->value << endl;
					data_division = stod(i->value);
					break;
				case setCvCoeffPropA:
					coefficients = getHeatCpConstants();
					coefficients.first = stod(i->value);
					setHeatCpConstants(coefficients);
					break;
				case setCvCoeffPropB:
					coefficients = getHeatCpConstants();
					coefficients.second = stod(i->value);
					setHeatCpConstants(coefficients);
					break;
				}
				i = scriptCommands.erase(i);
			}
			else {
				++i;
			}
		}
	}
}
/*
Processes pulse data and saves the results to a buffer called "powerExtremes
*/
void Simulator::addPowerExtremes()
{
	try {
		size_t cur_index = getCurrentIndex();
		if (last_sample_number != 0) {
			size_t dataStart = iterations_total - last_sample_number;
			// Check if this is the first calculation
			if (dataStart == 1) {
				powerExtremes->push_back(PowerExtreme((int)std::floor(std::log10(getCurrentPower())), time_[getOldestIndex()]));
				dataStart++;
			}

			// Iterate through the new data, searching for order changes
			double power_i;
			for (int i = (int)last_sample_number - 1; i >= 0; i--) {
				power_i = powerFromNeutrons(state_vector_[0][shiftIndex(getCurrentIndex(), -i)]);
				if (power_i == 0.) {
					bool setZero = false;
					if (powerExtremes->size()) {
						setZero = !powerExtremes->back().isZero;
					}
					else {
						setZero = !trailingExtreme.isZero;
					}
					if (setZero) {
						PowerExtreme zero = PowerExtreme();
						zero.when = time_[shiftIndex(cur_index, -i)];
						powerExtremes->push_back(zero);
					}
				}
				else {
					int order = (int)floor(log10(power_i));
					int lastOrder = powerExtremes->size() ? powerExtremes->back().order : trailingExtreme.order;

					// If the order changed, save the index and new order
					if (order != lastOrder) {
						powerExtremes->push_back(PowerExtreme(order, time_[shiftIndex(getCurrentIndex(), -i)]));
					}
				}

			}
		}
	}
	catch (exception e) {
		cerr << "Simulator.recalculatePowerExtremes() error" << endl;
	}
}

void Simulator::checkPulsingStatus() {
	// Check if there is a pulse happening right now
	if (pulsing) {
		const size_t currentIdx = getCurrentIndex();
		if (time_[currentIdx] - time_[pulse_start] >= 5.) { // check if pulse is finished
			pulsing = false;
			if(autoScramAfterPulse) scram(User); // automatic SCRAM after 5 seconds

			double currentPower = state_vector_[0][currentIdx];
			// Calculate FWHM
			int range[2] = { -1, -1 };
			for (int i = 0; i < 5000; i++) {
				if (range[0] < 0) {
					if (state_vector_[0][shiftIndex(currentIdx, i - 4999)] > (pulse_maxP + currentPower) / 2.) 
						range[0] = i - 1;
				}
				else {
					if (state_vector_[0][shiftIndex(currentIdx, i - 4999)] < (pulse_maxP + currentPower) / 2.) {
						range[1] = i - 1;
						break;
					}
				}
			}
			pulse_FWHM = static_cast<double>(range[1] - range[0]) * DT_STEP;
			// Calculate final power
			pulse_maxP = powerFromNeutrons(pulse_maxP);

			//tempMode = TemperatureMode::Asymptotic; // Revert from FH to the original model

			if (pulseCallback) { // Call pulse callback method if required
				PulseData data = PulseData();
				data.peakPower = pulse_maxP;
				data.timeAtMax = time_at_peak;
				data.FWHM = pulse_FWHM;
				data.releasedEnergy = pulse_energy;
				data.maxFuelTemp = pulse_maxT;
				data.powerBeforeSCRAM = powerFromNeutrons(currentPower);
				data.pulseStartIndex = pulse_start;
				pulseCallback(data);
			}
		}
	}
}
