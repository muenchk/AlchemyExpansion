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

	Data* data = Data::GetSingleton();

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
	std::vector<std::pair<std::string, std::pair<std::string, int>>> l_early;
	std::vector<std::pair<std::string, std::pair<std::string, int>>> l_defaultearly;
	std::vector<std::pair<std::string, std::pair<std::string, int>>> l_default;
	std::vector<std::pair<std::string, std::pair<std::string, int>>> l_defaultlate;
	std::vector<std::pair<std::string, std::pair<std::string, int>>> l_late;

	std::vector<std::pair<std::string, std::pair<std::string, int>>> l_final;

	const int chancearraysize = 5;

	int category = 2;

	// extract the rules from all files
	for (std::string file : files) {
		try {
			std::ifstream infile(file);
			if (infile.is_open()) {
				std::string line;
				while (std::getline(infile, line)) {
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

					// check whether a load category is defined
					if (Utility::ToLower(line).contains("load=early")) {
						category = 0;
						loginfo("[Settings] [LoadDistrRules] Set loadorder for file {} to Early", file);
						continue;
					}
					if (Utility::ToLower(line).contains("load=defaultearly")) {
						category = 1;
						loginfo("[Settings] [LoadDistrRules] Set loadorder for file {} to Default Early", file);
						continue;
					}
					if (Utility::ToLower(line).contains("load=default_")) {
						category = 2;
						loginfo("[Settings] [LoadDistrRules] Set loadorder for file {} to Default", file);
						continue;
					}
					if (Utility::ToLower(line).contains("load=defaultlate")) {
						category = 3;
						loginfo("[Settings] [LoadDistrRules] Set loadorder for file {} to Default Late", file);
						continue;
					}
					if (Utility::ToLower(line).contains("load=late")) {
						category = 4;
						loginfo("[Settings] [LoadDistrRules] Set loadorder for file {} to Late", file);
						continue;
					}
					
					// check file load conditions
					if (Utility::ToLower(line.substr(0, 11)).contains("if_plugin=")) {
						std::string condname = line.substr(10, line.size() - 10);
						if (Utility::Mods::GetPluginIndex(condname) == 0x1 && condname != "")
						{
							// mod isn't loaded
							logwarn("[Settings] [LoadDistrRules] File {} is not loaded since plugin \"{}\" is missing", file, condname);
								break;
						}
						continue;
					}
					
					switch (category)
					{
					case 0: // early
						l_early.push_back({ file, { line, category } });
						break;
					case 1: // default early
						l_defaultearly.push_back({ file, { line, category } });
						break;
					case 2: // default
						l_default.push_back({ file, { line, category } });
						break;
					case 3:  // default late
						l_defaultlate.push_back({ file, { line, category } });
						break;
					case 4: // late
						l_late.push_back({ file, { line, category } });
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

	// add lists together
	for (int i = 0; i < l_early.size(); i++)
	{
		l_final.push_back(l_early[i]);
	}
	l_early.clear();
	for (int i = 0; i < l_defaultearly.size(); i++) {
		l_final.push_back(l_defaultearly[i]);
	}
	l_defaultearly.clear();
	for (int i = 0; i < l_default.size(); i++) {
		l_final.push_back(l_default[i]);
	}
	l_default.clear();
	for (int i = 0; i < l_defaultlate.size(); i++) {
		l_final.push_back(l_defaultlate[i]);
	}
	l_defaultlate.clear();
	for (int i = 0; i < l_late.size(); i++) {
		l_final.push_back(l_late[i]);
	}
	l_late.clear();

	for (int i = 0; i < l_final.size(); i++)
	{
		std::string file = l_final[i].first;
		std::string line = l_final[i].second.first;
		category = l_final[i].second.second;
		std::string tmp = line;

		LOG1_1("{}[Settings] [LoadDistrRules] Loading rule: {}", line);

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
				case 1001:  // Effects
					{
						switch (category)
						{
						case 1:
						case 2:
						case 3:
						case 4:
								logwarn("[Settings] [LoadDistrRules] Cannot load effect in category {}. file: {}, rule:\"{}\", fields: {}", category, file, tmp, splits->size());
								delete splits;
							continue;
							break;
						default:
							break;
						}

						if (splits->size() != 9) {
							logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 9. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
							delete splits;
							continue;
						}

						std::string assoc = splits->at(splitindex);
						splitindex++;

						bool error = false;
						int total = 0;
						std::vector<std::tuple<AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);

						if (items.size() > 0 && std::get<0>(items[0]) & AssocType::kEffectSetting) {
							Effect* eff = new Effect();
							eff->formid = std::get<1>(items[0]);
							eff->pluginName = Utility::Mods::GetPluginNameFromID(eff->formid);
							eff->name = splits->at(splitindex);
							splitindex++;
							eff->editorID = splits->at(splitindex);
							splitindex++;
							try {
								eff->actorValue = static_cast<RE::ActorValue>(std::stol(splits->at(splitindex)));
								splitindex++;
							} catch (std::exception&) {
								logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"ActorValue\". file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								continue;
							}
							if (splits->at(splitindex) == "1")
								eff->overwrite = true;
							else
								eff->overwrite = false;
							splitindex++;
							if (eff->overwrite) {
								try {
									eff->alchemyEffect = splits->at(splitindex);
								} catch (std::exception& e) {
									logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"AlchemyEffect\". file: {}, rule:\"{}\" What: {}", file, tmp, e.what());
									delete splits;
									continue;
								}
							} else {
								eff->alchemyEffect = ConvertToAlchemyEffect(eff->actorValue);
							}
							splitindex++;
							if (splits->at(splitindex) == "1")
								eff->detrimental = true;
							else
								eff->detrimental = false;
							splitindex++;

							eff->effect = RE::TESForm::LookupByID<RE::EffectSetting>(eff->formid);
							if (eff->effect == nullptr) {
								logwarn("[Settings] [LoadDistrRules] Cannot find MagicEffect {}. file: {}, rule:\"{}\"", Utility::GetHex(eff->formid), file, tmp);
								delete splits;
								continue;
							}


							auto itr = data->GetEffectMap()->find(Utility::ToLower(eff->name));
							if (itr != data->GetEffectMap()->end()) {
								delete itr->second;
								data->GetEffectMap()->erase(Utility::ToLower(eff->name));
							}
							data->GetEffectMap()->insert_or_assign(Utility::ToLower(eff->name), eff);
						}

						delete splits;
					}
					break;
				case 1002:  // ingredient
					{
						switch (category) {
						case 0:
						case 4:
							logwarn("[Settings] [LoadDistrRules] Cannot load effect in category {}. file: {}, rule:\"{}\", fields: {}", category, file, tmp, splits->size());
							delete splits;
							continue;
							break;
						default:
							break;
						}

						if (splits->size() != 19) {
							logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 19. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
							continue;
						}

						std::string assoc = splits->at(splitindex);
						splitindex++;

						bool error = false;
						int total = 0;
						std::vector<std::tuple<AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);

						if (items.size() > 0 && std::get<0>(items[0]) & AssocType::kItem) {
							Ingredient* ing = new Ingredient();
							for (int i = 0; i < 4; i++) {
								ing->effects.push_back("");
								ing->durations.push_back(0);
								ing->magnitudes.push_back(0);
								ing->magicEffects.push_back(nullptr);
							}
							// formid
							ing->formid = std::get<1>(items[0]);
							// pluginname
							ing->pluginName = Utility::Mods::GetPluginNameFromID(ing->formid);
							// name skip
							splitindex++;
							// editorid
							ing->editorID = splits->at(splitindex);
							splitindex++;
							// Weight
							try {
								ing->weight = std::stof(splits->at(splitindex));
								splitindex++;
							} catch (std::exception& e) {
								logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Weight\". file: {}, rule:\"{}\" What: {}, num: {}", file, tmp, e.what(), splits->at(splitindex));
								delete splits;
								continue;
							}
							// Value
							try {
								ing->value = std::stoi(splits->at(splitindex));
								splitindex++;
							} catch (std::exception&) {
								logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Value\". file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								continue;
							}
							for (int i = 0; i < 4; i++) {
								// effect
								ing->effects[i] = splits->at(splitindex);
								splitindex++;
								// duration
								try {
									ing->durations[i] = std::stoi(splits->at(splitindex));
									splitindex++;
								} catch (std::exception&) {
									logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Duration{}\". file: {}, rule:\"{}\"", std::to_string(i), file, tmp);
									continue;
								}
								// magnitude
								try {
									ing->magnitudes[i] = std::stof(splits->at(splitindex));
									splitindex++;
								} catch (std::exception&) {
									logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Magnitude{}\". file: {}, rule:\"{}\"", std::to_string(i), file, tmp);
									continue;
								}
							}
							// done
							// get EffectSettings from registered effects
							for (int i = 0; i < 4; i++) {
								auto itr = data->GetEffectMap()->find(Utility::ToLower(ing->effects[i]));
								if (itr != data->GetEffectMap()->end()) {
									ing->magicEffects[i] = itr->second->effect;
								}
							}

							ing->item = RE::TESForm::LookupByID<RE::IngredientItem>(ing->formid);
							if (ing->item == nullptr) {
								logwarn("[Settings] [LoadDistrRules] Cannot find Ingredient {}. file: {}, rule:\"{}\"", Utility::GetHex(ing->formid), file, tmp);
								delete splits;
								continue;
							}

							auto itr = data->GetIngredientMap()->find(ing->formid);
							if (itr != data->GetIngredientMap()->end()) {
								delete itr->second;
								data->GetIngredientMap()->erase(ing->formid);
							}

							data->GetIngredientMap()->insert_or_assign(ing->formid, ing);
						}
						delete splits;
					}
					break;
				case 1003:  // Potion
					{
						switch (category) {
						case 0:
						case 4:
							logwarn("[Settings] [LoadDistrRules] Cannot load effect in category {}. file: {}, rule:\"{}\", fields: {}", category, file, tmp, splits->size());
							delete splits;
							continue;
							break;
						default:
							break;
						}

						if (splits->size() != 8) {
							logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 8. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
							delete splits;
							continue;
						}

						std::string assoc = splits->at(splitindex);
						splitindex++;

						bool error = false;
						int total = 0;
						std::vector<std::tuple<AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);

						if (items.size() > 0 && std::get<0>(items[0]) & AssocType::kItem) {
							Potion* pot = new Potion();
							// formid
							pot->formid = std::get<1>(items[0]);
							// pluginname
							pot->pluginName = Utility::Mods::GetPluginNameFromID(pot->formid);
							// name skip
							splitindex++;
							// editorid
							pot->editorID = splits->at(splitindex);
							pot->numeffects = 0;
							splitindex++;
							// Weight
							try {
								pot->weight = std::stof(splits->at(splitindex));
								splitindex++;
							} catch (std::exception&) {
								logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Weight\". file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								continue;
							}
							// Value
							try {
								pot->value = std::stoi(splits->at(splitindex));
								splitindex++;
							} catch (std::exception&) {
								logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Value\". file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								continue;
							}
							std::vector<std::string> effects = Utility::SplitString(splits->at(splitindex), ';', true);
							for (int i = 0; i < effects.size(); i++) {
								LOG_1("{}[Wild] Processing Effect");
								std::vector<std::string> effs = Utility::SplitString(effects[i], ',', true);
								if (effs.size() == 3) {
									// effect
									std::string eff = effs[0];
									int dur = 0;
									// duration
									try {
										dur = std::stoi(effs[1]);
									} catch (std::exception&) {
										logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Duration{}\". file: {}, rule:\"{}\"", std::to_string(i), file, tmp);
										continue;
									}
									float mag = 0;
									// magnitude
									try {
										mag = std::stof(effs[2]);
									} catch (std::exception&) {
										logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"Magnitude{}\". file: {}, rule:\"{}\"", std::to_string(i), file, tmp);
										continue;
									}
									pot->effects.push_back(eff);
									pot->durations.push_back(dur);
									pot->magnitudes.push_back(mag);
									pot->numeffects++;
								}
							}
							// done
							// get EffectSettings from registered effects
							for (int i = 0; i < pot->numeffects; i++) {
								LOG_1("{}[Wild] Finding EffectSetting");
								auto itr = data->GetEffectMap()->find(Utility::ToLower(pot->effects[i]));
								LOG_1("{}[Wild] Finding EffectSetting 2");
								if (itr != data->GetEffectMap()->end()) {
									LOG_1("{}[Wild] Finding EffectSetting 3");
									pot->magicEffects.push_back(itr->second->effect);
									LOG_1("{}[Wild] Finding EffectSetting 4");
								} else
									pot->magicEffects.push_back(nullptr);
							}

							LOG_1("{}[Wild] LookingUp");
							pot->item = RE::TESForm::LookupByID<RE::AlchemyItem>(pot->formid);
							LOG_1("{}[Wild] LookedUp");
							if (pot->item == nullptr) {
								logwarn("[Settings] [LoadDistrRules] Cannot find AlchemyItem {}. file: {}, rule:\"{}\"", Utility::GetHex(pot->formid), file, tmp);
								delete splits;
								continue;
							}

							auto itr = data->GetPotionMap()->find(pot->formid);
							if (itr != data->GetPotionMap()->end()) {
								delete itr->second;
								data->GetPotionMap()->erase(pot->formid);
							}

							data->GetPotionMap()->insert_or_assign(pot->formid, pot);
							LOG_1("{}[Wild] Done");
						}
						delete splits;
					}
					break;
				case 1005: // name override
					{
						switch (category) {
						case 0:
						case 1:
						case 2:
						case 3:
							logwarn("[Settings] [LoadDistrRules] Cannot load effect in category {}. file: {}, rule:\"{}\", fields: {}", category, file, tmp, splits->size());
							delete splits;
							continue;
							break;
						default:
							break;
						}

						if (splits->size() != 4) {
							logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
							delete splits;
							continue;
						}

						std::string assoc = splits->at(splitindex);
						splitindex++;

						bool error = false;
						int total = 0;
						std::vector<std::tuple<AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);

						if (items.size() > 0 && std::get<0>(items[0]) & AssocType::kItem) {
							// id
							RE::FormID id = std::get<1>(items[0]);
							// name
							std::string name = splits->at(splitindex);
							data->GetNameMap()->insert_or_assign(id, name);
						}
						delete splits;
					}
					break;
				case 1006:  // Effects
					{

						if (splits->size() != 4) {
							logwarn("[Settings] [LoadDistrRules] rule has wrong number of fields, expected 4. file: {}, rule:\"{}\", fields: {}", file, tmp, splits->size());
							delete splits;
							continue;
						}

						std::string assoc = splits->at(splitindex);
						splitindex++;

						bool error = false;
						int total = 0;
						std::vector<std::tuple<AssocType, RE::FormID>> items = Utility::ParseAssocObjects(assoc, error, file, tmp, total);

						if (items.size() > 0 && std::get<0>(items[0]) & AssocType::kEffectSetting) {
							// id
							RE::FormID id = std::get<1>(items[0]);
							// duration override
							int dur = 0;
							try {
								dur = std::stoi(splits->at(splitindex), nullptr, 16);
								splitindex++;
							} catch (std::exception&) {
								logwarn("[Settings] [LoadDistrRules] out-of-range expection in field \"DurationOverride\". file: {}, rule:\"{}\"", file, tmp);
								delete splits;
								continue;
							}
							data->GetEffectDurationOverride()->insert_or_assign(id, dur);
							delete splits;
						}
					}
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

static std::string GetFormEditorID(RE::MagicItem* fid)
{
	const auto& [map, lock] = RE::TESForm::GetAllFormsByEditorID();
	const RE::BSReadLockGuard locker{ lock };
	if (map) {
		for (auto& [id, form] : *map) {
			auto editorID = id.c_str();
			RE::FormID formID = form->GetFormID();
			if (formID == fid->GetFormID())
				return std::string(editorID);
		}
	}
	auto fullName = fid ? fid->As<RE::TESFullName>() : nullptr;
	if (fullName) {
		return std::string(fullName->fullName.c_str());
	}
	return "ERROR";
}

static std::string GetFormEditorID(RE::EffectSetting* fid)
{
	const auto& [map, lock] = RE::TESForm::GetAllFormsByEditorID();
	const RE::BSReadLockGuard locker{ lock };
	if (map) {
		for (auto& [id, form] : *map) {
			auto editorID = id.c_str();
			RE::FormID formID = form->GetFormID();
			if (formID == fid->GetFormID())
				return std::string(editorID);
		}
	}
	auto fullName = fid ? fid->As<RE::TESFullName>() : nullptr;
	if (fullName) {
		return std::string(fullName->fullName.c_str());
	}
	return "ERROR";
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

	std::vector<Ingredient*> ingredienteffectmap;
	std::vector<Potion*> potioneffectmap;

	// start sorting items

	auto begin = std::chrono::steady_clock::now();
	const auto& [hashtable, lock] = RE::TESForm::GetAllForms();
	{
		const RE::BSReadLockGuard locker{ lock };
		if (hashtable) {
			RE::AlchemyItem* item = nullptr;
			RE::IngredientItem* itemi = nullptr;
			for (auto& [id, form] : *hashtable) {
				if (form && form->IsMagicItem()) {
					item = form->As<RE::AlchemyItem>();
					if (item) {
						LOGL1_4("{}[Settings] [ClassifyItems] Found AlchemyItem {}", Utility::PrintForm(item));
						// unnamed items cannot appear in anyones inventory normally so son't add them to our lists
						if (item->GetName() == nullptr || item->GetName() == (const char*)"" || strlen(item->GetName()) == 0 ||
							std::string(item->GetName()).find(std::string("Dummy")) != std::string::npos ||
							std::string(item->GetName()).find(std::string("dummy")) != std::string::npos) {
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
						AlchemicEffect effects = std::get<0>(clas);

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
							if ((std::get<0>(clas) & AlchemicEffect::kBlood) > 0)
								_potionsBlood.insert(_potionsBlood.end(), { std::get<0>(clas), item });
							else if ((std::get<0>(clas) & AlchemicEffect::kHealth) > 0 ||
									 (std::get<0>(clas) & AlchemicEffect::kMagicka) > 0 ||
									 (std::get<0>(clas) & AlchemicEffect::kStamina) > 0) {
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
							} else if (std::get<0>(clas) != AlchemicEffect::kNone) {
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

						LOGL1_4("{}[Settings] [ClassifyItems] Found AlchemyItem {}", Utility::PrintForm(item));
						Potion* pot = new Potion();
						pot->name = item->GetFullName();
						pot->editorID = item->GetFormEditorID();
						pot->value = item->GetGoldValue();
						pot->weight = item->GetWeight();
						pot->item = item;
						pot->numeffects = (int)item->effects.size();
						for (int i = 0; i < (int)item->effects.size(); i++) {
							auto sett = item->effects[i]->baseEffect;
							// just retrieve the effects, we will analyze them later
							if (sett) {
								pot->effects.push_back(sett->GetFullName());
								pot->magnitudes.push_back(item->effects[i]->effectItem.magnitude);
								pot->durations.push_back(item->effects[i]->effectItem.duration);
								// the effects of ingredients may lead to valid potions being brewed, so we need to save that these effects actually exist in the game
								_alchemyEffectsFound |= ConvertToAlchemyEffectPrimary(sett);
							} else {
								pot->effects.push_back("");
								pot->magnitudes.push_back(0);
								pot->durations.push_back(0);
							}
						}
						potioneffectmap.push_back(pot);
					}

					itemi = form->As<RE::IngredientItem>();
					if (itemi) {
						LOGL1_4("{}[Settings] [ClassifyItems] Found IngredientItem {}", Utility::PrintForm(itemi));
						Ingredient* ing = new Ingredient();
						ing->name = itemi->GetFullName();
						ing->editorID = itemi->GetFormEditorID();
						ing->value = itemi->GetGoldValue();
						ing->weight = itemi->GetWeight();
						ing->item = itemi;
						for (int i = 0; i < (int)itemi->effects.size(); i++) {
							auto sett = itemi->effects[i]->baseEffect;
							// just retrieve the effects, we will analyze them later
							if (sett) {
								ing->effects.push_back(sett->GetFullName());
								ing->magnitudes.push_back(itemi->effects[i]->effectItem.magnitude);
								ing->durations.push_back(itemi->effects[i]->effectItem.duration);
								ing->magicEffects.push_back(sett);
								// the effects of ingredients may lead to valid potions being brewed, so we need to save that these effects actually exist in the game
								_alchemyEffectsFound |= ConvertToAlchemyEffectPrimary(sett);
							} else {
								ing->effects.push_back("");
								ing->magnitudes.push_back(0);
								ing->durations.push_back(0);
								ing->magicEffects.push_back(nullptr);
							}
						}
						for (int i = (int)itemi->effects.size(); i <= 4; i++) {
							ing->effects.push_back("");
							ing->magnitudes.push_back(0);
							ing->durations.push_back(0);
						}
						ingredienteffectmap.push_back(ing);
					}
				}
			}
		}
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


	
	std::string pathing = "Data\\SKSE\\Plugins\\AlchemyExpansion\\ingredients_ALCH_DIST.ini";
	std::filesystem::create_directories(std::filesystem::path(pathing).parent_path());
	std::ofstream outing = std::ofstream(pathing, std::ofstream::out);
	outing << ";RuleVersion = 1 | RuleType = 1002 | <FormID, PluginName> | Name | EditorID? | Weight | value | Effect1Name | Duration1 | Magnitude1 | Effect2Name | Duration2 | Magnitude2 | Effect3Name | Duration3 | Magnitude3 | Effect4Name | Duration4 | Magnitude4\n";
	for (auto & ing : ingredienteffectmap) {
		outing
			<< "1"
			<< "|1002"
			<< "|<"
			<< Utility::GetHex(Utility::Mods::GetIndexLessFormID(ing->item))
			<< ","
			<< Utility::Mods::GetPluginNameFromID(ing->item->GetFormID())
			<< ">"
			<< "|" << ing->item->GetFullName()
			<< "|" << GetFormEditorID(ing->item)
			<< "|" << std::to_string(ing->weight)
			<< "|" << std::to_string(ing->value)
			<< "|" << ing->effects[0]
			<< "|" << ing->durations[0]
			<< "|" << ing->magnitudes[0]
			<< "|" << ing->effects[1]
			<< "|" << ing->durations[1]
			<< "|" << ing->magnitudes[1]
			<< "|" << ing->effects[2]
			<< "|" << ing->durations[2]
			<< "|" << ing->magnitudes[2]
			<< "|" << ing->effects[3]
			<< "|" << ing->durations[3]
			<< "|" << ing->magnitudes[3] << "\n";
	}
	outing.close();

	{
		// sort "potions" into potions, food, poisons
		std::vector<Potion*> potions;
		std::vector<Potion*> food;
		std::vector<Potion*> poisons;

		for (auto& pot : potioneffectmap) {
			if (pot->item->IsPoison())
				poisons.push_back(pot);
			else if (pot->item->IsFood())
				food.push_back(pot);
			else
				potions.push_back(pot);
		}

		potioneffectmap.clear();

		for (auto& pot : potions)
		{
			potioneffectmap.push_back(pot);
		}
		for (auto& pot : poisons) {
			potioneffectmap.push_back(pot);
		}
		for (auto& pot : food) {
			potioneffectmap.push_back(pot);
		}
	}

	pathing = "Data\\SKSE\\Plugins\\AlchemyExpansion\\potions_ALCH_DIST.ini";
	std::filesystem::create_directories(std::filesystem::path(pathing).parent_path());
	outing = std::ofstream(pathing, std::ofstream::out);
	outing << ";RuleVersion = 1 | RuleType = 1003 | <FormID, PluginName> | Name | EditorID? | Weight | value | Effect1Name | Duration1 | Magnitude1 | Effect2Name | Duration2 | Magnitude2 | Effect3Name | Duration3 | Magnitude3 | Effect4Name | Duration4 | Magnitude4\n";
	for (auto& pot : potioneffectmap) {
		outing
			<< "1"
			<< "|1003"
			<< "|<"
			<< Utility::GetHex(Utility::Mods::GetIndexLessFormID(pot->item))
			<< ","
			<< Utility::Mods::GetPluginNameFromID(pot->item->GetFormID())
			<< ">"
			<< "|" << pot->item->GetFullName()
			<< "|" << GetFormEditorID(pot->item)
			<< "|" << std::to_string(pot->weight)
			<< "|" << std::to_string(pot->value)
			<< "|";
		if (pot->numeffects > 0)
		{
			outing << pot->effects[0] << ","
				   << pot->durations[0] << "," << pot->magnitudes[0];
		}
		for (int i = 1; i < pot->numeffects; i++)
		{
			outing << ";" << pot->effects[i] << ","
				   << pot->durations[i] << "," << pot->magnitudes[i];
		}
		outing << "\n";
	}
	outing.close();


	pathing = "Data\\SKSE\\Plugins\\AlchemyExpansion\\effects_ALCH_DIST.ini";
	std::filesystem::create_directories(std::filesystem::path(pathing).parent_path());
	outing = std::ofstream(pathing, std::ofstream::out);
	
	std::map<RE::FormID, RE::EffectSetting*> effectMap;
	for (auto& ing : ingredienteffectmap) {
		for (int i = 0; i < ing->magicEffects.size(); i++)
		{
			if (ing->magicEffects[i] != nullptr)
			{
				effectMap.insert_or_assign(ing->magicEffects[i]->GetFormID(), ing->magicEffects[i]);
			}
		}
	}
	for (auto& [id, effect] : effectMap)
	{
		outing
			<< "1"
			<< "|1001"
			<< "|<"
			<< Utility::GetHex(Utility::Mods::GetIndexLessFormID(effect))
			<< ","
			<< Utility::Mods::GetPluginNameFromID(id)
			<< ">"
			<< "|" << effect->GetFullName()
			<< "|" << GetFormEditorID(effect)
			<< "|" << static_cast<int>(effect->data.primaryAV)
			<< "|" << 0
			<< "|" << ConvertToAlchemyEffect(effect->data.primaryAV).string()
			<< "|" << (int)effect->IsDetrimental()
			<< "\n";
	}

	outing.close();
}

Settings::Effects::Identifier Settings::Effects::GetType(int value)
{
	switch (value) {
	case static_cast<int>(Identifier::_MagPositiveScaleBasicIdent):
		return Identifier::_MagPositiveScaleBasicIdent;
	case static_cast<int>(Identifier::_MagPositiveScaleBasicRareIdent):
		return Identifier::_MagPositiveScaleBasicRareIdent;
	case static_cast<int>(Identifier::_MagPositiveScaleBasicTimeIdent):
		return Identifier::_MagPositiveScaleBasicTimeIdent;
	case static_cast<int>(Identifier::_MagPositiveScaleTimeIdent):
		return Identifier::_MagPositiveScaleTimeIdent;
	case static_cast<int>(Identifier::_MagPositiveScaleTimeRareIdent):
		return Identifier::_MagPositiveScaleTimeRareIdent;
	case static_cast<int>(Identifier::_MagPositiveScaleTimeEpicIdent):
		return Identifier::_MagPositiveScaleTimeEpicIdent;
	case static_cast<int>(Identifier::_MagNegativeScaleBasicIdent):
		return Identifier::_MagNegativeScaleBasicIdent;
	case static_cast<int>(Identifier::_MagNegativeScaleBasicRareIdent):
		return Identifier::_MagNegativeScaleBasicRareIdent;
	case static_cast<int>(Identifier::_MagNegativeScaleBasicTimeIdent):
		return Identifier::_MagNegativeScaleBasicTimeIdent;
	case static_cast<int>(Identifier::_MagNegativeScaleTimeIdent):
		return Identifier::_MagNegativeScaleTimeIdent;
	case static_cast<int>(Identifier::_MagNegativeScaleTimeRareIdent):
		return Identifier::_MagNegativeScaleTimeRareIdent;
	case static_cast<int>(Identifier::_MagNegativeScaleTimeEpicIdent):
		return Identifier::_MagNegativeScaleTimeEpicIdent;
	case static_cast<int>(Identifier::_DurPositiveBasicIdent):
		return Identifier::_DurPositiveBasicIdent;
	case static_cast<int>(Identifier::_DurPositiveBasicTimeIdent):
		return Identifier::_DurPositiveBasicTimeIdent;
	case static_cast<int>(Identifier::_DurPositiveTimeIdent):
		return Identifier::_DurPositiveTimeIdent;
	case static_cast<int>(Identifier::_DurPositiveTimeAltIdent):
		return Identifier::_DurPositiveTimeAltIdent;
	case static_cast<int>(Identifier::_DurPositiveTimeRareIdent):
		return Identifier::_DurPositiveTimeRareIdent;
	case static_cast<int>(Identifier::_DurPositiveTimeEpicIdent):
		return Identifier::_DurPositiveTimeEpicIdent;
	case static_cast<int>(Identifier::_DurNegativeBasicIdent):
		return Identifier::_DurNegativeBasicIdent;
	case static_cast<int>(Identifier::_DurNegativeBasicTimeIdent):
		return Identifier::_DurNegativeBasicTimeIdent;
	case static_cast<int>(Identifier::_DurNegativeTimeIdent):
		return Identifier::_DurNegativeTimeIdent;
	case static_cast<int>(Identifier::_DurNegativeTimeAltIdent):
		return Identifier::_DurNegativeTimeAltIdent;
	case static_cast<int>(Identifier::_DurNegativeTimeRareIdent):
		return Identifier::_DurNegativeTimeRareIdent;
	case static_cast<int>(Identifier::_DurNegativeTimeEpicIdent):
		return Identifier::_DurNegativeTimeEpicIdent;
	default:
		return Identifier::_None;
	}
}

float Settings::Effects::GetValue(Settings::Effects::Identifier ident)
{
	switch (ident)
	{
	case Identifier::_MagPositiveScaleBasicIdent:
		return _MagPositiveScaleBasic;
	case Identifier::_MagPositiveScaleBasicRareIdent:
		return _MagPositiveScaleBasicRare;
	case Identifier::_MagPositiveScaleBasicTimeIdent:
		return _MagPositiveScaleBasicTime;
	case Identifier::_MagPositiveScaleTimeIdent:
		return _MagPositiveScaleTime;
	case Identifier::_MagPositiveScaleTimeRareIdent:
		return _MagPositiveScaleTimeRare;
	case Identifier::_MagPositiveScaleTimeEpicIdent:
		return _MagPositiveScaleTimeEpic;
	case Identifier::_MagNegativeScaleBasicIdent:
		return _MagNegativeScaleBasic;
	case Identifier::_MagNegativeScaleBasicRareIdent:
		return _MagNegativeScaleBasicRare;
	case Identifier::_MagNegativeScaleBasicTimeIdent:
		return _MagNegativeScaleBasicTime;
	case Identifier::_MagNegativeScaleTimeIdent:
		return _MagNegativeScaleTime;
	case Identifier::_MagNegativeScaleTimeRareIdent:
		return _MagNegativeScaleTimeRare;
	case Identifier::_MagNegativeScaleTimeEpicIdent:
		return _MagNegativeScaleTimeEpic;
	case Identifier::_DurPositiveBasicIdent:
		return _DurPositiveBasic;
	case Identifier::_DurPositiveBasicTimeIdent:
		return _DurPositiveBasicTime;
	case Identifier::_DurPositiveTimeIdent:
		return _DurPositiveTime;
	case Identifier::_DurPositiveTimeAltIdent:
		return _DurPositiveTimeAlt;
	case Identifier::_DurPositiveTimeRareIdent:
		return _DurPositiveTimeRare;
	case Identifier::_DurPositiveTimeEpicIdent:
		return _DurPositiveTimeEpic;
	case Identifier::_DurNegativeBasicIdent:
		return _DurNegativeBasic;
	case Identifier::_DurNegativeBasicTimeIdent:
		return _DurNegativeBasicTime;
	case Identifier::_DurNegativeTimeIdent:
		return _DurNegativeTime;
	case Identifier::_DurNegativeTimeAltIdent:
		return _DurNegativeTimeAlt;
	case Identifier::_DurNegativeTimeRareIdent:
		return _DurNegativeTimeRare;
	case Identifier::_DurNegativeTimeEpicIdent:
		return _DurNegativeTimeEpic;
	default:
		return 0;
	}
}

std::tuple<AlchemicEffect, ItemStrength, ItemType, int, float, bool> Settings::ClassifyItem(RE::AlchemyItem* item)
{
	RE::EffectSetting* sett = nullptr;
	if ((item->avEffectSetting) == nullptr && item->effects.size() == 0) {
		return { 0, ItemStrength::kStandard, ItemType::kFood, 0, 0.0f, false};
	}
	// we look at max 4 effects
	AlchemicEffect av[4]{
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
	AlchemicEffect tmp = 0;
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
				if ((tmp = ConvertToAlchemyEffectPrimary(sett)) > 0) {

					av[i] |= tmp;
				}
				if (sett->data.archetype == RE::EffectArchetypes::ArchetypeID::kDualValueModifier && (tmp = ConvertToAlchemyEffectSecondary(sett)) > 0) {

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
			av[0] = ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV);
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kMagicka:
			av[0] = ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV);
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		case RE::ActorValue::kStamina:
			av[0] = ConvertToAlchemyEffect(item->avEffectSetting->data.primaryAV);
			mag[0] = err.magnitude;
			dur[0] = 1;
			break;
		}
	}
	// analyze the effect types
	AlchemicEffect alch = 0;
	ItemStrength str = ItemStrength::kWeak;
	float maxmag = 0;
	int maxdur = 0;
	for (int i = 0; i < 4; i++) {
		if ((av[i] & AlchemicEffect::kHealth) > 0 ||
			(av[i] & AlchemicEffect::kMagicka) > 0 ||
			(av[i] & AlchemicEffect::kStamina) > 0 ||
			(av[i] & AlchemicEffect::kHealRate) > 0 ||
			(av[i] & AlchemicEffect::kMagickaRate) > 0 ||
			(av[i] & AlchemicEffect::kStaminaRate) > 0 ||
			(av[i] & AlchemicEffect::kHealRateMult) > 0 ||
			(av[i] & AlchemicEffect::kMagickaRateMult) > 0 ||
			(av[i] & AlchemicEffect::kStaminaRateMult) > 0)
			{
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
		alch = AlchemicEffect::kBlood;
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
	std::vector<AlchemicEffect> effectsToRemovePotion;
	std::vector<AlchemicEffect> effectsToRemovePoison;
	std::vector<AlchemicEffect> effectsToRemoveFood;
	// iterate over existing alchemy effects
	for (uint64_t i = 0; i <= 63; i++) {
		// potion
		if ((_potionEffectsFound & (AlchemicEffect(0, 1) << i)).IsValid()) {
			// found existing effect, which is not excluded
		} else {
			// effect excluded or not present in any items
			// remove from all distribution rules
			effectsToRemovePotion.push_back(AlchemicEffect(0, 1) << i);
		}
		// poison
		if ((_poisonEffectsFound & (AlchemicEffect(0, 1) << i)).IsValid()) {
			// found existing effect, which is not excluded
		} else {
			// effect excluded or not present in any items
			// remove from all distribution rules
			effectsToRemovePoison.push_back(AlchemicEffect(0, 1) << i);
		}
		// food
		if ((_foodEffectsFound & (AlchemicEffect(0, 1 << i))).IsValid()) {
			// found existing effect, which is not excluded
		} else {
			// effect excluded or not present in any items
			// remove from all distribution rules
			effectsToRemoveFood.push_back(AlchemicEffect(0, 1) << i);
		}
	}
}

#pragma endregion
