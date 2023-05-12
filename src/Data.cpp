#include <semaphore>
#include <algorithm>
#include <iterator>

#include "Data.h"
#include "Logging.h"
#include "Utility.h"
#include "BufferOperations.h"

void Data::Init()
{
	datahandler = RE::TESDataHandler::GetSingleton();
}

Data* Data::GetSingleton()
{
	static Data singleton;
	return std::addressof(singleton);
}

std::binary_semaphore lockdata{ 1 };

void Data::SetAlchItemEffects(uint32_t id, AlchemicEffect effects, int duration, float magnitude, bool detrimental, int dosage)
{
	std::tuple<AlchemicEffect, int, float, bool, int> t = { effects, duration, magnitude, detrimental, dosage };
	alchitemEffectMap.insert_or_assign(id, t);
}

std::tuple<bool, AlchemicEffect, int, float, bool, int> Data::GetAlchItemEffects(uint32_t id)
{
	auto itr = alchitemEffectMap.find(id);
	if (itr != alchitemEffectMap.end()) {
		auto [eff, dur, mag, detri, dosage] = itr->second;
		// found
		return { true, eff, dur, mag, detri, dosage };
	} else {
		// not found
		return { false, 0, 0, 0.0f, false, 0 };
	}
}

void Data::ResetAlchItemEffects()
{
	alchitemEffectMap.clear();
}

RE::TESForm* Data::FindForm(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second;
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form;
	}
	return nullptr;
}

RE::EffectSetting* Data::FindMagicEffect(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second->As<RE::EffectSetting>();
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form->As<RE::EffectSetting>();
	}
	return nullptr;
}

RE::BGSPerk* Data::FindPerk(uint32_t formid, std::string pluginname)
{
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end())
		return itr->second->As<RE::BGSPerk>();
	RE::TESForm* form = Utility::GetTESForm(datahandler, formid, pluginname, "");
	if (form != nullptr) {
		customItemFormMap.insert_or_assign(formid, form);
		return form->As<RE::BGSPerk>();
	}
	return nullptr;
}

void Data::DeleteFormCustom(RE::FormID formid)
{
	lockdata.acquire();
	auto itr = customItemFormMap.find(formid);
	if (itr != customItemFormMap.end()) {
		customItemFormMap.erase(formid);
	}
	lockdata.release();
}

void Data::UpdateLastMineActivatedTime()
{
	lastMineActivated = std::chrono::system_clock::now();
}

std::chrono::system_clock::time_point Data::GetLastMineActivatedTime()
{
	return lastMineActivated;
}



void Data::PatchGameData()
{
	LOG_1("{}[Data] [PatchGameData]");
	lockdata.acquire();
	// effectMap
	for (auto& [name, effect] : effectMap) {
		LOG2_1("{}[Data] [PatchGameData] Effect: {}\t EffectSettings: {}", name, Utility::PrintForm(effect->effect));
	}
	// ingredientMap
	LOG1_1("{}[Data] [PatchGameData] Ingredients {}", ingredientMap.size());
	for (auto& [id, ing] : ingredientMap) {
		// check that item is valid
		if (ing->item) {
			LOG1_1("{}[Data] [PatchGameData] Patching: {}", Utility::PrintForm(ing->item));
			//if (ing->item->As<RE::TESFullName>())
			//	ing->item->As<RE::TESFullName>()->fullName = RE::BSFixedString(ing->name);
			ing->item->weight = ing->weight;
			if (ing->item->As<RE::TESValueForm>())
				ing->item->As<RE::TESValueForm>()->value = ing->value;
			ing->item->effects.clear();
			for (int i = 0; i < 4 && i < ing->magicEffects.size(); i++) {
				if (ing->magicEffects[i] == nullptr) {
					LOG_2("Couldn't find magic effect");
					continue;
				}
				auto effect = new RE::Effect();
				effect->effectItem.magnitude = ing->magnitudes[i];
				effect->effectItem.duration = ing->durations[i];
				effect->effectItem.area = 0;
				effect->baseEffect = ing->magicEffects[i];
				effect->cost = 1;
				ing->item->effects.push_back(effect);
			}
		}
	}
	// potionMap
	LOG1_1("{}[Data] [PatchGameData] Ingredients {}", potionMap.size());
	for (auto& [id, pot] : potionMap) {
		// check that item is valid
		if (pot->item) {
			LOG1_1("{}[Data] [PatchGameData] Patching: {}", Utility::PrintForm(pot->item));
			//if (pot->item->As<RE::TESFullName>())
			//	pot->item->As<RE::TESFullName>()->fullName = RE::BSFixedString(pot->name);
			pot->item->weight = pot->weight;
			if (pot->value != 0) {
				pot->item->data.costOverride = pot->value;
			}
			pot->item->effects.clear();
			for (int i = 0; i < pot->magicEffects.size(); i++) {
				if (pot->magicEffects[i] == nullptr) {
					LOG_2("Couldn't find magic effect");
					continue;
				}
				auto effect = new RE::Effect();
				effect->effectItem.magnitude = pot->magnitudes[i];
				effect->effectItem.duration = pot->durations[i];
				effect->effectItem.area = 0;
				effect->baseEffect = pot->magicEffects[i];
				effect->cost = 1;
				pot->item->effects.push_back(effect);
			}
		}
	}

	lockdata.release();
}
