#pragma once
#ifndef lipSync_lipSyncCmd_h
#define lipSync_lipSyncCmd_h

#include <maya/MPxCommand.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MAnimCurveChange.h>
#include <maya/MGlobal.h>
#include <maya/MObjectArray.h>
#include <maya/MArgList.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MTime.h>




class lipSyncCmd : public MPxCommand

{

public:

	lipSyncCmd() {};
	~lipSyncCmd() {};

	MStatus doIt(const MArgList&);

	bool isUndoable() const { return true; };

	MStatus undoIt();
	MStatus redoIt();

	static MSyntax syntaxCreator();

	static void* creator() { return new lipSyncCmd; }


private:
	bool hasStartTime = false;
	bool hasEndTime = false;
	MString soundFile,lipCtrlObject;

	int startTime, endTime;
	int offset = 0;
	MTime st, et,offsetTime;
	//MTime offsetTime(offset, MTime::uiUnit());
	MAnimCurveChange animCurveChange;
	MObjectArray aniCurveObjArray;


	bool doLipSync();
	void clearAnimKeys();



};





#endif