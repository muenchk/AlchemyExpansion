#pragma once
#include "Settings.h"
#include <unordered_map>

class Compatibility
{
public:

	// apothecary
	static inline std::string Apothecary = "Apothecary.esp";

	// caco
	static inline std::string CACO = "Complete Alchemy & Cooking Overhaul.esp";
	// general section
private:
	/// <summary>
	/// Whether all objects for Apothecary have been found
	/// </summary>
	bool _loadedApothecary = false;
	/// <summary>
	/// Whether all objects for Complete Alchemy and Cooking Overhaul have been found
	/// </summary>
	bool _loadedCACO = false;

public:
	
	/// <summary>
	/// Returns a static Compatibility object
	/// </summary>
	/// <returns></returns>
	static Compatibility* GetSingleton();

	/// <summary>
	/// Loads all game items for the mods, and verifies that compatibility has been established
	/// </summary>
	void Load();

	/// <summary>
	/// Clears all loaded data from memory
	/// </summary>
	void Clear();

	/// <summary>
	/// returns whether compatibility for apothecary is enabled
	/// </summary>
	/// <returns></returns>
	bool LoadedApothecary()
	{
		return Settings::Compatibility::Apothecary::_CompatibilityApothecary && _loadedApothecary;
	}

	/// <summary>
	/// returns whether compatibility for caco is enabled
	/// </summary>
	/// <returns></returns>
	bool LoadedCACO()
	{
		return Settings::Compatibility::CACO::_CompatibilityCACO && _loadedCACO;
	}

	/// <summary>
	/// Registers Game Callbacks
	/// </summary>
	static void Register();
};
