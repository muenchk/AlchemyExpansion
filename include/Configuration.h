#pragma once

#include <unordered_map>
#include <unordered_set>

typedef uint32_t FormID;
typedef int8_t Number;
typedef int32_t Chance;

class Configuration
{
public:
	class HarvestItem
	{
	public:
		RE::TESBoundObject* object;
		Number num;
		Chance chance;
	};

private:
	static inline std::unordered_map<FormID, std::vector<HarvestItem*>> _dummyMap1;
	static inline std::unordered_set<FormID> _dummySet1;
	/// <summary>
	/// maps harvested items, to items that are given additionally to the initial harvest
	/// </summary>
	static inline std::unordered_map<FormID, std::vector<HarvestItem*>> _harvestMap;
	/// <summary>
	/// maps harvested items, to items that are given additionally to the initial harvest
	/// </summary>
	static inline std::unordered_map<FormID, std::vector<HarvestItem*>> _oreMap;

	/// <summary>
	/// set that defines all activators that are considered mines
	/// </summary>
	static inline std::unordered_set<FormID> _baseObjectMines;

public:
	static inline bool _initialised;

	/// <summary>
	/// return the harvested item - additionally harvest items map
	/// </summary>
	/// <returns></returns>
	static std::unordered_map<FormID, std::vector<HarvestItem*>>* harvestMap() { return _initialised ? &_harvestMap : &_dummyMap1; }
	/// <summary>
	/// return the mined ore - additionally mining items map
	/// </summary>
	/// <returns></returns>
	static std::unordered_map<FormID, std::vector<HarvestItem*>>* oreMap() { return _initialised ? &_oreMap : &_dummyMap1; }

	/// <summary>
	/// returns set that defines all activators that are considered mines
	/// </summary>
	/// <returns></returns>
	static std::unordered_set<FormID>* baseObjectMines() { return _initialised ? &_baseObjectMines : &_dummySet1; }


	static inline RE::BGSListForm* AlchExtListMines;

};
