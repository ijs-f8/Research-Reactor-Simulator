#ifndef SCRIPT_COMMAND_H
#define SCRIPT_COMMAND_H
#include <string>
#include <iostream>

enum operation {
	SET,
	MOVE
};

enum commands {
	setRegulatingRod,
	moveRegulatingRod,
	setShimRod,
	setSafetyRod,
	setStablePower,
	setAlpha0,
	setAlphaAtT1,
	setAlphaT1,
	setAlphaK,
	saveToFile,
	exitSimulator,
	setSimulationSpeed,
	setSimulationMode,
	holdPower,
	firePulse,
	setDataLogDivider,
	setRegulatingSteps,
	setCvCoeffC,
	setCvCoeffPropA,
	setCvCoeffPropB
};



struct Command {
	double timed;
	std::string strCommand;
	commands command;
	std::string value;
};

commands hashit(std::string const& strCommand);
bool compareByTime(const Command& a, const Command& b);
std::istream& operator>>(std::istream& is, Command& p);
std::ostream& operator<<(std::ostream& os, const Command& p);



#endif // !SCRIPT_COMMAND_H