#include "pch.h"
#include "OrbisEyeCam.h"

#include <stdio.h>
#include <direct.h>

using namespace orbiseye;

std::string getCurrentFirmwarePath()
{
	char moduleFileName[MAX_PATH];
	GetModuleFileNameA(NULL, moduleFileName, MAX_PATH);
	
	std::string executablePath(moduleFileName);
	size_t lastSlash = executablePath.find_last_of("\\");
	std::string executableDir = executablePath.substr(0, lastSlash + 1);
	
	executableDir.append("firmware_discord_and_gamma_fix.bin");
	return executableDir;
}

LONG __cdecl
_tmain(
    LONG     Argc,
    LPTSTR * Argv
    )
/*++

Routine description:

    Firmware loader for Usb Boot device using WinUSB

--*/
{
    

    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Argv);

	orbiseye::OrbisEyeCam::OrbisEyeCamRef eye;


	std::vector<OrbisEyeCam::OrbisEyeCamRef> devices(OrbisEyeCam::getDevices());

	if (devices.size() == 1)
	{
		eye = devices.at(0);

		//eye->firmware_path = "C:\\pathtoyourfirmware\\firmware.bin"; or use getCurrentFirmwarePath get .exe current path and add firmware.bin
		eye->firmware_path = getCurrentFirmwarePath();


		eye->firmware_upload();

	}
	else
	{
		debug("Usb Boot device not found...\n");
	}


	return 0;
   
}
