#include "Hooks.h"
#include "Logging.h"
#include "Utility.h"


namespace Hooks
{
	void Install()
	{
		//PoisonHook::Hook();
		Hook_OnMeleeHit::install();
		Hook_OnMeleeCollision::install();
	}

	void PoisonHook::PoisonObject(RE::InventoryEntryData* data, RE::AlchemyItem* a_alchItem, std::uint32_t a_count, uint64_t arg4, uint64_t arg5, RE::Actor* player, RE::TESObjectWEAP* weap)
	{
		profile("hook succeeded, Alch: {}, Count {}", Utility::PrintForm(a_alchItem), a_count);
		RE::DebugNotification("Poison");
		_PoisonObject(data, a_alchItem, a_count, arg4, arg5, player, weap);
		//int x;
		//return;
	}

	void Hook_OnMeleeHit::processHit(RE::Actor* victim, RE::HitData& hitData)
	{
		using HITFLAG = RE::HitData::Flag;
		auto aggressor = hitData.aggressor.get().get();
		if (!victim || !aggressor || victim->IsDead()) {
			_ProcessHit(victim, hitData);
			return;
		}
		profile("registered hit");
		if (auto x = hitData.sourceRef.get().get())
		{
			profile("source type {} ", x->GetBaseObject()->GetFormType());
		}
	}

	void Hook_OnMeleeCollision::processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr)
	{
		profile("kill hit");
		_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
	}
}
