/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Scripts for spells with SPELLFAMILY_WARLOCK and SPELLFAMILY_GENERIC spells used by warlock players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warl_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum WarlockSpells
{
    WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS    = 54435,
    WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER  = 54443,
    WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD    = 54508,
    WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER   = 54509,
    WARLOCK_DEMONIC_EMPOWERMENT_IMP         = 54444,
    WARLOCK_DEMONIC_CIRCLE_SUMMON           = 48018,
    WARLOCK_DEMONIC_CIRCLE_TELEPORT         = 48020,
    WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST       = 62388,
    WARLOCK_HAUNT                           = 48181,
    WARLOCK_HAUNT_HEAL                      = 48210,
    WARLOCK_UNSTABLE_AFFLICTION_DISPEL      = 31117,
    WARLOCK_IMPROVED_HEALTH_FUNNEL_R1       = 18703,
    WARLOCK_IMPROVED_HEALTH_FUNNEL_R2       = 18704,
    WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R1  = 60955,
    WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R2  = 60956,
    WARLOCK_SOULSHATTER                     = 32835,
    WARLOCK_LIFE_TAP_ENERGIZE               = 31818,
    WARLOCK_LIFE_TAP_ENERGIZE_2             = 32553,
    WARLOCK_IMPROVED_LIFE_TAP_ICON_ID       = 208,
    WARLOCK_MANA_FEED_ICON_ID               = 1982,
    WARLOCK_HEALTHSTONE_CREATE              = 34130,
    WARLOCK_HEALTHSTONE_HEAL                = 6262,
    WARLOCK_DARK_INTENT_EFFECT              = 85767,
    WARLOCK_DARK_INTENT_EFFECT_2            = 85768
};

/// Updated 4.3.4
class spell_warl_banish : public SpellScriptLoader
{
public:
    spell_warl_banish() : SpellScriptLoader("spell_warl_banish") { }

    class spell_warl_banish_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_banish_SpellScript);

        bool Load()
        {
            _removed = false;
            return true;
        }

        void HandleBanish()
        {
            /// Casting Banish on a banished target will cancel the effect
            /// Check if the target already has Banish, if so, do nothing.
            if (Unit* target = GetHitUnit())
            {
                if (target->GetAuraEffect(SPELL_AURA_SCHOOL_IMMUNITY, SPELLFAMILY_WARLOCK, 0, 0x08000000, 0))
                {
                    // No need to remove old aura since its removed due to not stack by current Banish aura
                    PreventHitDefaultEffect(EFFECT_0);
                    PreventHitDefaultEffect(EFFECT_1);
                    PreventHitDefaultEffect(EFFECT_2);
                    _removed = true;
                }
            }
        }

        void RemoveAura()
        {
            if (_removed)
                PreventHitAura();
        }

        void Register()
        {
            BeforeHit += SpellHitFn(spell_warl_banish_SpellScript::HandleBanish);
            AfterHit += SpellHitFn(spell_warl_banish_SpellScript::RemoveAura);
        }

        bool _removed;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warl_banish_SpellScript();
    }
};

// 47193 Demonic Empowerment
/// Updated 4.3.4
class spell_warl_demonic_empowerment : public SpellScriptLoader
{
    public:
        spell_warl_demonic_empowerment() : SpellScriptLoader("spell_warl_demonic_empowerment") { }

        class spell_warl_demonic_empowerment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_empowerment_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_IMP))
                    return false;
                return true;
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Creature* targetCreature = GetHitCreature())
                {
                    if (targetCreature->isPet())
                    {
                        CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(targetCreature->GetEntry());
                        switch (ci->family)
                        {
                        case CREATURE_FAMILY_SUCCUBUS:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS, true);
                            break;
                        case CREATURE_FAMILY_VOIDWALKER:
                        {
                            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER);
                            int32 hp = int32(targetCreature->CountPctFromMaxHealth(20));
                            targetCreature->CastCustomSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER, &hp, NULL, NULL, true);
                            break;
                        }
                        case CREATURE_FAMILY_FELGUARD:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD, true);
                            break;
                        case CREATURE_FAMILY_FELHUNTER:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER, true);
                            break;
                        case CREATURE_FAMILY_IMP:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_IMP, true);
                            break;
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_demonic_empowerment_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_empowerment_SpellScript();
        }
};

// 6201 Create Healthstone
class spell_warl_create_healthstone : public SpellScriptLoader
{
    public:
        spell_warl_create_healthstone() : SpellScriptLoader("spell_warl_create_healthstone") { }

        class spell_warl_create_healthstone_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_create_healthstone_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_HEALTHSTONE_CREATE))
                    return false;
                if (!sSpellMgr->GetSpellInfo(WARLOCK_HEALTHSTONE_HEAL))
                    return false;
                return true;
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                    unitTarget->CastSpell(unitTarget, WARLOCK_HEALTHSTONE_CREATE, false);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_create_healthstone_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_create_healthstone_SpellScript();
        }
};

// 47422 Everlasting Affliction
/// Updated 4.3.4
class spell_warl_everlasting_affliction : public SpellScriptLoader
{
    public:
        spell_warl_everlasting_affliction() : SpellScriptLoader("spell_warl_everlasting_affliction") { }

        class spell_warl_everlasting_affliction_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_everlasting_affliction_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                    // Refresh corruption on target
                    if (AuraEffect* aur = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK, 0x2, 0, 0, GetCaster()->GetGUID()))
                        aur->GetBase()->RefreshDuration();
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_everlasting_affliction_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_everlasting_affliction_SpellScript();
        }
};

// 27285 Seed of Corruption
/// Updated 4.3.4
class spell_warl_seed_of_corruption : public SpellScriptLoader
{
    public:
        spell_warl_seed_of_corruption() : SpellScriptLoader("spell_warl_seed_of_corruption") { }

        class spell_warl_seed_of_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_seed_of_corruption_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (GetExplTargetUnit())
                    targets.remove(GetExplTargetUnit());
            }

            void HandleScript()
            {
                if (!GetCaster())
                    return;

                // Remove Soul Burn aura
                if (GetCaster()->HasAura(86664))
                {
                    GetCaster()->EnergizeBySpell(GetCaster(), 86664, 1, POWER_SOUL_SHARDS);
                    GetCaster()->RemoveAurasDueToSpell(86664);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warl_seed_of_corruption_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                AfterHit += SpellHitFn(spell_warl_seed_of_corruption_SpellScript::HandleScript);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_seed_of_corruption_SpellScript();
        }
};

// 29858 Soulshatter
/// Updated 4.3.4
class spell_warl_soulshatter : public SpellScriptLoader
{
    public:
        spell_warl_soulshatter() : SpellScriptLoader("spell_warl_soulshatter") { }

        class spell_warl_soulshatter_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_soulshatter_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_SOULSHATTER))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                    if (target->CanHaveThreatList() && target->getThreatManager().getThreat(caster) > 0.0f)
                        caster->CastSpell(target, WARLOCK_SOULSHATTER, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_soulshatter_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_soulshatter_SpellScript();
        }
};

// 1454 Life Tap
/// Updated 4.3.4
class spell_warl_life_tap : public SpellScriptLoader
{
    public:
        spell_warl_life_tap() : SpellScriptLoader("spell_warl_life_tap") { }

        class spell_warl_life_tap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_life_tap_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_LIFE_TAP_ENERGIZE) || !sSpellMgr->GetSpellInfo(WARLOCK_LIFE_TAP_ENERGIZE_2))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Player* caster = GetCaster()->ToPlayer();
                if (Unit* target = GetHitUnit())
                {
                    int32 damage = caster->CountPctFromMaxHealth(GetSpellInfo()->Effects[EFFECT_2].CalcValue());
                    int32 mana = CalculatePct(damage, GetSpellInfo()->Effects[EFFECT_1].CalcValue());

                    // Shouldn't Appear in Combat Log
                    target->ModifyHealth(-damage);

                    // Improved Life Tap mod
                    if (AuraEffect const* aurEff = caster->GetDummyAuraEffect(SPELLFAMILY_WARLOCK, WARLOCK_IMPROVED_LIFE_TAP_ICON_ID, 0))
                        AddPct(mana, aurEff->GetAmount());

                    caster->CastCustomSpell(target, WARLOCK_LIFE_TAP_ENERGIZE, &mana, NULL, NULL, false);

                    // Mana Feed
                    if (AuraEffect const* aurEff = caster->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_WARLOCK, WARLOCK_MANA_FEED_ICON_ID, 0))
                    {
                        int32 manaFeedVal = aurEff->GetAmount();
                        ApplyPct(manaFeedVal, mana);
                        caster->CastCustomSpell(caster, WARLOCK_LIFE_TAP_ENERGIZE_2, &manaFeedVal, NULL, NULL, true, NULL);
                    }
                }
            }

            SpellCastResult CheckCast()
            {
                if (int32(GetCaster()->GetHealth()) > int32(GetCaster()->CountPctFromMaxHealth(GetSpellInfo()->Effects[EFFECT_2].CalcValue())))
                    return SPELL_CAST_OK;
                return SPELL_FAILED_FIZZLE;
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_life_tap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_warl_life_tap_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_life_tap_SpellScript();
        }
};

// 48018 Demonic Circle: Summon
/// Updated 4.3.4
class spell_warl_demonic_circle_summon : public SpellScriptLoader
{
    public:
        spell_warl_demonic_circle_summon() : SpellScriptLoader("spell_warl_demonic_circle_summon") { }

        class spell_warl_demonic_circle_summon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_circle_summon_AuraScript);

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
            {
                // If effect is removed by expire remove the summoned demonic circle too.
                if (!(mode & AURA_EFFECT_HANDLE_REAPPLY))
                    GetTarget()->RemoveGameObject(GetId(), true);

                GetTarget()->RemoveAura(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST);
            }

            void HandleDummyTick(AuraEffect const* /*aurEff*/)
            {
                if (GameObject* circle = GetTarget()->GetGameObject(GetId()))
                {
                    // Here we check if player is in demonic circle teleport range, if so add
                    // WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST; allowing him to cast the WARLOCK_DEMONIC_CIRCLE_TELEPORT.
                    // If not in range remove the WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST.

                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_CIRCLE_TELEPORT);

                    if (GetTarget()->IsWithinDist(circle, spellInfo->GetMaxRange(true)))
                    {
                        if (!GetTarget()->HasAura(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST))
                            GetTarget()->CastSpell(GetTarget(), WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, true);
                    }
                    else
                        GetTarget()->RemoveAura(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_demonic_circle_summon_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demonic_circle_summon_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_circle_summon_AuraScript();
        }
};

// 48020 Demonic Circle: Teleport
/// Updated 4.3.4
class spell_warl_demonic_circle_teleport : public SpellScriptLoader
{
    public:
        spell_warl_demonic_circle_teleport() : SpellScriptLoader("spell_warl_demonic_circle_teleport") { }

        class spell_warl_demonic_circle_teleport_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_circle_teleport_AuraScript);

            void HandleTeleport(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* player = GetTarget()->ToPlayer())
                {
                    if (GameObject* circle = player->GetGameObject(WARLOCK_DEMONIC_CIRCLE_SUMMON))
                    {
                        player->NearTeleportTo(circle->GetPositionX(), circle->GetPositionY(), circle->GetPositionZ(), circle->GetOrientation());
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_demonic_circle_teleport_AuraScript::HandleTeleport, EFFECT_0, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_circle_teleport_AuraScript();
        }
};

// 48181 Haunt
/// Updated 4.3.4
class spell_warl_haunt : public SpellScriptLoader
{
    public:
        spell_warl_haunt() : SpellScriptLoader("spell_warl_haunt") { }

        class spell_warl_haunt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_haunt_SpellScript);

            void HandleOnHit()
            {
                if (Aura* aura = GetHitAura())
                    if (AuraEffect* aurEff = aura->GetEffect(EFFECT_1))
                        aurEff->SetAmount(CalculatePct(aurEff->GetAmount(), GetHitDamage()));
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_warl_haunt_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_haunt_SpellScript();
        }
};

/// Updated 4.3.4
class spell_warl_unstable_affliction : public SpellScriptLoader
{
    public:
        spell_warl_unstable_affliction() : SpellScriptLoader("spell_warl_unstable_affliction") { }

        class spell_warl_unstable_affliction_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_unstable_affliction_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_UNSTABLE_AFFLICTION_DISPEL))
                    return false;
                return true;
            }

            void HandleDispel(DispelInfo* dispelInfo)
            {
                if (!GetCaster() || !dispelInfo->GetDispeller())
                    return;

                if (AuraEffect const* aurEff = GetEffect(EFFECT_1))
                {
                    int32 damage = aurEff->GetAmount() * 9;
                    GetCaster()->CastCustomSpell(dispelInfo->GetDispeller(), WARLOCK_UNSTABLE_AFFLICTION_DISPEL, &damage, NULL, NULL, true, NULL, aurEff);
                }
            }

            void Register()
            {
                AfterDispel += AuraDispelFn(spell_warl_unstable_affliction_AuraScript::HandleDispel);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_unstable_affliction_AuraScript();
        }
};

// 755 Health Funnel
/// Updated 4.3.4
class spell_warl_health_funnel : public SpellScriptLoader
{
public:
    spell_warl_health_funnel() : SpellScriptLoader("spell_warl_health_funnel") { }

    class spell_warl_health_funnel_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warl_health_funnel_AuraScript);

        void ApplyEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            Unit* target = GetTarget();
            if (caster->HasAura(WARLOCK_IMPROVED_HEALTH_FUNNEL_R2))
                target->CastSpell(target, WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R2, true);
            else if (caster->HasAura(WARLOCK_IMPROVED_HEALTH_FUNNEL_R1))
                target->CastSpell(target, WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R1, true);
        }

        void RemoveEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            target->RemoveAurasDueToSpell(WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R1);
            target->RemoveAurasDueToSpell(WARLOCK_IMPROVED_HEALTH_FUNNEL_BUFF_R2);
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_warl_health_funnel_AuraScript::RemoveEffect, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
            OnEffectApply += AuraEffectApplyFn(spell_warl_health_funnel_AuraScript::ApplyEffect, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_warl_health_funnel_AuraScript();
    }
};

//80398 Dark Intent
class spell_warlock_dark_intent : public SpellScriptLoader
{
public:
    spell_warlock_dark_intent() : SpellScriptLoader("spell_warlock_dark_intent") { }

    class spell_warlock_dark_intent_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warlock_dark_intent_SpellScript)

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();

            if(!caster || !target)
                return;

            caster->CastSpell(target, WARLOCK_DARK_INTENT_EFFECT_2, true);
            target->CastSpell(caster, WARLOCK_DARK_INTENT_EFFECT, true);        
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_warlock_dark_intent_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warlock_dark_intent_SpellScript();
    }
};


// Nether Protection
class spell_warlock_nether_protection_trigger : public SpellScriptLoader
{
public:
    spell_warlock_nether_protection_trigger() : SpellScriptLoader("spell_warlock_nether_protection_trigger") { }

    class spell_warlock_nether_protection_trigger_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_warlock_nether_protection_trigger_AuraScript);

        void Absorb(AuraEffect* aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (AuraEffect * eff = target->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_WARLOCK, 1985, 0))
            {
                for (uint8 school = SPELL_SCHOOL_HOLY; school != MAX_SPELL_SCHOOL; ++school)
                {
                    uint32 spellId = 0;
                    if (SpellSchoolMask(dmgInfo.GetSchoolMask()) & (1 << school))
                    {
                        switch(school)
                        {
                        case SPELL_SCHOOL_HOLY: spellId = 54370; break;
                        case SPELL_SCHOOL_FIRE: spellId = 54371; break;
                        case SPELL_SCHOOL_FROST: spellId = 54372; break;
                        case SPELL_SCHOOL_ARCANE: spellId = 54373; break;
                        case SPELL_SCHOOL_SHADOW: spellId = 54374; break;
                        case SPELL_SCHOOL_NATURE: spellId = 54375; break;
                        default: break;
                        }
                    }
                    if (spellId)
                    {
                        int32 bp0 = -eff->GetAmount();
                        target->CastCustomSpell(target, spellId, &bp0, 0, 0, true);
                    }
                }
            }
        }

        void Register()
        {
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_warlock_nether_protection_trigger_AuraScript::Absorb, EFFECT_0);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_warlock_nether_protection_trigger_AuraScript();
    }
};

// Flee (Special Ability)
class spell_warlock_flee: public SpellScriptLoader
{
public:
    spell_warlock_flee() : SpellScriptLoader("spell_warlock_flee") {}
    class spell_warlock_flee_SpellScript: public SpellScript
    {
        PrepareSpellScript(spell_warlock_flee_SpellScript);

        void Trigger()
        {
            if (Unit * target = GetCaster()->GetOwner())
            {
                Pet * pet = GetCaster()->ToPet();
                CharmInfo *charmInfo = pet->GetCharmInfo();
                pet->AttackStop();
                pet->InterruptNonMeleeSpells(false);
                pet->GetMotionMaster()->MoveFollow(target, PET_FOLLOW_DIST, pet->GetFollowAngle());
                charmInfo->SetCommandState(COMMAND_FOLLOW);

                charmInfo->SetIsCommandAttack(false);
                charmInfo->SetIsAtStay(false);
                charmInfo->SetIsReturning(true);
                charmInfo->SetIsCommandFollow(true);
                charmInfo->SetIsFollowing(false);

                GetCaster()->CastSpell(target, 93282, false);
            }
        }
        void Register()
        {
            OnHit += SpellHitFn(spell_warlock_flee_SpellScript::Trigger);
        }
    };
    SpellScript* GetSpellScript() const
    {
        return new spell_warlock_flee_SpellScript();
    }
};

class spell_warl_soul_stone : public SpellScriptLoader
{
public:
    spell_warl_soul_stone() : SpellScriptLoader("spell_warl_soul_stone") { }

    class spell_warl_soul_stone_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_soul_stone_SpellScript);

        SpellCastResult CheckCast()
        {
            if (GetCaster())
                if (InstanceScript* pInstance = GetCaster()->GetInstanceScript())
                    return (pInstance->CanUseResurrection() ? SPELL_CAST_OK : SPELL_FAILED_IN_COMBAT_RES_LIMIT_REACHED);
            return SPELL_CAST_OK;
        }

        void HandleSoulStone()
        {
            Unit* target = GetHitUnit();
            if (!target)
                return;

            if (target->isDead())
            {
                if (InstanceScript* pInstance = target->GetInstanceScript())
                    pInstance->UpdateResurrectionsCount();

                PreventHitAura();
                target->ToPlayer()->SetUInt32Value(PLAYER_SELF_RES_SPELL, 3026);
            }
        }

        void Register()
        {
            OnCheckCast += SpellCheckCastFn(spell_warl_soul_stone_SpellScript::CheckCast);
            AfterHit += SpellHitFn(spell_warl_soul_stone_SpellScript::HandleSoulStone);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warl_soul_stone_SpellScript();
    }
};

void AddSC_warlock_spell_scripts()
{
    new spell_warl_banish();
    new spell_warl_demonic_empowerment();
    new spell_warl_create_healthstone();
    new spell_warl_everlasting_affliction();
    new spell_warl_seed_of_corruption();
    new spell_warl_soulshatter();
    new spell_warl_life_tap();
    new spell_warl_demonic_circle_summon();
    new spell_warl_demonic_circle_teleport();
    new spell_warl_haunt();
    new spell_warl_unstable_affliction();
    new spell_warl_health_funnel();
    new spell_warlock_dark_intent();
    new spell_warlock_nether_protection_trigger();
    new spell_warlock_flee();
    new spell_warl_soul_stone();
}
