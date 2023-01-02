/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : FResp_Settings.h
* Description:
*   Read settings from registry and optionally write default if non-existent
*
* Created    : 11/13/2021
* Modified   : 01/02/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once

constexpr size_t MAX_KEY_LENGTH = 127;
constexpr size_t MAX_SETTING_LENGTH = 31;
constexpr size_t MAX_RESULT_LENGTH = 31;

bool FResp_ReadRegSZ(char const* szKey, char const* szSetting, char* szResult, char const* szDefault);


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/