/******************************************************************************
* Copyright (C) 2019 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
* @file xaiegbl.c
* @{
*
* This file contains the global initialization functions for the Tile.
* This is applicable for both the AIE tiles and Shim tiles.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------  -------- -----------------------------------------------------
* 1.0   Tejus   09/24/2019  Initial creation
* 1.1   Tejus   10/22/2019  Enable AIE initilization
* 1.2   Tejus   06/09/2020  Call IO init api from XAie_CfgInitialize
* 1.3   Tejus   06/10/2020  Add api to change backend at runtime.
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xaie_io.h"
#include "xaiegbl.h"
#include "xaiegbl_defs.h"
#include "xaiegbl_regdef.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/**************************** Macro Definitions ******************************/

/************************** Variable Definitions *****************************/
extern XAie_TileMod AieMod[XAIEGBL_TILE_TYPE_MAX];
extern XAie_TileMod Aie2Mod[XAIEGBL_TILE_TYPE_MAX];

/************************** Function Definitions *****************************/
/*****************************************************************************/
/**
*
* This is the global initialization function for all the tiles of the AIE array
* The function sets up the Device Instance pointer with the appropriate values
* from the ConfigPtr.
*
* @param	InstPtr - Global AIE instance structure.
* @param	ConfigPtr - Global AIE configuration pointer.
*
* @return	XAIE_OK on success and error code on failure
*
* @note		None.
*
******************************************************************************/
AieRC XAie_CfgInitialize(XAie_DevInst *InstPtr, XAie_Config *ConfigPtr)
{
	AieRC RC;

	if((InstPtr == XAIE_NULL) || (ConfigPtr == XAIE_NULL)) {
		XAieLib_print("Error %d: Invalid input arguments\n",
				XAIE_INVALID_ARGS);
		return XAIE_INVALID_ARGS;
	}

	if(InstPtr->IsReady)
		return XAIE_OK;

	/* Initialize device property according to Device Type */
	if(ConfigPtr->AieGen == XAIE_DEV_GEN_AIE2) {
		InstPtr->DevProp.DevMod = Aie2Mod;
		InstPtr->DevProp.DevGen = XAIE_DEV_GEN_AIE2;
	} else if(ConfigPtr->AieGen == XAIE_DEV_GEN_AIE) {
		InstPtr->DevProp.DevMod = AieMod;
		InstPtr->DevProp.DevGen = XAIE_DEV_GEN_AIE;
	} else {
		XAieLib_print("Error %d: Invalid device\n",
				XAIE_INVALID_DEVICE);
		return XAIE_INVALID_DEVICE;
	}

	InstPtr->IsReady = XAIE_COMPONENT_IS_READY;
	InstPtr->DevProp.RowShift = ConfigPtr->RowShift;
	InstPtr->DevProp.ColShift = ConfigPtr->ColShift;
	InstPtr->BaseAddr = ConfigPtr->BaseAddr;
	InstPtr->NumRows = ConfigPtr->NumRows;
	InstPtr->NumCols = ConfigPtr->NumCols;
	InstPtr->ShimRow = ConfigPtr->ShimRowNum;
	InstPtr->MemTileRowStart = ConfigPtr->MemTileRowStart;
	InstPtr->MemTileNumRows = ConfigPtr->MemTileNumRows;
	InstPtr->AieTileRowStart = ConfigPtr->AieTileRowStart;
	InstPtr->AieTileNumRows = ConfigPtr->AieTileNumRows;

	RC = XAie_IOInit(InstPtr);
	if(RC != XAIE_OK) {
		return RC;
	}

	return XAIE_OK;
}

/*****************************************************************************/
/**
*
* This is the api to set the IO backend of the driver at runtime.
*
* @param	DevInst - Global AIE device instance pointer.
* @param	Backend - Backend IO type to switch to.
*
* @return	XAIE_OK on success and error code on failure.
*
* @note		None.
*
******************************************************************************/
AieRC XAie_SetIOBackend(XAie_DevInst *DevInst, XAie_BackendType Backend)
{
	AieRC RC;
	const XAie_Backend *CurrBackend, *NewBackend;

	if((DevInst == XAIE_NULL) ||
			(DevInst->IsReady != XAIE_COMPONENT_IS_READY)) {
		XAieLib_print("Error: Invalid Device Instance\n");
		return XAIE_INVALID_ARGS;
	}

	if(Backend == XAIE_IO_BACKEND_MAX) {
		XAieLib_print("Error: Invalid backend request \n");
		return XAIE_INVALID_ARGS;
	}

	/* Release resources for current backend */
	CurrBackend = DevInst->Backend;
	RC = CurrBackend->Ops.Finish((void *)(DevInst->IOInst));
	if(RC != XAIE_OK) {
		XAieLib_print("Error: Failed to close backend instance."
				"Falling back to backend %d\n",
				CurrBackend->Type);
		return RC;
	}

	/* Get new backend and initialize the backend */
	NewBackend = _XAie_GetBackendPtr(Backend);
	RC = NewBackend->Ops.Init(DevInst);
	if(RC != XAIE_OK) {
		XAieLib_print("Error: Failed to initialize backend %d\n",
				Backend);
		return RC;
	}

	XAieLib_print("LOG: Switching backend to %d\n", Backend);
	DevInst->Backend = NewBackend;

	return XAIE_OK;
}

/** @} */
