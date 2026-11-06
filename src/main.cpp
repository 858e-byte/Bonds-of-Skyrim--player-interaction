// Bonds of Skyrim - Alone with G - BondsOfSkyrim_Solo.dll
// Copyright (C) 2026 nozarashi001
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program (LICENSE.txt).
// If not, see <https://www.gnu.org/licenses/>.
//
// Built with CommonLibSSE-NG (GPL-3.0-or-later WITH Modding Exception AND GPL-3.0 Linking Exception).

// BondsOfSkyrim_Solo.dll : le menu « seul avec G » (vue PrismaUI AMLS_Solo) et la liste de nourriture du joueur.
//
// Papyrus (script AMLS_Native, fonctions globales)
//   AMLS_Native.IsLoaded()                -> true (la DLL est la)
//   AMLS_Native.UIReady()                 -> la vue existe et PrismaUI repond
//   AMLS_Native.OpenUI(donnees, pause)    -> Show + Focus (jeu en pause ou non) + amlsOpen(donnees)
//   AMLS_Native.CloseUI()                 -> Unfocus + Hide
//   AMLS_Native.GetFoods(max)             -> nourriture et boissons du joueur (pas les poisons), les plus cheres d'abord
//   ModEvent "AMLS_UIAction" ("act", numero) <- amlsAction("<numero>") <- un choix (l'ecran se ferme)
//   ModEvent "AMLS_UIAction" ("close", 0)    <- amlsClose("")          <- Echap ou G

#include "PrismaUI_API.h"

#include <algorithm>
#include <vector>

namespace
{
	PRISMA_UI_API::IVPrismaUI1* g_prisma = nullptr;
	PrismaView g_view = 0;
	bool g_domReady = false;

	constexpr auto kViewPath = "AMLS_Solo/index.html";
	constexpr auto kEvent = "AMLS_UIAction";

	// Un ModEvent Papyrus doit partir du fil du jeu : la page appelle depuis le fil de PrismaUI.
	void SendModEvent(std::string a_arg, float a_num)
	{
		SKSE::GetTaskInterface()->AddTask([arg = std::move(a_arg), a_num]() {
			SKSE::ModCallbackEvent ev{ kEvent, RE::BSFixedString(arg.c_str()), a_num, nullptr };
			SKSE::GetModCallbackEventSource()->SendEvent(&ev);
		});
	}

	void CloseView()
	{
		if (!g_prisma || !g_view) {
			return;
		}
		if (g_prisma->HasFocus(g_view)) {
			g_prisma->Unfocus(g_view);
		}
		if (!g_prisma->IsHidden(g_view)) {
			g_prisma->Hide(g_view);
		}
	}

	void CreateView()
	{
		g_prisma = static_cast<PRISMA_UI_API::IVPrismaUI1*>(PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		if (!g_prisma) {
			logger::warn("PrismaUI introuvable : le menu est desactive");
			return;
		}
		g_view = g_prisma->CreateView(kViewPath, [](PrismaView a_view) {
			g_domReady = true;
			logger::info("vue {} prete", a_view);
		});
		g_prisma->RegisterJSListener(g_view, "amlsAction", [](const char* a_arg) {
			const float number = a_arg ? static_cast<float>(std::atoi(a_arg)) : -1.0f;
			logger::info("la page choisit : {}", number);
			SKSE::GetTaskInterface()->AddTask([]() { CloseView(); });
			SendModEvent("act", number);
		});
		g_prisma->RegisterJSListener(g_view, "amlsClose", [](const char*) {
			SKSE::GetTaskInterface()->AddTask([]() { CloseView(); });
			SendModEvent("close", 0.0f);
		});
		g_prisma->Hide(g_view);
		logger::info("vue {} creee ({})", g_view, kViewPath);
	}

	bool IsLoaded(RE::StaticFunctionTag*)
	{
		return true;
	}

	bool UIReady(RE::StaticFunctionTag*)
	{
		return g_prisma && g_view && g_domReady && g_prisma->IsValid(g_view);
	}

	void OpenUI(RE::StaticFunctionTag*, std::string a_data, bool a_pause)
	{
		if (!UIReady(nullptr)) {
			return;
		}
		g_prisma->Show(g_view);
		g_prisma->Focus(g_view, a_pause);
		g_prisma->InteropCall(g_view, "amlsOpen", a_data.c_str());
	}

	void CloseUI(RE::StaticFunctionTag*)
	{
		CloseView();
	}

	// Le joueur a parfois des centaines d'objets : les parcourir en Papyrus prendrait des secondes.
	std::vector<RE::TESForm*> GetFoods(RE::StaticFunctionTag*, std::int32_t a_max)
	{
		std::vector<RE::TESForm*> foods;
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || a_max <= 0) {
			return foods;
		}
		// 0.1.3: no GetPlayable() here: for potions it reads a flag they never have, and the list stayed empty (seen in
		// game). Food = the food flag, or the base game's VendorItemFood keyword (0x8CDEA), for mods that forget the flag.
		const auto vendorFood = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0008CDEA);
		const auto items = player->GetInventory([vendorFood](RE::TESBoundObject& a_obj) {
			const auto alch = a_obj.As<RE::AlchemyItem>();
			return alch && !alch->IsPoison() && (alch->IsFood() || (vendorFood && alch->HasKeyword(vendorFood)));
		});
		std::vector<std::pair<std::int32_t, RE::TESForm*>> found;
		for (const auto& [obj, data] : items) {
			if (obj && data.first > 0 && obj->GetName() && obj->GetName()[0]) {
				found.emplace_back(obj->GetGoldValue(), obj);
			}
		}
		std::ranges::sort(found, [](const auto& a, const auto& b) { return a.first > b.first; });
		for (const auto& [value, form] : found) {
			if (static_cast<std::int32_t>(foods.size()) >= a_max) {
				break;
			}
			foods.push_back(form);
		}
		return foods;
	}

	// (noms compares sans les majuscules : Papyrus partage ses chaines sans les distinguer)
	// 0.1.7 : le meilleur sort connu d'une ecole ("Destruction", "Restoration"...), pour l'entrainement a la magie.
	// Sorts ajoutes au joueur + ceux de sa base et de sa race ; sorts a lancer en main seulement (pas pouvoirs, pas
	// capacites, pas a deux mains). Le plus cher en magie d'abord (le plus avance).
	RE::TESForm* GetSchoolSpell(RE::StaticFunctionTag*, std::string a_school)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return nullptr;
		}
		RE::ActorValue wanted = RE::ActorValue::kNone;
		if (_stricmp(a_school.c_str(), "Destruction") == 0) wanted = RE::ActorValue::kDestruction;
		else if (_stricmp(a_school.c_str(), "Restoration") == 0) wanted = RE::ActorValue::kRestoration;
		else if (_stricmp(a_school.c_str(), "Alteration") == 0) wanted = RE::ActorValue::kAlteration;
		else if (_stricmp(a_school.c_str(), "Illusion") == 0) wanted = RE::ActorValue::kIllusion;
		else if (_stricmp(a_school.c_str(), "Conjuration") == 0) wanted = RE::ActorValue::kConjuration;
		if (wanted == RE::ActorValue::kNone) {
			return nullptr;
		}
		RE::SpellItem* best = nullptr;
		float bestCost = -1.0f;
		const auto consider = [&](RE::SpellItem* a_spell) {
			if (!a_spell || a_spell->GetSpellType() != RE::MagicSystem::SpellType::kSpell || a_spell->IsTwoHanded()) {
				return;
			}
			if (a_spell->GetCastingType() == RE::MagicSystem::CastingType::kConstantEffect || a_spell->GetAssociatedSkill() != wanted) {
				return;
			}
			const float cost = a_spell->CalculateMagickaCost(player);
			if (cost > bestCost) {
				bestCost = cost;
				best = a_spell;
			}
		};
		for (const auto spell : player->GetActorRuntimeData().addedSpells) {
			consider(spell);
		}
		const auto list = [&](RE::TESSpellList::SpellData* a_data) {
			if (a_data && a_data->spells) {
				for (std::uint32_t i = 0; i < a_data->numSpells; ++i) {
					consider(a_data->spells[i]);
				}
			}
		};
		if (const auto base = player->GetActorBase()) {
			list(base->GetSpellList());
		}
		if (const auto race = player->GetRace()) {
			list(race->actorEffects);
		}
		return best;
	}

	// 0.1.7 : la meilleure arme que porte le joueur, par genre : 1 a une main (epee, dague, hache, masse), 2 a deux mains,
	// 3 arc. La plus forte d'abord (degats de base).
	RE::TESForm* GetBestWeapon(RE::StaticFunctionTag*, std::int32_t a_kind)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return nullptr;
		}
		const auto items = player->GetInventory([a_kind](RE::TESBoundObject& a_obj) {
			const auto w = a_obj.As<RE::TESObjectWEAP>();
			if (!w || !w->GetPlayable()) {
				return false;
			}
			switch (a_kind) {
			case 1:
				return w->IsOneHandedSword() || w->IsOneHandedDagger() || w->IsOneHandedAxe() || w->IsOneHandedMace();
			case 2:
				return w->IsTwoHandedSword() || w->IsTwoHandedAxe();
			case 3:
				return w->IsBow();
			default:
				return false;
			}
		});
		RE::TESForm* best = nullptr;
		int bestDamage = -1;
		for (const auto& [obj, data] : items) {
			const auto w = obj ? obj->As<RE::TESObjectWEAP>() : nullptr;
			if (w && data.first > 0 && static_cast<int>(w->GetAttackDamage()) > bestDamage) {
				bestDamage = w->GetAttackDamage();
				best = w;
			}
		}
		return best;
	}

	// 0.1.7 : le meilleur bouclier que porte le joueur (note d'armure de base).
	RE::TESForm* GetBestShield(RE::StaticFunctionTag*)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return nullptr;
		}
		const auto items = player->GetInventory([](RE::TESBoundObject& a_obj) {
			const auto a = a_obj.As<RE::TESObjectARMO>();
			return a && a->IsShield();
		});
		RE::TESForm* best = nullptr;
		float bestRating = -1.0f;
		for (const auto& [obj, data] : items) {
			const auto a = obj ? obj->As<RE::TESObjectARMO>() : nullptr;
			if (a && data.first > 0 && a->GetArmorRating() > bestRating) {
				bestRating = a->GetArmorRating();
				best = a;
			}
		}
		return best;
	}

	bool RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("GetSchoolSpell", "AMLS_Native", GetSchoolSpell);
		a_vm->RegisterFunction("GetBestWeapon", "AMLS_Native", GetBestWeapon);
		a_vm->RegisterFunction("GetBestShield", "AMLS_Native", GetBestShield);
		a_vm->RegisterFunction("IsLoaded", "AMLS_Native", IsLoaded, true);
		a_vm->RegisterFunction("UIReady", "AMLS_Native", UIReady);
		a_vm->RegisterFunction("OpenUI", "AMLS_Native", OpenUI);
		a_vm->RegisterFunction("CloseUI", "AMLS_Native", CloseUI);
		a_vm->RegisterFunction("GetFoods", "AMLS_Native", GetFoods);
		return true;
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
			CreateView();
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	logger::info("BondsOfSkyrim_Solo charge");
	SKSE::GetPapyrusInterface()->Register(RegisterFunctions);
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
