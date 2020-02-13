#pragma once
#include <deque>
#define _USE_MATH_DEFINES
#define INTEGRAL_CURVE_POINTS	10000

#include <math.h>
#include <PeriodicalMode.h>
#include <Settings.h>

using std::deque;
using std::string;
using std::abs;

class ControlRod {
public:
	enum OperationModes : std::uint8_t {
		Manual,
		Simulation,
		Automatic,
		Pulse
	};
	
	enum CommandType : std::uint8_t {
		None,
		Top,
		Bottom,
		Fixed
	};
protected:
	bool enabled;
	float rod_exact_position;
	float rod_actual_position;
	float manual_command;
	float rod_exact_command;
	size_t rod_steps;
	float rod_worth;
	float rod_speed;
	const float fire_acc = 50.f;
	bool fireing;
	float parameters[2] = { 0.f, 1.f };
	float* stepData;
	double* derivativeTable;
	double scramTime;
	double timeSinceScram;
	float positionAtScram;
	float scramDuration;
	string name = "Control Rod";
	CommandType rodCommand = CommandType::None;

	OperationModes mode = OperationModes::Manual;
	SimulationModes simulationMode = SimulationModes::SquareWaveMode;

	SquareWave* sqw;
	Sine* sinMode;
	SawTooth* saw;

	float simulationStartPosition;

	double fireTimer = 0.;
	float holdPcm;
	bool hasData = false;
	int maxIndex = 0;
public:
	static const size_t dataPoints = INTEGRAL_CURVE_POINTS + 1;

	void recalculateStepData() {
		float* xPoints = new float[INTEGRAL_CURVE_POINTS + 1];
		float* yPoints = new float[INTEGRAL_CURVE_POINTS + 1];
		for (size_t i = 0; i <= INTEGRAL_CURVE_POINTS; i++) {
			float t = i / (float)INTEGRAL_CURVE_POINTS;
			xPoints[i] = t * (3 * (1 - t) * ((1 - t) * parameters[0] + t * parameters[1]) + pow(t, 2));
			yPoints[i] = t * (3 * (1 - t) * t + pow(t, 2));
		}
		double stepSize = 1. / rod_steps;
		if (hasData) {
			delete[] stepData, derivativeTable;
			hasData = false;
		}
		stepData = new float[rod_steps + 1];
		derivativeTable = new double[rod_steps + 1];
		hasData = true;
		stepData[0] = 0.;
		stepData[rod_steps] = 1.;
		derivativeTable[0] = 0.f;
		derivativeTable[rod_steps] = 0.f;
		size_t currentIndex = 0;
		for (size_t i = 1; i < rod_steps; i++) {
			while (xPoints[currentIndex] < stepSize*i) currentIndex++;
			double diff = xPoints[currentIndex] - xPoints[currentIndex - 1];
			double alpha = (stepSize * i - xPoints[currentIndex - 1]) / diff;
			stepData[i] = (float)(yPoints[currentIndex - 1] * (1 - alpha) + yPoints[currentIndex] * alpha);
			derivativeTable[i] = static_cast<double>(stepData[i] - stepData[i - 1]);
			if (derivativeTable[i] > derivativeTable[maxIndex]) {
				maxIndex = (int)i;
			}
		}
		delete[] xPoints, yPoints;
	}

	size_t maxDerivative() { return (size_t)maxIndex; }

	float *stepDataArray() {
		if(hasData) return stepData;
		return nullptr;
	}

	double *derivativeArray() {
		if (hasData) return derivativeTable;
		return nullptr;
	}

	SquareWave* squareWave() { return sqw; }
	Sine* sine() { return sinMode; }
	SawTooth* sawTooth() { return saw; }

	PeriodicalMode* getMode(SimulationModes sim) {
		switch (sim) {
		case SimulationModes::SquareWaveMode:
			return sqw;
			break;
		case SimulationModes::SineMode:
			return sinMode;
			break;
		case SimulationModes::SawToothMode:
			return saw;
			break;
		}
		return nullptr;
	}

	ControlRod(bool recalculateSteps = true) {
		resetRod();
		if (recalculateSteps) recalculateStepData();
		sqw = new SquareWave(SIMULATION_MODE_PERIOD_DEFAULT, SIMULATION_MODE_AMPLITUDE_DEFAULT);
		sinMode = new Sine(SIMULATION_MODE_PERIOD_DEFAULT, SIMULATION_MODE_AMPLITUDE_DEFAULT);
		saw = new SawTooth(SIMULATION_MODE_PERIOD_DEFAULT, SIMULATION_MODE_AMPLITUDE_DEFAULT);
	}

	~ControlRod() { delete sqw; delete sinMode; delete saw; if(hasData) delete [] stepData, derivativeTable; }

	// Sets control rod to initial state
	void resetRod() {
		fireTimer = 0.;
		holdPcm = 0.;
		simulationStartPosition = 0.f;
		scramDuration = .5f;
		positionAtScram = 0.f;
		timeSinceScram = 0.;
		enabled = true;
		rod_exact_position = 0.f;
		rod_actual_position = 0.f;
		manual_command = 0.f;
		rod_exact_command = 0.f;
		fireing = false;
		scramTime = -1.;
	}

	void setRodSteps(size_t steps, bool recalculateSteps = true) { rod_steps = steps; if (recalculateSteps) recalculateStepData(); }
	size_t * const getRodSteps() { return &rod_steps; }
	void commandMove(size_t destination) { commandMove((float)destination); }
	void commandMove(float destination) { rod_exact_command = destination; rodCommand = CommandType::Fixed; }
	void commandToTop() { rodCommand = CommandType::Top; }
	void commandToBottom() { rodCommand = CommandType::Bottom; }
	void clearCommands(CommandType onlyIf = CommandType::None) {
		if ((onlyIf != CommandType::None) && (onlyIf != rodCommand)) return;
		rod_exact_command = rod_exact_position;
		rodCommand = CommandType::None;
	}
	void moveRodToStep(float position, bool force = false)
	{
		rod_exact_position = std::max(std::min(position, (float)rod_steps), 0.f);
		if (enabled && !fireing) {
			rod_actual_position = rod_exact_position;
		}
		if (force) {
			rod_actual_position = position;
			rod_exact_position = position;
		}
	}
	void fire(bool status) {
		if (status) {
			if (mode == OperationModes::Pulse || rod_exact_command == 0.f) { // functionallity on JSI TRIGA
				setEnabled(true);
			}
			if (mode == Pulse) {
				fireing = true;
				fireTimer = 0.;
			}
		}
		else {
			fireing = false;
		}
	}
	float getPositionAtPcm(float position_pcm) {
		position_pcm /= rod_worth;
		if (position_pcm >= 1.f) {
			return (float)rod_steps;
		}
		else if (position_pcm <= 0.f) {
			return 0.f;
		}
		else {
			size_t i = 0;
			while (stepData[i] < position_pcm)i++;
			if (stepData[i] == position_pcm) {
				return (float)i;
			}
			else {
				return (position_pcm - stepData[i - 1]) / (stepData[i] - stepData[i - 1]) + i - 1;
			}
		}
	}
	void setRodWorth(float worth) { rod_worth = worth; }
	const float &getRodWorth() { return rod_worth; }
	void setParameter(size_t index, float value, bool recalculateSteps = true) { parameters[index] = value; if (recalculateSteps) recalculateStepData(); }
	float getPCMat(float position) {
		position = std::min(position, (float)rod_steps);
		position = std::max(position, 0.f);
		size_t floorP = (size_t)floor(position);
		size_t ceilP = (size_t)ceil(position);
		double pcmRelative;
		if (floorP == ceilP) {
			pcmRelative = stepData[floorP];
		}
		else {
			pcmRelative = static_cast<double>(stepData[floorP] * (ceilP - position) + stepData[ceilP] * (position - floorP));
		}
		return (float)(pcmRelative * rod_worth);
	}
	float getCurrentPCM() {
		return getPCMat(rod_actual_position);
	}
	size_t getPosition() { return (size_t)round(rod_exact_position); };
	bool * const isEnabled() { return &enabled; }
	void setEnabled(bool status, bool force = false)
	{
		if (status) {
			if ((rod_actual_position == 0.f) && (rod_exact_position == 0.f) && (mode == OperationModes::Manual) || force) {
				enabled = true;
			}
		}
		else {
			if (rod_actual_position > 0.f) {
				scramRod();
			}
			else {
				enabled = false;
			}
		}
	}
	float * const getExactPosition() { return &rod_exact_position; };
	float * const getActualPosition() { return &rod_actual_position; };
	const string &getRodName() { return name; }
	void setRodName(string value) { name = value; }
	const OperationModes &getOperationMode() { return mode; }
	const float &getAutoPcm() { return holdPcm; }
	void setAutoPcm(float pcm) { holdPcm = pcm; }
	const CommandType &getCommandType() { return rodCommand; }
	float getPosFromPcm(float pcm) {
		pcm = std::min(pcm, rod_worth);
		pcm = std::max(0.f, pcm);
		pcm /= rod_worth;
		int i = 0;
		while (stepData[i] < pcm) i++;
		if (stepData[i] == pcm) {
			return (float)i;
		}
		else {
			float a = (stepData[i] - pcm) / (stepData[i] - stepData[i - 1]);
			return (i - 1) * a + i * (1.f - a);
		}
	}
	void scramRod() {
		if (scramTime <= 0.) {
			scramTime = scramDuration * std::sqrt(rod_actual_position / rod_steps);
			enabled = false;
			positionAtScram = rod_actual_position;
			clearCommands();
			timeSinceScram = 0.;
		}
	}

	/* Gets or sets the speed of the control rod(s) in pcm/second.
	Setting the speed to a non-positive number makes the control rods move infinitely fast.
	Default is 0.*/
	void setRodSpeed(float value) {
		rod_speed = value;
		if(sqw){
			if (sqw->rodSpeed) sqw->rodSpeed = value;
		}
	}

	const float &getRodSpeed() { return rod_speed; }

	// Void for moving the control rod
	void refreshRod(double dt) {
		// Check if the rod is being scrammed
		if (scramTime > 0.) {
			timeSinceScram += dt;
			timeSinceScram = std::min(timeSinceScram, scramTime);
			rod_actual_position = (float)(positionAtScram * (1. - std::pow(timeSinceScram / scramTime, 2.)));
			if (timeSinceScram == scramTime) {
				scramTime = -1.;
			}
		}
		else {
			// If it's not being scrammed, move it accordinglly
			switch (mode) {
			case OperationModes::Manual:
				break;
			case OperationModes::Pulse:
				break;
			case OperationModes::Simulation:
			{
				PeriodicalMode* waveform = getMode(simulationMode);
				if (!waveform->getPaused()) {
					moveRodToStep(simulationStartPosition + waveform->getCurrentOffset());
					waveform->handleAddTime((float)dt);}
			}
				break;
			case OperationModes::Automatic:
				break;
			}


			float localCommand = rod_exact_command;
			if (rodCommand == CommandType::Bottom) {
				localCommand = 0.f;
			}
			else if (rodCommand == CommandType::Top) {
				localCommand = (float)rod_steps;
			}
			// Move the rod
			if ((rod_exact_position != localCommand) && (mode == OperationModes::Manual
				|| mode == OperationModes::Automatic || mode == OperationModes::Pulse))
			{
				if (rod_speed == 0.f && enabled) {
					moveRodToStep(localCommand);
				}
				else {
					if (!fireing) {
						float rod_move = (float)(dt * rod_speed);
						if (localCommand > rod_exact_position) {
							moveRodToStep(std::min(rod_exact_position + rod_move, localCommand));
						}
						else {
							moveRodToStep(std::max(rod_exact_position - rod_move, localCommand));
						}
					}
				}
			}
			if (fireing) {
				fireTimer += dt;
				float rod_pos = (float)(pow(fireTimer, 2) * fire_acc * .5 * rod_steps);
				if (rod_pos >= rod_exact_position) {
					rod_actual_position = rod_exact_position;
					fireing = false;
					clearCommands();
				}
				else {
					rod_actual_position = rod_pos;
				}
			}
		}

	}

	void setOperationMode(OperationModes value) {
		if (mode != value) {
			if (mode == OperationModes::Simulation) {
				moveRodToStep(simulationStartPosition);
				getMode(simulationMode)->reset();
			}
			clearCommands();
			if (value == OperationModes::Simulation) {

				simulationStartPosition = rod_exact_position;
			}
			mode = value;
			if (value == OperationModes::Pulse) {
				setEnabled(false);
			}
			else {
				setRodSpeed(10);
			}
		}
	}
	void setSimulationMode(SimulationModes value) {
		if (simulationMode != value) {
			if (mode == OperationModes::Simulation) {
				moveRodToStep(simulationStartPosition);
			}
			simulationMode = value;
			getMode(simulationMode)->reset();
		}
	}

	SimulationModes getSimulationMode() {
		return simulationMode;
	}

};
