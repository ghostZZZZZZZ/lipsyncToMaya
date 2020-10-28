#include "lipSyncCmd.h"
#include "OVRLipSync.h"
#include "Waveform.h"


std::string visemeNames[ovrLipSyncViseme_Count] = {
	"sil", "PP", "FF", "TH", "DD",
	"kk", "CH", "SS", "nn", "RR",
	"aa", "E", "ih", "oh", "ou",
};


MStatus lipSyncCmd::doIt(const MArgList & args)
{

	MArgDatabase argData(syntax(), args);

	MStatus status = MS::kSuccess;
	offsetTime = MTime(0, MTime::uiUnit());



	aniCurveObjArray.clear();
	bool skipNotLip = true;
	if (argData.isFlagSet("-skipNotLip"))
	{
		argData.getFlagArgument("-skipNotLip", 0, skipNotLip);
	}


	
	if (argData.isFlagSet("-ctrlObj"))
	{
		//lipCtrlObject
		argData.getFlagArgument("-ctrlObj", 0, lipCtrlObject);

		
		
		for (unsigned int i = 0;i < ovrLipSyncViseme_Count;i++)
		{
			MPlug plug;
			MSelectionList selection;
			MObjectArray animObjs;

			status = selection.add(lipCtrlObject + "." + visemeNames[i].c_str());
			if (status != MStatus::kSuccess )
			{
				if(!skipNotLip)
				{ 
					MString ErrorInfo = MString("get plug Error not existe object : ") + lipCtrlObject + "." +  visemeNames[i].c_str();
					MGlobal::displayError(ErrorInfo);
					return status;
				}
				else {
					MObject nullObj;
					aniCurveObjArray.append(nullObj);
					continue;
				}
			}
			status = selection.getPlug(0, plug);
			if (status != MStatus::kSuccess)
			{
				MString ErrorInfo = MString("get plug Error : ")  +lipCtrlObject + "." + visemeNames[i].c_str();
				MGlobal::displayError(ErrorInfo);
				return status;
			}
			MAnimUtil::findAnimation(plug, animObjs,&status);
			if (animObjs.length() == 1)
			{
				aniCurveObjArray.append(animObjs[0]);
			}
			if (animObjs.length() == 0)
			{
				MFnAnimCurve anicurve;
				MObject aniObj = anicurve.create(plug);
				aniCurveObjArray.append(aniObj);
			}
			

		}
	}
	else {
		MGlobal::displayError("Error ctrl obj is none");
		return MS::kFailure;
	}
	

	if (argData.isFlagSet("-startTime"))
	{
		hasStartTime = true;
		argData.getFlagArgument("-startTime", 0, startTime);
		st = MTime(startTime, MTime::uiUnit());
		//st.setUnit(MTime::k100FPS);
	}
	if (argData.isFlagSet("-endTime"))
	{

		hasEndTime = true;
		argData.getFlagArgument("-endTime", 0, endTime);
		et = MTime(endTime, MTime::uiUnit());
		//et.setUnit(MTime::k100FPS);
	}
	if (argData.isFlagSet("-offset"))
	{
		argData.getFlagArgument("-offset", 0, offset);
		offsetTime = MTime(offset, MTime::uiUnit());
		//offsetTime.setUnit(MTime::k100FPS);
	}
	if (argData.isFlagSet("-soundFile"))
	{
		argData.getFlagArgument("-soundFile", 0, soundFile);
	}
	else {
		MGlobal::displayError("Error SoundFile is none");
		return MS::kFailure;
	}
	clearAnimKeys();
	if (!doLipSync())
	{
		status=MStatus::kFailure;
	}
	return status;
}

MStatus lipSyncCmd::undoIt()
{
	animCurveChange.undoIt();

	return MS::kSuccess;
}

MStatus lipSyncCmd::redoIt()
{
	animCurveChange.redoIt();

	return MS::kSuccess;
}

MSyntax lipSyncCmd::syntaxCreator()
{

	MSyntax syntax;

	syntax.addFlag("-st","-startTime",MSyntax::kLong);
	syntax.addFlag("-et","-endTime",MSyntax::kLong);
	syntax.addFlag("-sf","-soundFile",MSyntax::kString);
	syntax.addFlag("-co", "-ctrlObj", MSyntax::kString);
	syntax.addFlag("-of", "-offset", MSyntax::kLong);
	syntax.addFlag("snl", "-skipNotLip", MSyntax::kBoolean);
	//syntax.addFlag("-rk","");


	return syntax;
}

bool lipSyncCmd::doLipSync()
{
	MStatus status = MS::kSuccess;
	Waveform wav;
	//wav = loadWAV(soundFile.asChar());

	ovrLipSyncContext ctx;
	ovrLipSyncFrame frame = {};
	float visemes[ovrLipSyncViseme_Count] = { 0.0f };

	frame.visemes = visemes;
	frame.visemesLength = ovrLipSyncViseme_Count;

	try {
		wav = loadWAV(soundFile.asChar());
	}
	catch (const std::runtime_error& e) {
		MGlobal::displayError("Failed to load :" + soundFile  + "\nError:"+ e.what());
		return false;
	}
	//10毫秒一次， 1秒100次
	
	//auto bufferSize = static_cast<unsigned int>(wav.sampleRate * 1e-2) * wav.chanNo;

	//以当前设置帧率设置缓冲大小
	MTime onSectime(1, MTime::kSeconds);
	onSectime.setUnit(MTime::uiUnit());
	auto bufferSize = static_cast<unsigned int>(wav.sampleRate *(1.0f / onSectime.value()) ) * wav.chanNo;


	auto rc = ovrLipSync_Initialize(wav.sampleRate, bufferSize);
	if (rc != ovrLipSyncSuccess) {
		MString errorMessage;
		errorMessage.set(rc);
		MGlobal::displayError("Failed to initialize ovrLipSync engine :" + errorMessage);
		return false;
	}
	rc = ovrLipSync_CreateContextEx(&ctx, ovrLipSyncContextProvider_Enhanced, wav.sampleRate, true);
	if (rc != ovrLipSyncSuccess) {
		MString errorMessage;
		errorMessage.set(rc);
		MGlobal::displayError("Failed to create ovrLipSync context:" + errorMessage);
		return false;

	}

	for (auto offs(0u); offs  + bufferSize < wav.sampleCount ; offs += bufferSize) {
		if(wav.chanNo==1)
			rc = ovrLipSync_ProcessFrame(ctx, wav.floatData() + offs, &frame);
		if(wav.chanNo==2)
			rc = ovrLipSync_ProcessFrameInterleaved(ctx,wav.floatData() + offs,&frame);
		if (rc != ovrLipSyncSuccess) {
			MString errorMessage;
			errorMessage.set(rc);
			MGlobal::displayError("Failed to process audio frame: " + errorMessage);
			return false;
		}

		if (hasStartTime)
		{
			if (frame.frameNumber + offsetTime.value() < st.value())
				continue;
		}
		if (hasEndTime)
			if (frame.frameNumber + offsetTime.value() > et.value())
				break;
		for (int i = 0;i < ovrLipSyncViseme_Count;i++) {
			
			if (aniCurveObjArray[i].isNull())
				continue;
			MFnAnimCurve aniCurve(aniCurveObjArray[i]);
			aniCurve.addKey(MTime(frame.frameNumber, MTime::uiUnit()) + offsetTime, visemes[i], MFnAnimCurve::kTangentGlobal, MFnAnimCurve::kTangentGlobal, &animCurveChange,&status);
			//aniCurve.addKey(MTime(frame.frameNumber, MTime::k100FPS), visemes[i]);
			if (status != MStatus::kSuccess) {
				MGlobal::displayError("add key error");
				return false;
			}
			//aniCurve.remove(2, &animCurveChange);
		}
		
	}
	rc = ovrLipSync_DestroyContext(ctx);
	rc = ovrLipSync_Shutdown();
	return true;
}

void lipSyncCmd::clearAnimKeys()
{
	
	for (int i = 0;i < ovrLipSyncViseme_Count;i++)
	{
		if (aniCurveObjArray[i].isNull())
			continue;
		MFnAnimCurve aniCurve(aniCurveObjArray[i]);
		//unsigned lnumKeys = aniCurve.numKeys()+1;
		int fRemoveKeyIndex = -1;
		int lRemoveKeyIndex = -1;
		for (int k = 0;k < aniCurve.numKeys();k++)
		{
			MTime keyTime = aniCurve.time(k);
			if (keyTime.value() > startTime)
			{
				fRemoveKeyIndex = k;
				break;
			}
				
		}
		for (int k = aniCurve.numKeys()-1;k > 0;k--)
		{
			MTime keyTime = aniCurve.time(k);
			if (keyTime.value() < endTime)
			{
				MString sst, set;
				sst.set(keyTime.value());
				set.set(k);
				MGlobal::displayInfo(sst + " " + set);
				lRemoveKeyIndex = k;
				break;
			}
		}
		

		for (int k = fRemoveKeyIndex;k < lRemoveKeyIndex+1;k++)
		{
			aniCurve.remove(fRemoveKeyIndex, &animCurveChange);
			
		}

	}
}
