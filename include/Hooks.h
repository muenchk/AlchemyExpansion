#pragma once

namespace Hooks
{
	class PoisonHook
	{
	public:
		static void Hook()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<uintptr_t> hook1{ RELOCATION_ID(15786, 16024) };
			_PoisonObject = trampoline.write_call<5>(hook1.address(), PoisonObject);
		}

	private:
		static void PoisonObject(RE::InventoryEntryData* data, RE::AlchemyItem* a_alchItem, std::uint32_t a_count, uint64_t arg4, uint64_t arg5, RE::Actor* player, RE::TESObjectWEAP* weap);

		static inline REL::Relocation<decltype(PoisonObject)> _PoisonObject;
	};

	class Hook_OnMeleeHit
	{
	public:
		static void install()
		{
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37673, 38627), REL::VariantOffset(0x3c0, 0x4A8, 0) };  //140628C20       14064E760
			auto& trampoline = SKSE::GetTrampoline();
			_ProcessHit = trampoline.write_call<5>(hook.address() /* + RELOCATION_OFFSET(0x3C0, 0x4A8)*/, processHit);
			logger::info("hook:OnMeleeHit");
		}

	private:
		static void processHit(RE::Actor* victim, RE::HitData& hitData);
		static inline REL::Relocation<decltype(processHit)> _ProcessHit;  //140626400       14064BAB0
	};

	class Hook_OnMeleeCollision
	{
	public:
		static void install()
		{
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37650, 38603), REL::VariantOffset(0x38B, 0x45A, 0) };  //SE:627930 + 38B AE:64D350 +  45A
			auto& trampoline = SKSE::GetTrampoline();

			_ProcessHit = trampoline.write_call<5>(hook.address(), processHit);

			logger::info("hook:OnMeleeCollision");
		}

	private:
		static void processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr);

		static inline REL::Relocation<decltype(processHit)> _ProcessHit;
	};

	void Install();
}
