#pragma once
enum SimulationModes : std::uint8_t {
	None,
	SquareWaveMode,
	SineMode,
	SawToothMode
};

struct PeriodicalMode {
private:
	static const size_t p_num(int index) {
		const size_t a[] = { 0, 8, 498, 6 };
		return a[index];
	}
protected:
	float period; // The period in seconds
	float amplitude; // The amount of steps by which to move the rod
	float time = 0.f; // The current time of the simulation
	bool isPaused = false; // Boolean value indicating if the simulation is paused
	bool newPeriod = false;
	SimulationModes sim_mode = SimulationModes::None;

	double tracker[2] = {0., 0.};
public:
	PeriodicalMode(float period_, float amplitude_) { period = period_; amplitude = amplitude_; };
	~PeriodicalMode() {};

	void setPeriod(float value) {
		time = getPeriodTime() * value;
		period = value;
	}
	void setAmplitude(float steps) {
		amplitude = steps;
	}
	const float &getAmplitude() { return amplitude; }

	double* getTrackerArray() { return tracker; }

	const size_t num_points() { return p_num((int)sim_mode) + 2; }
	static const size_t num_points(SimulationModes sim_mode_) { return p_num((int)sim_mode_) + 2; }

	// Start the simulation from a desired point in time (0 to 1; 1 meaning the period end)
	void play(float startFrom = 0.) {
		isPaused = false;
		time = startFrom*period;
	}

	// Pauses the simulation
	void pause() {
		isPaused = true;
	}

	// Resets the simulation time
	void reset() {
		time = 0.;
	}

	void handleAddTime(float dt) {
		if (!isPaused) {
			time += dt;
			newPeriod = time > period;
			time = fmodf(time, period);
			tracker[0] = (double)time;
			tracker[1] = (double)time;
		}
	}

	// Returns a value indicating the time of one period (0 - start, 1 - end)
	float getPeriodTime() {
		return fmod(time, period) / period;
	}

	const float &getPeriod() { return period; }

	bool isNewPeriod() { return newPeriod; }

	bool getPaused() { return isPaused; }

	virtual float getCurrentOffset(float /*t*/ = -1) { return 0.f; }

	virtual float getRelTimeOfPoint(size_t p_i) { return (float)p_i; }

	virtual float getRelOffsetOfPoint(size_t /*p_i*/) { return 0.f; }

	void fillXYaxis(float* x_axis, float* y_axis) {
		size_t nm = num_points() - 1;
		float relTime;
		x_axis[0] = 0.f;
		y_axis[0] = 0.f;
		x_axis[nm] = period;
		y_axis[nm] = 0.f;
		for (size_t i = 1; i < nm; i++) {
			relTime = getRelTimeOfPoint(i - 1);
			x_axis[i] = relTime * period;
			y_axis[i] = getRelOffsetOfPoint(i - 1) * amplitude;
		}
	}
	void fillXYaxis(double* x_axis, float* y_axis) {
		size_t nm = num_points() - 1;
		float relTime;
		x_axis[0] = 0.;
		y_axis[0] = 0.f;
		x_axis[nm] = (double)period;
		y_axis[nm] = 0.f;
		for (size_t i = 1; i < nm; i++) {
			relTime = getRelTimeOfPoint(i - 1);
			x_axis[i] = (double)relTime * period;
			y_axis[i] = getRelOffsetOfPoint(i - 1) * amplitude;
		}
	}
	void fillXYaxis(double* x_axis, double* y_axis) {
		size_t nm = num_points() - 1;
		float relTime;
		x_axis[0] = 0.;
		y_axis[0] = 0.;
		x_axis[nm] = (double)period;
		y_axis[nm] = 0.;
		for (size_t i = 1; i < nm; i++) {
			relTime = getRelTimeOfPoint(i - 1);
			x_axis[i] = (double)relTime * period;
			y_axis[i] = (double)getRelOffsetOfPoint(i - 1) * amplitude;
		}
	}

};

struct SquareWave : public PeriodicalMode {
public:
	float rodSpeed = 0.f;
	float xIndex[4] = { 0.f, 0.5f,0.5f,1.f };
	SquareWave(float period_, float amplitude_) : PeriodicalMode(period_, amplitude_) { sim_mode = SimulationModes::SquareWaveMode; }

	float getCurrentOffset(float t) override {
		if(t < 0) t = getPeriodTime();
		if (rodSpeed) {
			if (t <= xIndex[0]) { return 0.f; }
			else if (t <= xIndex[1]) { return std::min(amplitude, (t - xIndex[0])*getPeriod()*rodSpeed); }
			else if (t <= xIndex[2]) { return std::max(0.f, getRelOffsetOfPoint(2)*amplitude - (t - xIndex[1])*getPeriod()*rodSpeed); }
			else if (t <= xIndex[3]) { return -std::min(amplitude, (t - xIndex[2])*getPeriod()*rodSpeed); }
			else { return std::min(0.f, getRelOffsetOfPoint(6)*amplitude + (t - xIndex[3])*getPeriod()*rodSpeed); }
		}
		else {
			if (t >= xIndex[0] && t < xIndex[1]) {
				return amplitude;
			}
			else if (t >= xIndex[2] && t < xIndex[3]) {
				return -amplitude;
			}
			else {
				return 0.f;
			}
		}
	}

	float getRelTimeOfPoint(size_t p_i) override {
		if (rodSpeed) {
			if (p_i % 2) {
				float stepTime = amplitude / (rodSpeed * getPeriod());
				return std::min((p_i == 7) ? 1.f : xIndex[(p_i + 1) / 2], xIndex[p_i / 2] + stepTime);
			}
			else {
				return xIndex[p_i / 2];
			}
		}
		else {
			return xIndex[p_i/2];
		}
	}
	float getRelOffsetOfPoint(size_t p_i) override {
		if ((p_i % 4) % 3) {
			if (rodSpeed) {
				if (p_i < 3) {
					return std::min((xIndex[1] - xIndex[0])*getPeriod()*rodSpeed / amplitude, 1.f);
				}
				else {
					return -std::min((xIndex[3] - xIndex[2])*getPeriod()*rodSpeed / amplitude, 1.f);
				}
			}
			else {
				return (p_i < 3) ? 1.f : -1.f;
			}
		}
		else {
			return 0.f;
		}
	}
};

// Linspaced sine
struct Sine : public PeriodicalMode {
public:
	enum class SineMode {
		Normal,
		Quadratic,
	};
	SineMode mode = SineMode::Normal;
	Sine(float period_, float amplitude_) : PeriodicalMode(period_, amplitude_) { sim_mode = SimulationModes::SineMode; }

	float getCurrentOffset(float t) override {
		if(t < 0) t = getPeriodTime();
		double sinVal = 2. * M_PI * t;
		switch (mode) {
		case SineMode::Normal:
			return (float)(sin(sinVal) * amplitude);
			break;
		case SineMode::Quadratic:
			return (float)(pow(sin(sinVal), 2) * amplitude);
			break;
		default:
			return 0.f;
			break;
		}
	}

	float getRelTimeOfPoint(size_t p_i) override { return (p_i + 1) / 499.f; }
	float getRelOffsetOfPoint(size_t p_i) override {
		double sinVal = 2. * M_PI * getRelTimeOfPoint(p_i);
		switch (mode) {
		case SineMode::Normal:
			return (float)sin(sinVal);
			break;
		case SineMode::Quadratic:
			return (float)pow(sin(sinVal), 2);
			break;
		default:
			return 0.f;
			break;
		}
	}
};

struct SawTooth : public PeriodicalMode {
public:
	float xIndex[6] = { 0.f, 0.25f, 0.5f, 0.5f, 0.75f, 1.f };
	SawTooth(float period_, float amplitude_) : PeriodicalMode(period_, amplitude_) { sim_mode = SimulationModes::SawToothMode; }

	float getCurrentOffset(float t) override {
		if(t < 0) t = getPeriodTime();
		if (t < xIndex[0]) {
			return 0.f;
		}
		else if (t <= xIndex[1]) {
			return amplitude*(t - xIndex[0]) / (xIndex[1] - xIndex[0]);
		}
		else if (t < xIndex[2]) {
			return amplitude*(1.f - (t - xIndex[1]) / (xIndex[2] - xIndex[1]));
		}
		else if (t < xIndex[3]) {
			return 0.f;
		}
		else if (t <= xIndex[4]) {
			return -amplitude*(t - xIndex[3]) / (xIndex[4] - xIndex[3]);
		}
		else if (t < xIndex[5]) {
			return -amplitude*(1.f - (t - xIndex[4]) / (xIndex[5] - xIndex[4]));
		}
		else {
			return 0.f;
		}
	}

	float getRelTimeOfPoint(size_t p_i) override { return xIndex[p_i]; }
	float getRelOffsetOfPoint(size_t p_i) override {
		switch(p_i) {
		case 1: return 1.f; break;
		case 4: return -1.f; break;
		default: return 0.f; break;
		}
	}
};
