#include "Settings.h"
#include "Logging.h"
#include "Utility.h"
#include "Compatibility.h"

using Comp = Compatibility;

void Settings::Load()
{
	constexpr auto path = L"Data/SKSE/Plugins/AlchemyExpansion.ini";

	bool Ultimateoptions = false;

	CSimpleIniA ini;

	ini.SetUnicode();
	ini.LoadFile(path);

	{
		
		// debug
		Debug::EnableLog = ini.GetBoolValue("Debug", "EnableLogging", Debug::EnableLog);
		Logging::EnableLog = Debug::EnableLog;
		loginfo("[SETTINGS] {} {}", "EnableLogging", std::to_string(Debug::EnableLog));
		Debug::EnableLoadLog = ini.GetBoolValue("Debug", "EnableLoadLogging", Debug::EnableLoadLog);
		Logging::EnableLoadLog = Debug::EnableLoadLog;
		loginfo("[SETTINGS] {} {}", "EnableLoadLogging", std::to_string(Debug::EnableLoadLog));
		Debug::LogLevel = ini.GetLongValue("Debug", "LogLevel", Debug::LogLevel);
		Logging::LogLevel = Debug::LogLevel;
		loginfo("[SETTINGS] {} {}", "LogLevel", std::to_string(Debug::LogLevel));
		Debug::EnableProfiling = ini.GetBoolValue("Debug", "EnableProfiling", Debug::EnableProfiling);
		Logging::EnableProfiling = Debug::EnableProfiling;
		loginfo("[SETTINGS] {} {}", "EnableProfiling", std::to_string(Debug::EnableProfiling));
		Debug::ProfileLevel = ini.GetLongValue("Debug", "ProfileLevel", Debug::ProfileLevel);
		Logging::ProfileLevel = Debug::ProfileLevel;
		loginfo("[SETTINGS] {} {}", "ProfileLevel", std::to_string(Debug::LogLevel));
	}


	// save user settings, before applying adjustments
	Save();
	// Check for CACO
	{
		if (const uint32_t index = Utility::Mods::GetPluginIndex(Comp::CACO); index != 0x1) {
			loginfo("[SETTINGS] Complete Alchemy & Cooking Overhaul.esp is loaded, activating compatibility mode!");
			Compatibility::CACO::_CompatibilityCACO = true;
		}
	}
	// Check for Apothecary
	{
		if (const uint32_t index = Utility::Mods::GetPluginIndex(Comp::Apothecary); index != 0x1) {
			loginfo("[SETTINGS] Apothecary.esp is loaded, activating compatibility mode!");
			Compatibility::Apothecary::_CompatibilityApothecary = true;
		}
	}
	loginfo("[SETTINGS] checking for plugins end");
}


void Settings::Save()
{
	constexpr auto path = L"Data/SKSE/Plugins/AlchemyExpansion.ini";

	CSimpleIniA ini;

	ini.SetUnicode();

	// debug
	ini.SetBoolValue("Debug", "EnableLogging", Debug::EnableLog, "// Enables logging output. Use with care as logs may get very large.");
	ini.SetBoolValue("Debug", "EnableLoadLogging", Debug::EnableLoadLog, "// Enables logging output for plugin load, use if you want to \n"
																		 "// log rule issues");
	ini.SetLongValue("Debug", "LogLevel", Debug::LogLevel, "// 1 - layer 0 log entries, 2 - layer 1 log entries, 3 - layer 3 log entries, \n"
														   "// 4 - layer 4 log entries. Affects which functions write log entries, \n"
														   "// as well as what is written by those functions. ");
	ini.SetBoolValue("Debug", "EnableProfiling", Debug::EnableProfiling, "// Enables profiling output.");
	ini.SetLongValue("Debug", "ProfileLevel", Debug::ProfileLevel, "// 1 - only highest level functions write their executions times to \n"
																	"// the profile log, 2 - lower level functions are written, 3 - lowest level \n"
																	"// functions are written. Be aware that not all functions are supported \n"
																	"// as Profiling costs execution time.");

	
	ini.SaveFile(path);
}
