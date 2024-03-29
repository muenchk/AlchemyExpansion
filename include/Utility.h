#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <utility>

#include "Settings.h"

#define Base(x) static_cast<uint64_t>(x)

enum AssocType
{
	kKeyword = 1,
	kFaction = 2,
	kRace = 4,
	kActor = 8,
	kNPC = 16,
	kItem = 32,
	kClass = 64,
	kCombatStyle = 128,
	kEffectSetting = 256,
};

/// <summary>
/// Provides generic functions
/// </summary>
class Utility
{
public:

	#pragma region SortingFunctions
	// comparator used to sort magnitude - duration - AlchemyItem* lists for maximum magnitude descending
	/// <summary>
	/// Compares the magnitude of two alchemy items
	/// </summary>
	/// <param name="first"></param>
	/// <param name="second"></param>
	/// <returns></returns>
	static bool SortMagnitude(std::tuple<float, int, RE::AlchemyItem*, AlchemicEffect> first, std::tuple<float, int, RE::AlchemyItem*, AlchemicEffect> second);
	#pragma endregion

	/// <summary>
	/// Returns a string showing [val] as Hexadecimal number
	/// </summary>
	/// <param name="val"></param>
	/// <returns></returns>
	static std::string GetHex(long val)
	{
		std::stringstream ss;
		ss << std::hex << val;
		return ss.str();
	}
	/// <summary>
	/// Returns a string showing [val] as Hexadecimal number
	/// </summary>
	/// <param name="val"></param>
	/// <returns></returns>
	static std::string GetHex(uint64_t val)
	{
		std::stringstream ss;
		ss << std::hex << val;
		return ss.str();
	}
	/// <summary>
	/// Returns a string showing [val] as Hexadecimal number
	/// </summary>
	/// <param name="val"></param>
	/// <returns></returns>
	static std::string GetHex(uint32_t val)
	{
		std::stringstream ss;
		ss << std::hex << val;
		return ss.str();
	}
	/// <summary>
	/// Returns a string showing [val] as Hexadecimal number
	/// </summary>
	/// <param name="val"></param>
	/// <returns></returns>
	static std::string GetHex(int val)
	{
		std::stringstream ss;
		ss << std::hex << val;
		return ss.str();
	}
	/// <summary>
	/// Returns a string showing [val] as Hexadecimal number with padding
	/// </summary>
	/// <param name="val"></param>
	/// <returns></returns>
	static std::string GetHexFill(uint32_t val)
	{
		std::stringstream ss;
		ss << std::setw(16) << std::hex << std::setfill('0') << val;
		return ss.str();
	}
	/// <summary>
	/// Returns a string showing [val] as Hexadecimal number with padding
	/// </summary>
	/// <param name="val"></param>
	/// <returns></returns>
	static std::string GetHexFill(uint64_t val)
	{
		std::stringstream ss;
		ss << std::setw(16) << std::hex << std::setfill('0') << val;
		return ss.str();
	}

	/// <summary>
	/// Returns a string representing the given form
	/// </summary>
	/// <param name="form"></param>
	/// <returns></returns>
	template <class T>
	static std::string PrintForm(T* form)
	{
		if (form == nullptr || form->GetFormID() == 0 || Logging::EnableGenericLogging == false)
			return "None";
		std::string plugin = "";
		if ((form->GetFormID() & 0xFF000000) != 0xFE000000) {
			plugin = Settings::pluginnames[(form->GetFormID() >> 24)];
		} else
			plugin = Settings::pluginnames[256 + (((form->GetFormID() & 0x00FFF000)) >> 12)];

		return std::string("[") + typeid(T).name() + "<" + Utility::GetHex(form->GetFormID()) + "><" + form->GetName() + "><" + plugin + ">]";
	}
	/// <summary>
	/// Returns a string representing the given form
	/// </summary>
	/// <param name="form"></param>
	/// <returns></returns>
	template <class T>
	static std::string PrintFormNonDebug(T* form)
	{
		if (form == nullptr || form->GetFormID() == 0)
			return "None";
		std::string plugin = "";
		if ((form->GetFormID() & 0xFF000000) != 0xFE000000) {
			plugin = Settings::pluginnames[(form->GetFormID() >> 24)];
		} else
			plugin = Settings::pluginnames[256 + (((form->GetFormID() & 0x00FFF000)) >> 12)];

		return std::string("[") + typeid(T).name() + "<" + Utility::GetHex(form->GetFormID()) + "><" + form->GetName() + "><" + plugin + ">]";
	}

	/// <summary>
	/// Converts all symbols in a string into lower case.
	/// </summary>
	/// <param name="s"></param>
	/// <returns></returns>
	static std::string ToLower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) { return (unsigned char)std::tolower(c); }  // correct
		);
		return s;
	}

	/// <summary>
	/// Converts an AlchemyEffect to a String
	/// </summary>
	/// <param name="ae"></param>
	/// <returns></returns>
	static std::string ToString(AlchemicEffect ae);

	/// <summary>
	/// Splits a string at a delimiter and optionally removes empty results
	/// </summary>
	/// <param name="delimiter"></param>
	/// <param name="removeEmpty"></param>
	/// <returns></returns>
	static std::vector<std::string> SplitString(std::string str, char delimiter, bool removeEmpty = false);

	/// <summary>
	/// The current combat style of an actor
	/// </summary>
	enum class CurrentCombatStyle
	{
		Spellsword = 0x1, // combination spell and onehanded
		OneHandedShield = 0x2,
		TwoHanded = 0x4,
		MagicDestruction = 0x8,
		MagicConjuration = 0x10,
		MagicAlteration = 0x20,
		MagicIllusion = 0x40,
		MagicRestoration = 0x80,
		OneHanded = 0x100,
		Ranged = 0x200,
		DualWield = 0x400,
		Staffsword = 0x800,  // combination staff and onehanded
		HandToHand = 0x1000,
		Mage = 0x2000,
		DualStaff = 0x4000,
		Staff = 0x8000,
		MagicDamageFire = 0x100000,
		MagicDamageShock = 0x200000,
		MagicDamageFrost = 0x400000,
	};

	/// <summary>
	/// Converts a CurrentCombatStyle value into a string
	/// </summary>
	/// <param name="style">CurrentCombatStyle value to convert</param>
	/// <returns>String representing [style]</returns>
	static std::string ToStringCombatStyle(uint32_t style);

	/// <summary>
	/// Retrieves data about the current equiped items and spells of an actor
	/// </summary>
	/// <param name="actor">the actor to check</param>
	/// <returns>A linear combination of CurrentCombatStyle, representing the combat data of an actor</returns>
	static uint32_t GetCombatData(RE::Actor* actor);

	enum class CurrentArmor
	{
		None = 0,
		HeavyArmor = 1 << 0,
		LightArmor = 1 << 1,
		Clothing = 1 << 2,
	};

	/// <summary>
	/// Returns which armor types are worn by a given npc.
	/// </summary>
	/// <param name="actor">The NPC to check.</param>
	/// <returns>Linear combination of values of CurrentArmor</returns>
	static uint32_t GetArmorData(RE::Actor* actor);

	/// <summary>
	/// Returns the type of item, specific weapon type etc
	/// </summary>
	/// <param name="form"></param>
	/// <returns></returns>
	static int GetItemType(RE::TESForm* form);

	#pragma region Parsing

	/// <summary>
	/// Parses a string into a vector of int (array of int)
	/// </summary>
	/// <param name="line">string to parse</param>
	/// <returns>vector of int (array of int)</returns>
	static std::vector<int> ParseIntArray(std::string line);

	/// <summary>
	/// returns a TESForm* from various inputs
	/// </summary>
	/// <param name="datahandler">datahandler to get data from</param>
	/// <param name="formid">id or partial id of item (may be 0, if editorid is set)</param>
	/// <param name="pluginname">name of the plugin the item is included (may be the empty string, if item is in the basegame, or editorid is given)</param>
	/// <param name="editorid">editorid of the item, defaults to empty string</param>
	/// <returns></returns>
	static RE::TESForm* GetTESForm(RE::TESDataHandler* datahandler, RE::FormID formid, std::string pluginname, std::string editorid = "");

	/// <summary>
	/// Parses objects for distribution rules from a string input
	/// </summary>
	/// <param name="input">the string to parse</param>
	/// <param name="error">will be overwritten with [true] if an error occurs</param>
	/// <param name="file">the relative path of the file that contains the string</param>
	/// <param name="line">the line in the file that contains the string</param>
	/// <returns>a vector of parsed and validated objects</returns>
	static std::vector<std::tuple<AssocType, RE::FormID>> ParseAssocObjects(std::string input, bool& error, std::string file, std::string line, int& totalobjects);

	/// <summary>
	/// Returns an AssocType for the given RE::FormType. If the RE::FormType is not supported, [valid] is set to true
	/// </summary>
	/// <param name="type">RE::FormType to convert</param>
	/// <param name="valid">Overridable value, which is set to true if [type] is not supported.</param>
	/// <returns>AssocType associated with [type]</returns>
	static AssocType MatchValidFormType(RE::FormType type, bool& valid);

	/// <summary>
	/// Parses AlchemyEffects from an input string.
	/// </summary>
	/// <param name="input">string to parse</param>
	/// <param name="error">Overrisable value, which is set to true if there is an error during parsing.</param>
	/// <returns>A vector of AlchemyEffects and Weights</returns>
	static std::vector<std::tuple<uint64_t, float>> ParseAlchemyEffects(std::string input, bool& error);

	/// <summary>
	/// Computes a distribution from an effectmap.
	/// </summary>
	/// <param name="effectmap">effectmap containing effects and weights which will be translated into the distribution</param>
	/// <param name="range">range the distribution chances are computed for</param>
	/// <returns>Weighted Distribution</returns>
	static std::vector<std::tuple<int, AlchemicEffect>> GetDistribution(std::vector<std::tuple<AlchemicEffect, float>> effectmap, int range, bool chance = false);

	/// <summary>
	/// Computes a distribution from a unified effect map
	/// <param name="map">unified effect map the distribution is calculated from</param>
	/// <param name="range">range the distribution chances are computed for</param>
	/// </summary>
	static std::vector<std::tuple<int, AlchemicEffect>> GetDistribution(std::map<AlchemicEffect, float> map, int range, bool chance = false);

	/// <summary>
	/// Calculates a unified effect map, that contains at most one entry per AlchemyEffect present
	/// </summary>
	/// <param name="effectmap">effectmap containing effects and weights that shal be unified</param>
	/// <returns>map with alchemyeffects and their weights</returns>
	static std::map<AlchemicEffect, float> UnifyEffectMap(std::vector<std::tuple<AlchemicEffect, float>> effectmap);

	/// <summary>
	/// Sums the Alchemyeffects in [list]
	/// </summary>
	/// <param name="list">list with AlchemyEffects to sum</param>
	/// <returns>Combined value with all Alchemyeffects</returns>
	static AlchemicEffect SumAlchemyEffects(std::vector<std::tuple<int, AlchemicEffect>> list, bool chance = false);

	/// <summary>
	/// Checks whether poison can be applied to the weapons of an actor
	/// </summary>
	/// <param name="actor">Actor to check</param>
	/// <returns>Whether poison can be applied to the actors weapons</returns>
	static bool CanApplyPoison(RE::Actor* actor);

	/// <summary>
	/// Returns whether an NPC has poison applied to their weapons, and the poison applied
	/// </summary>
	/// <param name="actor">Actor to check</param>
	/// <param name="pois">Poison found [out]</param>
	/// <returns></returns>
	static bool GetAppliedPoison(RE::Actor* actor, RE::ExtraPoison*& pois);

	/// <summary>
	/// Returns whether an actor is valid and safe to work with
	/// </summary>
	/// <param name="actor">Actor to validate</param>
	/// <returns></returns>
	static bool ValidateActor(RE::Actor* actor);

	/// <summary>
	/// Returns whether a form is valid and safe to work with
	/// </summary>
	/// <param name="form">Form to validate</param>
	/// <returns></returns>
	template<class T>
	static bool ValidateForm(T* form)
	{
		if (form == nullptr || form->GetFormID() == 0 || form->formFlags & RE::TESForm::RecordFlags::kDeleted)
			return false;
		return true;
	}

	#pragma endregion


	#pragma region Mods

	class Mods
	{
	public:
		/// <summary>
		/// Returns the pluginname the form is defined in
		/// </summary>
		static std::string GetPluginName(RE::TESForm* form);

		/// <summary>
		/// Returns the pluginname the form is defined in
		/// </summary>
		static std::string GetPluginNameFromID(RE::FormID formid);

		/// <summary>
		/// Returns the pluginname corresponding to the pluginIndex
		/// </summary>
		static std::string GetPluginName(uint32_t pluginIndex);

		/// <summary>
		/// Returns the plugin index of the given plugin [returns 0x1 on failure]
		/// </summary>
		/// <param name="pluginname"></param>
		/// <returns></returns>
		static uint32_t GetPluginIndex(std::string pluginname);

		/// <summary>
		/// Returns the plugin index of the given form [returns 0x1 on failure]
		/// </summary>
		/// <param name="pluginname"></param>
		/// <returns></returns>
		static uint32_t GetPluginIndex(RE::TESForm* form);

		/// <summary>
		/// Returns the formid with the plugin index
		/// </summary>
		static uint32_t GetIndexLessFormID(RE::TESForm* form);

		/// <summary>
		/// Returns the formid with the plugin index
		/// </summary>
		static uint32_t GetIndexLessFormID(RE::FormID formid);

		/// <summary>
		/// Returns a vector with all forms of the given type in the plugin
		/// </summary>
		/// <typeparam name="T">form type</typeparam>
		/// <param name="pluginname">name of the plugin</param>
		/// <returns></returns>
		template <class T>
		static std::vector<T*> GetFormsInPlugin(std::string pluginname)
		{
			auto datahandler = RE::TESDataHandler::GetSingleton();
			const RE::TESFile* file = nullptr;
			uint32_t pindex = Utility::Mods::GetPluginIndex(pluginname);
			if ((pindex & 0x00FFF000) != 0)  // light mod
				file = datahandler->LookupLoadedLightModByIndex((uint16_t)((pindex & 0x00FFF000) >> 12));
			else  // normal mod
				file = datahandler->LookupLoadedModByIndex((uint8_t)(pindex >> 24));
			std::vector<T*> ret;
			if (file != nullptr) {
				uint32_t mask = 0;
				uint32_t index = 0;
				if (file->IsLight()) {
					mask = 0xFFFFF000;
					index = file->GetPartialIndex() << 12;
				} else {
					mask = 0xFF000000;
					index = file->GetPartialIndex() << 24;
				}
				auto forms = datahandler->GetFormArray<T>();
				for (int i = 0; i < (int)forms.size(); i++) {
					if ((forms[i]->GetFormID() & mask) == index)
						ret.push_back(forms[i]);
				}
			}
			return ret;
		}
	};

	#pragma endregion
};
