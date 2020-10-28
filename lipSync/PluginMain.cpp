#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>



#include "lipSyncCmd.h"


MStatus initializePlugin(MObject obj)
{

	MStatus status = MStatus::kSuccess;
	MFnPlugin plugin(obj, "lipSync", "1.0", "any");

	MString errorString;
	status = plugin.registerCommand("lipSync", lipSyncCmd::creator, lipSyncCmd::syntaxCreator);
	if (!status)
	{
		MGlobal::displayError("Error registering lipSync");
		return status;
	}
	MString addmenu;
	addmenu +=
	"global proc doLipSync()\
	{\
		global string $gPlayBackSlider;\
		string $select[] = `ls - sl`;\
		string $ctrl = $select[0];\
		string $sound = `timeControl - query - sound $gPlayBackSlider`;\
		string $soundfile = `getAttr ($sound + \".filename\")`;\
		int $offset = 0;\
		$offset += `getAttr ($sound + \".offset\")`;\
		$offset += `getAttr ($sound + \".silence\")`;\
		int $startTime, $endTime;\
		float $selTimeRange[] = `timeControl - q - rangeArray $gPlayBackSlider`;\
		$startTime = $selTimeRange[0];\
		$endTime = $selTimeRange[1];\
		if ($endTime - 1 == $startTime) {\
			$startTime = `playbackOptions -q -min`;\
			$endTime = `playbackOptions -q -max`;\
		}\
		lipSync -co $ctrl -sf $soundfile -of $offset -st $startTime -et $endTime;\
	}\
	if(`menuItem -q -ex LipSyncMenuItem`){deleteUI LipSyncMenuItem;}\
	menuItem -l \"LipSync\" -ia steppedPreviewItem -command \"doLipSync()\" LipSyncMenuItem\
	";
	MGlobal::executeCommand(addmenu, false, false);
	return status;
}

MStatus uninitializePlugin(MObject obj)
{

	MStatus status = MStatus::kSuccess;
	MFnPlugin plugin(obj);
	status = plugin.deregisterCommand("lipSync");
	if (!status)
	{
		MGlobal::displayError("Error deregistering command lipSync");
		return status;
	}

	MGlobal::executeCommand("if(`menuItem -q -ex LipSyncMenuItem`){deleteUI LipSyncMenuItem;}",false,false);

	return status;
}