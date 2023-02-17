#include <fstream>
#include <iostream>
#include <type_traits>
#include <utility>
#include <string_view>
#include <chrono>
#include <set>
#include <time.h>
#include <random>
#include <tuple>
#include <vector>

#include "AlchemyEffect.h"
#include "Data.h"
#include "Settings.h"
#include "Utility.h"
#include "Logging.h"
#include "Configuration.h"


static std::mt19937 randi((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
/// <summary>
/// trims random numbers to 1 to RR
/// </summary>
static std::uniform_int_distribution<signed> randRR(1, RandomRange);
static std::uniform_int_distribution<signed> rand100(1, 100);

#pragma region Settings

void Settings::InitGameStuff()
{
	loginfo("[SETTINGS] [InitGameStuff] Load Game Stuff");
	RE::TESDataHandler* datahandler = RE::TESDataHandler::GetSingleton();
	const RE::TESFile* file = nullptr;
	uint32_t index = 0;
	for (int i = 0; i <= 254; i++) {
		file = datahandler->LookupLoadedModByIndex((uint8_t)i);
		if (file) {
			pluginnames[i] = std::string(file->GetFilename());
			index = (uint32_t)i << 24;
			pluginNameMap.insert_or_assign(pluginnames[i], index);
			pluginIndexMap.insert_or_assign(index, pluginnames[i]);
		} else
			pluginnames[i] = "";
	}
	// 0xFF... is reserved for objects created by the game during runtime 
	pluginnames[255] = "runtime";
	for (int i = 0; i <= 4095; i++) {
		file = datahandler->LookupLoadedLightModByIndex((uint16_t)i);
		if (file) {
			pluginnames[256 + i] = std::string(file->GetFilename());
			index = 0xFE000000 | ((uint32_t)i << 12);
			pluginNameMap.insert_or_assign(pluginnames[256 + i], index);
			pluginIndexMap.insert_or_assign(index, pluginnames[256 + i]);
		} else
			pluginnames[256 + i] = "";
	}

	Settings::Equip_LeftHand = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F43);
	Settings::Equip_RightHand = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F42);
	Settings::Equip_EitherHand = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F44);
	Settings::Equip_BothHands = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F45);
	Settings::Equip_Shield = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x141E8);
	Settings::Equip_Voice = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x25BEE);
	Settings::Equip_Potion = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x35698);

	loginfo("[SETTINGS] [InitGameStuff] Finished");
}

void Settings::LoadDistrConfig()
{
	Configuration::_initialised = false;

	// disable generic logging, if load logging is disabled
	if (Logging::EnableLoadLog == false)
		Logging::EnableGenericLogging = false;

	std::vector<std::string> files;
	auto constexpr folder = R"(Data\SKSE\Plugins\)";
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		if (entry.exists() && !entry.path().empty() && entry.path().extension() == ".ini") {
			if (auto path = entry.path().string(); path.rfind("ALCH_DIST") != std::string::npos) {
				files.push_back(path);
			}
		}
	}
	if (files.empty()) {
		loginfo("[SETTINGS] [LoadDistrRules] No Distribution files were found");
	}
	// init datahandler
	auto datahandler = RE::TESDataHandler::GetSingleton();

	// change order of files handled, so that files that include "default" are loaded first, so other rules may override them
	int defind = 0;
	for (int k = 0; k < files.size(); k++)
	{
		if (Utility::ToLower(files[k]).find("default") != std::string::npos)
		{
			std::string tmp = files[defind];
			files[defind] = files[k];
			files[k] = tmp;
			defind++;
		}
	}
	for (int k = 0; k < files.size(); k++)
	{
		loginfo("[SETTINGS] [LoadDistrRules] found Distribution configuration file: {}", files[k]);
	}

	// vector of splits, filename and line
	std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> attachments;
	std::vector<std::tuple<std::vector<std::string>*, std::string, std::string>> copyrules;

	const int chancearraysize = 5;

	// extract the rules from all files
	for (std::string file : files) {
		try {
			std::ifstream infile(file);
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) {
					std::string tmp = line;
					// we read another line
					// check if its empty or with a comment
					if (line.empty())
						continue;
					// remove leading spaces and tabs
					while (line.length() > 0 && (line[0] == ' ' || line[0] == '\t')) {
						line = line.substr(1, line.length() - 1);
					}
					// check again
					if (line.length() == 0 || line[0] == ';')
						continue;
					// now begin the actual processing
					std::vector<std::string>* splits = new std::vector<std::string>();
					// split the string into parts
					size_t pos = line.find('|');
					while (pos != std::string::npos) {
						splits->push_back(line.substr(0, pos));
						line.erase(0, pos + 1);
						pos = line.find("|");
					}
					if (line.length() != 0)
						splits->push_back(line);
					int splitindex = 0;
					// check wether we actually have a rule
					if (splits->size() < 3) {  // why 3? Cause first two fields are RuleVersion and RuleType and we don't accept empty rules.
						logwarn("[Settings] [LoadDistrRules] Not a rule. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what rule version we have
					int ruleVersion = -1;
					try {
						ruleVersion = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RuleVersion\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// check what kind of rule we have
					int ruleType = -1;
					try {
						ruleType = std::stoi(splits->at(splitindex));
						splitindex++;
					} catch (std::out_of_range&) {
						logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					} catch (std::invalid_argument&) {
						logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"RuleType\". file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						continue;
					}
					// now we can actually make differences for the different rule version and types
					switch (ruleVersion) {
					case 1:
						{
							switch (ruleType) {
							/* case 10:  // set item strength
								{
									if (splits->size() != 4) {
										logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
										continue;
									}
									std::string strength = splits->at(splitindex);
									splitindex++;
									std::string assoc = splits->at(splitindex);
									splitindex++;
									bool error = false;
									int total = 0;
									std::vector<std::tuple<Distribution::AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);
									ItemStrength str = ItemStrength::kWeak;
									// arse item strength
									try {
										str = static_cast<ItemStrength>(std::stoi(strength));
									} catch (std::out_of_range&) {
										logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"ItemStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									} catch (std::invalid_argument&) {
										logwarn("[Settings] [LoadDistrRules] invalid-argument expection in field \"ItemStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}
									catch (std::exception&) {
										logwarn("[Settings] [LoadDistrRules] generic expection in field \"ItemStrength\". file: {}, rule:\"{}\"", file, tmp);
										delete splits;
										continue;
									}
									for (int i = 0; i < items.size(); i++) {
										switch (std::get<0>(items[i])) {
										case Distribution::AssocType::kItem:
											Distribution::_itemStrengthMap.insert_or_assign(std::get<1>(items[i]), str);
											break;
										}
										if (Logging::EnableLoadLog) {
											if (std::get<0>(items[i]) & Distribution::AssocType::kItem) {
												LOGLE1_2("[Settings] [LoadDistrRules] set item strength {}.", Utility::GetHex(std::get<1>(items[i])));
											} else {
												logwarn("[Settings] [LoadDistrRules] rule 10 is not applicable to object {}.", Utility::GetHex(std::get<1>(items[i])));
											}
										}
									}

									// since we are done delete splits
									delete splits;
								}
								break;*/
							case 1:
								break;
							default:
								logwarn("[Settings] [LoadDistrRules] Rule type does not exist. file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								break;
							}
						}
						break;
					default:
						logwarn("[Settings] [LoadDistrRules] Rule version does not exist. file: {}, rule:\"{}\"", file, tmp);
						delete splits;
						break;
					}
				}
			} else {
				logwarn("[Settings] [LoadDistrRules] file {} couldn't be read successfully", file);
			}

		} catch (std::exception&) {
			logwarn("[Settings] [LoadDistrRules] file {} couldn't be read successfully due to an error", file);
		}
	}



	// load our stuff like necessary forms
	// get VendorItemPotion keyword, if we don't find this, potion detection will be nearly impossible
	Settings::VendorItemPotion = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDEC);
	if (Settings::VendorItemPotion == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find VendorItemPotion Keyword in game.");
	}
	Settings::VendorItemPoison = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDED);
	if (Settings::VendorItemPoison == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find VendorItemPoison Keyword in game.");
	}
	Settings::VendorItemFood = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDEA);
	if (Settings::VendorItemFood == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find VendorItemFood Keyword in game.");
	}
	Settings::VendorItemFoodRaw = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000A0E56);
	if (Settings::VendorItemFoodRaw == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find VendorItemFoodRaw Keyword in game.");
	}
	Settings::CurrentFollowerFaction = RE::TESForm::LookupByID<RE::TESFaction>(0x0005C84E);
	if (Settings::CurrentFollowerFaction == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find CurrentFollowerFaction Faction in game.");
	}
	Settings::CurrentHirelingFaction = RE::TESForm::LookupByID<RE::TESFaction>(0xbd738);
	if (Settings::CurrentHirelingFaction == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find CurrentHirelingFaction Faction in game.");
	}
	Settings::ActorTypeDwarven = RE::TESForm::LookupByID<RE::BGSKeyword>(0x1397A);
	if (Settings::ActorTypeDwarven == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find ActorTypeDwarven Keyword in game.");
	}
	Settings::ActorTypeCreature = RE::TESForm::LookupByID<RE::BGSKeyword>(0x13795);
	if (Settings::ActorTypeCreature == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find ActorTypeCreature Keyword in game.");
	}
	Settings::ActorTypeAnimal = RE::TESForm::LookupByID<RE::BGSKeyword>(0x13798);
	if (Settings::ActorTypeAnimal == nullptr) {
		loginfo("[Settings] [INIT] Couldn't find ActorTypeAnimal Keyword in game.");
	}

	// hard exclude everyone that may become a follower
	//Distribution::_excludedAssoc.insert(0x0005C84E);

	// exclude currenhireling faction
	//Distribution::_excludedAssoc.insert(0xbd738);

	Settings::AlchemySkillBoosts = RE::TESForm::LookupByID<RE::BGSPerk>(0xA725C);
	if (Settings::AlchemySkillBoosts == nullptr)
		loginfo("[Settings] [INIT] Couldn't find AlchemySkillBoosts Perk in game.");
	Settings::PerkSkillBoosts = RE::TESForm::LookupByID<RE::BGSPerk>(0xCF788);
	if (Settings::PerkSkillBoosts == nullptr)
		loginfo("[Settings] [INIT] Couldn't find PerkSkillBoosts Perk in game.");





	// template:
	//if ((tmp = Utility::GetTESForm(datahandler, 0, "", "")) != nullptr)
	//	Distribution::_excludedItems.insert(tmp->GetFormID());

	Configuration::_initialised = true;

	// reactivate generic logging
	if (Logging::EnableLog == true)
		Logging::EnableGenericLogging = true;
	else
		Logging::EnableGenericLogging = false;
}


static bool IsLeveledChar(RE::TESNPC* npc)
{
	if (npc->baseTemplateForm == nullptr)
		return true;
	RE::TESNPC* tplt = npc->baseTemplateForm->As<RE::TESNPC>();
	RE::TESLevCharacter* lvl = npc->baseTemplateForm->As<RE::TESLevCharacter>();
	if (tplt) {
		return IsLeveledChar(tplt);
	} else if (lvl)
		return false;
	else
		;  //loginfo("template invalid");
	return false;
}

void Settings::ClassifyItems()
{
	// resetting all items
	_itemsInit = false;

	_potionsWeak_main.clear();
	_potionsWeak_rest.clear();
	_potionsStandard_main.clear();
	_potionsStandard_rest.clear();
	_potionsPotent_main.clear();
	_potionsPotent_rest.clear();
	_potionsInsane_main.clear();
	_potionsInsane_rest.clear();
	_potionsBlood.clear();
	_poisonsWeak.clear();
	_poisonsStandard.clear();
	_poisonsPotent.clear();
	_poisonsInsane.clear();
	_foodall.clear();

	Data* data = Data::GetSingleton();
	data->ResetAlchItemEffects();

	std::vector<std::tuple<std::string, std::string>> ingredienteffectmap;

	// start sorting items

	auto begin = std::chrono::steady_clock::now();
	auto hashtable = std::get<0>(RE::TESForm::GetAllForms());
	auto end = hashtable->end();
	auto iter = hashtable->begin();
	RE::AlchemyItem* item = nullptr;
	RE::IngredientItem* itemi = nullptr;
	while (iter != end) {
		if ((*iter).second && (*iter).second->IsMagicItem()) {
			item = (*iter).second->As<RE::AlchemyItem>();
			if (item) {
				LOGL1_4("{}[Settings] [ClassifyItems] Found AlchemyItem {}", Utility::PrintForm(item));
				// unnamed items cannot appear in anyones inventory normally so son't add them to our lists
				if (item->GetName() == nullptr || item->GetName() == (const char*)"" || strlen(item->GetName()) == 0 ||
					std::string(item->GetName()).find(std::string("Dummy")) != std::string::npos ||
					std::string(item->GetName()).find(std::string("dummy")) != std::string::npos) {
					iter++;
					continue;
				}

				auto clas = ClassifyItem(item);
				// set medicine flag for those who need it
				if (item->IsFood() == false && item->IsPoison() == false) {  //  && item->IsMedicine() == false
					item->data.flags = RE::AlchemyItem::AlchemyFlag::kMedicine | item->data.flags;
					if (Logging::EnableLoadLog && Logging::LogLevel >= 4) {
						//LOGLE1_1("Item: {}", Utility::PrintForm(item));
						if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kCostOverride)
							LOGLE_1("\tFlag: CostOverride");
						if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kFoodItem)
							LOGLE_1("\tFlag: FoodItem");
						if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kExtendDuration)
							LOGLE_1("\tFlag: ExtendedDuration");
						if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kMedicine)
							LOGLE_1("\tFlag: Medicine");
						if (item->data.flags & RE::AlchemyItem::AlchemyFlag::kPoison)
							LOGLE_1("\tFlag: Poison");
					}
					//LOGLE1_1("[Settings] [ClassifyItems] [AssignPotionFlag] {}", Utility::PrintForm(item));
				}
				// exclude item, if it has an alchemy effect that has been excluded
				AlchemyEffectBase effects = std::get<0>(clas);

				// since the item is not to be excluded, save which alchemic effects are present
				_alchemyEffectsFound |= std::get<0>(clas);

				// determine the type of item
				if (std::get<2>(clas) == ItemType::kFood &&
					(std::get<5>(clas) == false /*either we allow detrimental effects or there are none*/)) {
					_foodall.insert(_foodall.end(), { std::get<0>(clas), item });
					_foodEffectsFound |= std::get<0>(clas);
				} else if (std::get<2>(clas) == ItemType::kPoison &&
						   (std::get<5>(clas) == false /*either we allow positive effects or there are none*/)) {
					switch (std::get<1>(clas)) {
					case ItemStrength::kWeak:
						_poisonsWeak.insert(_poisonsWeak.end(), { std::get<0>(clas), item });
						break;
					case ItemStrength::kStandard:
						_poisonsStandard.insert(_poisonsStandard.end(), { std::get<0>(clas), item });
						break;
					case ItemStrength::kPotent:
						_poisonsPotent.insert(_poisonsPotent.end(), { std::get<0>(clas), item });
						break;
					case ItemStrength::kInsane:
						_poisonsInsane.insert(_poisonsInsane.end(), { std::get<0>(clas), item });
						break;
					}
					_poisonEffectsFound |= std::get<0>(clas);
				} else if (std::get<2>(clas) == ItemType::kPotion &&
						   (std::get<5>(clas) == false /*either we allow detrimental effects or there are none*/)) {
					if ((std::get<0>(clas) & static_cast<uint64_t>(AlchemyEffect::kBlood)) > 0)
						_potionsBlood.insert(_potionsBlood.end(), { std::get<0>(clas), item });
					else if ((std::get<0>(clas) & static_cast<uint64_t>(AlchemyEffect::kHealth)) > 0 ||
							 (std::get<0>(clas) & static_cast<uint64_t>(AlchemyEffect::kMagicka)) > 0 ||
							 (std::get<0>(clas) & static_cast<uint64_t>(AlchemyEffect::kStamina)) > 0) {
						switch (std::get<1>(clas)) {
						case ItemStrength::kWeak:
							_potionsWeak_main.insert(_potionsWeak_main.end(), { std::get<0>(clas), item });
							break;
						case ItemStrength::kStandard:
							_potionsStandard_main.insert(_potionsStandard_main.end(), { std::get<0>(clas), item });
							break;
						case ItemStrength::kPotent:
							_potionsPotent_main.insert(_potionsPotent_main.end(), { std::get<0>(clas), item });
							break;
						case ItemStrength::kInsane:
							_potionsInsane_main.insert(_potionsPotent_main.end(), { std::get<0>(clas), item });
							break;
						}
					} else if (std::get<0>(clas) != static_cast<uint64_t>(AlchemyEffect::kNone)) {
						switch (std::get<1>(clas)) {
						case ItemStrength::kWeak:
							_potionsWeak_rest.insert(_potionsWeak_rest.end(), { std::get<0>(clas), item });
							break;
						case ItemStrength::kStandard:
							_potionsStandard_rest.insert(_potionsStandard_rest.end(), { std::get<0>(clas), item });
							break;
						case ItemStrength::kPotent:
							_potionsPotent_rest.insert(_potionsPotent_rest.end(), { std::get<0>(clas), item });
							break;
						case ItemStrength::kInsane:
							_potionsInsane_rest.insert(_potionsInsane_rest.end(), { std::get<0>(clas), item });
							break;
						}
					}
					_potionEffectsFound |= std::get<0>(clas);
				}
				int dosage = 0;
				// add item into effect map
				data->SetAlchItemEffects(item->GetFormID(), std::get<0>(clas), std::get<3>(clas), std::get<4>(clas), std::get<5>(clas), dosage);
			}

			itemi = (*iter).second->As<RE::IngredientItem>();
			if (itemi) {
				LOGL1_4("{}[Settings] [ClassifyItems] Found IngredientItem {}", Utility::PrintForm(itemi));
				for (int i = 0; i < (int)itemi->effects.size(); i++) {
					auto sett = itemi->effects[i]->baseEffect;
					// just retrieve the effects, we will analyze them later
					if (sett) {
						ingredienteffectmap.push_back({ itemi->GetName(), Utility::ToString(ConvertToAlchemyEffectPrimary(sett)) });
						
						// the effects of ingredients may lead to valid potions being brewed, so we need to save that these effects actually exist in the game
						_alchemyEffectsFound |= static_cast<uint64_t>(ConvertToAlchemyEffectPrimary(sett));
					}
				}
			}
		}
		iter++;
	}
	PROF1_1("{}[ClassifyItems] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));

	// items initialised
	_itemsInit = true;

	LOGL1_1("{}[Settings] [ClassifyItems] _potionsWeak_main {}", potionsWeak_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsWeak_rest {}", potionsWeak_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsStandard_main {}", potionsStandard_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsStandard_rest {}", potionsStandard_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsPotent_main {}", potionsPotent_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsPotent_rest {}", potionsPotent_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsInsane_main {}", potionsInsane_main()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsInsane_rest {}", potionsInsane_rest()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _potionsBlood {}", potionsBlood()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsWeak {}", poisonsWeak()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsStandard {}", poisonsStandard()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsPotent {}", poisonsPotent()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _poisonsInsane {}", poisonsInsane()->size());
	LOGL1_1("{}[Settings] [ClassifyItems] _foodall {}", foodall()->size());

}

std::tuple<uint64_t, ItemStrength, ItemType, int, float, bool> Settings::ClassifyItem(RE::AlchemyItem* item)
{
	RE::EffectSetting* sett = nullptr;
	if ((item->avEffectSetting) == nullptr && item->effects.size() == 0) {
		return { 0, ItemStrength::kStandard, ItemType::kFood, 0, 0.0f, false};
	}
	// we look at max 4 effects
	AlchemyEffectBase av[4]{
		0,
		0,
		0,
		0
	};
	float mag[]{
		0,
		0,
		0,
		0
	};
	int dur[]{
		0,
		0,
		0,
		0
	};
	bool detrimental = false;
	bool positive = false;
	// we will not abort the loop, since the number of effects on one item is normally very
	// limited, so we don't have much iterations
	AlchemyEffectBase tmp = 0;
	if (item->effects.size() > 0) {
		for (uint32_t i = 0; i < item->effects.size() && i < 4; i++) {
			sett = item->effects[i]->baseEffect;
			// just retrieve the effects, we will analyze them later
			if (sett) {
				mag[i] = item->effects[i]->effectItem.magnitude;
				dur[i] = item->effects[i]->effectItem.duration;
				detrimental |= sett->IsDetrimental();
				positive |= !sett->IsDetrimental();

				uint32_t formid = sett->GetFormID();
				if ((tmp = (static_cast<uint64_t>(ConvertToAlchemyEffectPrimary(sett)))) > 0) {

					av[i] |= tmp;
				}
				if (sett->data.archetype == RE::EffectArchetypes::ArchetypeID::kDualValueModifier && (tmp = ((static_cast<uint64_t>(ConvertToAlchemyEffectSecondary(sett))))) > 0) {

					av[i] |= tmp;
				}
				// we only need this for magnitude calculations, so its not used as cooldown
				if (dur[i] == 0)
					dur[i] = 1;
			}
		}
	} else {
		// emergency fallback // more or less unused
		RE::MagicItem::SkillUsageData err;
		item->GetSkillUsageData(err);
		detrimental |= item->avEffectSetting->IsDetrimental();
		positive |= !item->avEffectSetting->IsDetrimental();
		switch (item->avEffectSetting->data.primaryAV) {
		case RE::ActorValue::kHealth:
			av[0] = static_cast<uint64_t>(ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV));
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kMagicka:
			av[0] = static_cast<uint64_t>(ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV));
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kStamina:
			av[0] = static_cast<uint64_t>(ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV));
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		}
	}
	// analyze the effect types
	AlchemyEffectBase alch = 0;
	ItemStrength str = ItemStrength::kWeak;
	float maxmag = 0;
	int maxdur = 0;
	for (int i = 0; i < 4; i++) {
		if ((av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kHealth)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kMagicka)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kStamina)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kHealRate)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kMagickaRate)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kStaminaRate)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kHealRateMult)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kMagickaRateMult)) > 0 ||
			(av[i] & static_cast<AlchemyEffectBase>(AlchemyEffect::kStaminaRateMult)) > 0) {
			if (mag[i] * dur[i] > maxmag) {
				maxmag = mag[i] * dur[i];
				maxdur = dur[i];
			}
		}
		alch |= av[i];
	}
	if (std::string(item->GetName()).find(std::string("Weak")) != std::string::npos)
		str = ItemStrength::kWeak;
	else if (std::string(item->GetName()).find(std::string("Standard")) != std::string::npos)
		str = ItemStrength::kStandard;
	else if (std::string(item->GetName()).find(std::string("Potent")) != std::string::npos)
		str = ItemStrength::kPotent;
	else if (maxmag == 0)
		str = ItemStrength::kStandard;
	else if (maxmag <= Distr::_MaxMagnitudeWeak)
		str = ItemStrength::kWeak;
	else if (maxmag <= Distr::_MaxMagnitudeStandard)
		str = ItemStrength::kStandard;
	else if (maxmag <= Distr::_MaxMagnitudePotent)
		str = ItemStrength::kPotent;
	else
		str = ItemStrength::kInsane;

	// if the potion is a blood potion it should only ever appear on vampires, no the
	// effects are overriden to AlchemyEffect::kBlood
	if (std::string(item->GetName()).find(std::string("Blood")) != std::string::npos &&
		std::string(item->GetName()).find(std::string("Potion")) != std::string::npos) {
		alch = static_cast<uint64_t>(AlchemyEffect::kBlood);
	}

	ItemType type = ItemType::kPotion;
	if (item->IsFood())
		type = ItemType::kFood;
	else if (item->IsPoison()) {
		type = ItemType::kPoison;
		return {
			alch,
			str,
			type,
			maxdur,
			maxmag,
			positive // return whether there is a positive effect on the poison
		};
	}

	return {
		alch,
		str,
		type,
		maxdur,
		maxmag,
		detrimental
	};
}

void Settings::CleanAlchemyEffects()
{
	std::vector<AlchemyEffect> effectsToRemovePotion;
	std::vector<AlchemyEffect> effectsToRemovePoison;
	std::vector<AlchemyEffect> effectsToRemoveFood;
	// iterate over existing alchemy effects
	for (uint64_t i = 0; i <= 63; i++) {
		// potion
		if (_potionEffectsFound & ((AlchemyEffectBase)1 << i)) {
			// found existing effect, which is not excluded
		} else {
			// effect excluded or not present in any items
			// remove from all distribution rules
			effectsToRemovePotion.push_back(static_cast<AlchemyEffect>((AlchemyEffectBase)1 << i));
		}
		// poison
		if (_poisonEffectsFound & ((AlchemyEffectBase)1 << i)) {
			// found existing effect, which is not excluded
		} else {
			// effect excluded or not present in any items
			// remove from all distribution rules
			effectsToRemovePoison.push_back(static_cast<AlchemyEffect>((AlchemyEffectBase)1 << i));
		}
		// food
		if (_foodEffectsFound & ((AlchemyEffectBase)1 << i)) {
			// found existing effect, which is not excluded
		} else {
			// effect excluded or not present in any items
			// remove from all distribution rules
			effectsToRemoveFood.push_back(static_cast<AlchemyEffect>((AlchemyEffectBase)1 << i));
		}
	}
}

#pragma endregion
