#include "TCPAnalyzer.h"
#include "Controller.h" // for forward declaration

void TCPAnalyzer::updatePauseOutput(bool update)
{
	Controller::pauseOutput = update;
}
