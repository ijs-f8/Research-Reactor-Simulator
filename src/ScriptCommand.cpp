#include <ScriptCommand.h>

commands hashit(std::string const& strCommand) {
	if (strCommand == "setRegulatingRod") return setRegulatingRod;
	if (strCommand == "moveRegulatingRod") return moveRegulatingRod;
	if (strCommand == "setShimRod") return setShimRod;
	if (strCommand == "setSafetyRod") return setSafetyRod;
	if (strCommand == "setStablePower") return setStablePower;
	if (strCommand == "setAlpha0") return setAlpha0;
	if (strCommand == "setAlphaAtT1") return setAlphaAtT1;
	if (strCommand == "setAlphaT1") return setAlphaT1;
	if (strCommand == "setAlphaK") return setAlphaK;
	if (strCommand == "saveToFile") return saveToFile;
	if (strCommand == "exitSimulator") return exitSimulator;
	if (strCommand == "setSimulationSpeed") return setSimulationSpeed;
	if (strCommand == "setSimulationMode") return setSimulationMode;
	if (strCommand == "holdPower") return holdPower;
	if (strCommand == "firePulse") return firePulse;
	if (strCommand == "setDataLogDivider") return setDataLogDivider;
	if (strCommand == "setRegulatingSteps") return setRegulatingSteps;
	if (strCommand == "setCvCoeffPropA") return setCvCoeffPropA;
	if (strCommand == "setCvCoeffPropB") return setCvCoeffPropB;
}


bool compareByTime(const Command& a, const Command& b) {
	return a.timed < b.timed;
}

std::istream& operator>>(std::istream& is, Command& p)
{
	Command c;
	std::string timestr;
	if (is >> c.timed >> c.strCommand >> c.value)
	{ 
		c.command = hashit(c.strCommand);
		p = c;
	}

	return is;
}

std::ostream& operator<<(std::ostream& os, const Command& p)
{
	os << p.timed << '\t' << p.strCommand << '\t' << p.value << std::endl;
	return os;
}
