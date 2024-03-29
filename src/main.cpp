#include "Events.h"
#include "Settings.h"
#include "Game.h"
#include "DataStorage.h"
#include "Compatibility.h"
#include "Papyrus.h"
#include "Data.h"
#include "Hooks.h"

namespace
{
	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;
#else
		const auto level = spdlog::level::info;
#endif
		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		//spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
		spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);

		Profile::Init(Settings::PluginNamePlain);
	}
}
/*
#if defined(SKYRIM_SUPPORT_AE353)
	// AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);

	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

#elif defined(SKYRIM_SUPPORT_AE)

// AE after to 1.6.353
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);

	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

#else
// SSE
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
	InitializeLog();
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}
	return true;
} 
#endif
*/

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	InitializeLog();
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}
	return true;
} 

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("KoeniglichePM");

	v.UsesAddressLibrary();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST, SKSE::RUNTIME_LATEST_VR });
	v.HasNoStructUse();

	return v;
}();

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
		auto begin = std::chrono::steady_clock::now();
		// init Data
		Data::GetSingleton()->Init();
		// init game objects and load pluginnames
		Settings::InitGameStuff();
		// load settings
		Settings::Load(); // also resaves the file
		logger::info("Settings loaded");
		// load distribution settings
		Settings::LoadDistrConfig();
		logger::info("Distribution configuration loaded");
		// classify currently loaded game items
		Settings::ClassifyItems();
		Settings::CleanAlchemyEffects();
		logger::info("Items classified");
		// register data storage
		// datastorage must always register game callbacks before events, to ensure read data is present
		Storage::Register();
		// register compatibility
		Compatibility::Register();
		// register eventhandlers
		Events::RegisterAllEventHandlers();
		logger::info("Registered Events");
		logger::info("Registered Console Commands");

		// patch potions and ingredients
		Data::GetSingleton()->PatchGameData();
		logger::info("Patched Game Data");
		PROF1_1("{}[main] [Startup] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
	}
}
/*
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifdef SKYRIM_SUPPORT_AE
	InitializeLog();
#endif

	loginfo("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());
	profile("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);

	//SKSE::AllocTrampoline(1<<4);

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		logger::error("[LOADPLUGIN] couldn't register listener");
		return false;
	}

	auto serialization = (SKSE::SerializationInterface*)a_skse->QueryInterface(SKSE::LoadInterface::kSerialization);
	if (!serialization) {
		logger::error("[LOADPLUGIN] couldn't get serialization interface");
		return false;
	}

	// register game load save events
	Game::SaveLoad::GetSingleton()->Register(serialization, 0xFD34899E);

	// register papyrus functions
	SKSE::GetPapyrusInterface()->Register(Papyrus::Register);

	return true;
}
*/

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	auto version = plugin->GetVersion();
	loginfo("{} v{}"sv, plugin->GetName(), version);
	profile("{} v{}"sv, plugin->GetName(), version);

	
    SKSE::Init(a_skse);
	SKSE::AllocTrampoline(1 << 7);


	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		logger::error("[LOADPLUGIN] couldn't register listener");
		return false;
	}

	auto serialization = (SKSE::SerializationInterface*)a_skse->QueryInterface(SKSE::LoadInterface::kSerialization);
	if (!serialization) {
		logger::error("[LOADPLUGIN] couldn't get serialization interface");
		return false;
	}

	// register game load save events
	Game::SaveLoad::GetSingleton()->Register(serialization, 0xFD34899E);

	// register papyrus functions
	SKSE::GetPapyrusInterface()->Register(Papyrus::Register);

	Hooks::Install();

	return true;
}


extern "C" DLLEXPORT void* SKSEAPI RequestPluginAPI()
{
	//auto api = NPCsUsePotions::NUPInterface::GetSingleton();

	logger::info("[API] ALCH API requested and returned");

	//return static_cast<void*>(api);
	return (void*)nullptr;
}
