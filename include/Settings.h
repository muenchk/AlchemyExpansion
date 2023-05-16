#pragma once

#include <fstream>
#include <iostream>
#include <type_traits>
#include <utility>
#include <string_view>
#include <chrono>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <time.h>
#include <random>
#include <tuple>
#include <vector>
#include <string.h>
#include <thread>
#include <forward_list>
#include <semaphore>
#include <limits>

#include "SimpleIni.h"
#include "Logging.h"
#include "AlchemyEffect.h"

/// <summary>
/// Determines the strength of an Item
/// </summary>
enum class ItemStrength
{
	kWeak = 1,
	kStandard = 2,
	kPotent = 3,
	kInsane = 4
};

/// <summary>
/// Supported types of Items
/// </summary>
enum class ItemType
{
	kPoison = 1,
	kPotion = 2,
	kFood = 4,
	kFortifyPotion = 8,
};

#define RandomRange 1000

class Settings
{
public:
	/// <summary>
	/// Name of this plugin
	/// </summary>
	static inline std::string PluginName = "AlchemyExpansion.esp";
	/// <summary>
	/// Plain name of this plugin
	/// </summary>
	static inline std::string PluginNamePlain = "AlchemyExpansion";

	/// <summary>
	/// Loads the distribution configuration
	/// </summary>
	static void LoadDistrConfig();

	/// <summary>
	/// General settings
	/// </summary>
	struct General
	{
		
	};

	struct Effects
	{
		enum class Identifier
		{
			_None = 0,
			_MagPositiveScaleBasicIdent = 0xFFFFF00,
			_MagPositiveScaleBasicRareIdent = 0xFFFFF01,
			_MagPositiveScaleBasicTimeIdent = 0xFFFFF02,
			_MagPositiveScaleTimeIdent = 0xFFFFF03,
			_MagPositiveScaleTimeRareIdent = 0xFFFFF04,
			_MagPositiveScaleTimeEpicIdent = 0xFFFFF05,
			_MagNegativeScaleBasicIdent = 0xFFFFF06,
			_MagNegativeScaleBasicRareIdent = 0xFFFFF07,
			_MagNegativeScaleBasicTimeIdent = 0xFFFFF08,
			_MagNegativeScaleTimeIdent = 0xFFFFF09,
			_MagNegativeScaleTimeRareIdent = 0xFFFFF0A,
			_MagNegativeScaleTimeEpicIdent = 0xFFFFF0B,
			_DurPositiveBasicIdent = 0xFFFFF0C,
			_DurPositiveBasicTimeIdent = 0xFFFFF0D,
			_DurPositiveTimeIdent = 0xFFFFF0E,
			_DurPositiveTimeAltIdent = 0xFFFFF17,
			_DurPositiveTimeRareIdent = 0xFFFFF0F,
			_DurPositiveTimeEpicIdent = 0xFFFFF10,
			_DurNegativeBasicIdent = 0xFFFFF11,
			_DurNegativeBasicTimeIdent = 0xFFFFF12,
			_DurNegativeTimeIdent = 0xFFFFF13,
			_DurNegativeTimeAltIdent = 0xFFFFF16,
			_DurNegativeTimeRareIdent = 0xFFFFF14,
			_DurNegativeTimeEpicIdent = 0xFFFFF15,

			// max : 0xFFFFF17
		};

		/// <summary>
		/// Magnitude scale factor for positive basic effects (e.g. Restore Health)
		/// </summary>
		static inline float _MagPositiveScaleBasic = 1.0;
		/// <summary>
		/// Magnitude scale factor for rare positive basic effects (e.g. stronger Restore Health)
		/// </summary>
		static inline float _MagPositiveScaleBasicRare = 1.0;
		/// <summary>
		/// Magnitude scale factor for positive basic effects over time (e.g. Regenerate Health)
		/// </summary>
		static inline float _MagPositiveScaleBasicTime = 1.0;
		/// <summary>
		/// Magnitude scale factor for positive long-term effects (e.g. Fortify Block)
		/// </summary>
		static inline float _MagPositiveScaleTime = 1.0;
		/// <summary>
		/// Magnitude scale factor for rare positive long-term effects (e.g. Quick Drawing)
		/// </summary>
		static inline float _MagPositiveScaleTimeRare = 1.0;
		/// <summary>
		/// Magnitude scale factor for epic positive long-term effects (e.g. Reflect Damage)
		/// </summary>
		static inline float _MagPositiveScaleTimeEpic = 1.0;

		/// <summary>
		/// Magnitude scale for basic negative effects (e.g. Damage Health)
		/// </summary>
		static inline float _MagNegativeScaleBasic = 1.0;
		/// <summary>
		/// Magnitude scale for basic negative effects (e.g. Ravage Health)
		/// </summary>
		static inline float _MagNegativeScaleBasicRare = 1.0;
		/// <summary>
		/// Magnitude scale for basic negative effects (e.g. Lingering Damage Health)
		/// </summary>
		static inline float _MagNegativeScaleBasicTime = 1.0;
		/// <summary>
		/// Magnitude scale for negative long-term effects (e.g. Damage Block)
		/// </summary>
		static inline float _MagNegativeScaleTime = 1.0;
		/// <summary>
		/// Magnitude scale for rare negative long-term effects (e.g. Frailty)
		/// </summary>
		static inline float _MagNegativeScaleTimeRare = 1.0;
		/// <summary>
		/// Magnitude scale for epic negative long-term effects (e.g. )
		/// </summary>
		static inline float _MagNegativeScaleTimeEpic = 1.0;

		/// <summary>
		/// Duration for basic positive effects (e.g. Restore Health)
		/// </summary>
		static inline float _DurPositiveBasic = 10;
		/// <summary>
		/// Duration for basic positive effects over time (e.g. Regenerate Health)
		/// </summary>
		static inline float _DurPositiveBasicTime = 20;
		/// <summary>
		/// Duration for positive long-term effects (e.g. Fortify Block)
		/// </summary>
		static inline float _DurPositiveTime = 60;
		/// <summary>
		/// Duration for alternative positive long-term effects (e.g. Fortify Health Regeneration)
		/// </summary>
		static inline float _DurPositiveTimeAlt = 300;
		/// <summary>
		/// Duration for Rare positive long-term effects (e.g. Quick Drawing)
		/// </summary>
		static inline float _DurPositiveTimeRare = 40;
		/// <summary>
		/// Duration for Epic positive long-term effects (e.g. Reflect Damage)
		/// </summary>
		static inline float _DurPositiveTimeEpic = 30;

		/// <summary>
		/// Duration for basic negative effects (e.g. Damage Health)
		/// </summary>
		static inline float _DurNegativeBasic = 5;
		/// <summary>
		/// Duration for basic negative effects (e.g. Lingering Damage Health)
		/// </summary>
		static inline float _DurNegativeBasicTime = 20;
		/// <summary>
		/// Duration for negative long-term effects (e.g. Damage Block)
		/// </summary>
		static inline float _DurNegativeTime = 60;
		/// <summary>
		/// Duration for alternative negative long-term effects (e.g. Reduce Health Regen)
		/// </summary>
		static inline float _DurNegativeTimeAlt = 300;
		/// <summary>
		/// Duration for rare negative long-term effects (e.g. Frailty)
		/// </summary>
		static inline float _DurNegativeTimeRare = 40;
		/// <summary>
		/// Duration for epic negative long-term effects (e.g. )
		/// </summary>
		static inline float _DurNegativeTimeEpic = 30;

		static Identifier GetType(int value);
		static float GetValue(Identifier ident);
	};

	struct Distr
	{
		static inline int _MaxMagnitudeWeak;
		static inline int _MaxMagnitudeStandard;
		static inline int _MaxMagnitudePotent;
	};

	struct Compatibility
	{
		struct Apothecary
		{
			/// <summary>
			/// [Automatic] if apothecary is installed
			/// </summary>
			static inline bool _CompatibilityApothecary = false;
		};

		struct CACO
		{
			/// <summary>
			/// [Automatic] if CACO is installed
			/// </summary>
			static inline bool _CompatibilityCACO = false;
		};
	};

	/// <summary>
	/// Settings for debug features
	/// </summary>
	struct Debug
	{
		/// <summary>
		/// [Setting] Enables general logging
		/// </summary>
		static inline bool EnableLog = false;      
		/// <summary>
		/// [Setting] Enables logging for plugin load
		/// </summary>
		static inline bool EnableLoadLog = false;  

		/// <summary>
		/// [Setting] log level
		/// 0 - only highest level
		/// 1 - highest to layer 1 function logging
		/// 2 - highest to layer 2 function logging
		/// 3 - highest to layer 3 function logging
		/// </summary>
		static inline int LogLevel = 0;        
		/// <summary>
		/// [Setting] profiling level
		/// 0 - highest level only
		/// 1 - highest and layer 1
		/// 2 - highest and layer 2
		/// </summary>
		static inline int ProfileLevel = 0;       
		/// <summary>
		/// [Setting] Enables profiling
		/// </summary>
		static inline bool EnableProfiling = false;
	};

	// intern
private:
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _dummylist{};
	/// <summary>
	/// list of Weak-tiered potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsWeak_main{};
	/// <summary>
	/// list of Weak-tiered fortify potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsWeak_rest{};
	/// <summary>
	/// list of Standard-tiered potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsStandard_main{};
	/// <summary>
	/// list of Standard-tiered fortify potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsStandard_rest{};
	/// <summary>
	/// list of Potent-tiered potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsPotent_main{};
	/// <summary>
	/// list of Potent-tiered fortify potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsPotent_rest{};
	/// <summary>
	/// list of Insane-tiered potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsInsane_main{};
	/// <summary>
	/// list of Insane-tiered fortify potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsInsane_rest{};
	/// <summary>
	/// list of blood potions
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _potionsBlood{};
	/// <summary>
	/// list of Weak-tiered poisons
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _poisonsWeak{};
	/// <summary>
	/// list of Standard-tiered poisons
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _poisonsStandard{};
	/// <summary>
	/// list of Potent-tiered poisons
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _poisonsPotent{};
	/// <summary>
	/// list of Insane-tiered poisons
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _poisonsInsane{};
	/// <summary>
	/// list of all food
	/// </summary>
	static inline std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>> _foodall{};

	static inline AlchemicEffect _alchemyEffectsFound;
	static inline AlchemicEffect _potionEffectsFound;
	static inline AlchemicEffect _poisonEffectsFound;
	static inline AlchemicEffect _foodEffectsFound;

	static inline bool _itemsInit = false;

	// tuples of effect, magnitude and duration
	/// <summary>
	/// Maps items to effect, magnitude and duration
	/// </summary>
	static inline std::unordered_map<RE::FormID, std::tuple<AlchemicEffect, float, int>> _itemEffectMap;

public:
	/// <summary>
	/// Returns the list of Weak-tiered potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsWeak_main() { return _itemsInit ? &_potionsWeak_main : &_dummylist; }
	/// <summary>
	/// Returns the list of Weak-tiered fortify potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsWeak_rest() { return _itemsInit ? &_potionsWeak_rest : &_dummylist; }
	/// <summary>
	/// Returns the list of Standard-tiered potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsStandard_main() { return _itemsInit ? &_potionsStandard_main : &_dummylist; }
	/// <summary>
	/// Returns the list of Standard-tiered fortify potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsStandard_rest() { return _itemsInit ? &_potionsStandard_rest : &_dummylist; }
	/// <summary>
	/// Returns the list of Potent-tiered potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsPotent_main() { return _itemsInit ? &_potionsPotent_main : &_dummylist; }
	/// <summary>
	/// returns the list of Potent-tiered fortify potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsPotent_rest() { return _itemsInit ? &_potionsPotent_rest : &_dummylist; }
	/// <summary>
	/// Returns the list of Insane-tiered potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsInsane_main() { return _itemsInit ? &_potionsInsane_main : &_dummylist; }
	/// <summary>
	/// Returns the list of Insane-tiered fortify potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsInsane_rest() { return _itemsInit ? &_potionsInsane_rest : &_dummylist; }
	/// <summary>
	/// Returns the list of blood potions
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* potionsBlood() { return _itemsInit ? &_potionsBlood : &_dummylist; }
	/// <summary>
	/// Returns the list of Weak-tiered poison
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* poisonsWeak() { return _itemsInit ? &_poisonsWeak : &_dummylist; }
	/// <summary>
	/// Returns the list of Standard-tiered poisons
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* poisonsStandard() { return _itemsInit ? &_poisonsStandard : &_dummylist; }
	/// <summary>
	/// Returns the list of Potent-tiered poisons
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* poisonsPotent() { return _itemsInit ? &_poisonsPotent : &_dummylist; }
	/// <summary>
	/// Returns the list of Insane-tiered poisons
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* poisonsInsane() { return _itemsInit ? &_poisonsInsane : &_dummylist; }
	/// <summary>
	/// Returns the list of all food items
	/// </summary>
	/// <returns></returns>
	static std::list<std::pair<AlchemicEffect, RE::AlchemyItem*>>* foodall() { return _itemsInit ? &_foodall : &_dummylist; }

	static inline RE::BGSKeyword* VendorItemPotion;
	static inline RE::BGSKeyword* VendorItemFood;
	static inline RE::BGSKeyword* VendorItemFoodRaw;
	static inline RE::BGSKeyword* VendorItemPoison;
	static inline RE::BGSKeyword* ActorTypeDwarven;
	static inline RE::BGSKeyword* ActorTypeCreature;
	static inline RE::BGSKeyword* ActorTypeAnimal;

	static inline RE::TESFaction* CurrentFollowerFaction;
	static inline RE::TESFaction* CurrentHirelingFaction;

	/// <summary>
	/// Game perk for Alchemy Skill Boosts
	/// </summary>
	static inline RE::BGSPerk* AlchemySkillBoosts;
	/// <summary>
	/// Game perk for Skill Boosts
	/// </summary>
	static inline RE::BGSPerk* PerkSkillBoosts;

	/// <summary>
	/// Game equip slot for the left hand
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_LeftHand;
	/// <summary>
	/// Game equip slot for the right hand
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_RightHand;
	/// <summary>
	/// Game equip slot for either hand
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_EitherHand;
	/// <summary>
	/// Game equip slot for both hands
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_BothHands;
	/// <summary>
	/// Game equip slots for shields
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_Shield;
	/// <summary>
	/// Game equip slot for voice
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_Voice;
	/// <summary>
	/// Game equip slots for potions
	/// </summary>
	static inline RE::BGSEquipSlot* Equip_Potion;

	static inline std::string pluginnames[256+4096];
	static inline std::unordered_map<std::string, uint32_t> pluginNameMap;
	static inline std::unordered_map<uint32_t, std::string> pluginIndexMap;

	/// <summary>
	/// Loads game objects
	/// </summary>
	static void InitGameStuff();

	/// <summary>
	/// Loads the plugin configuration
	/// </summary>
	static void Load();
	
	/// <summary>
	/// Saves the plugin configuration
	/// </summary>
	static void Save();


	/// <summary>
	/// classifies a single item based on its effects
	/// </summary>
	/// <param name="item"></param>
	/// <returns>effects, strength, type, duration, magnitude, whether an effect is detrimental</returns>
	static std::tuple<AlchemicEffect, ItemStrength, ItemType, int, float, bool> ClassifyItem(RE::AlchemyItem* item);

	/// <summary>
	/// classifies all AlchemyItems in the game according to its effects
	/// </summary>
	static void ClassifyItems();

	/// <summary>
	/// returns all item from [list] with the alchemy effect [effect]
	/// </summary>
	/// <param name="list"></param>
	/// <param name="effect"></param>
	/// <returns></returns>
	static std::vector<RE::AlchemyItem*> GetMatchingItems(std::list<std::pair<uint64_t, RE::AlchemyItem*>>& list, uint64_t effect)
	{
		std::vector<RE::AlchemyItem*> ret;
		for (auto entry : list) {
			if ((std::get<0>(entry) & effect) > 0)
				ret.push_back(std::get<1>(entry));
		}
		return ret;
	}

	/// <summary>
	/// Deletes all AlchemyEffects, that are not used by any potions, etc. from all distribution rules
	/// </summary>
	static void CleanAlchemyEffects();
};
