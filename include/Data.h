#pragma once

#include <unordered_map>
#include <unordered_set>

#include "AlchemyEffect.h"
#include "Misc.h"

class Data
{
private:

	/// <summary>
	/// map that maps potionids to potion properties (effect, duration, magnitude, detrimental, dosage)
	/// </summary>
	std::unordered_map<uint32_t, std::tuple<AlchemyEffectBase, int, float, bool, int>> alchitemEffectMap;

	/// <summary>
	/// map that contains game objects that are used in custom object conditions, for fast access
	/// </summary>
	std::unordered_map<uint32_t, RE::TESForm*> customItemFormMap;

	/// <summary>
	/// datahandler
	/// </summary>
	RE::TESDataHandler* datahandler = nullptr;

	/// <summary>
	/// time the player last activated a mine
	/// </summary>
	std::chrono::system_clock::time_point lastMineActivated;

	/// <summary>
	/// map that contains all effects to be modified
	/// </summary>
	std::unordered_map<std::string, Effect*> effectMap;

	/// <summary>
	/// map that contains all ingredients to be modified
	/// </summary>
	std::unordered_map<RE::FormID, Ingredient*> ingredientMap;

	/// <summary>
	/// map that contains all potions to be modified
	/// </summary>
	std::unordered_map<RE::FormID, Potion*> potionMap;

public:
	/// <summary>
	/// Initializes data.
	/// </summary>
	void Init();
	/// <summary>
	/// returns a pointer to a static Data object
	/// </summary>
	/// <returns></returns>
	static Data* GetSingleton();

	/// <summary>
	/// Saves the alchemic effects of an AlchemyItem for later use
	/// </summary>
	/// <param name="id">id of the AlchemyItem</param>
	/// <param name="effects">the alchemic effects of the item</param>
	void SetAlchItemEffects(uint32_t id, AlchemyEffectBase effects, int duration, float magnitude, bool detrimental, int dosage);
	/// <summary>
	/// Retrieves the alchemic effects of an AlchemyItem
	/// </summary>
	/// <param name="id">the id of the AlchemyItem</param>
	/// <returns>whether the item was found, and the saved effects [found, effect, duration, magnitude, detrimental, dosage]</returns>
	std::tuple<bool, AlchemyEffectBase, int, float, bool, int> GetAlchItemEffects(uint32_t id);
	/// <summary>
	/// Resets all saved AlchemyItem effects
	/// </summary>
	void ResetAlchItemEffects();

	/// <summary>
	/// Returns the TESForm associated with the formid from an internal buffer
	/// </summary>
	/// <param name="formid"></param>
	/// <param name="pluginname"></param>
	/// <returns></returns>
	RE::TESForm* FindForm(uint32_t formid, std::string pluginname);
	/// <summary>
	/// Returns the MagicEffect associated with the formid from an internal buffer
	/// </summary>
	/// <param name="formid"></param>
	/// <param name="pluginname"></param>
	/// <returns></returns>
	RE::EffectSetting* FindMagicEffect(uint32_t formid, std::string pluginname);
	/// <summary>
	/// Returns the Perk associated with the formid from an internal buffer
	/// </summary>
	/// <param name="formid"></param>
	/// <param name="pluginname"></param>
	/// <returns></returns>
	RE::BGSPerk* FindPerk(uint32_t formid, std::string pluginname);
	/// <summary>
	/// Removes an actor from the data
	/// </summary>
	/// <param name="actor"></param>
	void DeleteFormCustom(RE::FormID actorid);

	/// <summary>
	/// Updates the time a player last activated a mine
	/// </summary>
	void UpdateLastMineActivatedTime();
	/// <summary>
	/// Returns the time a player last activated a mine
	/// </summary>
	/// <returns></returns>
	std::chrono::system_clock::time_point GetLastMineActivatedTime();

	/// <summary>
	/// Returns the map that stores all alchemy effects to be modified
	/// </summary>
	/// <returns></returns>
	std::unordered_map<std::string, Effect*>* GetEffectMap() { return &effectMap; }
	/// <summary>
	/// Returns the map that stores all ingredients to be modified
	/// </summary>
	/// <returns></returns>
	std::unordered_map<RE::FormID, Ingredient*>* GetIngredientMap() { return &ingredientMap; }
	/// <summary>
	/// Returns the map that stores all potions to be modified
	/// </summary>
	/// <returns></returns>
	std::unordered_map<RE::FormID, Potion*>* GetPotionMap() { return &potionMap; }
};
