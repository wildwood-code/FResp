/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : FResp_Settings.cpp
* Description:
*   Read and Set registry settings
*
* Created    : 11/13/2021
* Modified   : 11/13/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/

#include "FResp_Settings.h"
#include <Windows.h>
#include <winreg.h>
#include <stdlib.h>


/*******************************************************************************
* Function   : FResp_ReadRegSZ
* Arguments  : szKey      = registry key in HKCU (e.g., "SOFTWARE\\Company\\ProgName\\Settings\\"
*              szSetting  = registry value in the key (e.g., "OscopeResource")
*              szResult   = receives the result stored in the registry
*              szDefault  = default value (or NULL if no default)
* Returns    : true = success, false = failure
* Description:
*   Reads a setting from the registry and writes a default if non-existent.
*   If szDefault is NULL, then no setting is written to the registry.
*/
bool FResp_ReadRegSZ(char const* szKey, char const* szSetting, char* szResult, char const* szDefault)
{
	bool bResult = false;

	size_t nconv;
	wchar_t szwSettingsKey[MAX_KEY_LENGTH + 1];
	wchar_t szwSetting[MAX_SETTING_LENGTH + 1];
	wchar_t szwResult[MAX_RESULT_LENGTH + 1];
	DWORD cbData;

	// convert these to wide char strings
	mbstowcs_s(&nconv, szwSettingsKey, szKey, MAX_KEY_LENGTH);
	mbstowcs_s(&nconv, szwSetting, szSetting, MAX_SETTING_LENGTH);

	// try to read the value
	cbData = MAX_RESULT_LENGTH * sizeof(wchar_t);
	auto result = RegGetValueW(HKEY_CURRENT_USER, szwSettingsKey, szwSetting, RRF_RT_REG_SZ, nullptr, szwResult, &cbData);

	if (result == ERROR_SUCCESS)
	{	// read REG_SZ from registry, convert to szResult
		wcstombs_s(&nconv, szResult, (MAX_RESULT_LENGTH + 1) * sizeof(char), szwResult, MAX_RESULT_LENGTH * sizeof(char));
		bResult = true;
	}
	else if (szDefault)
	{	// create the key and value
		HKEY hKey;
		result = RegCreateKeyW(HKEY_CURRENT_USER, szwSettingsKey, &hKey);
		if (result == ERROR_SUCCESS)
		{
			mbstowcs_s(&nconv, szwResult, szDefault, MAX_RESULT_LENGTH);
			result = RegSetValueExW(hKey, szwSetting, 0, REG_SZ, (LPBYTE)szwResult, DWORD((wcslen(szwResult) + 1) * sizeof(wchar_t)));

			if (result == ERROR_SUCCESS)
			{	// copy to return value
				wcstombs_s(&nconv, szResult, (MAX_RESULT_LENGTH + 1) * sizeof(char), szwResult, MAX_RESULT_LENGTH * sizeof(char));
				bResult = true;
			}
			else
			{	// clear result to empty value
				bResult = false;
				szResult[0] = 0;
			}

			RegCloseKey(hKey);
		}
		else
		{	// clear result to empty value
			bResult = false;
			szResult[0] = 0;
		}
	}
	else
	{	// clear result to empty value
		bResult = false;
		szResult[0] = 0;
	}

	return bResult;
}


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/