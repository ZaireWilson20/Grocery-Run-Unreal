// Copyright Epic Games, Inc. All Rights Reserved.

#include "GroceryRun.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, GroceryRun, "GroceryRun" );

class GroceryRunModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override{
		//Noesis::RegisterComponent<UserControls::NumericUpDown>();
		//Noesis::RegisterComponent<UserControls::ColorConverter>();
	}

	virtual void ShutdownModule() override{
		//Noesis::UnregisterComponent<UserControls::NumericUpDown>();
		//Noesis::UnregisterComponent<UserControls::ColorConverter>();
	}
};

//IMPLEMENT_MODULE(GroceryRunModule, GroceryRun);
