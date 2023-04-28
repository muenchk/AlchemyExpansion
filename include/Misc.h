#pragma once

class Effect
{
public:
	std::string name = "";
	RE::FormID formid = 0;
	std::string editorID = "";
	std::string pluginName = "";
	RE::ActorValue actorValue = RE::ActorValue::kNone;
	AlchemyEffect alchemyEffect = AlchemyEffect::kNone;
	bool overwrite = false;
	bool detrimental = false;

	RE::EffectSetting* effect = nullptr;
};

class Ingredient
{
public:
	std::string name = "";
	RE::FormID formid = 0;
	std::string editorID = "";
	std::string pluginName = "";
	float weight = 0;
	int value = 0;
	std::vector<std::string> effects;
	std::vector<int> durations;
	std::vector<float> magnitudes;
	std::vector<RE::EffectSetting*> magicEffects;

	RE::IngredientItem* item = nullptr;
};
