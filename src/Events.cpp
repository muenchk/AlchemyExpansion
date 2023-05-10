#include <string.h>
#include <chrono>
#include <thread>
#include <forward_list>
#include <semaphore>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <fstream>
#include <iostream>
#include <limits>
#include <filesystem>
#include <deque>

#include "Game.h"
#include "Data.h"
#include "Compatibility.h"
#include "Settings.h"
#include "Events.h"
#include "Utility.h"
#include "Configuration.h"
		
namespace Events
{
	using AlchemyEffect = AlchemyEffect;

	/// <summary>
	/// random number generator for processing probabilities
	/// </summary>
	/// <param name=""></param>
	/// <returns></returns>
	static std::mt19937 rand((unsigned int)(std::chrono::system_clock::now().time_since_epoch().count()));
	/// <summary>
	/// trims random numbers to 1 to 100
	/// </summary>
	static std::uniform_int_distribution<signed> rand100(1, 100);
	/// <summary>
	/// trims random numbers to 1 to 3
	/// </summary>
	static std::uniform_int_distribution<signed> rand3(1, 3);

#define Look(s) RE::TESForm::LookupByEditorID(s)

#pragma region Data

	static Data* data = nullptr;

	static Compatibility* comp = nullptr;

	/// <summary>
	/// determines whether events and functions are run
	/// </summary>
	static bool initialized = false;

	/// <summary>
	/// enables all active functions
	/// </summary>
	static bool enableProcessing = false;
#define EvalProcessing()   \
	if (!enableProcessing) \
		return;
#define GetProcessing() \
	enableProcessing
#define WaitProcessing() \
	while (!enableProcessing) \
		std::this_thread::sleep_for(100ms);
#define EvalProcessingEvent() \
	if (!enableProcessing)    \
		return EventResult::kContinue;
	/// <summary>
	/// Returns whether processing of actors is allowed
	/// </summary>
	/// <returns></returns>
	bool CanProcess()
	{
		return enableProcessing;
	}

	/// <summary>
	/// Temporarily locks processing for all functions
	/// </summary>
	/// <returns></returns>
	bool LockProcessing()
	{
		bool val = enableProcessing;
		enableProcessing = false;
		return val;
	}
	/// <summary>
	/// Unlocks processing for all functions
	/// </summary>
	void UnlockProcessing()
	{
		enableProcessing = true;
	}

	/// <summary>
	/// signals whether the player has died
	/// </summary>
	static bool playerdied = false;

#define CheckDeadEvent                       \
	LOG1_1("{}[PlayerDead] {}", playerdied); \
	if (playerdied == true) {                \
		return EventResult::kContinue;       \
	}

#define ReEvalPlayerDeath                                         \
	if (RE::PlayerCharacter::GetSingleton()->IsDead() == false) { \
		playerdied = false;                                       \
	}                                                        

#define CheckDeadCheckHandlerLoop \
	if (playerdied) {             \
		break;                    \
	}

	/// <summary>
	/// since the TESDeathEvent seems to be able to fire more than once for an actor we need to track the deaths
	/// </summary>
	static std::unordered_set<RE::FormID> deads;

#pragma endregion

	/// <summary>
	/// initializes important variables, which need to be initialized every time a game is loaded
	/// </summary>
	void InitializeCompatibilityObjects()
	{
		EvalProcessing();
		// now that the game was loaded we can try to initialize all our variables we conuldn't before
		if (!initialized) {
			// if we are in com mode, try to find the needed items. If we cannot find them, deactivate comp mode
			initialized = true;
		}
	}

	/// <summary>
	/// Enables processing and starts handlers
	/// </summary>
	void LoadGameSub()
	{
		auto begin = std::chrono::steady_clock::now();
		LOG_1("{}[Events] [LoadGameSub]");
		// set player to alive
		ReEvalPlayerDeath;

		enableProcessing = true;

		InitializeCompatibilityObjects();

		LOG_1("{}[Events] [LoadGameSub] end");
		PROF1_1("{}[Events] [LoadGameSub] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
	}

	/// <summary>
	/// EventHandler for TESDeathEvent
	/// removed unused potions and poisons from actor, to avoid economy instability
	/// only registered if itemremoval is activated in the settings
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		EvalProcessingEvent();
		auto begin = std::chrono::steady_clock::now();
		LOG_1("{}[Events] [TESDeathEvent]");
		InitializeCompatibilityObjects();
		RE::Actor* actor = nullptr;
		if (a_event == nullptr || a_event->actorDying == nullptr) {
			LOG_4("{}[Events] [TESDeathEvent] Died due to invalid event");
			goto TESDeathEventEnd;
		}
		actor = a_event->actorDying->As<RE::Actor>();
		if (Utility::ValidateActor(actor)) {
			if (actor->IsPlayerRef()) {
				LOG_4("{}[Events] [TESDeathEvent] player died");
				playerdied = true;
			} else {
				// if not already dead, do stuff
				if (deads.contains(actor->GetFormID()) == false) {
				}
			}
		}
	TESDeathEventEnd:
		PROF1_1("{}[Events] [TESDeathEvent] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
		return EventResult::kContinue;
	}

	/// <summary>
	/// Processes TESHitEvents
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*)
	{
		EvalProcessingEvent();
		
		if (a_event && a_event->target.get()) {
			RE::Actor* actor = a_event->target.get()->As<RE::Actor>();
			if (actor) {
				
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// handles TESCombatEvent
	/// registers the actor for tracking and handles giving them potions, poisons and food, beforehand.
	/// also makes then eat food before the fight.
	/// </summary>
	/// <param name="a_event">event parameters like the actor we need to handle</param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*)
	{
		EvalProcessingEvent();
		//if (!Settings::_featDisableOutOfCombatProcessing)
		//	return EventResult::kContinue;
		auto begin = std::chrono::steady_clock::now();
		LOG_1("{}[Events] [TESCombatEvent]");
		InitializeCompatibilityObjects();
		auto actor = a_event->actor->As<RE::Actor>();
		if (Utility::ValidateActor(actor) && !actor->IsDead() && actor != RE::PlayerCharacter::GetSingleton() && actor->IsChild() == false) {
			
		}
		PROF1_2("{}[Events] [TESCombatEvent] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Actors being attached / detached
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESCellAttachDetachEvent* a_event, RE::BSTEventSource<RE::TESCellAttachDetachEvent>*)
	{
		EvalProcessingEvent();
		// return if feature disabled
		ReEvalPlayerDeath;
		//auto begin = std::chrono::steady_clock::now();

		if (a_event && a_event->reference) {
			RE::Actor* actor = a_event->reference->As<RE::Actor>();
			if (Utility::ValidateActor(actor) && deads.find(actor->GetFormID()) == deads.end() && !actor->IsDead() && !actor->IsPlayerRef()) {
				if (a_event->attached) {
				} else {
				}
			}
			//PROF1_2("{}[Events] [CellAttachDetachEvent] execution time: {} µs", std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count()));
		}
		return EventResult::kContinue;
	}

	std::unordered_set<RE::FormID> cells;
	/// <summary>
	/// EventHandler to fix not playing potion, poison, food sound
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*)
	{
		EvalProcessingEvent();
		//LOG_1("{}[Events] [BGSActorCellEvent]");
		if (a_event) {

		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Debug purposes. It calculates the distribution rules for all npcs in the cell
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		EvalProcessingEvent();
		auto actor = a_event->actor->As<RE::Actor>();
		if (Utility::ValidateActor(actor)) {
			if (actor->IsPlayerRef()) {

			} else {

			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Activation Events
	/// </summary>
	/// <param name="a_event">event parameters like the actor we need to handle</param>
	/// <param name=""></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESActivateEvent* a_event, RE::BSTEventSource<RE::TESActivateEvent>*)
	{
		EvalProcessingEvent();
		if (a_event && a_event->actionRef.get() && a_event->objectActivated.get()) {
			RE::TESObjectREFR* refr = a_event->objectActivated.get();
			RE::TESBoundObject* base = refr->GetBaseObject();
			RE::TESObjectACTI* baseactivator = base->As<RE::TESObjectACTI>();
			RE::TESObjectREFR* activator = a_event->actionRef.get();

			if (baseactivator) {
				if (Configuration::baseObjectMines()->contains(baseactivator->GetFormID())) {
					// we found a mine, reset timer for last time a mine was accessed, if and only if the player is 
					// the activator
					if (activator->IsPlayerRef()) {
						data->UpdateLastMineActivatedTime();
					}
				}
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// Handles an item being removed from a container
	/// </summary>
	/// <param name="container">The container the item was removed from</param>
	/// <param name="baseObj">The base object that has been removed</param>
	/// <param name="count">The number of items that have been removed</param>
	/// <param name="destinationContainer">The container the items have been moved to</param>
	/// <param name="a_event">The event information</param>
	void OnItemRemoved(RE::TESObjectREFR* container, RE::TESBoundObject* baseObj, int /*count*/, RE::TESObjectREFR* /*destinationContainer*/, const RE::TESContainerChangedEvent* /*a_event*/)
	{
		LOG2_1("{}[Events] [OnItemRemovedEvent] {} removed from {}", Utility::PrintForm(baseObj), Utility::PrintForm(container));
		RE::Actor* actor = container->As<RE::Actor>();
		if (Utility::ValidateActor(actor)) {

		}
		
		// handle event for generic reference
		
		return;
	}

	/// <summary>
	/// Handles an item being added to a container
	/// </summary>
	/// <param name="container">The container the item is added to</param>
	/// <param name="baseObj">The base object that has been added</param>
	/// <param name="count">The number of items added</param>
	/// <param name="sourceContainer">The container the item was in before</param>
	/// <param name="a_event">The event information</param>
	void OnItemAdded(RE::TESObjectREFR* container, RE::TESBoundObject* baseObj, int /*count*/, RE::TESObjectREFR* /*sourceContainer*/, const RE::TESContainerChangedEvent* /*a_event*/)
	{
		LOG2_1("{}[Events] [OnItemAddedEvent] {} added to {}", Utility::PrintForm(baseObj), Utility::PrintForm(container));
		RE::Actor* actor = container->As<RE::Actor>();
		if (Utility::ValidateActor(actor)) {
			if (actor->IsPlayerRef()) {


				// the assumption is that if an ore is added to the player without a source container, than
				// they have mined the ore
				// as additional restriction, this only applied if the last activation of a mine, has been
				// less than 15 seconds ago
				if (Configuration::oreMap()->contains(baseObj->GetFormID()) && data->GetLastMineActivatedTime() - std::chrono::system_clock::now() > -15s) {
					// get vector with entries to give
					auto itr = Configuration::oreMap()->find(baseObj->GetFormID());
					if (itr != Configuration::oreMap()->end()) {
						auto vec = itr->second;
						auto iter = vec.begin();
						while (iter != vec.end()) {
							if (*iter == nullptr) {
								// error item not valid, clean up vector
								iter = vec.erase(iter);
								Configuration::oreMap()->insert_or_assign(baseObj->GetFormID(), vec);
							} else if ((*iter)->object == nullptr) {
								delete *iter;
								Configuration::oreMap()->insert_or_assign(baseObj->GetFormID(), vec);
							} else {
								for (int x = 0; x < (*iter)->num; x++) {
									if (rand100(rand) < (*iter)->chance) {
										container->AddObjectToContainer((*iter)->object, nullptr, 1, nullptr);
									}
								}
							}
							iter++;
						}
					}
				}
			}
		}
		
		// handle event for generic objects
		return;
	}

	EventResult EventHandler::ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>* /*a_eventSource*/)
	{
		// this event handles all object transfers between containers in the game
		// this can be deived into multiple base events: OnItemRemoved and OnItemAdded
		EvalProcessingEvent();

		if (a_event && a_event->baseObj != 0 && a_event->itemCount != 0) {
			RE::TESObjectREFR* oldCont = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_event->oldContainer);
			RE::TESObjectREFR* newCont = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_event->newContainer);
			RE::TESBoundObject* baseObj = RE::TESForm::LookupByID<RE::TESBoundObject>(a_event->baseObj);
			if (baseObj && oldCont) {
				OnItemRemoved(oldCont, baseObj, a_event->itemCount, newCont, a_event);
			}
			if (baseObj && newCont) {
				OnItemAdded(newCont, baseObj, a_event->itemCount, oldCont, a_event);
			}
		}

		return EventResult::kContinue;
	}

	/// <summary>
	/// EventHandler for Harvested Items
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESHarvestedEvent::ItemHarvested* a_event, RE::BSTEventSource<RE::TESHarvestedEvent::ItemHarvested>*)
	{
		EvalProcessingEvent();
		LOG_1("{}[Events] [HarvestEvent]");
		ReEvalPlayerDeath;
		// check that all event fields are valid
		if (a_event && a_event->produceItem && a_event->harvester) {
			LOG2_1("{}[Events] [HarvestEvent] harvested: {} {}", Utility::GetHex(a_event->produceItem->GetFormID()), a_event->produceItem->GetName());
			auto itr = Configuration::harvestMap()->find(a_event->produceItem->GetFormID());
			if (itr != Configuration::harvestMap()->end()) {
				auto vec = itr->second;
				auto iter = vec.begin();
				while (iter != vec.end()) {
					if (*iter == nullptr) {
						// error item not valid, clean up vector
						iter = vec.erase(iter);
						Configuration::harvestMap()->insert_or_assign(a_event->produceItem->GetFormID(), vec);
					} else if ((*iter)->object == nullptr) {
						delete *iter;
						Configuration::harvestMap()->insert_or_assign(a_event->produceItem->GetFormID(), vec);
					} else {
						for (int x = 0; x < (*iter)->num; x++) {
							if (rand100(rand) < (*iter)->chance) {
								a_event->harvester->AddObjectToContainer((*iter)->object, nullptr, 1, nullptr);
							}
						}
					}
					iter++;
				}
			}
		}

		return EventResult::kContinue;
	}
	
	/// <summary>
	/// EventHandler for catching deleted forms / actors
	/// </summary>
	/// <param name="a_event"></param>
	/// <param name="a_eventSource"></param>
	/// <returns></returns>
	EventResult EventHandler::ProcessEvent(const RE::TESFormDeleteEvent* a_event, RE::BSTEventSource<RE::TESFormDeleteEvent>*)
	{
		// very important event. Allows to catch actors and other stuff that gets deleted, without dying, which could cause CTDs otherwise
		if (a_event && a_event->formID != 0) {
			data->DeleteFormCustom(a_event->formID);
		}
		return EventResult::kContinue;
	}

	/// <summary>
	/// Callback on saving the game
	/// </summary>
	/// <param name=""></param>
	void SaveGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [SaveGameCallback]");

	}

	/// <summary>
	/// Callback on loading a save game, initializes actor processing
	/// </summary>
	/// <param name=""></param>
	void LoadGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [LoadGameCallback]");
		LoadGameSub();
	}

	/// <summary>
	/// Callback on reverting the game. Disables processing and stops all handlers
	/// </summary>
	/// <param name=""></param>
	void RevertGameCallback(SKSE::SerializationInterface* /*a_intfc*/)
	{
		LOG_1("{}[Events] [RevertGameCallback]");
		enableProcessing = false;
	}

    /// <summary>
    /// returns singleton to the EventHandler
    /// </summary>
    /// <returns></returns>
    EventHandler* EventHandler::GetSingleton()
    {
        static EventHandler singleton;
        return std::addressof(singleton);
    }


    /// <summary>
    /// Registers us for all Events we want to receive
    /// </summary>
	void EventHandler::Register()
	{
		auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		scriptEventSourceHolder->GetEventSource<RE::TESHitEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHitEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESCombatEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESCombatEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESEquipEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESEquipEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESDeathEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESDeathEvent).name());
		RE::PlayerCharacter::GetSingleton()->AsBGSActorCellEventSource()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::BGSActorCellEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESCellAttachDetachEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESCellAttachDetachEvent).name());
		scriptEventSourceHolder->GetEventSource<RE::TESFormDeleteEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESFormDeleteEvent).name())
		scriptEventSourceHolder->GetEventSource<RE::TESContainerChangedEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESContainerChangedEvent).name())
		scriptEventSourceHolder->GetEventSource<RE::TESActivateEvent>()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESActivateEvent).name())
		RE::TESHarvestedEvent::GetEventSource()->AddEventSink(EventHandler::GetSingleton());
		LOG1_1("{}Registered {}", typeid(RE::TESHarvestedEvent::ItemHarvested).name());

		Game::SaveLoad::GetSingleton()->RegisterForLoadCallback(0xFF000001, LoadGameCallback);
		LOG1_1("{}Registered {}", typeid(LoadGameCallback).name());
		Game::SaveLoad::GetSingleton()->RegisterForRevertCallback(0xFF000002, RevertGameCallback);
		LOG1_1("{}Registered {}", typeid(RevertGameCallback).name());
		Game::SaveLoad::GetSingleton()->RegisterForSaveCallback(0xFF000003, SaveGameCallback);
		LOG1_1("{}Registered {}", typeid(SaveGameCallback).name());

		data = Data::GetSingleton();
		comp = Compatibility::GetSingleton();
	}

	/// <summary>
	/// Registers all EventHandlers, if we would have multiple
	/// </summary>
	void RegisterAllEventHandlers()
	{
		EventHandler::Register();
		LOG_1("{}Registered all event handlers"sv);
	}

	/// <summary>
	/// sets the main threads to stop on the next iteration
	/// </summary>
	void DisableThreads()
	{
	}
}
