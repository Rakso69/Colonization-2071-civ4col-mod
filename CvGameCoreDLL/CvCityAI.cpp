// cityAI.cpp

#include "CvGameCoreDLL.h"
#include <cstring>
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvCityAI.h"
#include "CvUnitAI.h"
#include "CvGameAI.h"
#include "CvPlot.h"
#include "CvArea.h"
#include "CvPlayerAI.h"
#include "CvTeamAI.h"
#include "CyCity.h"
#include "CyArgsList.h"
#include "CvInfos.h"
#include "FProfiler.h"

#include "CvDLLPythonIFaceBase.h"
#include "CvDLLInterfaceIFaceBase.h"
#include "CvDLLFAStarIFaceBase.h"


#define BUILDINGFOCUS_NO_RECURSION			(1 << 31)
#define BUILDINGFOCUS_BUILD_ANYTHING		(1 << 30)

#define YIELD_DISCOUNT_TURNS 			10

namespace
{
	int cityBuildingValueCacheSize()
	{
		// External rule callbacks may change the state while candidates are checked.
		if (GC.getUSE_CAN_CONSTRUCT_CALLBACK() || GC.getUSE_CANNOT_CONSTRUCT_CALLBACK()
			|| GC.getUSE_CAN_DO_CIVIC_CALLBACK() || GC.getUSE_CANNOT_DO_CIVIC_CALLBACK()
			|| GC.getUSE_GET_BUILDING_COST_MOD_CALLBACK() || GC.getUSE_GET_UNIT_COST_MOD_CALLBACK()
			|| GC.getUSE_CAN_TRAIN_CALLBACK() || GC.getUSE_CANNOT_TRAIN_CALLBACK()) return 0;
		return GC.getNumSpecialBuildingInfos();
	}
}


struct CvCityAIProfessionValueCache
{
	int aiBaseProduced[NUM_YIELD_TYPES];
	int aiConsumed[NUM_YIELD_TYPES];
	int aiModifiers[NUM_YIELD_TYPES];
	int aiTargets[NUM_YIELD_TYPES];
	bool abBaseProduced[NUM_YIELD_TYPES];
	bool abConsumed[NUM_YIELD_TYPES];
	bool abModifiers[NUM_YIELD_TYPES];
	bool abTargets[NUM_YIELD_TYPES];
	const CvCity* pResearchCity;
	bool bResearchCityKnown;

	CvCityAIProfessionValueCache() : pResearchCity(NULL), bResearchCityKnown(false)
	{
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
			abBaseProduced[i] = abConsumed[i] = abModifiers[i] = abTargets[i] = false;
	}

	int baseProduced(const CvCityAI& kCity, YieldTypes eYield)
	{
		if (!abBaseProduced[eYield])
		{
			aiBaseProduced[eYield] = kCity.getBaseRawYieldProduced(eYield);
			abBaseProduced[eYield] = true;
		}
		return aiBaseProduced[eYield];
	}

	int consumed(const CvCityAI& kCity, YieldTypes eYield)
	{
		if (!abConsumed[eYield])
		{
			aiConsumed[eYield] = kCity.getRawYieldConsumed(eYield);
			abConsumed[eYield] = true;
		}
		return aiConsumed[eYield];
	}

	int modifier(const CvCityAI& kCity, YieldTypes eYield)
	{
		if (!abModifiers[eYield])
		{
			aiModifiers[eYield] = kCity.getBaseYieldRateModifier(eYield);
			abModifiers[eYield] = true;
		}
		return aiModifiers[eYield];
	}

	int produced(const CvCityAI& kCity, YieldTypes eYield)
	{
		return baseProduced(kCity, eYield) * modifier(kCity, eYield) / 100;
	}

	int target(const CvPlayerAI& kOwner, const CvCityAI& kCity, YieldTypes eYield)
	{
		if (!abTargets[eYield])
		{
			aiTargets[eYield] = kOwner.AI_cityYieldTarget(&kCity, eYield);
			abTargets[eYield] = true;
		}
		return aiTargets[eYield];
	}

	const CvCity* researchCity(const CvPlayerAI& kOwner)
	{
		if (!bResearchCityKnown)
		{
			pResearchCity = kOwner.AI_nativeResearchCity();
			bResearchCityKnown = true;
		}
		return pResearchCity;
	}
};

// Public Functions...

CvCityAI::CvCityAI()
{
	m_aiYieldOutputWeight = new int[NUM_YIELD_TYPES];
	m_aiNeededYield = new int[NUM_YIELD_TYPES];
	m_aiTradeBalance = new int[NUM_YIELD_TYPES];
	m_aiYieldAdvantage = new int[NUM_YIELD_TYPES];

	m_aiEmphasizeYieldCount = new int[NUM_YIELD_TYPES];
	m_bForceEmphasizeCulture = false;
	m_aiPlayerCloseness = new int[MAX_PLAYERS];

	m_abEmphasize = NULL;

	AI_reset();
}


CvCityAI::~CvCityAI()
{
	AI_uninit();

	SAFE_DELETE_ARRAY(m_aiYieldOutputWeight);
	SAFE_DELETE_ARRAY(m_aiNeededYield);
	SAFE_DELETE_ARRAY(m_aiTradeBalance);
	SAFE_DELETE_ARRAY(m_aiYieldAdvantage);
	SAFE_DELETE_ARRAY(m_aiEmphasizeYieldCount);
	SAFE_DELETE_ARRAY(m_aiPlayerCloseness);
}


void CvCityAI::AI_init()
{
	AI_reset();

	//--------------------------------
	// Init other game data
	AI_assignWorkingPlots();

	AI_updateBestBuild();

	AI_assignDesiredYield();

	m_iFoundValue = plot()->getFoundValue(getOwner());
}


void CvCityAI::AI_uninit()
{
	SAFE_DELETE_ARRAY(m_abEmphasize);
}


// FUNCTION: AI_reset()
// Initializes data members that are serialized.
void CvCityAI::AI_reset()
{
	int iI;

	AI_uninit();

	m_iGiftTimer = 0;
	m_eDesiredYield = NO_YIELD;

	m_iTargetSize = 0;
	m_iFoundValue = 0;

	m_iEmphasizeAvoidGrowthCount = 0;
	m_bForceEmphasizeCulture = false;

	m_bPort = false;
	m_bAssignWorkDirty = false;
	m_bChooseProductionDirty = false;

	m_iWorkforceHack = 0;

	m_routeToCity.reset();

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiYieldOutputWeight[iI] = 0;
		m_aiNeededYield[iI] = 0;
		m_aiTradeBalance[iI] = 0;
		m_aiYieldAdvantage[iI] = 0;
	}

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_aiEmphasizeYieldCount[iI] = 0;
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		m_aiBestBuildValue[iI] = NO_BUILD;
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		m_aeBestBuild[iI] = NO_BUILD;
	}

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aiPlayerCloseness[iI] = 0;
	}
	m_iCachePlayerClosenessTurn = -1;
	m_iCachePlayerClosenessDistance = -1;

	m_iNeededFloatingDefenders = -1;
	m_iNeededFloatingDefendersCacheTurn = -1;

	m_iWorkersNeeded = 0;
	m_iWorkersHave = 0;

	FAssertMsg(m_abEmphasize == NULL, "m_abEmphasize not NULL!!!");
	FAssertMsg(GC.getNumEmphasizeInfos() > 0,  "GC.getNumEmphasizeInfos() is not greater than zero but an array is being allocated in CvCityAI::AI_reset");
	m_abEmphasize = new bool[GC.getNumEmphasizeInfos()];
	for (iI = 0; iI < GC.getNumEmphasizeInfos(); iI++)
	{
		m_abEmphasize[iI] = false;
	}
}


void CvCityAI::AI_doTurn()
{
	PROFILE_FUNC();

	AI_doTradedYields();

	if (!isHuman())
	{
		AI_updateRequiredYieldLevels();
	}

	AI_updateWorkersNeededHere();

	AI_updateBestBuild();

	AI_updateRouteToCity();

	if (AI_getGiftTimer() > 0)
	{
		AI_changeGiftTimer(-1);
	}

	if (isHuman())
	{
	    if (isProductionAutomated())
	    {
	        AI_doHurry();
	    }
		return;
	}

	AI_doHurry();

	AI_doEmphasize();
}

//struct PopUnit
//{
//	CvUnit* m_pUnit;
//	ProfessionTypes m_eIdealProfession;
//
//	int calculateValue() const
//	{
//		int iValue = 100;
//		if (eIdealProfession != NO_PROFESSION)
//		{
//			iValue += 100;
//			if (GC.getProfessionInfo(eIdealProfession).getYieldProduced() == YIELD_FOOD)
//			{
//				iValue += 50;
//			}
//		}
//		return iValue;
//	}
//
//	bool operator < (const PopUnit& rhs) const
//	{
//		return calculateValue() < rhs.calculateValue();
//	}
//};

void CvCityAI::AI_assignWorkingPlots()
{
	PROFILE_FUNC();

	if (isOccupation())
	{
	    return;
	}

	if (getPopulation() == 0)
	{
		return;
	}

	GET_PLAYER(getOwnerINLINE()).AI_manageEconomy();
	AI_updateNeededYields();

	//remove non-city people
	removeNonCityPopulationUnits();


	/*
	Citizen Algorithm:
	Take all citizens which aren't locked in place, put them in a pool.
	Now take the first citizen from the pool and place it on the highest value plot,
	unless the existing worker has even higher value.
	If a worker is displaced, it is returned to the pool.
	Take the next citizen from the pool..
	Once the pool is empty, the workforce is optimally allocated.

	This algorithm will find a good workforce
	it's kind of expensive, but not THAT expensive with only 8 plots per colony
	and the population numbers being low.
	*/
	std::deque<CvUnit*> citizens;
	std::deque<CvUnit*> aCitizenPasses[4];

	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pUnit = m_aPopulationUnits[i];
		if (!pUnit->isColonistLocked())
		{
			int iUnitPass = 2;
			for (int iProfession = 0; iProfession < GC.getNumProfessionInfos(); ++iProfession)
			{
				ProfessionTypes eProfession = (ProfessionTypes)iProfession;
				if (GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pUnit->getUnitType(), eProfession)
					&& pUnit->canHaveProfession(eProfession, true, NULL))
				{
					iUnitPass = 1;
					if (isNative() && !isHuman() && GC.getProfessionInfo(eProfession).getYieldsProduced(0) == YIELD_FOOD)
					{
						iUnitPass = 3;
						break;
					}
					if (GC.getProfessionInfo(eProfession).getYieldsConsumed(0, getOwnerINLINE()) != NO_YIELD)
					{
						iUnitPass = 0;
						break;
					}
				}
			}
			aCitizenPasses[iUnitPass].push_back(pUnit);
		}
	}
	for (int iPass = 0; iPass < 4; ++iPass)
	{
		citizens.insert(citizens.end(), aCitizenPasses[iPass].begin(), aCitizenPasses[iPass].end());
	}

	//Kaszkaj fix: .\.\CvCityAI.cpp, Line:  307, Expression:  false.
	// Vacate unlocked jobs before allocation; each replacement must improve a fixed production score.
	for (uint i = 0; i < citizens.size(); ++i)
	{
		CvPlot* pWorkedPlot = getPlotWorkedByUnit(citizens[i]);
		if (pWorkedPlot != NULL) clearUnitWorkingPlot(pWorkedPlot);
		citizens[i]->setProfession(NO_PROFESSION);
	}
	//Kaszkaj - Allow useful worker replacement chains; stop when a full pass makes no improvement.
	uint iMaxIterations = citizens.size() * (1 + (NUM_CITY_PLOTS + citizens.size()) * GC.getNumProfessionInfos());

	uint iCount = 0;
	while (!citizens.empty())
	{
		CvUnit* pUnit = citizens.back();
		citizens.pop_back();
		FAssert (pUnit != NULL);

		CvPlot* pWorkedPlot =  getPlotWorkedByUnit(pUnit);
		if (pWorkedPlot != NULL)
		{
			clearUnitWorkingPlot(pWorkedPlot);
		}

		CvUnit* pOldUnit = AI_assignToBestJob(pUnit);
		if (pOldUnit != NULL)
		{
			if (std::find(citizens.begin(), citizens.end(), pOldUnit) == citizens.end())
			{
				citizens.push_front(pOldUnit);
			}
		}
		iCount++;
		if (iCount > iMaxIterations)
		{
			FAssertMsg(false, "AI plot assignment confusion");
			break;
		}
	}

//orlanth aliens	if (isNative())
//	{
//		AI_setAssignWorkDirty(false);
//		return;
//end orlanth aliens	}


	//Now see if swapping citizens will help.
	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pUnit = m_aPopulationUnits[i];
		if (pUnit != NULL)
		{
			if (!pUnit->isColonistLocked())
			{
				if (pUnit->getProfession() != NO_PROFESSION)
				{
					AI_juggleColonist(pUnit);
				}
			}
		}
	}

	AI_setAssignWorkDirty(false);

	if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
	{
		gDLL->getInterfaceIFace()->setDirty(CitizenButtons_DIRTY_BIT, true);
	}
}


void CvCityAI::AI_updateAssignWork()
{
	if (AI_isAssignWorkDirty())
	{
		AI_assignWorkingPlots();
	}
}


bool CvCityAI::AI_avoidGrowth() const
{
	PROFILE_FUNC();

	if (AI_isEmphasizeAvoidGrowth())
	{
		return true;
	}

	return false;
}

void CvCityAI::AI_setAvoidGrowth(bool bNewValue)
{
	bool bCurrentValue = AI_isEmphasizeAvoidGrowth();

	if (bCurrentValue == bNewValue)
	{
		return;
	}

	for (int i = 0; i < GC.getNumEmphasizeInfos(); ++i)
	{
		if (GC.getEmphasizeInfo((EmphasizeTypes)i).isAvoidGrowth())
		{
			AI_setEmphasize((EmphasizeTypes)i, bNewValue);
		}
	}
}


bool CvCityAI::AI_ignoreGrowth() const
{
	PROFILE_FUNC();

	if (AI_getEmphasizeYieldCount(YIELD_FOOD) <= 0)
	{
		if (!AI_foodAvailable((isHuman()) ? 0 : 1))
		{
			return true;
		}
	}

	return false;
}


//Kaszkaj - Select one safe, legal Eye colony, then assess crusade readiness once before shared production begins.
CvCity* CvCity::AI_getTranscendenceCity(PlayerTypes ePlayer)
{
	if (ePlayer < 0 || ePlayer >= MAX_PLAYERS) return NULL;
	CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
	CvGame& kGame = GC.getGameINLINE();
	if (kPlayer.isHuman() || kPlayer.isEurope() || kPlayer.isBarbarian() || !kGame.canStartTranscendence(ePlayer)) return NULL;
	BuildingTypes eEye = kGame.getTranscendenceBuilding();
	CvCity* pBestCity = kPlayer.getCapitalCity();
	if (pBestCity != NULL && !pBestCity->isDisorder() && !pBestCity->AI_isDanger() && pBestCity->canConstruct(eEye))
	{
		return kPlayer.AI_isReadyForTranscendence(pBestCity) ? pBestCity : NULL;
	}
	pBestCity = NULL;
	int iBestValue = -1;
	int iLoop;
	for (CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
	{
		if (pCity->isDisorder() || pCity->AI_isDanger() || !pCity->canConstruct(eEye)) continue;
		int iValue = pCity->getPopulation() * 4 + pCity->getYieldRate(YIELD_FOOD) + pCity->getCultureLevel();
		iValue = iValue * (pCity->calculateCulturePercent(ePlayer) + 100) / 100;
		if (iValue > iBestValue)
		{
			iBestValue = iValue;
			pBestCity = pCity;
		}
	}
	return pBestCity != NULL && kPlayer.AI_isReadyForTranscendence(pBestCity) ? pBestCity : NULL;
}

void CvCityAI::AI_chooseProduction()
{
	PROFILE_FUNC();

	CvArea* pWaterArea;

	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());

	//Kaszkaj - Routine project selection cannot disturb the shared Eye construction.
	CvGame& kGame = GC.getGameINLINE();
	if (kGame.isTranscendenceActive() && kGame.getTranscendencePlayer() == getOwnerINLINE())
	{
		AI_setChooseProductionDirty(false);
		return;
	}

	if (isProduction())
	{
		if (getProduction() > 0)
		{
			// if less than 3 turns left, keep building current item
			//Kaszkaj - Reconsider completed Industry production when missing cargo materials still block completion.
			bool bBlocked = false;
			if (getProduction() >= getProductionNeeded(YIELD_HAMMERS))
			{
				for (int i = 0; i < NUM_YIELD_TYPES; ++i)
				{
					YieldTypes eYield = (YieldTypes)i;
					if (GC.getYieldInfo(eYield).isCargo() && getProductionNeeded(eYield) > getYieldStored(eYield) + getYieldRushed(eYield)) bBlocked = true;
				}
			}
			if (!bBlocked && getProductionTurnsLeft() <= 3)
			{
				return;
			}
		}
		clearOrderQueue();
	}

	// only clear the dirty bit if we actually do a check, multiple items might be queued
	AI_setChooseProductionDirty(false);

	// allow python to handle it
	if (GC.getUSE_AI_CHOOSE_PRODUCTION_CALLBACK())
	{
		CyCity* pyCity = new CyCity(this);
		CyArgsList argsList;
		argsList.add(gDLL->getPythonIFace()->makePythonObject(pyCity));	// pass in city class
		long lResult=0;
		gDLL->getPythonIFace()->callFunction(PYGameModule, "AI_chooseProduction", argsList.makeFunctionArgs(), &lResult);
		delete pyCity;	// python fxn must not hold on to this pointer
		if (lResult == 1)
		{
			return;
		}
	}

	CvArea* pArea = area();
	pWaterArea = waterArea();
	bool bMaybeWaterArea = false;

	if (pWaterArea != NULL)
	{
		bMaybeWaterArea = true;
		if (!GET_TEAM(getTeam()).AI_isWaterAreaRelevant(pWaterArea))
		{
			pWaterArea = NULL;
		}
	}

	//Kaszkaj - Meet defence, transport and builder needs before choosing routine buildings for either AI.
	if (!isHuman() && !kPlayer.isEurope() && !kPlayer.isBarbarian())
	{
		if (AI_isDanger() && !AI_isDefended() && AI_chooseUnit(UNITAI_DEFENSIVE, false)) return;
		if (isNative() && kPlayer.AI_nativeDefenderCount(this) == 0 && AI_chooseUnit(UNITAI_DEFENSIVE, false)) return;
		UnitTypes eNeededUnit = NO_UNIT;
		UnitAITypes eNeededAI = NO_UNITAI;
		int iBestNeed = 0;
		BuildingTypes eNeededBuilding = NO_BUILDING;
		int iBestBuildingNeed = 0;
		if (isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
		{
			int iTransportShortfall = kPlayer.AI_transportCapacityNeeded() - kPlayer.AI_transportCapacity();
			UnitClassTypes eFreighterClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_GALLEON", true);
			for (int i = 0; i < GC.getNumUnitClassInfos(); ++i)
			{
				UnitTypes eUnit = (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(i);
				if (eUnit == NO_UNIT) continue;
				const CvUnitInfo& kUnit = GC.getUnitInfo(eUnit);
				if (kUnit.getDomainType() != DOMAIN_SEA) continue;
				bool bWorker = kPlayer.AI_isDedicatedWorker(eUnit);
				UnitAITypes eRole = bWorker ? UNITAI_WORKER : UNITAI_TRANSPORT_SEA;
				int iNeed = bWorker ? 200 * kPlayer.AI_neededSeaBuilders(eUnit)
					: (kUnit.getUnitAIType(UNITAI_TRANSPORT_SEA) ? 100 * std::max(0, iTransportShortfall) : 0);
				bool bFreighter = kUnit.getUnitClassType() == eFreighterClass;
				bool bFirstFreighter = bFreighter && kPlayer.getUnitClassCountPlusMaking(eFreighterClass) == 0;
				if (bFreighter)
				{
					iNeed *= 2;
					if (bFirstFreighter && kPlayer.AI_totalUnitAIs(UNITAI_TREASURE) > 0) iNeed = std::max(iNeed, 2000);
				}
				//Kaszkaj - Prepare access to Freighter or Carrier production, while ordering ships only for actual transport demand.
				int iBuildingNeed = bFirstFreighter ? std::max(iNeed, 300) : iNeed;
				if (iNeed <= 0 && iBuildingNeed <= 0) continue;
				if (iNeed > 0 && canTrain(eUnit))
				{
					int iValue = iNeed * (bWorker ? 3 : std::max(1, kUnit.getCargoSpace()))
						/ std::max(4, getProductionTurnsLeft(eUnit, 0));
					iValue = iValue * AI_productionSupplyPercent(eUnit, NO_BUILDING) / 100;
					if (iValue > iBestNeed)
					{
						eNeededUnit = eUnit;
						eNeededAI = eRole;
						iBestNeed = iValue;
					}
				}
				BuildingClassTypes ePrereq = (BuildingClassTypes)kUnit.getPrereqBuilding();
				if (ePrereq != NO_BUILDINGCLASS)
				{
					BuildingTypes eBuilding = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(ePrereq);
					SpecialBuildingTypes eSpecial = eBuilding == NO_BUILDING ? NO_SPECIALBUILDING
						: (SpecialBuildingTypes)GC.getBuildingInfo(eBuilding).getSpecialBuildingType();
					bool bExempt = bFreighter
						&& eSpecial != NO_SPECIALBUILDING && kPlayer.isSpecialBuildingNotRequired(eSpecial);
					bool bCanConstruct = !bExempt && eBuilding != NO_BUILDING && !isHasConceptualBuilding(eBuilding) && canConstruct(eBuilding);
					//Kaszkaj - Build the earlier required Dock when the Freighter or Carrier production building cannot yet be constructed.
					if (!bExempt && !bCanConstruct && eBuilding != NO_BUILDING && !isHasConceptualBuilding(eBuilding)
						&& bFreighter)
					{
						const CvBuildingInfo& kPrereq = GC.getBuildingInfo(eBuilding);
						for (int iClass = 0; iClass < GC.getNumBuildingClassInfos(); ++iClass)
						{
							if (!kPrereq.isBuildingClassNeededInCity(iClass)) continue;
							BuildingTypes eEarlier = (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iClass);
							if (eEarlier != NO_BUILDING && !isHasConceptualBuilding(eEarlier) && canConstruct(eEarlier))
							{
								eBuilding = eEarlier;
								bCanConstruct = true;
								break;
							}
						}
					}
					if (bCanConstruct)
					{
						int iValue = iBuildingNeed / std::max(4, getProductionTurnsLeft(eBuilding, 0));
						if (iValue > iBestBuildingNeed)
						{
							eNeededBuilding = eBuilding;
							iBestBuildingNeed = iValue;
						}
					}
				}
			}
		}
		//Kaszkaj - Alien industry needs manufactured specialists as well as buildings and ships.
		if (isNative())
		{
			const char* aszTypes[] = {"UNIT_POLYMATH", "UNIT_ELDER", "UNIT_CYBORG", "UNIT_ELITE", "UNIT_CULTIVATOR"};
			for (int i = 0; i < 5; ++i)
			{
				UnitTypes eUnit = (UnitTypes)GC.getInfoTypeForString(aszTypes[i], true);
				if (eUnit == NO_UNIT) continue;
				const CvUnitInfo& kUnit = GC.getUnitInfo(eUnit);
				if (GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(kUnit.getUnitClassType()) != eUnit) continue;
				int iNeed = kPlayer.AI_nativeUnitProductionValue(this, eUnit);
				if (iNeed <= 0) continue;
				if (canTrain(eUnit))
				{
					int iValue = iNeed / std::max(4, getProductionTurnsLeft(eUnit, 0));
					iValue = iValue * AI_productionSupplyPercent(eUnit, NO_BUILDING) / 100;
					if (iValue > iBestNeed)
					{
						eNeededUnit = eUnit;
						eNeededAI = i < 3 ? UNITAI_COLONIST : (i == 3 ? UNITAI_SCOUT : UNITAI_WORKER);
						iBestNeed = iValue;
					}
				}
				BuildingClassTypes ePrereq = (BuildingClassTypes)kUnit.getPrereqBuilding();
				BuildingTypes eBuilding = ePrereq == NO_BUILDINGCLASS ? NO_BUILDING
					: (BuildingTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(ePrereq);
				if (eBuilding != NO_BUILDING && !isHasBuilding(eBuilding) && canConstruct(eBuilding))
				{
					int iValue = iNeed / std::max(4, getProductionTurnsLeft(eBuilding, 0));
					if (iValue > iBestBuildingNeed)
					{
						eNeededBuilding = eBuilding;
						iBestBuildingNeed = iValue;
					}
				}
			}
		}
		if (!isNative())
		{
			UnitClassTypes eClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_PIONEER", true);
			UnitTypes ePioneer = eClass == NO_UNITCLASS ? NO_UNIT
				: (UnitTypes)GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(eClass);
			if (ePioneer != NO_UNIT && kPlayer.getUnitClassCountPlusMaking(eClass) == 0
				&& kPlayer.AI_builderTaskCount(ePioneer) > 0 && canTrain(ePioneer))
			{
				pushOrder(ORDER_TRAIN, ePioneer, UNITAI_WORKER, false, false, false, false);
				return;
			}
		}
		if (eNeededUnit != NO_UNIT && iBestNeed >= iBestBuildingNeed)
		{
			//Kaszkaj - Compare useful economic buildings with recruitment instead of letting recruits always come first.
			BuildingTypes eEconomicBuilding = AI_bestBuilding(0, MAX_INT);
			if (eEconomicBuilding != NO_BUILDING)
			{
				int iBuildingNeed = 20 * std::max(0, AI_buildingValue(eEconomicBuilding))
					/ std::max(4, getProductionTurnsLeft(eEconomicBuilding, 0));
				if (iBuildingNeed > iBestNeed)
				{
					pushOrder(ORDER_CONSTRUCT, eEconomicBuilding, -1, false, false, false, false);
					return;
				}
			}
			pushOrder(ORDER_TRAIN, eNeededUnit, eNeededAI, false, false, false, false);
			return;
		}
		if (eNeededBuilding != NO_BUILDING)
		{
			pushOrder(ORDER_CONSTRUCT, eNeededBuilding, -1, false, false, false, false);
			return;
		}
	}
//orlanth aliens
	int iAreaCities = pArea->getCitiesPerPlayer(getOwnerINLINE());
	if (iAreaCities > 1)
	{
		if ((pArea->getNumAIUnits(getOwnerINLINE(), UNITAI_WAGON) + pArea->getNumTrainAIUnits(getOwnerINLINE(), UNITAI_WAGON)) < (iAreaCities / 3))
		{
			if (AI_chooseUnit(UNITAI_WAGON))
			{
				return;
			}
		}
	}
	if (AI_chooseBuilding(0, MAX_INT, 8))
	{
		return;
	}
//orlanth aliens
//	if (isNative())
//	{
//		if (AI_chooseUnit(UNITAI_DEFENSIVE, false))
//		{
//			return;
//		}
//end orlanth aliens	}

	if (AI_chooseUnit(NO_UNITAI, false))
		{
			return;
		}

	if (AI_chooseBuilding(BUILDINGFOCUS_BUILD_ANYTHING, MAX_INT, 8))
	{
		return;
	}

	if (AI_chooseUnit(NO_UNITAI, true))
	{
		return;
	}

	//Kaszkaj - Produce points for Board Directors when no unit or building can be chosen.
	for (int iI = 0; iI < GC.getNumFatherPointInfos(); ++iI)
	{
		FatherPointTypes eFatherPoint = (FatherPointTypes)iI;

		if (canConvince(eFatherPoint))
		{
			pushOrder(ORDER_CONVINCE, iI, -1, false, false, false, false);
			return;
		}
	}

	//colonies should always be building something
	FAssertMsg(isNative(), "AI not building anything.");
}


UnitTypes CvCityAI::AI_bestUnit(bool bAsync, UnitAITypes* peBestUnitAI, bool bPickAny) const
{
	int aiUnitAIVal[NUM_UNITAI_TYPES];
	UnitTypes eUnit = NO_UNIT;
	UnitTypes eBestUnit = NO_UNIT;

	int iBestValue = 0;
	int iI;

	if (peBestUnitAI != NULL)
	{
		*peBestUnitAI = NO_UNITAI;
	}

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		aiUnitAIVal[iI] = 0;
	}

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		if (bAsync)
		{
			aiUnitAIVal[iI] += GC.getASyncRand().get(100, "AI Best UnitAI ASYNC");
		}
		else
		{
			aiUnitAIVal[iI] += GC.getGameINLINE().getSorenRandNum(100, "AI Best UnitAI");
		}
	}

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		aiUnitAIVal[iI] *= std::max(0, (GC.getLeaderHeadInfo(getPersonalityType()).getUnitAIWeightModifier(iI) + 100));
		aiUnitAIVal[iI] /= 100;

		if (!bPickAny)
		{
			aiUnitAIVal[iI] *= GET_PLAYER(getOwnerINLINE()).AI_unitAIValueMultipler((UnitAITypes)iI);
			aiUnitAIVal[iI] /= 100;
		}
	}

	for (iI = 0; iI < NUM_UNITAI_TYPES; iI++)
	{
		if (aiUnitAIVal[iI] > iBestValue)
		{
			eUnit = AI_bestUnitAI(((UnitAITypes)iI), bAsync);

			if (eUnit != NO_UNIT)
			{
				iBestValue = aiUnitAIVal[iI];
				eBestUnit = eUnit;
				if (peBestUnitAI != NULL)
				{
					*peBestUnitAI = ((UnitAITypes)iI);
				}
			}
		}
	}

	return eBestUnit;
}


UnitTypes CvCityAI::AI_bestUnitAI(UnitAITypes eUnitAI, bool bAsync) const
{
	UnitTypes eLoopUnit;
	UnitTypes eBestUnit;
	int iValue;
	int iBestValue;
	int iOriginalValue;
	int iBestOriginalValue;
	int iI, iJ, iK;


	FAssertMsg(eUnitAI != NO_UNITAI, "UnitAI is not assigned a valid value");

	iBestOriginalValue = 0;
	bool bCacheUnitValues = cityBuildingValueCacheSize() != 0 && !GC.getUSE_CAN_BUILD_CALLBACK()
		&& !GC.getUSE_UNIT_CANNOT_MOVE_INTO_CALLBACK() && !GC.getUSE_CAN_DECLARE_WAR_CALLBACK();
	std::vector<int> aiUnitValues(bCacheUnitValues ? GC.getNumUnitClassInfos() : 0, -1);

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
			{

				if (canTrain(eLoopUnit))
				{
					iOriginalValue = GET_PLAYER(getOwnerINLINE()).AI_unitValue(eLoopUnit, eUnitAI, area());
					if (bCacheUnitValues) aiUnitValues[iI] = iOriginalValue;

					if (iOriginalValue > iBestOriginalValue)
					{
						iBestOriginalValue = iOriginalValue;
					}
				}
			}
		}
	}

	iBestValue = 0;
	eBestUnit = NO_UNIT;

	for (iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if (!isHuman() || (GC.getUnitInfo(eLoopUnit).getDefaultUnitAIType() == eUnitAI))
			{

				if (bCacheUnitValues ? aiUnitValues[iI] >= 0 : canTrain(eLoopUnit))
					{
						iValue = bCacheUnitValues ? aiUnitValues[iI]
							: GET_PLAYER(getOwnerINLINE()).AI_unitValue(eLoopUnit, eUnitAI, area());

						if (iValue > ((iBestOriginalValue * 2) / 3))
						{
							iValue *= (getProductionExperience(eLoopUnit) + 10);
							iValue /= 10;

                            //free promotions. slow?
                            //only 1 promotion per source is counted (ie protective isn't counted twice)
                            int iPromotionValue = 0;
                            //buildings
                            for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
                            {
                                if (isFreePromotion((PromotionTypes)iJ) && !GC.getUnitInfo(eLoopUnit).getFreePromotions((PromotionTypes)iJ))
                                {
                                    if ((GC.getUnitInfo(eLoopUnit).getUnitCombatType() != NO_UNITCOMBAT) && GC.getPromotionInfo((PromotionTypes)iJ).getUnitCombat(GC.getUnitInfo(eLoopUnit).getUnitCombatType()))
                                    {
                                        iPromotionValue += 15;
                                        break;
                                    }
                                }
                            }

                            //special to the unit
                            for (iJ = 0; iJ < GC.getNumPromotionInfos(); iJ++)
                            {
                                if (GC.getUnitInfo(eLoopUnit).getFreePromotions(iJ))
                                {
                                    iPromotionValue += 15;
                                    break;
                                }
                            }

                            //traits
                            for (iJ = 0; iJ < GC.getNumTraitInfos(); iJ++)
                            {
                                if (hasTrait((TraitTypes)iJ))
                                {
                                    for (iK = 0; iK < GC.getNumPromotionInfos(); iK++)
                                    {
                                        if (GC.getTraitInfo((TraitTypes) iJ).isFreePromotion(iK))
                                        {
                                            if ((GC.getUnitInfo(eLoopUnit).getUnitCombatType() != NO_UNITCOMBAT) && GC.getTraitInfo((TraitTypes) iJ).isFreePromotionUnitCombat(GC.getUnitInfo(eLoopUnit).getUnitCombatType()))
                                            {
                                                iPromotionValue += 15;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            iValue *= (iPromotionValue + 100);
                            iValue /= 100;

							if (bAsync)
							{
								iValue *= (GC.getASyncRand().get(50, "AI Best Unit ASYNC") + 100);
								iValue /= 100;
							}
							else
							{
								iValue *= (GC.getGameINLINE().getSorenRandNum(50, "AI Best Unit") + 100);
								iValue /= 100;
							}


							iValue *= (GET_PLAYER(getOwnerINLINE()).getNumCities() * 2);
							iValue /= (GET_PLAYER(getOwnerINLINE()).getUnitClassCountPlusMaking((UnitClassTypes)iI) + GET_PLAYER(getOwnerINLINE()).getNumCities() + 1);

							FAssert((MAX_INT / 1000) > iValue);
							iValue *= 1000;

							iValue /= std::max(1, (4 + getProductionTurnsLeft(eLoopUnit, 0)));

							iValue = std::max(1, iValue);

							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestUnit = eLoopUnit;
							}
						}
					}
			}
		}
	}

	return eBestUnit;
}


BuildingTypes CvCityAI::AI_bestBuilding(int iFocusFlags, int iMaxTurns, bool bAsync) const
{
	return AI_bestBuildingThreshold(iFocusFlags, iMaxTurns, /*iMinThreshold*/ 0, bAsync);
}

BuildingTypes CvCityAI::AI_bestBuildingThreshold(int iFocusFlags, int iMaxTurns, int iMinThreshold, bool bAsync) const
{
	const int iCacheSize = cityBuildingValueCacheSize();
	std::vector<int> aiExpertValues(iCacheSize, MIN_INT);
	std::vector<int> aiProductionValues(iCacheSize, MIN_INT);

	bool bAreaAlone = GET_PLAYER(getOwnerINLINE()).AI_isAreaAlone(area());

	int iProductionRank = findYieldRateRank(YIELD_HAMMERS);

	int iBestValue = 0;
	BuildingTypes eBestBuilding = NO_BUILDING;

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if ((eLoopBuilding != NO_BUILDING) && (!isHasConceptualBuilding(eLoopBuilding)))
		{
			if (canConstruct(eLoopBuilding))
			{
				int iValue = AI_buildingValueWithCache(eLoopBuilding, iFocusFlags, aiExpertValues, aiProductionValues);

				if (iValue > 0)
				{
					int iTurnsLeft = getProductionTurnsLeft(eLoopBuilding, 0);

					if (bAsync)
					{
						iValue *= (GC.getASyncRand().get(25, "AI Best Building ASYNC") + 100);
						iValue /= 100;
					}
					else
					{
						iValue *= (GC.getGameINLINE().getSorenRandNum(25, "AI Best Building") + 100);
						iValue /= 100;
					}

					iValue += getBuildingProduction(eLoopBuilding);


					bool bValid = ((iMaxTurns <= 0) ? true : false);
					if (!bValid)
					{
						bValid = (iTurnsLeft <= GC.getGameINLINE().AI_turnsPercent(iMaxTurns, GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getConstructPercent()));
					}

					if (bValid)
					{
						iValue = iValue / 2 + iValue / (1 + iTurnsLeft);

						iValue = std::max(1, iValue);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							eBestBuilding = eLoopBuilding;
						}
					}
				}
			}
		}
	}
	return eBestBuilding;
}

BuildingTypes CvCityAI::AI_bestBuildingIgnoreRequirements(int iFocusFlags, int iMaxTurns)
{
	const int iCacheSize = cityBuildingValueCacheSize();
	std::vector<int> aiExpertValues(iCacheSize, MIN_INT);
	std::vector<int> aiProductionValues(iCacheSize, MIN_INT);

	int iBestValue = 0;
	BuildingTypes eBestBuilding = NO_BUILDING;

	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if ((eLoopBuilding != NO_BUILDING) && (!isHasConceptualBuilding(eLoopBuilding)))
		{
			//Kaszkaj - Pass the planning flags to canConstruct; a comma expression incorrectly accepted every building.
			if (canConstruct(eLoopBuilding, true, true, true))
			{
				int iValue = AI_buildingValueWithCache(eLoopBuilding, iFocusFlags, aiExpertValues, aiProductionValues);

				if (getProductionBuilding() == eLoopBuilding)
				{
					iValue *= 125;
					iValue /= 100;
				}

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestBuilding = eLoopBuilding;
				}
			}
		}
	}

	return eBestBuilding;
}


int CvCityAI::AI_buildingValue(BuildingTypes eBuilding, int iFocusFlags) const
{
	std::vector<int> aiExpertValues;
	std::vector<int> aiProductionValues;
	return AI_buildingValueWithCache(eBuilding, iFocusFlags, aiExpertValues, aiProductionValues);
}

int CvCityAI::AI_buildingValueWithCache(BuildingTypes eBuilding, int iFocusFlags, std::vector<int>& aiExpertValues, std::vector<int>& aiProductionValues) const
{
	//Kaszkaj - Begin the Eye only through the once-per-turn military readiness decision.
	if (eBuilding == GC.getGameINLINE().getTranscendenceBuilding()) return 0;

	//
	bool bIsStarted = getBuildingProduction(eBuilding) > 0;

	CvBuildingInfo& kBuildingInfo = GC.getBuildingInfo(eBuilding);
	BuildingClassTypes eBuildingClass = (BuildingClassTypes)kBuildingInfo.getBuildingClassType();
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	bool bIsMilitary = false;

	int iValue = 0;

	bool bIsMajorCity = AI_isMajorCity();

	//Kaszkaj - Alien AI builds or upgrades schools when Convicts can train into useful specialists.
	if (isNative() && !isHuman() && kBuildingInfo.getSpecialBuildingType() == GC.getInfoTypeForString("SPECIALBUILDING_EDUCATION", true))
	{
		int iExpertValue = 0;
		for (int i = 0; i < GC.getNumUnitInfos(); ++i)
		{
			iExpertValue = std::max(iExpertValue, kOwner.AI_educationUnitValue((UnitTypes)i));
		}
		for (int i = 0; i < getPopulation(); ++i)
		{
			CvUnit* pUnit = getPopulationUnitByIndex(i);
			if (pUnit == NULL || std::strcmp(pUnit->getUnitInfo().getType(), "UNIT_CRIMINAL") != 0
				|| pUnit->getUnitInfo().getStudentWeight() <= 0 || pUnit->isColonistLocked())
			{
				continue;
			}
			for (int iProfession = 0; iProfession < GC.getNumProfessionInfos(); ++iProfession)
			{
				ProfessionTypes eProfession = (ProfessionTypes)iProfession;
				const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
				if (kProfession.getSpecialBuilding() == kBuildingInfo.getSpecialBuildingType()
					&& kProfession.getNumYieldsProduced() > 0 && kProfession.getYieldsProduced(0) == YIELD_EDUCATION
					&& kOwner.isProfessionValid(eProfession, pUnit->getUnitType()))
				{
					int iNewRate = std::max(0, (kBuildingInfo.getProfessionOutput()
						+ pUnit->getUnitInfo().getYieldChange(YIELD_EDUCATION))
						* (100 + pUnit->getUnitInfo().getYieldModifier(YIELD_EDUCATION)) / 100);
					int iOldRate = getProfessionOutput(eProfession, pUnit);
					iValue += iExpertValue * std::max(0, iNewRate - iOldRate);
					break;
				}
			}
		}
	}

	//Kaszkaj - Value buildings that provide jobs for resident Alien experts, even when Alien AI assigns zero value to the relevant yields.
	if (isNative() && !isHuman() && kBuildingInfo.getSpecialBuildingType() != NO_SPECIALBUILDING)
	{
		for (int i = 0; i < getPopulation(); ++i)
		{
			CvUnit* pUnit = getPopulationUnitByIndex(i);
			if (pUnit == NULL || !kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())) continue;
			for (int j = 0; j < GC.getNumProfessionInfos(); ++j)
			{
				ProfessionTypes eProfession = (ProfessionTypes)j;
				const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
				if (kProfession.getSpecialBuilding() != kBuildingInfo.getSpecialBuildingType()
					|| !kOwner.isProfessionValid(eProfession, pUnit->getUnitType())) continue;
				YieldTypes eYield = (YieldTypes)kProfession.getYieldsProduced(0);
				if (eYield == YIELD_IDEAS && (!kOwner.AI_hasResearchTarget() || canResearch() <= 0)) continue;
				YieldTypes eInput = (YieldTypes)kProfession.getYieldsConsumed(0, getOwnerINLINE());
				if (eInput != NO_YIELD && getYieldStored(eInput) + getRawYieldProduced(eInput) <= 0) continue;
				int iPriority = kOwner.AI_nativeProfessionPriority(pUnit->getUnitType(), eProfession, NULL, this);
				if (iPriority <= 0) continue;
				//Kaszkaj - Compare the new building with the same worker bonuses, so expert output does not hide useful upgrades.
				int iNewOutput = std::max(0, (kBuildingInfo.getProfessionOutput()
					+ kOwner.AI_getUnitYieldChange(pUnit->getUnitType(), eYield))
					* (100 + kOwner.AI_getUnitYieldModifier(pUnit->getUnitType(), eYield)) / 100);
				int iOutputGain = std::max(0, iNewOutput - getProfessionOutput(eProfession, pUnit));
				iValue += 10 * iPriority * iOutputGain;
			}
		}
	}

	if (kBuildingInfo.getYieldStorage() != 0)
	{
		int iCityCapacity = getMaxYieldCapacity();

		if (isHasBuilding(eBuilding))
		{
			iCityCapacity += kBuildingInfo.getYieldStorage() * GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getStoragePercent() / 100;
		}

		int iTotalExcess = 0;
		int iHighestPercentFull = 0;

		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			YieldTypes eLoopYield = (YieldTypes)i;

			if ((eLoopYield != YIELD_FOOD) && GC.getYieldInfo(eLoopYield).isCargo())
			{
				int iExcess = getYieldStored(eLoopYield) - iCityCapacity;
				if (iExcess > 0)
				{
					iTotalExcess += iExcess;
				}

				iHighestPercentFull = std::max(iHighestPercentFull, 100 * getYieldStored(eLoopYield) / iCityCapacity);
			}
		}

		int iTempValue = kBuildingInfo.getYieldStorage();

		iValue += iTempValue / 3;
		iValue += iHighestPercentFull;
		iValue += 10 * iTotalExcess;

		bIsMilitary = true;
	}

	if (kBuildingInfo.getDefenseModifier() != 0)
	{
		bool bAtWar = GET_TEAM(getTeam()).getAnyWarPlanCount();
		int iCityDefense = getDefenseModifier();
		int iDefense = kBuildingInfo.getDefenseModifier();

		iValue += (iDefense * (40 + 4 * getPopulation())) / (100 + 2 * iCityDefense);
		if (bAtWar)
		{
			if (iCityDefense < 50)
			{
				iValue += 100;
			}
		}
		bIsMilitary = true;
	}

	if (kBuildingInfo.isWorksWater())
	{
		if (!isWorksWater())
		{
			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
				if (pLoopPlot != NULL)
				{
					if (pLoopPlot->isWater())
					{
						iValue += 8 * std::max(0, pLoopPlot->getYield(YIELD_FOOD) - GC.getFOOD_CONSUMPTION_PER_POPULATION());
					}
				}
			}
		}
	}

	bool bIsGoodProfession = false;
	for (int iI = 0; iI < GC.getNumProfessionInfos(); iI++)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)iI;
		if (GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession))
		{
			CvProfessionInfo& kLoopProfession = GC.getProfessionInfo(eLoopProfession);

			//Kaszkaj - Buildings without indoor jobs must not match outdoor professions and hide their passive benefits.
			if (kBuildingInfo.getSpecialBuildingType() != NO_SPECIALBUILDING
				&& kLoopProfession.getSpecialBuilding() == kBuildingInfo.getSpecialBuildingType())
			{
				// MultipleYieldsProduced Start by Aymerick 22/01/2010**
				YieldTypes eYieldConsumed = (YieldTypes)kLoopProfession.getYieldsConsumed(0, GET_PLAYER(getOwner()).getID());
				YieldTypes eYieldProduced = (YieldTypes) kLoopProfession.getYieldsProduced(0);
				// MultipleYieldsProduced End
				//Kaszkaj - Value a local Industry job even outside major Colonies; another Colony's output does not remove its usefulness here.
				bool bLocalIndustry = eYieldProduced == YIELD_HAMMERS;
				if (bLocalIndustry && !kOwner.isProfessionValid(eLoopProfession, NO_UNIT)) continue;
				if (((eYieldProduced != NO_YIELD) && !kOwner.AI_isYieldFinalProduct(eYieldProduced) && (eYieldProduced != YIELD_HAMMERS)) || bIsMajorCity || bLocalIndustry)
				{
					int iHighestOutput = bLocalIndustry ? getProfessionOutput(eLoopProfession, NULL) : kOwner.AI_highestProfessionOutput(eLoopProfession, this);
					int iOutput = kBuildingInfo.getProfessionOutput();

					int iModifiedOutput = iOutput;
					if(eYieldProduced != NO_YIELD)
					{
						iModifiedOutput *= 100 + kBuildingInfo.getYieldModifier(eYieldProduced);
						iModifiedOutput /= 100;
					}

					if (iModifiedOutput > iHighestOutput)
					{
						if (iOutput != 0)
						{
							int iRawYieldProduced = getRawYieldProduced(eYieldProduced);
							if (iOutput > 0)
							{
								int iTempValue = AI_estimateYieldValue(eYieldProduced, iModifiedOutput);
								if (iRawYieldProduced > 0)
								{
									iTempValue *= 150 + 10 * iRawYieldProduced;
									iTempValue /= 100;
								}
								if (bIsStarted || getPopulation() >= 5)
								{
									if ((eYieldProduced == YIELD_HAMMERS) && !kOwner.AI_isStrategy(STRATEGY_CASH_FOCUS))
									{
										iTempValue *= 100 + 20 * (getPopulation() - 4);
										iTempValue /= 100;
									}
								}

								if (eYieldConsumed != NO_YIELD)
								{
									int iAvailable = getRawYieldProduced(eYieldConsumed) + std::max(0, AI_getTradeBalance(eYieldConsumed));

									if (iAvailable < iOutput)
									{
										int iMax = std::max(1, GC.getGameINLINE().getCargoYieldCapacity());

										int iPercent = 100 * getYieldStored(eYieldConsumed) / iMax;
										iPercent = std::max(iPercent, 100 * kOwner.AI_getBestPlotYield(eYieldConsumed) / iOutput);

										if (iPercent > 100)
										{
											iPercent = 100 + (iPercent - 100) / 2;
										}

										iTempValue *= iPercent;
										iTempValue /= 100;
									}
								}
//orlanth aliens								if ((eYieldProduced == YIELD_HORSES))
//								{
//									if (kOwner.isNative())
//									{
//										iTempValue *= 10;
//									}
//									else
//									{
//										iTempValue /= 10;
//									}
//end orlanth aliens								}

								if (iHighestOutput == 0)
								{
									int iMultiplier = 150;
									for (int i = 0; i < getPopulation(); ++i)
									{
										CvUnit* pLoopUnit = getPopulationUnitByIndex(i);
										if (kOwner.AI_isProfessionExpert(pLoopUnit->getUnitType(), eLoopProfession))
										{
											iMultiplier += 100;
										}
									}
									iTempValue *= iMultiplier;
									iTempValue /= 100;
								}

								if (eYieldProduced == YIELD_HORSES || eYieldProduced == YIELD_MUSKETS || eYieldProduced == YIELD_TOOLS || eYieldProduced == YIELD_FOOD)
								{
									bIsMilitary = true;
								}
								iValue += iTempValue;
								bIsGoodProfession = true;
							}
						}
					}
				}
			}
		}
	}

	if (bIsGoodProfession)
	{
		if (getPopulation() < 3)
		{
			iValue *= 1 + getPopulation();
			iValue /= 4;
		}
	}

	bool bHasPlotBonus = false;
	//Kaszkaj - Passive production and sea bonuses remain useful independently of a building's indoor profession score.
	{
		//XXX - underlying gameplay may be changed...
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			YieldTypes eLoopYield = (YieldTypes)i;
			if (kBuildingInfo.getRiverPlotYieldChange(i) != 0
				|| (kBuildingInfo.getSpecialBuildingType() == NO_SPECIALBUILDING && kBuildingInfo.getSeaPlotYieldChange(i) != 0))
				bHasPlotBonus = true;

			int iAdded = 0;

			iAdded += kBuildingInfo.getYieldChange(eLoopYield);
			iAdded += getBuildingYieldChange((BuildingClassTypes)kBuildingInfo.getBuildingClassType(), eLoopYield);
			iAdded += kOwner.getBuildingYieldChange((BuildingClassTypes)kBuildingInfo.getBuildingClassType(), eLoopYield);

			if (kBuildingInfo.getYieldModifier(eLoopYield) > 0)
			{
				iAdded += ((2 * getRawYieldProduced(eLoopYield)) * kBuildingInfo.getYieldModifier(eLoopYield)) / 100;
			}

			if (iAdded != 0)
			{
				if (eLoopYield == YIELD_HORSES || eLoopYield == YIELD_MUSKETS || eLoopYield == YIELD_TOOLS || eLoopYield == YIELD_FOOD)
				{
					bIsMilitary = true;
				}
				iValue += AI_estimateYieldValue(eLoopYield, iAdded);
			}

			if (kBuildingInfo.getSeaPlotYieldChange(i) != 0 && kBuildingInfo.getSpecialBuildingType() != NO_SPECIALBUILDING)
			{
				int iYieldChange = kBuildingInfo.getSeaPlotYieldChange(i);
				int iTempValue = 0;

				int iFood = 0;
				int iNumLandPlots = 0;

				for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
				{
					CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
					if ((pLoopPlot != NULL) && (pLoopPlot->getWorkingCity() == this))
					{
						if (pLoopPlot->isWater())
						{
							iTempValue += iYieldChange;
							if (pLoopPlot->isBeingWorked())
							{
								iTempValue += iYieldChange;
							}
							if (pLoopPlot->getBonusType() != NO_BONUS)
							{
								iTempValue += iYieldChange * 3;
							}
						}
						else if (iI != CITY_HOME_PLOT)
						{
							iFood += pLoopPlot->getYield(YIELD_FOOD);
							iNumLandPlots++;
						}
					}
				}

				iTempValue = AI_estimateYieldValue(eLoopYield, iTempValue);

				if (eLoopYield == YIELD_FOOD && iTempValue > 0 && iNumLandPlots > 0)
				{
					if (iFood / iNumLandPlots < 2)
					{
						iTempValue += 10;
						iTempValue += iNumLandPlots * 4 - iFood * 2;
					}
				}

				iValue += iTempValue;
				bIsMilitary = true;
			}
		}
	}

	//Kaszkaj - Include river production, protection against bombardment, additional overflow-sale Credits and fuel-supported Refinery benefits.
	if (bHasPlotBonus) iValue += AI_buildingPlotYieldValue(eBuilding);
	iValue += AI_buildingBombardValue(eBuilding);
	iValue += AI_buildingOverflowValue(eBuilding);
	iValue += AI_buildingRefineryValue(eBuilding);

	int iUnitsTrainedCount = 0;

	for (int i = 0; i < GC.getNumUnitInfos(); ++i)
	{
		if (GC.getUnitInfo((UnitTypes)i).getPrereqBuilding() == kBuildingInfo.getBuildingClassType())
		{
			iUnitsTrainedCount++;
		}
	}

	if (iUnitsTrainedCount > 0)
	{
		int iBuildingCount = kOwner.getBuildingClassCountPlusMaking(eBuildingClass);

		int iTargetBuildingCount = 1 + kOwner.getNumCities() / 10;

		if (iBuildingCount < iTargetBuildingCount)
		{
			iValue += 5 * calculateNetYield(YIELD_HAMMERS);
		}
		bIsMilitary = true;
	}

	if (bIsMajorCity && !(iFocusFlags & BUILDINGFOCUS_NO_RECURSION))
	{
		for (int i = 0; i < GC.getNumBuildingInfos(); ++i)
		{
			BuildingTypes eLoopBuilding = (BuildingTypes)i;
			if (!isHasBuilding(eLoopBuilding))
			{
				CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);

				if (kLoopBuilding.isBuildingClassNeededInCity(kBuildingInfo.getBuildingClassType()))
				{
					bool bOthersNeeded = false;
					for (int j = 0; j < GC.getNumBuildingInfos(); ++j)
					{
						BuildingTypes eLoopBuilding2 = (BuildingTypes)j;
						if ((eLoopBuilding2 != eBuilding) && isHasBuilding(eLoopBuilding2))
						{
							if (kLoopBuilding.isBuildingClassNeededInCity(GC.getBuildingInfo(eLoopBuilding2).getBuildingClassType()))
							{
								bOthersNeeded = true;
								break;
							}
						}
					 }
					if (bOthersNeeded)
					{
						iValue += AI_buildingValueWithCache(eLoopBuilding, iFocusFlags | BUILDINGFOCUS_NO_RECURSION, aiExpertValues, aiProductionValues) / 3;
					}
				}
			}
		}
	}

	if ((kBuildingInfo.getSpecialBuildingType() != NO_SPECIALBUILDING) && kBuildingInfo.getYieldStorage() == 0)
	{
		if (!isHasConceptualBuilding(eBuilding))//Prevents recursion.
		{
			int iBestValue = -1;
			BuildingTypes eBestExisting = NO_BUILDING;
			for (int iBuildingClass = 0; iBuildingClass < GC.getNumBuildingClassInfos(); ++iBuildingClass)
			{
				BuildingTypes eLoopBuilding = (BuildingTypes) GC.getCivilizationInfo(GET_PLAYER(getOwnerINLINE()).getCivilizationType()).getCivilizationBuildings(iBuildingClass);
				if ((NO_BUILDING != eLoopBuilding) && (eLoopBuilding != eBuilding))
				{
					CvBuildingInfo& kLoopBuilding = GC.getBuildingInfo(eLoopBuilding);
					if (kLoopBuilding.getSpecialBuildingType() == kBuildingInfo.getSpecialBuildingType())
					{
						if (isHasConceptualBuilding(eLoopBuilding))
						{
							int iValue = kLoopBuilding.getSpecialBuildingPriority();
							if (iValue > iBestValue)
							{
								iBestValue = iValue;
								eBestExisting = eLoopBuilding;
							}
						}
					}
				}
			}
			if (eBestExisting != NO_BUILDING)
			{
				iValue -= AI_buildingValueWithCache(eBestExisting, iFocusFlags, aiExpertValues, aiProductionValues);
			}
		}
	}

	//increase building value if only needs hammers
	if (!isHasConceptualBuilding(eBuilding) && (iFocusFlags & BUILDINGFOCUS_BUILD_ANYTHING))
	{
		iValue += 10;

		bool bNonHammerCost = false;
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			if ((kBuildingInfo.getYieldCost(i) > 0) && (i != YIELD_HAMMERS))
			{
				bNonHammerCost = true;
				break;
			}
		}

		if (!bNonHammerCost)
		{
			iValue += 10;
		}
	}

	//Kaszkaj - Give production buildings a higher priority when the Colony has suitable experts.
	if (!isHasConceptualBuilding(eBuilding))
	{
		iValue += AI_cachedExpertBuildingValue(eBuilding, aiExpertValues);
		if (!isHuman())
		{
			iValue = iValue * AI_cachedProductionBuildingValue(eBuilding, aiProductionValues) / 100;
			iValue = iValue * AI_productionSupplyPercent(NO_UNIT, eBuilding) / 100;
		}
	}

	return iValue;
}

//Kaszkaj - Value river bonuses and passive sea bonuses through legal local jobs; the Colony plot keeps its single natural cargo yield.
int CvCityAI::AI_buildingPlotYieldValue(BuildingTypes eBuilding) const
{
	const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	bool bHasPlotBonus = false;
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		if (kBuilding.getRiverPlotYieldChange(i) != 0
			|| (kBuilding.getSpecialBuildingType() == NO_SPECIALBUILDING && kBuilding.getSeaPlotYieldChange(i) != 0))
		{
			bHasPlotBonus = true;
			break;
		}
	}
	if (!bHasPlotBonus || isOccupation()) return 0;

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	std::vector<ProfessionTypes> aeProspectiveProfessions;
	bool bProspectiveReady = false;

	int iValue = 0;
	for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; ++iPlot)
	{
		CvPlot* pPlot = getCityIndexPlot(iPlot);
		if (pPlot == NULL || pPlot->isImpassable() || !canWork(pPlot)) continue;
		if (!pPlot->isRiver() && (!pPlot->isWater() || kBuilding.getSpecialBuildingType() != NO_SPECIALBUILDING)) continue;

		CvUnit* pUnit = getUnitWorkingPlot(iPlot);
		if (iPlot == CITY_HOME_PLOT)
		{
			YieldTypes eBestCargo = NO_YIELD;
			int iBestNature = 0;
			for (int i = 0; i < NUM_YIELD_TYPES; ++i)
			{
				if (i == YIELD_FOOD || i == YIELD_LUMBER) continue;
				int iNature = pPlot->calculateNatureYield((YieldTypes)i, getTeam(), false);
				if (iNature > iBestNature)
				{
					iBestNature = iNature;
					eBestCargo = (YieldTypes)i;
				}
			}
			for (int i = 0; i < NUM_YIELD_TYPES; ++i)
			{
				YieldTypes eYield = (YieldTypes)i;
				if (eYield != YIELD_FOOD && GC.getYieldInfo(eYield).isCargo() && eYield != eBestCargo) continue;
				int iChange = pPlot->isRiver() ? kBuilding.getRiverPlotYieldChange(i) : 0;
				if (pPlot->isWater() && kBuilding.getSpecialBuildingType() == NO_SPECIALBUILDING)
					iChange += kBuilding.getSeaPlotYieldChange(i);
				int iCityModifier = getBaseYieldRateModifier(eYield);
				if (!isHasBuilding(eBuilding)) iCityModifier += kBuilding.getYieldModifier(eYield);
				if (iChange != 0) iValue += AI_estimateYieldValue(eYield, iChange * iCityModifier / 100);
			}
			continue;
		}

		if (pUnit == NULL && !bProspectiveReady)
		{
			for (int i = 0; i < GC.getNumProfessionInfos(); ++i)
			{
				ProfessionTypes eProfession = (ProfessionTypes)i;
				const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
				if (kProfession.isCitizen() && kProfession.isWorkPlot() && kOwner.isProfessionValid(eProfession, NO_UNIT))
					aeProspectiveProfessions.push_back(eProfession);
			}
			bProspectiveReady = true;
		}
		int iBestValue = 0;
		ProfessionTypes eCurrentProfession = pUnit != NULL ? pUnit->getProfession() : NO_PROFESSION;
		int iProfessionCount = pUnit != NULL ? 1 : (int)aeProspectiveProfessions.size();
		for (int iProfession = 0; iProfession < iProfessionCount; ++iProfession)
		{
			ProfessionTypes eProfession = pUnit != NULL ? eCurrentProfession : aeProspectiveProfessions[iProfession];
			if (eProfession == NO_PROFESSION) continue;
			const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
			if (!kProfession.isWorkPlot() || kProfession.isWater() != pPlot->isWater()
				|| (pUnit != NULL && !kOwner.isProfessionValid(eProfession, pUnit->getUnitType()))) continue;
			int iProfessionValue = 0;
			for (int i = 0; i < kProfession.getNumYieldsProduced(); ++i)
			{
				YieldTypes eYield = (YieldTypes)kProfession.getYieldsProduced(i);
				if (eYield < 0 || eYield >= NUM_YIELD_TYPES) continue;
				int iChange = pPlot->isRiver() ? kBuilding.getRiverPlotYieldChange(eYield) : 0;
				if (pPlot->isWater() && kBuilding.getSpecialBuildingType() == NO_SPECIALBUILDING)
					iChange += kBuilding.getSeaPlotYieldChange(eYield);
				if (iChange == 0) continue;
				int iUnitModifier = pUnit != NULL ? 100 + pUnit->getUnitInfo().getYieldModifier(eYield) : 100;
				int iCityModifier = getBaseYieldRateModifier(eYield);
				if (!isHasBuilding(eBuilding)) iCityModifier += kBuilding.getYieldModifier(eYield);
				int iGain = iChange * iUnitModifier / 100;
				iProfessionValue += AI_estimateYieldValue(eYield, iGain * iCityModifier / 100);
			}
			if (pUnit != NULL) iBestValue = iProfessionValue;
			else iBestValue = std::max(iBestValue, iProfessionValue);
		}
		iValue += pUnit != NULL ? iBestValue : iBestValue / 2;
	}
	return iValue;
}

//Kaszkaj - Value only additional effective bombard protection, with greater priority during war or immediate danger.
int CvCityAI::AI_buildingBombardValue(BuildingTypes eBuilding) const
{
	int iProtection = GC.getBuildingInfo(eBuilding).getBombardDefenseModifier();
	if (iProtection <= 0 || getTotalDefense() <= 0) return 0;
	iProtection = std::min(iProtection, 100 - range(getBuildingBombardDefense(), 0, 100));
	if (iProtection <= 0) return 0;
	int iValue = std::max(1, iProtection * (40 + 4 * getPopulation()) * std::min(100, getTotalDefense()) / 10000);
	if (GET_TEAM(getTeam()).getAnyWarPlanCount() > 0) iValue *= 3;
	if (AI_isDanger()) iValue *= 2;
	return iValue;
}

//Kaszkaj - Value additional overflow-sale Credits after actual production, domestic demand, decay, Earth tax and trade modifiers.
int CvCityAI::AI_buildingOverflowValue(BuildingTypes eBuilding) const
{
	int iCandidatePercent = GC.getBuildingInfo(eBuilding).getOverflowSellPercent();
	if (iCandidatePercent <= 0) return 0;
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	if (kOwner.getParent() == NO_PLAYER) return 0;
	int iOldPercent = getOverflowYieldSellPercent();
	if (!isHuman() && !kOwner.isEurope() && kOwner.canTradeWithEurope())
		iOldPercent = std::max(iOldPercent, range(GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIMinimumStorageLossSellPercentage(), 0, 100));
	int iNewPercent = std::max(iOldPercent, iCandidatePercent);
	if (iNewPercent == iOldPercent) return 0;

	int aiNetYields[NUM_YIELD_TYPES];
	calculateNetYields(aiNetYields);
	int iCapacity = getMaxYieldCapacity();
	int iDecayPercent = GC.getDefineINT("CITY_YIELD_DECAY_PERCENT");
	int iMinimumDecay = GC.getDefineINT("MIN_CITY_YIELD_DECAY");
	int iTradeModifier = kOwner.getExtraTradeMultiplier(kOwner.getParent());
	int iValue = 0;
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		YieldTypes eYield = (YieldTypes)i;
		if (eYield == YIELD_FOOD || !GC.getYieldInfo(eYield).isCargo()) continue;
		int iStoredAfterProduction = std::max(0, getYieldStored(eYield) + aiNetYields[i]);
		int iExcess = std::max(0, iStoredAfterProduction - std::max(0, getYieldDemand(eYield))) - iCapacity;
		if (iExcess <= 0) continue;
		int iLoss = std::min(iExcess, std::max(iDecayPercent * iExcess / 100, iMinimumDecay));
		int iProfit = kOwner.getSellToEuropeProfit(eYield, iLoss);
		if (iProfit <= 0) continue;
		int iOldCredits = (iOldPercent * iProfit / 100) * iTradeModifier / 100;
		int iNewCredits = (iNewPercent * iProfit / 100) * iTradeModifier / 100;
		iValue += std::max(0, iNewCredits - iOldCredits);
	}
	return iValue;
}

//Kaszkaj - Value the Oil Refinery's additional indoor cargo production only when Hydrocarbons and all extra input goods support a positive net gain.
int CvCityAI::AI_buildingRefineryValue(BuildingTypes eBuilding) const
{
	if (eBuilding != (BuildingTypes)GC.getDefineINT("BUILDING_OIL_REFINERY") || isOccupation() || isHasRealBuilding(eBuilding)) return 0;
	int iConsumedHydrocarbons = getRawYieldConsumed(YIELD_HYDROCARBONS);
	if (iConsumedHydrocarbons <= 0) return 0;
	int iSurplus = getYieldStored(YIELD_HYDROCARBONS) + getRawYieldProduced(YIELD_HYDROCARBONS) - iConsumedHydrocarbons;
	int iDivisor = std::max(1, GC.getDefineINT("TK_OIL_REFINERY_HYDROCARBONS_PER_PRODUCTION"));
	int iExtra = 0;
	if (iSurplus != -iConsumedHydrocarbons)
	{
		if (iSurplus > iConsumedHydrocarbons && iSurplus != 0) iExtra = iConsumedHydrocarbons / iDivisor;
		else if (iSurplus < iConsumedHydrocarbons) iExtra = (iConsumedHydrocarbons + iSurplus) / iDivisor;
		else if (iSurplus == 0) iExtra = iConsumedHydrocarbons / iDivisor;
	}
	if (iExtra <= 0) return 0;

	bool abProduced[NUM_YIELD_TYPES];
	bool abConsumed[NUM_YIELD_TYPES];
	bool bHasCargoProduction = false;
	for (int i = 0; i < NUM_YIELD_TYPES; ++i) abProduced[i] = abConsumed[i] = false;
	abConsumed[YIELD_FOOD] = getPopulation() > 0;
	for (int iUnit = 0; iUnit < getPopulation(); ++iUnit)
	{
		CvUnit* pUnit = getPopulationUnitByIndex(iUnit);
		if (pUnit == NULL || pUnit->getProfession() == NO_PROFESSION) continue;
		ProfessionTypes eProfession = pUnit->getProfession();
		const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
		YieldTypes eProduct = (YieldTypes)kProfession.getYieldsProduced(0);
		if (eProduct < 0 || eProduct >= NUM_YIELD_TYPES || kProfession.isWorkPlot() || getProfessionOutput(eProfession, pUnit) <= 0) continue;
		if (eProduct == YIELD_IDEAS && canResearch() <= 0) continue;
		abProduced[eProduct] = true;
		if (eProduct != YIELD_HYDROCARBONS && GC.getYieldInfo(eProduct).getUnitClass() != NO_UNITCLASS) bHasCargoProduction = true;
		if (getProfessionInput(eProfession, pUnit) > 0)
		{
			for (int i = 0; i < kProfession.getNumYieldsConsumed(getOwnerINLINE()); ++i)
			{
				YieldTypes eInput = (YieldTypes)kProfession.getYieldsConsumed(i, getOwnerINLINE());
				if (eInput >= 0 && eInput < NUM_YIELD_TYPES) abConsumed[eInput] = true;
			}
		}
	}
	if (!bHasCargoProduction) return 0;

	int aiNetYields[NUM_YIELD_TYPES];
	calculateNetYields(aiNetYields);
	const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iValue = 0;
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		YieldTypes eYield = (YieldTypes)i;
		if (eYield == YIELD_HYDROCARBONS || GC.getYieldInfo(eYield).getUnitClass() == NO_UNITCLASS) continue;
		if (abConsumed[i])
		{
			int iFlatProduction = kBuilding.getYieldChange(i) + getBuildingYieldChange((BuildingClassTypes)kBuilding.getBuildingClassType(), eYield)
				+ kOwner.getBuildingYieldChange((BuildingClassTypes)kBuilding.getBuildingClassType(), eYield);
			int iAvailable = getYieldStored(eYield) + aiNetYields[i] + iFlatProduction * getBaseYieldRateModifier(eYield) / 100;
			if (iAvailable < iExtra) return 0;
			iValue -= AI_estimateYieldValue(eYield, iExtra);
		}
		if (abProduced[i]) iValue += AI_estimateYieldValue(eYield, iExtra * getBaseYieldRateModifier(eYield) / 100);
	}
	return std::max(0, iValue);
}

int CvCityAI::AI_cachedExpertBuildingValue(BuildingTypes eBuilding, std::vector<int>& aiValues) const
{
	const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	SpecialBuildingTypes eSpecialBuilding = (SpecialBuildingTypes)kBuilding.getSpecialBuildingType();
	if (aiValues.empty() || isHuman() || eSpecialBuilding == NO_SPECIALBUILDING
		|| kBuilding.getProfessionOutput() <= 0 || kBuilding.getMaxWorkers() <= 0)
	{
		return AI_expertBuildingValue(eBuilding);
	}
	int& iValue = aiValues[eSpecialBuilding];
	if (iValue == MIN_INT) iValue = AI_expertBuildingValue(eBuilding);
	return iValue;
}

int CvCityAI::AI_cachedProductionBuildingValue(BuildingTypes eBuilding, std::vector<int>& aiValues) const
{
	const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	SpecialBuildingTypes eSpecialBuilding = (SpecialBuildingTypes)kBuilding.getSpecialBuildingType();
	if (aiValues.empty() || eSpecialBuilding == NO_SPECIALBUILDING || kBuilding.getProfessionOutput() <= 0)
	{
		return AI_productionBuildingValue(eBuilding);
	}
	int& iValue = aiValues[eSpecialBuilding];
	if (iValue == MIN_INT) iValue = AI_productionBuildingValue(eBuilding);
	return iValue;
}

//Kaszkaj - Keep Food for residents, growth and recruitment before sending it to another Colony.
int CvCityAI::AI_foodReserve() const
{
	int iTurns = std::max(1, GC.getDefineINT("AI_MATERIAL_FOOD_RESERVE_TURNS"));
	int iGrowth = GET_PLAYER(getOwnerINLINE()).getGrowthThreshold(getPopulation());
	return std::max(std::max(0, GC.getDefineINT("AI_MATERIAL_RESERVE")), iGrowth + iTurns * foodConsumption());
}

//Kaszkaj - Prefer production that can obtain its materials soon, without making unfinished supply chains impossible.
int CvCityAI::AI_productionSupplyPercent(UnitTypes eUnit, BuildingTypes eBuilding) const
{
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iPercent = 100;
	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		YieldTypes eYield = (YieldTypes)i;
		if (!GC.getYieldInfo(eYield).isCargo()) continue;
		int iNeeded = eUnit != NO_UNIT ? getYieldProductionNeeded(eUnit, eYield)
			: (eBuilding != NO_BUILDING ? getYieldProductionNeeded(eBuilding, eYield) : 0);
		if (iNeeded <= 0) continue;
		int iAvailable = getYieldStored(eYield) + getYieldRushed(eYield);
		if (eYield == YIELD_FOOD) iAvailable -= AI_foodReserve();
		int iRate = getRawYieldProduced(eYield) - getRawYieldConsumed(eYield) + std::max(0, AI_getTradeBalance(eYield));
		if (eYield == YIELD_FOOD) iRate -= foodConsumption();
		iAvailable += 5 * std::max(0, iRate);
		if (iAvailable < iNeeded)
		{
			int iLoop;
			for (CvCity* pCity = kOwner.firstCity(&iLoop); pCity != NULL; pCity = kOwner.nextCity(&iLoop))
			{
				if (pCity == this || pCity->isOccupation()) continue;
				if (eYield == YIELD_FOOD && (!kOwner.isNative() || pCity->foodDifference() < 0)) continue;
				int iReserve = eYield == YIELD_FOOD ? pCity->AI_foodReserve() : GC.getDefineINT("AI_MATERIAL_RESERVE");
				int iOwnNeed = pCity->getProductionNeeded(eYield);
				if (iOwnNeed != MAX_INT) iReserve += std::max(0, iOwnNeed - pCity->getYieldRushed(eYield));
				iAvailable += std::max(0, pCity->getYieldStored(eYield) - iReserve) / 2;
			}
			iPercent = std::min(iPercent, range(25 + 75 * std::max(0, iAvailable) / iNeeded, 25, 100));
		}
	}
	return iPercent;
}

//Kaszkaj - Place factories where workers and all input goods are available; duplicate factories remain possible when useful.
int CvCityAI::AI_productionBuildingValue(BuildingTypes eBuilding) const
{
	const CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	if (kBuilding.getSpecialBuildingType() == NO_SPECIALBUILDING || kBuilding.getProfessionOutput() <= 0) return 100;
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iBestPercent = 0;
	bool bIndustrial = false;
	for (int i = 0; i < GC.getNumProfessionInfos(); ++i)
	{
		ProfessionTypes eProfession = (ProfessionTypes)i;
		const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
		if (!kProfession.isCitizen() || kProfession.isWorkPlot()
			|| kProfession.getSpecialBuilding() != kBuilding.getSpecialBuildingType()
			|| !kOwner.isProfessionValid(eProfession, NO_UNIT)) continue;
		YieldTypes eProduct = (YieldTypes)kProfession.getYieldsProduced(0);
		if (eProduct < 0 || eProduct >= NUM_YIELD_TYPES || !GC.getYieldInfo(eProduct).isCargo()) continue;
		bIndustrial = true;
		int iBestScore = 1;
		int iOurScore = 0;
		int iDuplicates = 0;
		int iLoop;
		for (CvCity* pCity = kOwner.firstCity(&iLoop); pCity != NULL; pCity = kOwner.nextCity(&iLoop))
		{
			if (pCity->isOccupation()) continue;
			int iSupply = 20;
			for (int j = 0; j < kProfession.getNumYieldsConsumed(getOwnerINLINE()); ++j)
			{
				YieldTypes eInput = (YieldTypes)kProfession.getYieldsConsumed(j, getOwnerINLINE());
				if (eInput < 0 || eInput >= NUM_YIELD_TYPES) continue;
				int iPotential = 0;
				for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; ++iPlot)
				{
					CvPlot* pPlot = plotCity(pCity->getX_INLINE(), pCity->getY_INLINE(), iPlot);
					if (pPlot != NULL && pPlot->getWorkingCity() == pCity && pCity->canWork(pPlot))
						iPotential = std::max(iPotential, pPlot->calculatePotentialYield(eInput, getOwnerINLINE(), pPlot->getImprovementType(), false, pPlot->getRouteType(), NO_UNIT, false));
				}
				int iAvailable = std::max(0, pCity->getRawYieldProduced(eInput) - pCity->getRawYieldConsumed(eInput))
					+ std::max(0, pCity->AI_getTradeBalance(eInput)) + std::min(20, pCity->getYieldStored(eInput) / 10) + iPotential;
				iSupply = std::min(iSupply, iAvailable);
			}
			int iExperts = 0;
			for (int j = 0; j < pCity->getPopulation(); ++j)
			{
				const CvUnit* pUnit = pCity->getPopulationUnitByIndex(j);
				if (pUnit != NULL && kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession)) ++iExperts;
			}
			int iScore = 4 * iSupply + 2 * pCity->getPopulation() + 8 * iExperts;
			iBestScore = std::max(iBestScore, iScore);
			if (pCity == this) iOurScore = iScore;
			else if (pCity->getNumProfessionBuildingSlots(eProfession) > 0) ++iDuplicates;
			else
			{
				for (CLLNode<OrderData>* pNode = pCity->headOrderQueueNode(); pNode != NULL; pNode = pCity->nextOrderQueueNode(pNode))
				{
					if (pNode->m_data.eOrderType == ORDER_CONSTRUCT && GC.getBuildingInfo((BuildingTypes)pNode->m_data.iData1).getSpecialBuildingType() == kBuilding.getSpecialBuildingType())
					{
						++iDuplicates;
						break;
					}
				}
			}
		}
		int iPercent = range(75 + 75 * iOurScore / iBestScore, 50, 150);
		if (professionCount(eProfession) == 0 && kOwner.AI_cityYieldTarget(this, eProduct) <= getYieldStored(eProduct))
			iPercent = std::max(50, iPercent * 100 / (100 + 40 * iDuplicates));
		iBestPercent = std::max(iBestPercent, iPercent);
	}
	return bIndustrial ? std::max(50, iBestPercent) : 100;
}

//Kaszkaj - Score production buildings using the Colony's available specialists.
int CvCityAI::AI_expertBuildingValue(BuildingTypes eBuilding) const
{
	CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
	if (isHuman() || kBuilding.getSpecialBuildingType() == NO_SPECIALBUILDING
		|| kBuilding.getProfessionOutput() <= 0 || kBuilding.getMaxWorkers() <= 0)
	{
		return 0;
	}
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iValue = 0;
	for (int iUnit = 0; iUnit < getPopulation(); ++iUnit)
	{
		CvUnit* pUnit = getPopulationUnitByIndex(iUnit);
		if (pUnit == NULL)
		{
			continue;
		}
		CvUnitInfo& kUnit = GC.getUnitInfo(pUnit->getUnitType());
		int iBestBonus = 0;
		for (int iProfession = 0; iProfession < GC.getNumProfessionInfos(); ++iProfession)
		{
			ProfessionTypes eProfession = (ProfessionTypes)iProfession;
			CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
			if (!kProfession.isCitizen() || kProfession.isWorkPlot()
				|| kProfession.getSpecialBuilding() != kBuilding.getSpecialBuildingType()
				|| !kOwner.isProfessionValid(eProfession, pUnit->getUnitType())
				|| kUnit.getProfessionsNotAllowed(eProfession)
				|| (pUnit->isColonistLocked() && pUnit->getProfession() != eProfession))
			{
				continue;
			}
			//Kaszkaj - Only count an expert's building bonus when its input goods are available or being imported.
			bool bHasInputs = true;
			for (int iInput = 0; iInput < kProfession.getNumYieldsConsumed(getOwnerINLINE()); ++iInput)
			{
				YieldTypes eInput = (YieldTypes)kProfession.getYieldsConsumed(iInput, getOwnerINLINE());
				if (eInput != NO_YIELD && (eInput < 0 || eInput >= NUM_YIELD_TYPES
					|| (getYieldStored(eInput) <= 0 && getRawYieldProduced(eInput) <= getRawYieldConsumed(eInput)
						&& AI_getTradeBalance(eInput) <= 0)))
				{
					bHasInputs = false;
					break;
				}
			}
			if (!bHasInputs)
			{
				continue;
			}
			for (int iYield = 0; iYield < kProfession.getNumYieldsProduced(); ++iYield)
			{
				YieldTypes eYield = (YieldTypes)kProfession.getYieldsProduced(iYield);
				if (eYield >= 0 && eYield < NUM_YIELD_TYPES
					&& kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession)
					&& kOwner.AI_yieldValue(eYield) > 0)
				{
					int iBonus = 10 + std::max(0, kOwner.AI_getUnitYieldModifier(pUnit->getUnitType(), eYield)) / 5
						+ 2 * std::max(0, kOwner.AI_getUnitYieldChange(pUnit->getUnitType(), eYield));
					iBestBonus = std::max(iBestBonus, std::min(100, iBonus));
				}
			}
		}
		iValue += iBestBonus;
	}
	return iValue;
}

int CvCityAI::AI_neededSeaWorkers() const
{
	CvArea* pWaterArea;
	int iNeededSeaWorkers = 0;

	pWaterArea = waterArea();

	if (pWaterArea == NULL)
	{
		return 0;
	}

	bool bNeedRoute = false;

	if (bNeedRoute)
	{
		iNeededSeaWorkers++;
	}

	return iNeededSeaWorkers;
}


bool CvCityAI::AI_isDefended(int iExtra) const
{
	PROFILE_FUNC();

	return ((AI_numDefenders(true, !isNative()) + iExtra) >= AI_neededDefenders()); // XXX check for other team's units?
}


int CvCityAI::AI_neededDefenders() const
{
	PROFILE_FUNC();
	int iDefenders = 0;
	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());

	if (isNative())
	{
		int iNeeded = 2 + (getPopulation() + 1) / 2;

		if (eAreaAI == AREAAI_OFFENSIVE)
		{
			iNeeded--;
		}
		else if (eAreaAI == AREAAI_DEFENSIVE)
		{
			iNeeded++;
		}
		return iNeeded;
	}

	if (GET_PLAYER(getOwnerINLINE()).AI_isKing())
	{
		return 2 + getHighestPopulation() / 2;
	}

	iDefenders = 2;

	if (GET_PLAYER(getOwnerINLINE()).AI_isStrategy(STRATEGY_REVOLUTION_PREPARING))
	{
		if (plot()->getNearestEurope() != NO_EUROPE)
		{
			iDefenders += 2;
			if (GET_PLAYER(getOwnerINLINE()).AI_isStrategy(STRATEGY_REVOLUTION_DECLARING))
			{
				iDefenders += 3;
			}
		}
	}

	iDefenders += getPopulation() / 2;

	return iDefenders;
}

int CvCityAI::AI_numDefenders(bool bDefenseOnly, bool bIncludePotential) const
{
	int iNum = plot()->plotCount(PUF_canDefendGroupHead, -1, -1, getOwnerINLINE(), NO_TEAM, bDefenseOnly ? PUF_isCityAIType : NULL);
	if (bIncludePotential)
	{
		iNum += AI_numPotentialDefenders();
	}
	return iNum;
}

int CvCityAI::AI_numPotentialDefenders() const
{
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iMaxEquipable = 0;
	for (int i = 0; i < GC.getNumProfessionInfos(); ++i)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)i;
		CvProfessionInfo& kProfession = GC.getProfessionInfo(eLoopProfession);

		if (kOwner.AI_professionValue(eLoopProfession, UNITAI_DEFENSIVE) > 0)
		{
			int iEquipable = getPopulation();
			for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
			{
				int iAmount = kOwner.getYieldEquipmentAmount(eLoopProfession, (YieldTypes)iYield);

				if (iAmount > 0)
				{
					iEquipable = std::min(iEquipable, getYieldStored((YieldTypes)iYield) / iAmount);
				}
			}

			iMaxEquipable = std::max(iEquipable, iMaxEquipable);
		}
	}

	return iMaxEquipable;
}

int CvCityAI::AI_minDefenders() const
{
	int iDefenders = 1;
	int iEra = GET_PLAYER(getOwnerINLINE()).getCurrentEra();
	if (iEra > 0)
	{
		iDefenders++;
	}
	if (((iEra - GC.getGame().getStartEra() / 2) >= GC.getNumEraInfos() / 2) && isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
	{
		iDefenders++;
	}

	return iDefenders;
}

int CvCityAI::AI_neededFloatingDefenders()
{
	if (m_iNeededFloatingDefendersCacheTurn != GC.getGame().getGameTurn())
	{
		AI_updateNeededFloatingDefenders();
	}
	return m_iNeededFloatingDefenders;
}

void CvCityAI::AI_updateNeededFloatingDefenders()
{
	int iFloatingDefenders = GET_PLAYER(getOwnerINLINE()).AI_getTotalFloatingDefendersNeeded(area());

	int iTotalThreat = std::max(1, GET_PLAYER(getOwnerINLINE()).AI_getTotalAreaCityThreat(area()));

	iFloatingDefenders -= area()->getCitiesPerPlayer(getOwnerINLINE());

	iFloatingDefenders *= AI_cityThreat();
	iFloatingDefenders += (iTotalThreat / 2);
	iFloatingDefenders /= iTotalThreat;

	m_iNeededFloatingDefenders = iFloatingDefenders;
	m_iNeededFloatingDefendersCacheTurn = GC.getGame().getGameTurn();
}

bool CvCityAI::AI_isDanger() const
{
	return GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 2, false);
}


int CvCityAI::AI_getEmphasizeAvoidGrowthCount() const
{
	return m_iEmphasizeAvoidGrowthCount;
}


bool CvCityAI::AI_isEmphasizeAvoidGrowth() const
{
	return (AI_getEmphasizeAvoidGrowthCount() > 0);
}


bool CvCityAI::AI_isAssignWorkDirty() const
{
	return m_bAssignWorkDirty;
}


void CvCityAI::AI_setAssignWorkDirty(bool bNewValue)
{
	m_bAssignWorkDirty = bNewValue;
}


bool CvCityAI::AI_isChooseProductionDirty() const
{
	return m_bChooseProductionDirty;
}


void CvCityAI::AI_setChooseProductionDirty(bool bNewValue)
{
	m_bChooseProductionDirty = bNewValue;
}


CvCity* CvCityAI::AI_getRouteToCity() const
{
	return getCity(m_routeToCity);
}


void CvCityAI::AI_updateRouteToCity()
{
	CvCity* pLoopCity;
	CvCity* pBestCity;
	int iValue;
	int iBestValue;
	int iLoop;
	int iI;

	gDLL->getFAStarIFace()->ForceReset(&GC.getRouteFinder());

	iBestValue = MAX_INT;
	pBestCity = NULL;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam())
		{
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				if (pLoopCity != this)
				{
					if (pLoopCity->area() == area())
					{
						if (!(gDLL->getFAStarIFace()->GeneratePath(&GC.getRouteFinder(), getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE(), false, getOwnerINLINE(), true)))
						{
							iValue = plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

							if (iValue < iBestValue)
							{
								iBestValue = iValue;
								pBestCity = pLoopCity;
							}
						}
					}
				}
			}
		}
	}

	if (pBestCity != NULL)
	{
		m_routeToCity = pBestCity->getIDInfo();
	}
	else
	{
		m_routeToCity.reset();
	}
}


int CvCityAI::AI_getEmphasizeYieldCount(YieldTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < NUM_YIELD_TYPES, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiEmphasizeYieldCount[eIndex];
}

bool CvCityAI::AI_isEmphasize(EmphasizeTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumEmphasizeInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");
	FAssertMsg(m_abEmphasize != NULL, "m_abEmphasize is not expected to be equal with NULL");
	return m_abEmphasize[eIndex];
}


void CvCityAI::AI_setEmphasize(EmphasizeTypes eIndex, bool bNewValue)
{
	FAssertMsg(eIndex >= 0, "eIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(eIndex < GC.getNumEmphasizeInfos(), "eIndex is expected to be within maximum bounds (invalid Index)");

	if (AI_isEmphasize(eIndex) != bNewValue)
	{
		m_abEmphasize[eIndex] = bNewValue;

		if (GC.getEmphasizeInfo(eIndex).isAvoidGrowth())
		{
			m_iEmphasizeAvoidGrowthCount += ((AI_isEmphasize(eIndex)) ? 1 : -1);
			FAssert(AI_getEmphasizeAvoidGrowthCount() >= 0);
		}

		for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			int iYieldChange = GC.getEmphasizeInfo(eIndex).getYieldChange(iI);
			if (iYieldChange != 0)
			{
				m_aiEmphasizeYieldCount[iI] += ((AI_isEmphasize(eIndex)) ? iYieldChange : -iYieldChange);
			}
		}

		AI_assignWorkingPlots();

		if ((getOwnerINLINE() == GC.getGameINLINE().getActivePlayer()) && isCitySelected())
		{
			gDLL->getInterfaceIFace()->setDirty(SelectionButtons_DIRTY_BIT, true);
			gDLL->getInterfaceIFace()->setDirty(Domestic_Advisor_DIRTY_BIT, true);
		}
	}
}

void CvCityAI::AI_forceEmphasizeCulture(bool bNewValue)
{
	if (m_bForceEmphasizeCulture != bNewValue)
	{
		m_bForceEmphasizeCulture = bNewValue;

		m_aiEmphasizeYieldCount[YIELD_CROSSES] += (bNewValue ? 1 : -1);
		FAssert(m_aiEmphasizeYieldCount[YIELD_CROSSES] >= 0);
	}
}


int CvCityAI::AI_getBestBuildValue(int iIndex) const
{
	FAssertMsg(iIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aiBestBuildValue[iIndex];
}


int CvCityAI::AI_totalBestBuildValue(CvArea* pArea) const
{
	CvPlot* pLoopPlot;
	int iTotalValue;
	int iI;

	iTotalValue = 0;

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pArea)
				{
					if ((pLoopPlot->getImprovementType() == NO_IMPROVEMENT) || !(GET_PLAYER(getOwnerINLINE()).isOption(PLAYEROPTION_SAFE_AUTOMATION) && !(pLoopPlot->getImprovementType() == (GC.getDefineINT("RUINS_IMPROVEMENT")))))
					{
						iTotalValue += AI_getBestBuildValue(iI);
					}
				}
			}
		}
	}

	return iTotalValue;
}

int CvCityAI::AI_clearFeatureValue(int iIndex)
{
	CvPlot* pPlot = plotCity(getX_INLINE(), getY_INLINE(), iIndex);
	FAssert(pPlot != NULL);

	FeatureTypes eFeature = pPlot->getFeatureType();
	FAssert(eFeature != NO_FEATURE);

	CvFeatureInfo& kFeatureInfo = GC.getFeatureInfo(eFeature);

	int iValue = 0;
	iValue += kFeatureInfo.getYieldChange(YIELD_FOOD) * 100;

	if (iValue > 0 && pPlot->isBeingWorked())
	{
		iValue *= 3;
		iValue /= 2;
	}
	if (iValue != 0)
	{
		BonusTypes eBonus = pPlot->getBonusType();
		if (eBonus != NO_BONUS)
		{
			iValue *= 3;
		}
	}

	if (iValue > 0)
	{
		if (pPlot->getImprovementType() != NO_IMPROVEMENT)
		{
			if (GC.getImprovementInfo(pPlot->getImprovementType()).isRequiresFeature())
			{
				iValue += 500;
			}
		}

		if (GET_PLAYER(getOwnerINLINE()).getAdvancedStartPoints() >= 0)
		{
			iValue += 400;
		}
	}

	return -iValue;
}

BuildTypes CvCityAI::AI_getBestBuild(int iIndex) const
{
	FAssertMsg(iIndex >= 0, "iIndex is expected to be non-negative (invalid Index)");
	FAssertMsg(iIndex < NUM_CITY_PLOTS, "eIndex is expected to be within maximum bounds (invalid Index)");
	return m_aeBestBuild[iIndex];
}


int CvCityAI::AI_countBestBuilds(CvArea* pArea) const
{
	CvPlot* pLoopPlot;
	int iCount;
	int iI;

	iCount = 0;

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

			if (pLoopPlot != NULL)
			{
				if (pLoopPlot->area() == pArea)
				{
					if (AI_getBestBuild(iI) != NO_BUILD)
					{
						iCount++;
					}
				}
			}
		}
	}

	return iCount;
}


// Improved worker AI provided by Blake - thank you!
void CvCityAI::AI_updateBestBuild()
{
	PROFILE_FUNC();

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		m_aiBestBuildValue[iI] = 0;
		m_aeBestBuild[iI] = NO_BUILD;

		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

			if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
			{
				AI_bestPlotBuild(pLoopPlot, &(m_aiBestBuildValue[iI]), &(m_aeBestBuild[iI]));

				if (m_aiBestBuildValue[iI] > 0)
				{
					FAssert(m_aeBestBuild[iI] != NO_BUILD);
				}
				if (m_aeBestBuild[iI] != NO_BUILD)
				{
					FAssert(m_aiBestBuildValue[iI] > 0);
				}
			}
		}
	}
}

// Protected Functions...

void CvCityAI::AI_doHurry(bool bForce)
{
	PROFILE_FUNC();
	FAssert(!isHuman() || isProductionAutomated());


	if (getProduction() == 0)
	{
		return;
	}

	HurryTypes eGoldHurry = NO_HURRY;
	for (int i = 0; i < GC.getNumHurryInfos(); ++i)
	{
		if (GC.getHurryInfo((HurryTypes)i).getGoldPerProduction() > 0)
		{
			eGoldHurry = (HurryTypes)i;
			break;
		}
	}

	int iHurryValue = 0;


	if (getProduction() >= getProductionNeeded(YIELD_HAMMERS))
	{
		iHurryValue += 100;
	}

	bool bCritical = false;

	if (getProductionUnit() != NO_UNIT)
	{
		if (getProductionUnitAI() == UNITAI_WAGON)
		{
			if (area()->getNumAIUnits(getOwnerINLINE(),UNITAI_WAGON) == 0)
			{
				iHurryValue += 100;
			}
		}
	}
	else if (getProductionBuilding() != NO_BUILDING)
	{
		iHurryValue += AI_buildingValue(getProductionBuilding());
		if (getDefenseModifier() == 0)
		{
			iHurryValue += GC.getBuildingInfo(getProductionBuilding()).getDefenseModifier() * 2;
			if (AI_isDanger())
			{
				bCritical = true;
			}
		}
	}

	int iThreshold = 50;
	if (getPopulation() > 3)
	{
		iThreshold -= 3 * (getPopulation() - 2);
		iThreshold = std::max(20, iThreshold);
	}

	bool bAffordable = GET_PLAYER(getOwnerINLINE()).AI_getHurrySpending() < GET_PLAYER(getOwnerINLINE()).AI_getTotalIncome() / 2;
	if (getHurryYieldDeficit(eGoldHurry, YIELD_LUMBER) == 0)
	{
		iHurryValue += 25;
		if (bAffordable)
		{
			iHurryValue += 25;
		}
	}

	if (!bCritical && (iHurryValue < iThreshold))
	{
		if (getPopulation() < 4)
		{
			return;
		}

		if (GC.getGameINLINE().getSorenRandNum(100, "AI Hurry") > 25)
		{
			return;
		}

		if (bAffordable)
		{
			return;
		}
	}


		for (int i = 0; i < GC.getNumHurryInfos(); ++i)
		{
			if (canHurry((HurryTypes)i))
			{
				//Kaszkaj - Keep the worker budget unless immediate defence or replacement of the last transport is essential.
				bool bEmergency = bCritical || bForce || (AI_isDanger() && getProductionUnitAI() == UNITAI_DEFENSIVE)
					|| (getProductionUnitAI() == UNITAI_TRANSPORT_SEA && GET_PLAYER(getOwnerINLINE()).AI_transportCapacity(false) == 0);
				if (!isHuman() && !bEmergency && GET_PLAYER(getOwnerINLINE()).getGold() - hurryGold((HurryTypes)i)
					< GET_PLAYER(getOwnerINLINE()).AI_goldTarget()) continue;
				hurry((HurryTypes)i);
			return;
		}
	}
	return;
}

void CvCityAI::AI_doNativeTrade()
{
	//Each turn a random yield (weighted by quantity) will be
	//instantly delivered to a random city (weighted inversely by quantity)

	int iBestYieldValue = 0;
	YieldTypes eBestYield = NO_YIELD;


	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eYield = (YieldTypes)iI;

		if (GC.getYieldInfo(eYield).getNativeSellPrice() == -1)
		{
			int iValue = getYieldStored(eYield);
			if (iValue > 0)
			{
				int iProduced = getRawYieldProduced(eYield);
				if (iValue > AI_getRequiredYieldLevel(eYield) || iProduced > 0)
				{
					if (eYield == YIELD_FOOD)
					{
						iValue /= 5;
					}
					else if ((eYield == YIELD_HORSES) || (eYield == YIELD_MUSKETS))
					{
						iValue *= 2;
					}
					else if (eYield == YIELD_LUMBER)
					{
						iValue *= 2;
					}

					iValue = 1 + GC.getGameINLINE().getSorenRandNum(iValue, "AI best Yield to Trade");
					if (iValue > iBestYieldValue)
					{
						iBestYieldValue = iValue;
						eBestYield = eYield;
					}
				}
			}
		}
	}

	if (eBestYield == NO_YIELD)
	{
		return;
	}

	CvCity* pBestCity = NULL;
	int iBestCityValue = 0;

	CvYieldInfo& kBestYield = GC.getYieldInfo(eBestYield);
	CvPlayer& kOwner = GET_PLAYER(getOwner());
	int iLoop;
	CvCity* pLoopCity;
	for (pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
	{
		if (pLoopCity != this)
		{

			int iValue = 10 * std::max(0, pLoopCity->AI_getRequiredYieldLevel(eBestYield) - pLoopCity->getYieldStored(eBestYield));
			iValue = std::max(iValue, pLoopCity->getMaxYieldCapacity() - pLoopCity->getYieldStored(eBestYield));
			if (eBestYield == YIELD_HORSES)
			{
				iValue *= 3 + pLoopCity->foodDifference();
				iValue /= 3;
			}
			if (iValue > 0)
			{
				int iYieldNeeded = pLoopCity->AI_getRequiredYieldLevel(eBestYield) - pLoopCity->getYieldStored(eBestYield);
				if (iYieldNeeded > 0)
				{
					iValue *= 5;
					if (pLoopCity->getYieldStored(eBestYield) > 0)
					{
						//HUGELY bias in favor of nearly-full cities
						iValue *= (88 / iYieldNeeded);
					}
				}
				int iDistance = plotDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());

				iValue *= 50 + GC.getGameINLINE().getSorenRandNum(50, "AI best city to trade yield to");
				iValue /= 5 + iDistance;


				if (iValue > iBestCityValue)
				{
					iBestCityValue = iValue;
					pBestCity = pLoopCity;
				}
			}
		}
	}

	if (pBestCity == NULL)
	{
		return;
	}
	int iChange = 0;
	if (pBestCity->AI_getRequiredYieldLevel(eBestYield) > pBestCity->getYieldStored(eBestYield))
	{
		iChange = pBestCity->AI_getRequiredYieldLevel(eBestYield) - pBestCity->getYieldStored(eBestYield);
		iChange = std::min(iChange, getYieldStored(eBestYield));
	}
	else
	{
		iChange = getYieldStored(eBestYield) - AI_getRequiredYieldLevel(eBestYield);
		if (AI_getRequiredYieldLevel(eBestYield) == 0)
		{
			iChange /= 2;
		}
	}

	changeYieldStored(eBestYield, -iChange);
	pBestCity->changeYieldStored(eBestYield, iChange);
}

void CvCityAI::AI_doNative()
{
	//Kaszkaj - Alien Convict residents may seek better lessons at Alien Colonies while keeping students at school and enough Food in the Colony.
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	if (isNative() && !isHuman() && !isDisorder() && getPopulation() > 1 && foodDifference() >= 0
		&& kOwner.AI_getPlotDanger(plot(), 2) == 0)
	{
		for (int i = getPopulation() - 1; i >= 0; --i)
		{
			CvUnit* pUnit = getPopulationUnitByIndex(i);
			if (pUnit == NULL || pUnit->isColonistLocked()
				|| std::strcmp(pUnit->getUnitInfo().getType(), "UNIT_CRIMINAL") != 0)
			{
				continue;
			}
			ProfessionTypes eOldProfession = pUnit->getProfession();
			if (eOldProfession != NO_PROFESSION && GC.getProfessionInfo(eOldProfession).getNumYieldsProduced() > 0)
			{
				YieldTypes eYield = (YieldTypes)GC.getProfessionInfo(eOldProfession).getYieldsProduced(0);
				if (eYield == YIELD_EDUCATION)
				{
					continue;
				}
			}
			ProfessionTypes eMapProfession = (ProfessionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultProfession();
			if (eMapProfession == NO_PROFESSION || !pUnit->canHaveProfession(eMapProfession, false))
			{
				continue;
			}
			CvPlot* pWorkedPlot = getPlotWorkedByUnit(pUnit);
			int iKeepValue = AI_professionValue(eOldProfession, pUnit, pWorkedPlot, NULL);
			for (int iProfession = 0; iProfession < GC.getNumProfessionInfos(); ++iProfession)
			{
				ProfessionTypes eProfession = (ProfessionTypes)iProfession;
				if (GC.getProfessionInfo(eProfession).getSpecialBuilding() == GC.getInfoTypeForString("SPECIALBUILDING_EDUCATION", true)
					&& pUnit->canHaveProfession(eProfession, false))
				{
					iKeepValue = std::max(iKeepValue, AI_professionValue(eProfession, pUnit, NULL, NULL));
				}
			}
			int iPriority = 100 + std::max(0, kOwner.AI_getUnitYieldModifier(pUnit->getUnitType(), YIELD_EDUCATION))
				+ 22 * std::max(0, kOwner.AI_getUnitYieldChange(pUnit->getUnitType(), YIELD_EDUCATION));
			int iPlanningTurns = std::max(1, GC.getDefineINT("EDUCATION_THRESHOLD")
				* GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getGrowthPercent() / 100);
			bool bUsefulLesson = false;
			for (int iPlayer = 0; iPlayer < MAX_PLAYERS && !bUsefulLesson; ++iPlayer)
			{
				CvPlayerAI& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
				if (iPlayer == getOwnerINLINE() || !kPlayer.isAlive() || !kPlayer.isNative())
				{
					continue;
				}
				int iLoop;
				for (CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
				{
					if (pCity->getArea() != getArea() || atWar(getTeam(), pCity->getTeam())
						|| !pCity->plot()->isRevealed(getTeam(), false) || pCity->plot()->isVisibleEnemyUnit(pUnit)
						|| !pUnit->canEnterArea(pCity->getOwnerINLINE(), pCity->area()) || !pUnit->canLearn(pCity->plot(), true))
					{
						continue;
					}
					int iExpertValue = kOwner.AI_educationUnitValue(pUnit->getLearnUnitType(pCity->plot(), true));
					int iTurns = std::max(0, pUnit->getLearnTime(pCity->plot(), true))
						+ plotDistance(getX_INLINE(), getY_INLINE(), pCity->getX_INLINE(), pCity->getY_INLINE()) / std::max(1, pUnit->baseMoves());
					if (iExpertValue * iPriority * iPlanningTurns / std::max(1, iTurns) > iKeepValue)
					{
						bUsefulLesson = true;
						break;
					}
				}
			}
			if (bUsefulLesson)
			{
				UnitAITypes eOldUnitAI = pUnit->AI_getUnitAIType();
				if (removePopulationUnit(pUnit, false, eMapProfession))
				{
					pUnit->AI_setUnitAIType(UNITAI_COLONIST);
					if (foodDifference() >= 0 && static_cast<CvUnitAI*>(pUnit)->AI_learn())
					{
						return;
					}
					//Kaszkaj - Restore the worker and its plot if leaving would cause a Food shortage or no training route is available.
					addPopulationUnit(pUnit, eOldProfession);
					pUnit->AI_setUnitAIType(eOldUnitAI);
					if (pWorkedPlot != NULL)
					{
						setUnitWorkingPlot(pWorkedPlot, pUnit->getID());
					}
				}
			}
		}
	}

//orlanth aliens
//	AI_doNativeTrade();

//	FAssert(isNative());
//	CvPlayer& kPlayer = GET_PLAYER(getOwner());
//	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
//	{
//		YieldTypes eYield = (YieldTypes)iI;
//		if ((eYield != YIELD_FOOD) && (eYield != YIELD_HORSES) && (eYield != YIELD_MUSKETS))
//		{
//			int iTotalStored = kPlayer.countTotalYieldStored(eYield);
//			int iMaxStored = kPlayer.getNumCities() * GC.getGameINLINE().getCargoYieldCapacity();
//			iMaxStored *= GC.getYieldInfo(eYield).getNativeConsumptionPercent();
//
//			int iDestructionModifier = 50 + ((50 * iTotalStored) / iMaxStored);
//
//			int iAmountLost = 0;
//			int iPercent = 3;
//			iPercent += GC.getGame().getSorenRandNum(8, "AI doNative destroy yield percent");
//
//			iAmountLost += ((getYieldStored(eYield) * iPercent) + 50) / 100;
//
//
//			if (GC.getGame().getSorenRandNum(100, "AI doNative destroy more yield") < (iDestructionModifier / 3))
//			{
//				iAmountLost += std::min(getYieldStored(eYield), getPopulation());
//			}
//
//			iAmountLost = std::min(iAmountLost, getYieldStored(eYield));
//
//			if (iAmountLost > 0)
//			{
//				changeYieldStored(eYield, -iAmountLost);
//				int iSellPrice = GC.getYieldInfo(eYield).getNativeSellPrice();
//				if (iSellPrice > 0)
//				{
//					kPlayer.changeGold(((iAmountLost * iSellPrice) * 4) / 100);
//				}
//			}
//		}
//	}
//
//			iAmountLost = std::min(iAmountLost, getYieldStored(eYield));
//
//			if (iAmountLost > 0)
//			{
//				changeYieldStored(eYield, -iAmountLost);
//				int iSellPrice = GC.getYieldInfo(eYield).getNativeSellPrice();
//				if (iSellPrice > 0)
//				{
//					kPlayer.changeGold(((iAmountLost * iSellPrice) * 4) / 100);
//				}
//			}
//		}
//	}
//end orlanth aliens

//	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
//
//	if (eAreaAI != AREAAI_NEUTRAL)
//	{
//		int iLowestPopulation = getHighestPopulation();
//
//		if (eAreaAI == AREAAI_OFFENSIVE)
//		{
//			iLowestPopulation *= 49;
//		}
//		else if (eAreaAI == AREAAI_BALANCED)
//		{
//			iLowestPopulation *= 85;
//		}
//		else if (eAreaAI == AREAAI_DEFENSIVE)
//		{
//			iLowestPopulation *= 64;
//		}
//		else
//		{
//			iLowestPopulation *= 100;
//		}
//		iLowestPopulation /= 100;
//
//		iLowestPopulation = std::max(iLowestPopulation, AI_getTargetSize() - ((eAreaAI == AREAAI_OFFENSIVE) ? 1 : 0));
//
//		if (getPopulation() > iLowestPopulation)
//		{
//			ProfessionTypes eBraveProfession = GET_PLAYER(getOwnerINLINE()).AI_idealProfessionForUnitAIType(UNITAI_DEFENSIVE, this);
//			if (eBraveProfession != NO_PROFESSION)
//			{
//				for (int i = 0; i < getPopulation(); ++i)
//				{
//					CvUnit* pUnit = getPopulationUnitByIndex(i);
//					if (pUnit != NULL)
//					{
//						if (pUnit->canHaveProfession(eBraveProfession, false))
//						{
//							alterUnitProfession(pUnit->getID(), eBraveProfession);
//							break;
//						}
//					}
//				}
//			}
//		}
//	}

}

void CvCityAI::AI_resetTradedYields()
{
	for (int i = 0; i < NUM_YIELD_TYPES; i++)
	{
		YieldTypes eLoopYield = (YieldTypes)i;
		m_aiTradeBalance[eLoopYield] = 0;
	}
}

//This should only be called once per turn.
void CvCityAI::AI_doTradedYields()
{

	int iDiscountPercent = 100 - 100 / YIELD_DISCOUNT_TURNS;
	iDiscountPercent -= 2;

	for (int i = 0; i < NUM_YIELD_TYPES; i++)
	{
		YieldTypes eLoopYield = (YieldTypes)i;

		if (GC.getYieldInfo(eLoopYield).isCargo())
		{
			m_aiTradeBalance[eLoopYield] *= iDiscountPercent;
			m_aiTradeBalance[eLoopYield] /= 100;
		}
	}
}

// Improved use of emphasize by Blake, to go with his whipping strategy - thank you!
void CvCityAI::AI_doEmphasize()
	{
	PROFILE_FUNC();

	FAssert(!isHuman());

	for (int iI = 0; iI < GC.getNumEmphasizeInfos(); iI++)
	{
		AI_setEmphasize(((EmphasizeTypes)iI), false);
	}
}

bool CvCityAI::AI_chooseBuild()
{
	const int iCacheSize = cityBuildingValueCacheSize();
	std::vector<int> aiExpertValues(iCacheSize, MIN_INT);
	std::vector<int> aiProductionValues(iCacheSize, MIN_INT);

	//These are now directly comparable.
	int iBestValue = 0;
	BuildingTypes eBestBuilding = NO_BUILDING;
	UnitTypes eBestUnit = NO_UNIT;
	int iFocusFlags = 0;

	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
	{
		BuildingTypes eLoopBuilding = ((BuildingTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationBuildings(iI)));

		if ((eLoopBuilding != NO_BUILDING) && (!isHasConceptualBuilding(eLoopBuilding)))
		{
			if (canConstruct(eLoopBuilding))
			{
				int iValue = AI_buildingValueWithCache(eLoopBuilding, iFocusFlags, aiExpertValues, aiProductionValues);

				if (iValue > 0)
				{
					int iTurnsLeft = getProductionTurnsLeft(eLoopBuilding, 0);


					iValue *= (GC.getGameINLINE().getSorenRandNum(25, "AI Best Building") + 100);
					iValue /= 100;

					iValue += getBuildingProduction(eLoopBuilding);


					FAssert((MAX_INT / 1000) > iValue);
					iValue *= 1000;
					iValue /= std::max(1, (iTurnsLeft + 3));

					iValue = std::max(1, iValue);

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestBuilding = eLoopBuilding;
					}
				}
			}
		}
	}

	for (int iI = 0; iI < GC.getNumUnitClassInfos(); iI++)
	{
		UnitTypes eLoopUnit = ((UnitTypes)(GC.getCivilizationInfo(getCivilizationType()).getCivilizationUnits(iI)));

		if (eLoopUnit != NO_UNIT)
		{
			if (canTrain(eLoopUnit))
			{
				UnitAITypes eUnitAI = NO_UNITAI;
				int iValue = GET_PLAYER(getOwnerINLINE()).AI_unitEconomicValue(eLoopUnit, &eUnitAI, this);

				iValue *= (GC.getGameINLINE().getSorenRandNum(25, "AI Best Unit") + 100);
				iValue /= 100;

//				iValue *= (GET_PLAYER(getOwnerINLINE()).getNumCities() * 2);
//				iValue /= (GET_PLAYER(getOwnerINLINE()).getUnitClassCountPlusMaking((UnitClassTypes)iI) + GET_PLAYER(getOwnerINLINE()).getNumCities() + 1);

				FAssert((MAX_INT / 1000) > iValue);
				iValue *= 1000;

				iValue /= std::max(1, (4 + getProductionTurnsLeft(eLoopUnit, 0)));

				iValue = std::max(1, iValue);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestUnit = eLoopUnit;
					eBestBuilding = NO_BUILDING;
				}
			}
		}
	}

	if (eBestBuilding != NULL)
	{
		FAssert(eBestUnit == NULL);
		pushOrder(ORDER_TRAIN, eBestUnit, NO_UNITAI, false, false, false);

	}
	else if (eBestUnit != NULL)
	{
		FAssert(eBestBuilding == NULL);
		pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);

	}

	return false;

}

bool CvCityAI::AI_chooseUnit(UnitAITypes eUnitAI, bool bPickAny)
{
	UnitTypes eBestUnit;

	if (eUnitAI != NO_UNITAI)
	{
		eBestUnit = AI_bestUnitAI(eUnitAI);
	}
	else
	{
		eBestUnit = AI_bestUnit(false, &eUnitAI, bPickAny);
	}

	if (eBestUnit != NO_UNIT)
	{
		pushOrder(ORDER_TRAIN, eBestUnit, eUnitAI, false, false, false);
		return true;
	}

	return false;
}

bool CvCityAI::AI_chooseUnit(UnitTypes eUnit, UnitAITypes eUnitAI)
{
	if (eUnit != NO_UNIT)
	{

		pushOrder(ORDER_TRAIN, eUnit, eUnitAI, false, false, false);
		return true;
	}
	return false;
}


bool CvCityAI::AI_chooseDefender()
{

	if (plot()->plotCheck(PUF_isUnitAIType, UNITAI_DEFENSIVE, -1, getOwnerINLINE()) == NULL)
	{
		if (AI_chooseUnit(UNITAI_DEFENSIVE))
		{
			return true;
		}
	}

	if (AI_chooseUnit(UNITAI_COUNTER))
	{
		return true;
	}

	return false;
}

bool CvCityAI::AI_chooseLeastRepresentedUnit(UnitTypeWeightArray &allowedTypes)
{
	int iValue;

	UnitTypeWeightArray::iterator it;

 	std::multimap<int, UnitAITypes, std::greater<int> > bestTypes;
 	std::multimap<int, UnitAITypes, std::greater<int> >::iterator best_it;


	for (it = allowedTypes.begin(); it != allowedTypes.end(); it++)
	{
		iValue = (*it).second;
		iValue *= 750 + GC.getGameINLINE().getSorenRandNum(250, "AI choose least represented unit");
		iValue /= 1 + GET_PLAYER(getOwnerINLINE()).AI_totalAreaUnitAIs(area(), (*it).first);
		bestTypes.insert(std::make_pair(iValue, (*it).first));
	}

 	for (best_it = bestTypes.begin(); best_it != bestTypes.end(); best_it++)
 	{
		if (AI_chooseUnit(best_it->second))
		{
			return true;
		}
 	}
	return false;
}

bool CvCityAI::AI_bestSpreadUnit(bool bMissionary, int iBaseChance, UnitTypes* eBestSpreadUnit, int* iBestSpreadUnitValue) const
{
	CvPlayerAI& kPlayer = GET_PLAYER(getOwnerINLINE());
	CvTeamAI& kTeam = GET_TEAM(getTeam());
	CvGame& kGame = GC.getGame();

	FAssert(eBestSpreadUnit != NULL && iBestSpreadUnitValue != NULL);

	return (*eBestSpreadUnit != NULL);
}

bool CvCityAI::AI_chooseBuilding(int iFocusFlags, int iMaxTurns, int iMinThreshold)
{
	BuildingTypes eBestBuilding;

	eBestBuilding = AI_bestBuildingThreshold(iFocusFlags, iMaxTurns, iMinThreshold);

	if (eBestBuilding != NO_BUILDING)
	{
		pushOrder(ORDER_CONSTRUCT, eBestBuilding, -1, false, false, false);
		return true;
	}

	return false;
}


// Returns true if a worker was added to a plot...
bool CvCityAI::AI_addBestCitizen()
{
	PROFILE_FUNC();

	bool bAvoidGrowth = AI_avoidGrowth();
	bool bIgnoreGrowth = AI_ignoreGrowth();

	int iUnitId = getNextFreeUnitId();
	CvUnit* pUnit = getPopulationUnitById(iUnitId);
	ProfessionTypes eCurrentProfession = pUnit->getProfession();
	if ((NO_PROFESSION == eCurrentProfession) || !GC.getProfessionInfo(eCurrentProfession).isWorkPlot() || (GC.getProfessionInfo(eCurrentProfession).isWorkPlot()))
	{
		int iBestValue = 0;
		int iBestPlot = -1;
		ProfessionTypes eBestProfession = NO_PROFESSION;

		for (int i=0;i<GC.getNumProfessionInfos();i++)
		{
			ProfessionTypes eLoopProfession = (ProfessionTypes) i;
			if (GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession))
			{
				if (GC.getProfessionInfo(eLoopProfession).isCitizen())
				{
					if (pUnit->canHaveProfession(eLoopProfession, false))
					{
						if (GC.getProfessionInfo(eLoopProfession).isWorkPlot())
						{
							for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
							{
								if (iI != CITY_HOME_PLOT)
								{
									CvPlot* pLoopPlot = getCityIndexPlot(iI);

									if (pLoopPlot != NULL)
									{
										if (!isUnitWorkingPlot(iI) || (getUnitWorkingPlot(iI) == pUnit))
										{
											if (canWork(pLoopPlot))
											{
												int iValue = AI_professionValue(eLoopProfession, pUnit, pLoopPlot, NULL);

												if (iValue > iBestValue)
												{
													eBestProfession = eLoopProfession;
													iBestValue = iValue;
													iBestPlot = iI;
												}
											}
										}
									}
								}
							}
						}
						else
						{
							int iValue = AI_professionValue(eLoopProfession, pUnit, NULL, NULL);
							if (iValue > iBestValue)
							{
								eBestProfession = eLoopProfession;
								iBestValue = iValue;
								iBestPlot = -1;
							}
						}
					}
				}
			}
		}

		pUnit->setProfession(eBestProfession);
		eCurrentProfession = pUnit->getProfession();
		if(eCurrentProfession == NO_PROFESSION)
		{
			//FAssertMsg(false, "Could not assign citizen any profession.");
			return false;
		}

		if (iBestPlot != -1)
		{
			FAssert(GC.getProfessionInfo(eCurrentProfession).isWorkPlot());
			if (getUnitWorkingPlot(iBestPlot) != pUnit)
			{
				setUnitWorkingPlot(iBestPlot, iUnitId);
			}
		}
		return true;
	}

	//already assigned to valid indoor profession
	if(!GC.getProfessionInfo(eCurrentProfession).isWorkPlot())
	{
		return true;
	}

	int iBestPlot = AI_bestProfessionPlot(eCurrentProfession, pUnit);

	if (iBestPlot != -1)
	{
		if (getUnitWorkingPlot(iBestPlot) != pUnit)
		{
			setUnitWorkingPlot(iBestPlot, iUnitId);
		}
		return true;
	}

	return false;
}


// Returns true if a worker was removed from a plot...
bool CvCityAI::AI_removeWorstCitizen()
{
	CvPlot* pLoopPlot;
	bool bAvoidGrowth;
	bool bIgnoreGrowth;
	int iWorstPlot;
	int iValue;
	int iWorstValue;
	int iI;

	bAvoidGrowth = AI_avoidGrowth();
	bIgnoreGrowth = AI_ignoreGrowth();

	iWorstValue = MAX_INT;
	iWorstPlot = -1;

	// check all the plots we working
	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			if (isUnitWorkingPlot(iI))
			{
				pLoopPlot = getCityIndexPlot(iI);

				if (pLoopPlot != NULL)
				{
					CvUnit* pUnit = getUnitWorkingPlot(iI);
					if (pUnit == NULL)
					{
						FAssert(false);
					}
					iValue = AI_professionValue(pUnit->getProfession(), pUnit, pLoopPlot, NULL);

					if (iValue < iWorstValue)
					{
						iWorstValue = iValue;
						iWorstPlot = iI;
					}
				}
			}
		}
	}

	if (iWorstPlot != -1)
	{
		clearUnitWorkingPlot(iWorstPlot);
		return true;
	}

	return false;
}

bool CvCityAI::AI_removeWorstPopulationUnit(bool bDelete)
{
	for (int i = (int) m_aPopulationUnits.size() - 1; i >= 0; --i)
	{
		ProfessionTypes eEjectProfession = (ProfessionTypes) GC.getCivilizationInfo(getCivilizationType()).getDefaultProfession();
		if (m_aPopulationUnits[i]->canHaveProfession(eEjectProfession, false))
		{
			if (removePopulationUnit(m_aPopulationUnits[i], bDelete, eEjectProfession))
			{
				return true;
			}
		}
	}

	return false;
}

CvUnit* CvCityAI::AI_bestPopulationUnit(UnitAITypes eUnitAI, ProfessionTypes eProfession)
{
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	if (eProfession == NO_PROFESSION)
	{
		eProfession = kOwner.AI_idealProfessionForUnitAIType(eUnitAI, this);
	}

	//Kaszkaj fix: .\.\CvCityAI.cpp, Line:  2466, Expression:  eProfession != NO_PROFESSION.
	// Return NULL when no valid profession exists so the caller can skip ejecting a resident.
	if (eProfession == NO_PROFESSION)
	{
		return NULL;
	}

	FAssert(!GC.getProfessionInfo(eProfession).isCitizen());

	int iBestValue = 0;
	CvUnit* pBestUnit = NULL;
	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pLoopUnit = getPopulationUnitByIndex(i);
		if (!kOwner.AI_isNativeHumanSpecialist(pLoopUnit->getUnitType())
			&& !kOwner.AI_isNativeStudent(pLoopUnit->getUnitType())
			&& pLoopUnit->canHaveProfession(eProfession, false))
		{
			int iValue = kOwner.AI_professionSuitability(pLoopUnit, eProfession, plot(), eUnitAI);

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				pBestUnit = pLoopUnit;
			}
		}
	}
	if (pBestUnit != NULL)
	{
		removePopulationUnit(pBestUnit, false, eProfession);
		pBestUnit->AI_setUnitAIType(eUnitAI);
	}

	return pBestUnit;
}

void CvCityAI::AI_juggleCitizens()
{
	return; //do not juggle citizens...

	bool bAvoidGrowth = AI_avoidGrowth();
	bool bIgnoreGrowth = AI_ignoreGrowth();

	// one at a time, remove the worst citizen, then add the best citizen
	// until we add back the same one we removed
	for (int iPass = 0; iPass < 2; iPass++)
	{
		bool bCompletedChecks = false;
		int iCount = 0;

		std::vector<int> aWorstPlots;

		while (!bCompletedChecks)
		{
			int iLowestValue = MAX_INT;
			int iWorstPlot = -1;
			int iValue;

			for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
			{
				if (iI != CITY_HOME_PLOT)
				{
					if (isUnitWorkingPlot(iI))
					{
						CvPlot* pLoopPlot = getCityIndexPlot(iI);

						if (pLoopPlot != NULL)
						{
								iValue = AI_plotValue(pLoopPlot, bAvoidGrowth, /*bRemove*/ true, /*bIgnoreFood*/ false, bIgnoreGrowth, (iPass == 0));

								// use <= so that we pick the last one that is lowest, to avoid infinite loop with AI_addBestCitizen
								if (iValue <= iLowestValue)
								{
									iLowestValue = iValue;
									iWorstPlot = iI;
								}
							}
						}
					}
				}

			// if no worst plot, or we looped back around and are trying to remove the first plot we removed, stop
			if (iWorstPlot == -1 || std::find(aWorstPlots.begin(), aWorstPlots.end(), iWorstPlot) != aWorstPlots.end())
			{
				bCompletedChecks = true;
			}
			else
			{
				// if this the first worst plot, remember it
				aWorstPlots.push_back(iWorstPlot);

				clearUnitWorkingPlot(iWorstPlot);

				if (AI_addBestCitizen())
				{
					if (isUnitWorkingPlot(iWorstPlot))
					{
						bCompletedChecks = true;
					}
				}
			}

			iCount++;
			if (iCount > (NUM_CITY_PLOTS + 1))
			{
				FAssertMsg(false, "infinite loop");
				break; // XXX
			}
		}

		if ((iPass == 0) && (foodDifference() >= 0))
		{
			//good enough, the starvation code
			break;
		}
	}
}

//Returns the displaced unit, if any.
CvUnit* CvCityAI::AI_assignToBestJob(CvUnit* pUnit, bool bIndoorOnly)
{
	int iBestValue = 0;
	int iBestPlot = -1;
	ProfessionTypes eBestProfession = NO_PROFESSION;
	if (cityBuildingValueCacheSize() == 0) return AI_assignToBestJobUncached(pUnit, bIndoorOnly);
	CvCityAIProfessionValueCache kYieldCache;
	CvCityAIProfessionValueCache* pYieldCache = GC.getUSE_UNIT_CANNOT_MOVE_INTO_CALLBACK()
		|| GC.getUSE_CAN_DECLARE_WAR_CALLBACK() ? NULL : &kYieldCache;
	int aiIncumbentValues[NUM_CITY_PLOTS];
	bool abIncumbentValues[NUM_CITY_PLOTS] = {false};

	//Kaszkaj - Let suitable specialists replace workers in occupied jobs.
	for (int i = 0; i < GC.getNumProfessionInfos(); ++i)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)i;
		CvProfessionInfo& kProfession = GC.getProfessionInfo(eLoopProfession);
		if (!GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession)
			|| !kProfession.isCitizen() || !pUnit->canHaveProfession(eLoopProfession, true))
		{
			continue;
		}

		if (kProfession.isWorkPlot())
		{
			if (bIndoorOnly)
			{
				continue;
			}
			for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; ++iPlot)
			{
				CvPlot* pLoopPlot = getCityIndexPlot(iPlot);
				if (iPlot == CITY_HOME_PLOT || pLoopPlot == NULL || !canWork(pLoopPlot))
				{
					continue;
				}
				CvUnit* pWorkingUnit = NULL;
				if (pLoopPlot->isBeingWorked())
				{
					pWorkingUnit = getUnitWorkingPlot(iPlot);
					if (pWorkingUnit == NULL || pWorkingUnit == pUnit || pWorkingUnit->isColonistLocked())
					{
						continue;
					}
				}
				int iValue = AI_professionValueWithCache(eLoopProfession, pUnit, pLoopPlot, pWorkingUnit, pYieldCache);
				int iCandidateValue = 0;
				int iIncumbentValue = 0;
				if (pWorkingUnit != NULL)
				{
					iCandidateValue = AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot);
					if (!abIncumbentValues[iPlot])
					{
						aiIncumbentValues[iPlot] = AI_jobReplacementValue(pWorkingUnit->getProfession(), pWorkingUnit, pLoopPlot);
						abIncumbentValues[iPlot] = true;
					}
					iIncumbentValue = aiIncumbentValues[iPlot];
					if (iCandidateValue <= iIncumbentValue) continue;
				}
				//Kaszkaj - Place human experts, including Expert Farmers and Expert Astronauts, where the real production gain is greatest.
				if (iValue > 0 && GET_PLAYER(getOwnerINLINE()).AI_isNativeHumanSpecialist(pUnit->getUnitType())
					&& GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pUnit->getUnitType(), eLoopProfession))
				{
					if (pWorkingUnit == NULL) iCandidateValue = AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot);
					int iGain = iCandidateValue - iIncumbentValue;
					iValue = isNative() && !isHuman()
						? (iValue / 500) * 500 + std::min(499, std::max(1, iGain / 100))
						: std::max(1, iGain);
				}
				if (iValue > iBestValue)
				{
					eBestProfession = eLoopProfession;
					iBestValue = iValue;
					iBestPlot = iPlot;
				}
			}
		}
		else
		{
			CvUnit* pWorstUnit = NULL;
			if (!pUnit->canHaveProfession(eLoopProfession, false))
			{
				pWorstUnit = AI_getWorstProfessionUnit(eLoopProfession);
				if (pWorstUnit == NULL || pWorstUnit == pUnit || pWorstUnit->isColonistLocked())
				{
					continue;
				}
			}
			int iValue = AI_professionValueWithCache(eLoopProfession, pUnit, NULL, pWorstUnit, pYieldCache);
			int iCandidateValue = 0;
			int iIncumbentValue = 0;
			if (pWorstUnit != NULL)
			{
				iCandidateValue = AI_jobReplacementValue(eLoopProfession, pUnit, NULL);
				iIncumbentValue = AI_jobReplacementValue(eLoopProfession, pWorstUnit, NULL);
				if (iCandidateValue <= iIncumbentValue) continue;
			}
			//Kaszkaj - Let human experts replace occupied Alien specialist jobs when they produce more.
			if (iValue > 0 && GET_PLAYER(getOwnerINLINE()).AI_isNativeHumanSpecialist(pUnit->getUnitType())
				&& GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pUnit->getUnitType(), eLoopProfession))
			{
				if (pWorstUnit == NULL) iCandidateValue = AI_jobReplacementValue(eLoopProfession, pUnit, NULL);
				int iGain = iCandidateValue - iIncumbentValue;
				iValue = isNative() && !isHuman()
					? (iValue / 500) * 500 + std::min(499, std::max(1, iGain / 100))
					: std::max(1, iGain);
			}
			if (iValue > iBestValue)
			{
				eBestProfession = eLoopProfession;
				iBestValue = iValue;
				iBestPlot = -1;
			}
		}
	}

	if (eBestProfession == NO_PROFESSION)
	{
		if (getPopulation() > 1)
		{
			//Kaszkaj fix: .\.\CvCityAI.cpp, Line:  2922, Expression:  bSuccess.
			// Keep an unassigned resident in the city until its default map profession is available.
			ProfessionTypes eEjectProfession = (ProfessionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultProfession();
			if (pUnit->canHaveProfession(eEjectProfession, false))
			{
				bool bSuccess = removePopulationUnit(pUnit, false, eEjectProfession);
				FAssertMsg(bSuccess, "Failed to remove useless citizen");
			}
		}
		return NULL;
	}

	CvUnit* pDisplacedUnit = NULL;
	if (!GC.getProfessionInfo(eBestProfession).isWorkPlot())
	{
		if (!pUnit->canHaveProfession(eBestProfession, false))
		{
			pDisplacedUnit = AI_getWorstProfessionUnit(eBestProfession);
			FAssert(pDisplacedUnit != NULL);
			//Kaszkaj - Verify the same fixed production score used when selecting the replacement.
			FAssert(AI_jobReplacementValue(eBestProfession, pUnit, NULL) > AI_jobReplacementValue(pDisplacedUnit->getProfession(), pDisplacedUnit, NULL));
		}
	}
	else
	{
		FAssert(iBestPlot != -1);
		if (isUnitWorkingPlot(iBestPlot))
		{
			pDisplacedUnit = getUnitWorkingPlot(iBestPlot);
			FAssert(pDisplacedUnit != NULL);
			FAssert(AI_jobReplacementValue(eBestProfession, pUnit, getCityIndexPlot(iBestPlot)) > AI_jobReplacementValue(pDisplacedUnit->getProfession(), pDisplacedUnit, getCityIndexPlot(iBestPlot)));
			clearUnitWorkingPlot(iBestPlot);
		}
	}

	if (pDisplacedUnit != NULL)
	{
		pDisplacedUnit->setProfession(NO_PROFESSION);
	}
	pUnit->setProfession(eBestProfession);

	FAssert(!isUnitWorkingAnyPlot(pUnit));
	if (iBestPlot != -1)
	{
		setUnitWorkingPlot(iBestPlot, pUnit->getID());
	}
	FAssert(iBestPlot != -1 || !GC.getProfessionInfo(eBestProfession).isWorkPlot());
	FAssert(iBestPlot == -1 || isUnitWorkingAnyPlot(pUnit));
	return pDisplacedUnit;
}

// Keep the original evaluation sequence when external rules may change state.
CvUnit* CvCityAI::AI_assignToBestJobUncached(CvUnit* pUnit, bool bIndoorOnly)
{
	int iBestValue = 0;
	int iBestPlot = -1;
	ProfessionTypes eBestProfession = NO_PROFESSION;

	for (int i = 0; i < GC.getNumProfessionInfos(); ++i)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)i;
		CvProfessionInfo& kProfession = GC.getProfessionInfo(eLoopProfession);
		if (!GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession)
			|| !kProfession.isCitizen() || !pUnit->canHaveProfession(eLoopProfession, true))
		{
			continue;
		}

		if (kProfession.isWorkPlot())
		{
			if (bIndoorOnly)
			{
				continue;
			}
			for (int iPlot = 0; iPlot < NUM_CITY_PLOTS; ++iPlot)
			{
				CvPlot* pLoopPlot = getCityIndexPlot(iPlot);
				if (iPlot == CITY_HOME_PLOT || pLoopPlot == NULL || !canWork(pLoopPlot))
				{
					continue;
				}
				CvUnit* pWorkingUnit = NULL;
				if (pLoopPlot->isBeingWorked())
				{
					pWorkingUnit = getUnitWorkingPlot(iPlot);
					if (pWorkingUnit == NULL || pWorkingUnit == pUnit || pWorkingUnit->isColonistLocked())
					{
						continue;
					}
				}
				int iValue = AI_professionValue(eLoopProfession, pUnit, pLoopPlot, pWorkingUnit);
				if (pWorkingUnit != NULL
					&& AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot) <= AI_jobReplacementValue(pWorkingUnit->getProfession(), pWorkingUnit, pLoopPlot))
				{
					continue;
				}
				if (iValue > 0 && GET_PLAYER(getOwnerINLINE()).AI_isNativeHumanSpecialist(pUnit->getUnitType())
					&& GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pUnit->getUnitType(), eLoopProfession))
				{
					int iGain = AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot)
						- (pWorkingUnit == NULL ? 0 : AI_jobReplacementValue(pWorkingUnit->getProfession(), pWorkingUnit, pLoopPlot));
					iValue = isNative() && !isHuman()
						? (iValue / 500) * 500 + std::min(499, std::max(1, iGain / 100))
						: std::max(1, iGain);
				}
				if (iValue > iBestValue)
				{
					eBestProfession = eLoopProfession;
					iBestValue = iValue;
					iBestPlot = iPlot;
				}
			}
		}
		else
		{
			CvUnit* pWorstUnit = NULL;
			if (!pUnit->canHaveProfession(eLoopProfession, false))
			{
				pWorstUnit = AI_getWorstProfessionUnit(eLoopProfession);
				if (pWorstUnit == NULL || pWorstUnit == pUnit || pWorstUnit->isColonistLocked())
				{
					continue;
				}
			}
			int iValue = AI_professionValue(eLoopProfession, pUnit, NULL, pWorstUnit);
			if (pWorstUnit != NULL && AI_jobReplacementValue(eLoopProfession, pUnit, NULL) <= AI_jobReplacementValue(eLoopProfession, pWorstUnit, NULL))
			{
				continue;
			}
			if (iValue > 0 && GET_PLAYER(getOwnerINLINE()).AI_isNativeHumanSpecialist(pUnit->getUnitType())
				&& GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pUnit->getUnitType(), eLoopProfession))
			{
				int iGain = AI_jobReplacementValue(eLoopProfession, pUnit, NULL)
					- (pWorstUnit == NULL ? 0 : AI_jobReplacementValue(eLoopProfession, pWorstUnit, NULL));
				iValue = isNative() && !isHuman()
					? (iValue / 500) * 500 + std::min(499, std::max(1, iGain / 100))
					: std::max(1, iGain);
			}
			if (iValue > iBestValue)
			{
				eBestProfession = eLoopProfession;
				iBestValue = iValue;
				iBestPlot = -1;
			}
		}
	}

	if (eBestProfession == NO_PROFESSION)
	{
		if (getPopulation() > 1)
		{
			ProfessionTypes eEjectProfession = (ProfessionTypes)GC.getCivilizationInfo(getCivilizationType()).getDefaultProfession();
			if (pUnit->canHaveProfession(eEjectProfession, false))
			{
				bool bSuccess = removePopulationUnit(pUnit, false, eEjectProfession);
				FAssertMsg(bSuccess, "Failed to remove useless citizen");
			}
		}
		return NULL;
	}

	CvUnit* pDisplacedUnit = NULL;
	if (!GC.getProfessionInfo(eBestProfession).isWorkPlot())
	{
		if (!pUnit->canHaveProfession(eBestProfession, false))
		{
			pDisplacedUnit = AI_getWorstProfessionUnit(eBestProfession);
			FAssert(pDisplacedUnit != NULL);
			FAssert(AI_jobReplacementValue(eBestProfession, pUnit, NULL) > AI_jobReplacementValue(pDisplacedUnit->getProfession(), pDisplacedUnit, NULL));
		}
	}
	else
	{
		FAssert(iBestPlot != -1);
		if (isUnitWorkingPlot(iBestPlot))
		{
			pDisplacedUnit = getUnitWorkingPlot(iBestPlot);
			FAssert(pDisplacedUnit != NULL);
			FAssert(AI_jobReplacementValue(eBestProfession, pUnit, getCityIndexPlot(iBestPlot)) > AI_jobReplacementValue(pDisplacedUnit->getProfession(), pDisplacedUnit, getCityIndexPlot(iBestPlot)));
			clearUnitWorkingPlot(iBestPlot);
		}
	}

	if (pDisplacedUnit != NULL)
	{
		pDisplacedUnit->setProfession(NO_PROFESSION);
	}
	pUnit->setProfession(eBestProfession);

	FAssert(!isUnitWorkingAnyPlot(pUnit));
	if (iBestPlot != -1)
	{
		setUnitWorkingPlot(iBestPlot, pUnit->getID());
	}
	FAssert(iBestPlot != -1 || !GC.getProfessionInfo(eBestProfession).isWorkPlot());
	FAssert(iBestPlot == -1 || isUnitWorkingAnyPlot(pUnit));
	return pDisplacedUnit;
}

//iValueA1 - Value of passed unit with original profession.
//iValueA2 - Value of passed unit with loop profession.
//iValueB1  - Value of loop unit with original profession.
//iValueB2 - Value of loop unit with loop profession.

CvUnit* CvCityAI::AI_juggleColonist(CvUnit* pUnit)
{
	if (cityBuildingValueCacheSize() == 0) return AI_juggleColonistUncached(pUnit);
	CvCityAIProfessionValueCache kYieldCache;
	CvCityAIProfessionValueCache* pYieldCache = GC.getUSE_UNIT_CANNOT_MOVE_INTO_CALLBACK()
		|| GC.getUSE_CAN_DECLARE_WAR_CALLBACK() ? NULL : &kYieldCache;
	ProfessionTypes eProfession = pUnit->getProfession();
	CvPlot* pPlot = getPlotWorkedByUnit(pUnit);

	CvUnit* pBestUnit = NULL;
	int iBestValue = 0;
	int iCurrentValue = MIN_INT;

	AI_setWorkforceHack(true);
	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pLoopUnit = m_aPopulationUnits[i];
		if ((pLoopUnit != NULL) && (pUnit != pLoopUnit))
		{
			if (!pLoopUnit->isColonistLocked())
			{
				CvPlot* pLoopPlot = getPlotWorkedByUnit(pLoopUnit);
				ProfessionTypes eLoopProfession = pLoopUnit->getProfession();

				if (pLoopUnit->canHaveProfession(eProfession, true, pPlot) && pUnit->canHaveProfession(eLoopProfession, true, pLoopPlot))
				{
					//Kaszkaj - Only make legal, useful swaps which improve the same workforce score; equal swaps cannot cycle.
					if (AI_professionValueWithCache(eLoopProfession, pUnit, pLoopPlot, pLoopUnit, pYieldCache) <= 0
						|| AI_professionValueWithCache(eProfession, pLoopUnit, pPlot, pUnit, pYieldCache) <= 0) continue;
					//Kaszkaj - Swaps keep Alien job priorities unless a better human expert takes that job or the old job is no longer useful.
					CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
					int iCachedA2 = MIN_INT;
					int iCachedB1 = MIN_INT;
					int iCachedB2 = MIN_INT;
					if (isNative() && !isHuman())
					{
						bool bBetterExpertA = !kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())
							&& std::strcmp(pUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eLoopProfession)
							&& (iCachedA2 = AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot))
							> (iCachedB1 = AI_jobReplacementValue(eLoopProfession, pLoopUnit, pLoopPlot));
						bool bBetterExpertB = !kOwner.AI_isNativeCitySpecialist(pLoopUnit->getUnitType())
							&& std::strcmp(pLoopUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& kOwner.AI_isProfessionExpert(pLoopUnit->getUnitType(), eProfession)
							&& (iCachedB2 = AI_jobReplacementValue(eProfession, pLoopUnit, pPlot))
							> (iCurrentValue == MIN_INT ? (iCurrentValue = AI_jobReplacementValue(eProfession, pUnit, pPlot)) : iCurrentValue);
						//Kaszkaj - Ordinary Alien swaps follow net production gain instead of the former fixed profession tiers.
						if (std::strcmp(pUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& !bBetterExpertB && kOwner.AI_nativeProfessionPriority(pUnit->getUnitType(), eLoopProfession, pLoopPlot, this)
							< kOwner.AI_nativeProfessionPriority(pUnit->getUnitType(), eProfession, pPlot, this)
							&& AI_professionValueWithCache(eProfession, pUnit, pPlot, NULL, pYieldCache) > 0) continue;
						if (std::strcmp(pLoopUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& !bBetterExpertA && kOwner.AI_nativeProfessionPriority(pLoopUnit->getUnitType(), eProfession, pPlot, this)
							< kOwner.AI_nativeProfessionPriority(pLoopUnit->getUnitType(), eLoopProfession, pLoopPlot, this)
							&& AI_professionValueWithCache(eLoopProfession, pLoopUnit, pLoopPlot, NULL, pYieldCache) > 0) continue;
					}
					if (iCurrentValue == MIN_INT) iCurrentValue = AI_jobReplacementValue(eProfession, pUnit, pPlot);
					int iValueA1 = iCurrentValue;
					int iValueB1 = iCachedB1 == MIN_INT ? AI_jobReplacementValue(eLoopProfession, pLoopUnit, pLoopPlot) : iCachedB1;
					int iValueA2 = iCachedA2 == MIN_INT ? AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot) : iCachedA2;
					int iValueB2 = iCachedB2 == MIN_INT ? AI_jobReplacementValue(eProfession, pLoopUnit, pPlot) : iCachedB2;

					//if ((iValueA2 > iValueA1 && iValueB2 >= iValueB1) || (iValueA2 >= iValueA1 && iValueB2 > iValueB1))
					{
						int iValue = (iValueA2 - iValueA1) + (iValueB2 - iValueB1);
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}
	AI_setWorkforceHack(false);

	if (pBestUnit != NULL)
	{
		AI_swapUnits(pUnit, pBestUnit);
	}

	return pBestUnit;

}

CvUnit* CvCityAI::AI_juggleColonistUncached(CvUnit* pUnit)
{
	ProfessionTypes eProfession = pUnit->getProfession();
	CvPlot* pPlot = getPlotWorkedByUnit(pUnit);

	CvUnit* pBestUnit = NULL;
	int iBestValue = 0;

	AI_setWorkforceHack(true);
	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pLoopUnit = m_aPopulationUnits[i];
		if ((pLoopUnit != NULL) && (pUnit != pLoopUnit))
		{
			if (!pLoopUnit->isColonistLocked())
			{
				CvPlot* pLoopPlot = getPlotWorkedByUnit(pLoopUnit);
				ProfessionTypes eLoopProfession = pLoopUnit->getProfession();

				if (pLoopUnit->canHaveProfession(eProfession, true, pPlot) && pUnit->canHaveProfession(eLoopProfession, true, pLoopPlot))
				{
					
					if (AI_professionValue(eLoopProfession, pUnit, pLoopPlot, pLoopUnit) <= 0
						|| AI_professionValue(eProfession, pLoopUnit, pPlot, pUnit) <= 0) continue;
					
					CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
					if (isNative() && !isHuman())
					{
						bool bBetterExpertA = !kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())
							&& std::strcmp(pUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eLoopProfession)
							&& AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot) > AI_jobReplacementValue(eLoopProfession, pLoopUnit, pLoopPlot);
						bool bBetterExpertB = !kOwner.AI_isNativeCitySpecialist(pLoopUnit->getUnitType())
							&& std::strcmp(pLoopUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& kOwner.AI_isProfessionExpert(pLoopUnit->getUnitType(), eProfession)
							&& AI_jobReplacementValue(eProfession, pLoopUnit, pPlot) > AI_jobReplacementValue(eProfession, pUnit, pPlot);
						
						if (std::strcmp(pUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& !bBetterExpertB && kOwner.AI_nativeProfessionPriority(pUnit->getUnitType(), eLoopProfession, pLoopPlot, this)
							< kOwner.AI_nativeProfessionPriority(pUnit->getUnitType(), eProfession, pPlot, this)
							&& AI_professionValue(eProfession, pUnit, pPlot, NULL) > 0) continue;
						if (std::strcmp(pLoopUnit->getUnitInfo().getType(), "UNIT_NATIVE") != 0
							&& !bBetterExpertA && kOwner.AI_nativeProfessionPriority(pLoopUnit->getUnitType(), eProfession, pPlot, this)
							< kOwner.AI_nativeProfessionPriority(pLoopUnit->getUnitType(), eLoopProfession, pLoopPlot, this)
							&& AI_professionValue(eLoopProfession, pLoopUnit, pLoopPlot, NULL) > 0) continue;
					}
					int iValueA1 = AI_jobReplacementValue(eProfession, pUnit, pPlot);
					int iValueB1 = AI_jobReplacementValue(eLoopProfession, pLoopUnit, pLoopPlot);
					int iValueA2 = AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot);
					int iValueB2 = AI_jobReplacementValue(eProfession, pLoopUnit, pPlot);

					
					{
						int iValue = (iValueA2 - iValueA1) + (iValueB2 - iValueB1);
						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							pBestUnit = pLoopUnit;
						}
					}
				}
			}
		}
	}
	AI_setWorkforceHack(false);

	if (pBestUnit != NULL)
	{
		AI_swapUnits(pUnit, pBestUnit);
	}

	return pBestUnit;

}

void CvCityAI::AI_swapUnits(CvUnit* pUnitA, CvUnit* pUnitB)
{
	ProfessionTypes eProfessionA = pUnitA->getProfession();
	CvPlot* pPlotA = getPlotWorkedByUnit(pUnitA);

	ProfessionTypes eProfessionB = pUnitB->getProfession();
	CvPlot* pPlotB = getPlotWorkedByUnit(pUnitB);

	//remove from plot
	if (pPlotA != NULL)
	{
		clearUnitWorkingPlot(pPlotA);
	}
	if (pPlotB != NULL)
	{
		clearUnitWorkingPlot(pPlotB);
	}

	//remove from building

	pUnitA->setProfession(NO_PROFESSION);
	pUnitB->setProfession(NO_PROFESSION);

	pUnitA->setProfession(eProfessionB);
	if (pPlotB != NULL)
	{
		setUnitWorkingPlot(pPlotB, pUnitA->getID());
	}

	pUnitB->setProfession(eProfessionA);
	if (pPlotA != NULL)
	{
		setUnitWorkingPlot(pPlotA, pUnitB->getID());
	}
}



//Kaszkaj - Give useful input-free Research and advanced goods from Bonuses or Improvements a proportional job preference.
int CvCityAI::AI_directPlotYieldBonusPercent(ProfessionTypes eProfession, const CvPlot* pPlot) const
{
	if (isHuman() || eProfession == NO_PROFESSION || pPlot == NULL) return 0;
	const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
	if (!kProfession.isCitizen() || !kProfession.isWorkPlot() || kProfession.isWater() != pPlot->isWater()) return 0;
	YieldTypes eYield = (YieldTypes)kProfession.getYieldsProduced(0);
	switch (eYield)
	{
		case YIELD_IDEAS: case YIELD_HAMMERS: case YIELD_TOOLS: case YIELD_MUSKETS: case YIELD_HORSES:
		case YIELD_COATS: case YIELD_CLOTH: case YIELD_RUM: case YIELD_CIGARS: case YIELD_TRADE_GOODS:
			break;
		default: return 0;
	}
	for (int i = 0; i < kProfession.getNumYieldsConsumed(getOwnerINLINE()); ++i)
		if (kProfession.getYieldsConsumed(i, getOwnerINLINE()) != NO_YIELD) return 0;
	bool bDirectYield = pPlot->getBonusType() != NO_BONUS && GC.getBonusInfo(pPlot->getBonusType()).getYieldChange(eYield) > 0;
	if (!bDirectYield && pPlot->getImprovementType() != NO_IMPROVEMENT)
		bDirectYield = pPlot->calculateImprovementYieldChange(pPlot->getImprovementType(), eYield, getOwnerINLINE(), false) > 0;
	return bDirectYield ? (eYield == YIELD_IDEAS ? 25 : 15) : 0;
}

int CvCityAI::AI_professionValue(ProfessionTypes eProfession, const CvUnit* pUnit, const CvPlot* pPlot, const CvUnit* pDisplaceUnit) const
{
	return AI_professionValueWithCache(eProfession, pUnit, pPlot, pDisplaceUnit, NULL);
}

int CvCityAI::AI_professionValueWithCache(ProfessionTypes eProfession, const CvUnit* pUnit, const CvPlot* pPlot, const CvUnit* pDisplaceUnit, CvCityAIProfessionValueCache* pCache) const
{
	if (eProfession == NO_PROFESSION)
	{
		return 0;
	}

	CvProfessionInfo& kProfessionInfo = GC.getProfessionInfo(eProfession);
	int iIncome = 0;
	int iTarget = 0;
	int iYieldOutput = 0;
	int iYieldInput = 0;
	int iExtraYieldOutput = 0;
	int iProductionPercent = 100;
	bool bFirstRevolutionBellsWorker = false;

	YieldTypes eYieldProducedType = NO_YIELD;
	YieldTypes eYieldConsumedType = NO_YIELD;

	FAssert(pUnit != NULL);

	if (!pUnit->isOnMap())
	{
		if (!pUnit->canHaveProfession(eProfession, pDisplaceUnit != NULL, pPlot))
		{
			return 0;
		}
	}
	// MultipleYieldsProduced Start by Aymerick 22/01/2010**
	if (kProfessionInfo.getYieldsProduced(0) == YIELD_EDUCATION)
	{
		if (pUnit->getUnitInfo().getStudentWeight() <= 0)
		{
			return 0;
		}
	}
	// MultipleYieldsProduced End
	if (!GC.getProfessionInfo(eProfession).isCitizen())
	{
		return 0;
	}

	CvUnitInfo& kUnit = GC.getUnitInfo(pUnit->getUnitType());
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	eYieldProducedType = (YieldTypes)kProfessionInfo.getYieldsProduced(0);
	int iNativeFoodAvailable = 0;
	int iNativeFoodNeeded = 0;
	if (isNative() && !isHuman())
	{
		int iFoodBaseAvailable = (pCache == NULL ? getBaseRawYieldProduced(YIELD_FOOD) : pCache->baseProduced(*this, YIELD_FOOD));
		iNativeFoodNeeded = (pCache == NULL ? getRawYieldConsumed(YIELD_FOOD) : pCache->consumed(*this, YIELD_FOOD))
			+ (pUnit->isOnMap() ? GC.getFOOD_CONSUMPTION_PER_POPULATION() : 0);
		const CvUnit* apWorkers[2] = {pUnit, pDisplaceUnit};
		for (int i = 0; i < 2; ++i)
		{
			if (apWorkers[i] == NULL || (i == 1 && apWorkers[i] == pUnit)) continue;
			CvPlot* pWorked = getPlotWorkedByUnit(apWorkers[i]);
			if (pWorked != NULL) iFoodBaseAvailable -= pWorked->getYield(YIELD_FOOD);
		}
		iNativeFoodAvailable = iFoodBaseAvailable * (pCache == NULL ? getBaseYieldRateModifier(YIELD_FOOD) : pCache->modifier(*this, YIELD_FOOD)) / 100;
	}
	//Kaszkaj - Alien AI prioritises Convict students, but training time uses their real XML penalty.
	if (isNative() && !isHuman() && std::strcmp(kUnit.getType(), "UNIT_CRIMINAL") == 0
		&& eYieldProducedType == YIELD_EDUCATION
		&& kProfessionInfo.getSpecialBuilding() == GC.getInfoTypeForString("SPECIALBUILDING_EDUCATION", true))
	{
		//Kaszkaj - Keep the last resident working until the Colony can spare a student.
		if ((getPopulation() <= 1 && !pUnit->isOnMap()) || iNativeFoodAvailable < iNativeFoodNeeded)
		{
			return 0;
		}
		int iTurns = getEducationTurnsLeft(pUnit, eProfession);
		if (iTurns == MAX_INT)
		{
			return 0;
		}
		int iBestExpertValue = 0;
		for (int i = 0; i < GC.getNumUnitInfos(); ++i)
		{
			if (getSpecialistTuition((UnitTypes)i) == 0)
			{
				iBestExpertValue = std::max(iBestExpertValue, kOwner.AI_educationUnitValue((UnitTypes)i));
			}
		}
		int iPriority = 100 + std::max(0, kOwner.AI_getUnitYieldModifier(pUnit->getUnitType(), YIELD_EDUCATION))
			+ 22 * std::max(0, kOwner.AI_getUnitYieldChange(pUnit->getUnitType(), YIELD_EDUCATION));
		int iPlanningTurns = std::max(1, GC.getDefineINT("EDUCATION_THRESHOLD")
			* GC.getGameSpeedInfo(GC.getGameINLINE().getGameSpeedType()).getGrowthPercent() / 100);
		return iBestExpertValue * iPriority * iPlanningTurns / std::max(1, iTurns);
	}
	if (!isHuman() && eYieldProducedType == YIELD_IDEAS && (!kOwner.AI_hasResearchTarget() || canResearch() <= 0))
	{
		return 0;
	}
	//Kaszkaj - Use one Colony for ordinary Aliens' indoor Research, and reconsider its jobs when that Colony changes.
	if (isNative() && !isHuman() && eYieldProducedType == YIELD_IDEAS && !kProfessionInfo.isWorkPlot()
		&& std::strcmp(kUnit.getType(), "UNIT_NATIVE") == 0)
	{
		if (this != (pCache == NULL ? kOwner.AI_nativeResearchCity() : pCache->researchCity(kOwner))) return 0;
		int iLoop;
		for (CvCity* pCity = kOwner.firstCity(&iLoop); pCity != NULL; pCity = kOwner.nextCity(&iLoop))
		{
			if (pCity == this) continue;
			for (int i = 0; i < pCity->getPopulation(); ++i)
			{
				const CvUnit* pWorker = pCity->getPopulationUnitByIndex(i);
				if (pWorker != NULL && std::strcmp(pWorker->getUnitInfo().getType(), "UNIT_NATIVE") == 0
					&& pWorker->getProfession() != NO_PROFESSION && !GC.getProfessionInfo(pWorker->getProfession()).isWorkPlot()
					&& GC.getProfessionInfo(pWorker->getProfession()).getYieldsProduced(0) == YIELD_IDEAS) return 0;
			}
		}
	}
	//Kaszkaj - Apply Alien specialist job priorities only to usable work; Research jobs also need an available research target.
	//Kaszkaj - Ordinary Aliens compare routine jobs by their net benefit; specialist role priorities remain in place.
	bool bOrdinaryNativeWorker = isNative() && !isHuman() && std::strcmp(kUnit.getType(), "UNIT_NATIVE") == 0;
	int iNativePriority = bOrdinaryNativeWorker ? 0 : kOwner.AI_nativeProfessionPriority(pUnit->getUnitType(), eProfession, pPlot, this);
	int iUnitModifier, iUnitChange, iUnitBonusChange;
	bool bOverrideYield = kOwner.AI_getUnitYieldBonuses(pUnit->getUnitType(), eYieldProducedType, iUnitModifier, iUnitChange, iUnitBonusChange);
	//Kaszkaj - Use real Alien output, including XML penalties, for job production and Food forecasts.
	if (std::strcmp(kUnit.getType(), "UNIT_NATIVE") == 0) bOverrideYield = false;

	if (GC.getProfessionInfo(eProfession).isWorkPlot())
	{
		if (pPlot == NULL)
		{
			return 0;
		}
		//Kaszkaj - AI-only worker bonuses cannot make a land profession productive on water, or a water profession on land.
		if (kProfessionInfo.isWater() != pPlot->isWater()) return 0;
		FAssert(canWork(pPlot));

		if (bOverrideYield && eYieldProducedType >= 0 && eYieldProducedType < NUM_YIELD_TYPES)
		{
			//Kaszkaj - Apply AI worker bonuses to the current plot, including its existing feature.
			iYieldOutput = pPlot->calculatePotentialYield(eYieldProducedType, getOwnerINLINE(), pPlot->getImprovementType(), false, pPlot->getRouteType(), NO_UNIT, false);
			if (iYieldOutput > 0 && pPlot->isValidYieldChanges(pUnit->getUnitType()))
			{
				iYieldOutput += iUnitChange;
				if (pPlot->getBonusType() != NO_BONUS && GC.getBonusInfo(pPlot->getBonusType()).getYieldChange(eYieldProducedType) > 0)
				{
					iYieldOutput += iUnitBonusChange;
				}
			}
			iYieldOutput = std::max(0, iYieldOutput * (100 + iUnitModifier) / 100);
		}
		else
		{
			iYieldOutput = pPlot->calculatePotentialProfessionYieldAmount(eProfession, pUnit, false);
		}
		// MultipleYieldsProduced Start by Aymerick 22/01/2010**
		eYieldProducedType = (YieldTypes) kProfessionInfo.getYieldsProduced(0);
		// MultipleYieldsProduced End
		if ((eYieldProducedType != NO_YIELD) && iUnitChange > 0)
		{
			int iYieldChange = iUnitChange;
			if (pPlot->getBonusType() != NO_BONUS && GC.getBonusInfo(pPlot->getBonusType()).getYieldChange(eYieldProducedType) > 0)
			{
				iYieldChange += iUnitBonusChange;

			}
			if (pPlot->isWater())
			{
				if (kProfessionInfo.isWater() && kUnit.isWaterYieldChanges())
				{
					iExtraYieldOutput += iYieldChange;
				}
			}
			else
			{
				if (!kProfessionInfo.isWater() && kUnit.isLandYieldChanges())
				{
					iExtraYieldOutput += iYieldChange;
				}
			}
		}
	}
	else
	{
		FAssertMsg(pPlot == NULL, "passing in a plot for an indoors profession? Why?");

		iYieldOutput = bOverrideYield ? AI_professionBasicOutput(eProfession, pUnit->getUnitType(), NULL)
			: getProfessionOutput(eProfession, pUnit);
		iYieldInput = getProfessionInput(eProfession, pUnit);
		eYieldConsumedType = (YieldTypes) kProfessionInfo.getYieldsConsumed(0, GET_PLAYER(getOwner()).getID());
		// MultipleYieldsProduced Start by Aymerick 22/01/2010**
		eYieldProducedType = (YieldTypes) kProfessionInfo.getYieldsProduced(0);
		// MultipleYieldsProduced End
		if (eYieldProducedType != NO_YIELD)
		{
			iExtraYieldOutput += iUnitChange;
		}
	}

	if (eYieldProducedType == NO_YIELD)
	{
		FAssert(iYieldOutput == 0);
		return 0;
	}

	//Kaszkaj - Let qualified specialists, including Industrious Cyborgs, take the Mechanicus profession.
	if (eYieldProducedType == YIELD_HORSES && !isNative()
		&& !GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pUnit->getUnitType(), eProfession))
	{
		return 0;
	}

	//Kaszkaj fix: .\.\CvCityAI.cpp, Line:  3429, Expression:  iYieldInput > 0.
	// Reject conversion jobs with no actual input before comparing workers with AI-only yield bonuses.
	if (iYieldOutput <= 0 || (!kProfessionInfo.isWorkPlot() && eYieldConsumedType != NO_YIELD && iYieldInput <= 0))
	{
		return 0;
	}

	iYieldOutput *= (pCache == NULL ? getBaseYieldRateModifier(eYieldProducedType) : pCache->modifier(*this, eYieldProducedType));
	iYieldOutput /= 100;

	int iNetYield = (pCache == NULL ? getBaseRawYieldProduced(eYieldProducedType) : pCache->baseProduced(*this, eYieldProducedType));

	CvUnit* pOldUnit = NULL;
	if (GC.getProfessionInfo(eProfession).isWorkPlot())
	{
		CvPlot* pWorkedPlot = getPlotWorkedByUnit(pUnit);
		if (pWorkedPlot != NULL)
		{
			iNetYield -= pWorkedPlot->getYield(eYieldProducedType);

			if ((kProfessionInfo.isWater() && kUnit.isWaterYieldChanges()) || !kProfessionInfo.isWater() && kUnit.isLandYieldChanges())
			{
				if (pWorkedPlot->getBonusType() != NO_BONUS && GC.getBonusInfo(pWorkedPlot->getBonusType()).getYieldChange(eYieldProducedType) > 0)
				{
					iExtraYieldOutput += iUnitBonusChange;
				}
			}
		}

		if (pPlot != NULL && pPlot->isBeingWorked() && pWorkedPlot != pPlot)
		{
			iNetYield -= pPlot->getYield(eYieldProducedType);
			pOldUnit = getUnitWorkingPlot(pPlot);
		}
	}
	else
	{
		ProfessionTypes eWorkedProfession = pUnit->getProfession();
		if (eWorkedProfession != NO_PROFESSION)
		{
			// MultipleYieldsProduced Start by Aymerick 22/01/2010**
			if (GC.getProfessionInfo(eWorkedProfession).getYieldsProduced(0) == eYieldProducedType)
			{
				iNetYield -= getProfessionOutput(eWorkedProfession, pUnit);
			}
			if (GC.getProfessionInfo(eWorkedProfession).getYieldsConsumed(0, GET_PLAYER(getOwner()).getID()) == eYieldProducedType)
			{
				iNetYield += getProfessionInput(eWorkedProfession, pUnit);
			}
			// MultipleYieldsProduced End
		}
	}

	iNetYield *= (pCache == NULL ? getBaseYieldRateModifier(eYieldProducedType) : pCache->modifier(*this, eYieldProducedType));
	iNetYield /= 100;

	iNetYield -= (pCache == NULL ? getRawYieldConsumed(eYieldProducedType) : pCache->consumed(*this, eYieldProducedType));

	//Kaszkaj - Allow Alien and human experts to use productive jobs even when Alien AI assigns zero value to the produced yield.
	int iOutputYieldValue = kOwner.AI_yieldValue(eYieldProducedType);
	bool bPreferredNativeWorker = iNativePriority > 0 || (isNative() && !isHuman() && kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession));
	if (bPreferredNativeWorker && iOutputYieldValue == 0)
	{
		iOutputYieldValue = std::max(1, GC.getYieldInfo(eYieldProducedType).getAIBaseValue());
	}
	int iOutputValue = 0;
	int iInputValue = 0;


	if (!kProfessionInfo.isWorkPlot() && (eYieldProducedType != YIELD_EDUCATION))
	{
		int iConsumedAlready = (eYieldConsumedType == NO_YIELD) ? 0 : (pCache == NULL ? getRawYieldConsumed(eYieldConsumedType) : pCache->consumed(*this, eYieldConsumedType));
		int iRealInputAvailable = (eYieldConsumedType == NO_YIELD) ? 0 : (pCache == NULL ? getRawYieldProduced(eYieldConsumedType) : pCache->produced(*this, eYieldConsumedType)) - iConsumedAlready;

		if (eYieldConsumedType != NO_YIELD)
		{
			bool bDontDiplace = false;
			if (pPlot != NULL)
			{
				if (pPlot->isBeingWorked())
				{
					CvUnit* pWorkingUnit = getUnitWorkingPlot(pPlot);
					if (pWorkingUnit != pUnit)
					{
						iRealInputAvailable -= pPlot->getYield(eYieldConsumedType);
						if (pWorkingUnit == pDisplaceUnit)
						{
							bDontDiplace = true;
						}
					}
				}
			}

			if (!bDontDiplace)
			{
				if (pDisplaceUnit != NULL)
				{
					if (pDisplaceUnit->getProfession() == eProfession)
					{
						//Kaszkaj - Count the displaced worker's inputs as available to its replacement.
						iRealInputAvailable += getProfessionInput(eProfession, pDisplaceUnit);
					}
				}
			}

			if (pUnit->getProfession() == eProfession)
			{
				iRealInputAvailable += getProfessionInput(eProfession, pUnit);
			}
		}

		int iEstimatedInputAvailable = 0;

		if (eYieldConsumedType != NO_YIELD)
		{
			iEstimatedInputAvailable += iRealInputAvailable + getYieldStored(eYieldConsumedType) / 10;

			int iImports = AI_getTradeBalance(eYieldConsumedType);
			if (iImports > 0)
			{
				iEstimatedInputAvailable += std::min(iImports, getYieldStored(eYieldConsumedType));
			}
		}

		if (eYieldConsumedType == NO_YIELD || ((iRealInputAvailable + getYieldStored(eYieldConsumedType)) > 0 && iEstimatedInputAvailable > 0))
		{
			if (eYieldConsumedType != NO_YIELD)
			{
				//Kaszkaj - Scale factory priorities to the inputs it can actually process.
				iProductionPercent = 100 * std::min(iYieldInput, iEstimatedInputAvailable) / std::max(1, iYieldInput);
			}
			//Kaszkaj - Check every expert role so a unit's equally good specialisations remain useful.
			CvUnit* pIdealAssignedUnit = NULL;
			CvUnit* pIdealUnassignedUnit = NULL;
			int iProfessionCount = 0;
			for (int i = 0; i < getPopulation(); ++i)
			{
				CvUnit* pLoopUnit = getPopulationUnitByIndex(i);
				if (pLoopUnit->getProfession() == eProfession)
				{
					iProfessionCount ++;
				}
				if (kOwner.AI_isProfessionExpert(pLoopUnit->getUnitType(), eProfession))
				{
					if (pLoopUnit->getProfession() == eProfession)
					{
						pIdealAssignedUnit = pLoopUnit;
					}
					else if (!pLoopUnit->isColonistLocked())
					{
						pIdealUnassignedUnit = pLoopUnit;
					}
				}
			}

			if ((pDisplaceUnit != NULL) && (pDisplaceUnit != pUnit))
			{
				if (pDisplaceUnit->getProfession() == eProfession)
				{
					iProfessionCount--;
				}
			}
			if (pUnit->getProfession() == eProfession)
			{
				iProfessionCount--;
			}
			FAssert(iProfessionCount >= 0);
			bFirstRevolutionBellsWorker = isNative() && !isHuman() && eYieldProducedType == YIELD_BELLS
				&& iProfessionCount == 0 && kOwner.getParent() != NO_PLAYER
				&& kOwner.AI_isStrategy(STRATEGY_REVOLUTION_PREPARING) && getRebelPercent() < 75;

			if (eYieldConsumedType == NO_YIELD)
			{
				iOutputValue += bPreferredNativeWorker ? 100 * iOutputYieldValue * iYieldOutput
					: 100 * kOwner.AI_yieldValue(eYieldProducedType, true, iYieldOutput);
			}
			else
			{
				FAssert(iYieldInput > 0);
				iOutputValue += 100 * iOutputYieldValue * iYieldOutput * std::min(iYieldInput, iEstimatedInputAvailable) / std::max(1, iYieldInput);
				iInputValue += 100 * kOwner.AI_yieldValue(eYieldConsumedType, false) * std::min(iYieldInput, iEstimatedInputAvailable);
			}


			if (pIdealUnassignedUnit != NULL)
			{
				if (!kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession))
				{
					iOutputValue /= 3;
				}
			}

			//If the ideal Unit isn't assigned to this profession. What right does this unit have?
			if (pIdealAssignedUnit != NULL)
			{
				//Kaszkaj - Prefer keeping an expert over replacing it with a less suitable worker.
				if (!kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession))
				{
					iOutputValue /= 3;

					iOutputValue *= kOwner.AI_professionSuitability(pUnit->getUnitType(), eProfession);
					iOutputValue /= std::max(1, kOwner.AI_professionSuitability(pIdealAssignedUnit->getUnitType(), eProfession));
				}
			}

			if (eYieldConsumedType != NO_YIELD)
			{
				//Strongly discourage conversion of raw materials by poorly qualified units.
				if (iEstimatedInputAvailable < iYieldInput)
				{
					if (iRealInputAvailable + getYieldStored(eYieldConsumedType) < iYieldInput)
					{
						iOutputValue /= 4;
					}

					if (!kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession) && pIdealUnassignedUnit != NULL)
					{
						iOutputValue *= iEstimatedInputAvailable;
						iOutputValue /= iYieldInput;
					}
				}
			}
			else
			{
				if (pIdealAssignedUnit != NULL)
				{
					//Somewhat discourage employment by poorly qualified units.
					if (!kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession))
					{
						if (pIdealAssignedUnit->getProfession() == eProfession)
						{
							if (eYieldProducedType == YIELD_CROSSES)
							{
								iOutputValue *= 50;
								iOutputValue /= 100;
							}
							else
							{
								iOutputValue *= 75;
								iOutputValue /= 100;
							}
						}
					}
				}
			}
			if (eYieldProducedType == YIELD_BELLS && kOwner.AI_isStrategy(STRATEGY_FAST_BELLS))
			{
				if ((iProfessionCount == 0) && (getPopulation() > 3))
				{
					iOutputValue *= 2;
				}
			}
		}
	}
	else
	{
		iOutputValue += (100 * iYieldOutput + 25 * iExtraYieldOutput) * iOutputYieldValue;
	}


	iOutputValue *= AI_getYieldOutputWeight(eYieldProducedType);
	iOutputValue /= 100;

	//Kaszkaj - Prefer direct manufactured goods from a worked plot over a chain that consumes other goods.
	//Kaszkaj - Input-free output receives a proportional preference instead of overriding a much larger net production gain.
	int iDirectBonusPercent = AI_directPlotYieldBonusPercent(eProfession, pPlot);
	if (iDirectBonusPercent > 0) iOutputValue += iOutputValue * iDirectBonusPercent / 100;

	int iSupplyPriority = 0;
	if (!isHuman() && iYieldOutput > 0 && iOutputValue > 0
		&& (eYieldProducedType == YIELD_HAMMERS || (eYieldProducedType != YIELD_FOOD && GC.getYieldInfo(eYieldProducedType).isCargo())))
	{
		int iTarget = (pCache == NULL ? kOwner.AI_cityYieldTarget(this, eYieldProducedType) : pCache->target(kOwner, *this, eYieldProducedType));
		int iAvailable = getYieldStored(eYieldProducedType);
		if (eYieldProducedType == YIELD_HAMMERS)
		{
			iAvailable = (pCache == NULL ? getRawYieldProduced(YIELD_HAMMERS) : pCache->produced(*this, YIELD_HAMMERS));
			const CvUnit* apWorkers[2] = {pUnit, pDisplaceUnit};
			for (int i = 0; i < 2; ++i)
			{
				const CvUnit* pWorker = apWorkers[i];
				if (pWorker == NULL || pWorker->isOnMap() || (i == 1 && pWorker == pUnit)
					|| pWorker->getProfession() == NO_PROFESSION
					|| GC.getProfessionInfo(pWorker->getProfession()).getYieldsProduced(0) != YIELD_HAMMERS) continue;
				CvPlot* pWorked = getPlotWorkedByUnit(pWorker);
				iAvailable -= pWorked == NULL ? getProfessionOutput(pWorker->getProfession(), pWorker) : pWorked->getYield(YIELD_HAMMERS);
			}
		}
		if (iTarget > iAvailable)
		{
			iSupplyPriority = eYieldProducedType == YIELD_MUSKETS || eYieldProducedType == YIELD_HORSES ? 16
				: (eYieldProducedType == YIELD_HAMMERS ? 15 : (eYieldProducedType == YIELD_TOOLS ? 14
					: (eYieldProducedType == YIELD_COATS || eYieldProducedType == YIELD_RUM
						|| eYieldProducedType == YIELD_CIGARS || eYieldProducedType == YIELD_CLOTH ? 13 : 15)));
		}
	}

	if (kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())
		&& !kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession)) iSupplyPriority = 0;
	iSupplyPriority = iSupplyPriority * iProductionPercent / 100;
	iNativePriority = iNativePriority * iProductionPercent / 100;

	//Kaszkaj - Keep enough Food after moving either worker before preferring input-free output or supplying construction.
	int iWorkerFoodAvailable = iNativeFoodAvailable;
	int iWorkerFoodNeeded = iNativeFoodNeeded;
	if (!isNative() && !isHuman() && (iDirectBonusPercent > 0 || iSupplyPriority > 0))
	{
		iWorkerFoodAvailable = (pCache == NULL ? getBaseRawYieldProduced(YIELD_FOOD) : pCache->baseProduced(*this, YIELD_FOOD));
		iWorkerFoodNeeded = (pCache == NULL ? getRawYieldConsumed(YIELD_FOOD) : pCache->consumed(*this, YIELD_FOOD)) + (pUnit->isOnMap() ? GC.getFOOD_CONSUMPTION_PER_POPULATION() : 0);
		const CvUnit* apWorkers[2] = {pUnit, pDisplaceUnit};
		for (int i = 0; i < 2; ++i)
		{
			if (apWorkers[i] == NULL || (i == 1 && apWorkers[i] == pUnit)) continue;
			CvPlot* pWorked = getPlotWorkedByUnit(apWorkers[i]);
			if (pWorked != NULL) iWorkerFoodAvailable -= pWorked->getYield(YIELD_FOOD);
		}
		iWorkerFoodAvailable = iWorkerFoodAvailable * (pCache == NULL ? getBaseYieldRateModifier(YIELD_FOOD) : pCache->modifier(*this, YIELD_FOOD)) / 100;
	}
	if (!isHuman() && iDirectBonusPercent > 0 && iWorkerFoodAvailable < iWorkerFoodNeeded) return 0;

	if (isNative())
	{
		int iValue = iOutputValue - iInputValue;
		if (!isHuman())
		{
			//Kaszkaj - Feed the Colony before following specialist priorities; keep workers needed to prevent starvation.
			int iFoodAvailable = iNativeFoodAvailable;
			int iFoodNeeded = iNativeFoodNeeded;
			if (eYieldProducedType == YIELD_FOOD && iFoodAvailable < iFoodNeeded && iValue > 0)
			{
				return 20000 + std::min(999, iValue / 100 + 10 * iYieldOutput);
			}
			if (eYieldProducedType != YIELD_FOOD && iFoodAvailable < iFoodNeeded)
			{
				return 0;
			}
			//Kaszkaj - Alien recruitment also needs stored Food; use ordinary residents to meet the queued cost.
			if (eYieldProducedType == YIELD_FOOD && iValue > 0
				&& !kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())
				&& getYieldStored(YIELD_FOOD) < (pCache == NULL ? kOwner.AI_cityYieldTarget(this, YIELD_FOOD) : pCache->target(kOwner, *this, YIELD_FOOD)))
			{
				return 18000 + std::min(999, iValue / 100 + 10 * iYieldOutput);
			}
			//Kaszkaj - Reserve a useful Liberty job while Alien Colonies prepare for independence.
			if (bFirstRevolutionBellsWorker && iValue > 0)
			{
				return 17500 + std::min(499, iValue / 100 + 10 * iYieldOutput);
			}
			//Kaszkaj - Grow lunar and Alien improvements without moving specialists out of their expert jobs.
			if (iValue > 0 && AI_improvementUpgradeWorkValue(pPlot) > 0
				&& (!kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())
					|| kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession)))
			{
				return 17000 + std::min(499, iValue / 100 + 10 * iYieldOutput);
			}

			//Kaszkaj - Food covers upkeep and modest growth; construction and equipment respond to actual shortages.
			if (eYieldProducedType == YIELD_FOOD && iValue > 0
				&& !kOwner.AI_isNativeCitySpecialist(pUnit->getUnitType())
				&& iFoodAvailable < iFoodNeeded + std::min(6, std::max(2, getPopulation() / 3)))
			{
				return 8000 + std::min(999, iValue / 100 + 10 * iYieldOutput);
			}
			if (!bOrdinaryNativeWorker && iSupplyPriority > 0)
			{
				return 1000 * iSupplyPriority + std::min(999, std::max(0, iValue) / 100 + 10 * iYieldOutput);
			}
			if (iNativePriority > 0 && iValue > 0)
			{
				return 1000 * iNativePriority + std::min(999, iValue / 100 + 10 * iYieldOutput);
			}
		}
		//Kaszkaj - Compare ordinary Alien jobs on the same scale as production, not a hundred times higher.
		if (!bOrdinaryNativeWorker) return isHuman() ? iValue : std::min(999, iValue / 100);
	}
	//Kaszkaj - Save the Food needed to finish a colonial unit, including a Colony Ship.
	if (!isHuman() && eYieldProducedType == YIELD_FOOD && iOutputValue > 0
		&& getYieldStored(YIELD_FOOD) < (pCache == NULL ? kOwner.AI_cityYieldTarget(this, YIELD_FOOD) : pCache->target(kOwner, *this, YIELD_FOOD)))
	{
		return 18000 + std::min(999, iOutputValue / 50 + 40 * iYieldOutput);
	}

	//Kaszkaj - Supply colonial construction and equipment without taking workers needed to feed the Colony.
	//Kaszkaj - Compare needed production by positive net benefit, including its consumed goods, for Colonists and ordinary Aliens.
	if (!isHuman() && iSupplyPriority > 0 && iOutputValue > iInputValue)
	{
		if (iWorkerFoodAvailable < iWorkerFoodNeeded) return 0;
		return 1000 * iSupplyPriority + std::min(999, (iOutputValue - iInputValue) / 50);
	}
	//Kaszkaj - Colonial workers also value useful upgrades.
	iOutputValue += AI_improvementUpgradeWorkValue(pPlot);

	if (eYieldProducedType != NO_YIELD)
	{
		iOutputValue *= 100 + (kOwner.AI_professionSuitability(pUnit, eProfession, pPlot) - 100) / 2;
		iOutputValue /= 100;

		if (eYieldConsumedType != NO_YIELD)
		{
			iOutputValue *= 50 + AI_getYieldAdvantage(eYieldProducedType);
			iOutputValue /= 150;
		}
	}

	if (eYieldConsumedType != NO_YIELD && eYieldConsumedType != YIELD_FOOD)
	{
		if (getYieldStored(eYieldConsumedType) > getMaxYieldCapacity())
		{
			iInputValue /= 5;
		}
	}

	if ((eYieldProducedType != YIELD_FOOD) && GC.getYieldInfo(eYieldProducedType).isCargo())
	{
		int iNeededYield = AI_getNeededYield(eYieldProducedType) - iNetYield;

		if (iNeededYield > 0)
		{
			int iTraded = AI_getTradeBalance(eYieldProducedType);
			if (iTraded > 0)
			{
				iNeededYield = std::max(1, iNeededYield - iTraded);
			}

			iNeededYield = std::min(iNeededYield, iYieldOutput);

			int iExtraValue = iNeededYield * (50 + 100 * (getMaxYieldCapacity() - getYieldStored(eYieldProducedType)) / getMaxYieldCapacity());
			iExtraValue *= AI_getYieldOutputWeight(eYieldProducedType);
			iExtraValue /= 100;
			iOutputValue += iExtraValue;
		}

		int iPercentWasted = 0;
		int iOverflowCredits = 0;
		int iProjectedStock = getYieldStored(eYieldProducedType) + iNetYield;
		int iCapacity = getMaxYieldCapacity();
		if (iYieldOutput > 0 && iProjectedStock + iYieldOutput > iCapacity)
		{
			int iDemand = std::max(0, getYieldDemand(eYieldProducedType));
			int iOldExcess = std::max(0, std::max(0, iProjectedStock) - iDemand) - iCapacity;
			int iNewExcess = std::max(0, std::max(0, iProjectedStock + iYieldOutput) - iDemand) - iCapacity;
			if (iNewExcess > 0)
			{
				//Kaszkaj - Charge a job only for its additional warehouse loss and credit the additional overflow-sale Credits.
				int iDecayPercent = GC.getDefineINT("CITY_YIELD_DECAY_PERCENT");
				int iMinimumDecay = GC.getDefineINT("MIN_CITY_YIELD_DECAY");
				int iOldLoss = iOldExcess > 0 ? std::min(iOldExcess, std::max(iDecayPercent * iOldExcess / 100, iMinimumDecay)) : 0;
				int iNewLoss = std::min(iNewExcess, std::max(iDecayPercent * iNewExcess / 100, iMinimumDecay));
				int iLostOutput = std::min(iYieldOutput, std::max(0, iNewLoss - iOldLoss));
				iPercentWasted = 100 - 100 * (iYieldOutput - iLostOutput) / iYieldOutput;
				if (iNewLoss > iOldLoss && kOwner.getParent() != NO_PLAYER)
				{
					int iSellPercent = getOverflowYieldSellPercent();
					if (!isHuman() && !kOwner.isEurope() && kOwner.canTradeWithEurope())
						iSellPercent = std::max(iSellPercent, range(GC.getHandicapInfo(GC.getGameINLINE().getHandicapType()).getAIMinimumStorageLossSellPercentage(), 0, 100));
					if (iSellPercent > 0)
					{
						int iTradeModifier = kOwner.getExtraTradeMultiplier(kOwner.getParent());
						int iOldCredits = (iSellPercent * kOwner.getSellToEuropeProfit(eYieldProducedType, iOldLoss) / 100) * iTradeModifier / 100;
						int iNewCredits = (iSellPercent * kOwner.getSellToEuropeProfit(eYieldProducedType, iNewLoss) / 100) * iTradeModifier / 100;
						iOverflowCredits = std::max(0, iNewCredits - iOldCredits);
					}
				}
			}
		}

		if (iPercentWasted > 0)
		{
			int iRetainedValue = iOutputValue * (100 - iPercentWasted) / 100;
			int iStubbornness = 10;
			if (kOwner.AI_isProfessionExpert(pUnit->getUnitType(), eProfession))
			{
				iStubbornness += 15;
			}
			if (!isHuman())
			{
				iStubbornness *= 2;
			}

			iOutputValue = (iOutputValue * (100 - iPercentWasted)) + iStubbornness * iOutputValue * iPercentWasted / 100;
			iOutputValue /= 100;
			iOutputValue = std::max(iOutputValue, iRetainedValue + 100 * iOverflowCredits);
		}
	}

	iOutputValue /= 100;
	iInputValue /= 100;

	if (kProfessionInfo.isWorkPlot() && pPlot != NULL)
	{
		if (pPlot->getBonusType() != NO_BONUS)
		{
			CvBonusInfo& kBonus = GC.getBonusInfo(pPlot->getBonusType());
			if (iDirectBonusPercent == 0 && kBonus.getYieldChange(eYieldProducedType) <= 0)
			{
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
				{
					iOutputValue -= kBonus.getYieldChange(iYield);
				}
			}
		}
	}

	if (eYieldProducedType == YIELD_FOOD)
	{
//orlanth aliens want internal growth
		if (isNative())
		{
			iOutputValue += 20;
		}
//end orlanth aliens want internal growth
		int iBaseFood = iNetYield;

		int iDifference = (iBaseFood + getYieldStored(YIELD_FOOD));

		if (iDifference < 0)
		{
			iOutputValue += (50 * std::min(iYieldOutput, -iDifference));
		}
	}

	int iNetValue = (iOutputValue - iInputValue);

	int iMinProfessionValue = kOwner.AI_yieldValue(YIELD_FOOD, true, GC.getFOOD_CONSUMPTION_PER_POPULATION());
	//Kaszkaj - Keep positive input-free work visible even at a small yield; it still requires enough Food and competes by net benefit.
	if (iNetValue <= iMinProfessionValue && !(iNetValue > 0 && (iDirectBonusPercent > 0 || bOrdinaryNativeWorker)))
	{
		if (!isHuman())
		{
			return 0;
		}
		else
		{
			iNetValue /= 2;
		}
	}

	if (pOldUnit != NULL && pOldUnit->getProfession() != eProfession && pOldUnit->getProfession() != NO_PROFESSION)
	{
		const CvPlot* pOldPlot = NULL;
		if (GC.getProfessionInfo(pOldUnit->getProfession()).isWorkPlot())
		{
			pOldPlot = pPlot;
		}

		if (iNetValue <= AI_professionValue(pOldUnit->getProfession(), pOldUnit, pOldPlot, NULL))
		{
			return 0;
		}
	}

	return std::max(1, iNetValue);
}

//Kaszkaj - Compare actual production gains when a human expert replaces an Alien, Venerable Elder, Brilliant Polymath or Industrious Cyborg.
int CvCityAI::AI_jobReplacementValue(ProfessionTypes eProfession, const CvUnit* pUnit, const CvPlot* pPlot) const
{
	if (eProfession == NO_PROFESSION || pUnit == NULL) return 0;
	const CvProfessionInfo& kProfession = GC.getProfessionInfo(eProfession);
	int iOutput = kProfession.isWorkPlot()
		? (pPlot == NULL ? 0 : pPlot->calculatePotentialProfessionYieldAmount(eProfession, pUnit, false))
		: getProfessionOutput(eProfession, pUnit);
	int iValue = 0;
	for (int i = 0; i < kProfession.getNumYieldsProduced(); ++i)
	{
		YieldTypes eYield = (YieldTypes)kProfession.getYieldsProduced(i);
		if (eYield < 0 || eYield >= NUM_YIELD_TYPES) continue;
		int iWeight = std::max(1, GET_PLAYER(getOwnerINLINE()).AI_yieldValue(eYield));
		iValue += std::max(0, iOutput) * iWeight * getBaseYieldRateModifier(eYield);
	}
	YieldTypes eInput = (YieldTypes)kProfession.getYieldsConsumed(0, getOwnerINLINE());
	if (!kProfession.isWorkPlot() && eInput != NO_YIELD)
	{
		iValue -= getProfessionInput(eProfession, pUnit) * std::max(1, GET_PLAYER(getOwnerINLINE()).AI_yieldValue(eInput, false)) * 100;
	}
	//Kaszkaj - Use the same input-free preference when deciding whether replacing a worker improves production.
	int iDirectBonusPercent = AI_directPlotYieldBonusPercent(eProfession, pPlot);
	if (iValue > 0 && iDirectBonusPercent > 0) iValue += iValue * iDirectBonusPercent / 100;
	return std::max(0, iValue);
}

//Kaszkaj - Value working lunar and Alien improvements towards their next available upgrade.
int CvCityAI::AI_improvementUpgradeWorkValue(const CvPlot* pPlot) const
{
	if (isHuman() || pPlot == NULL || pPlot->getImprovementType() == NO_IMPROVEMENT) return 0;
	const CvImprovementInfo& kImprovement = GC.getImprovementInfo(pPlot->getImprovementType());
	ImprovementTypes eUpgrade = (ImprovementTypes)kImprovement.getImprovementUpgrade();
	if (eUpgrade == NO_IMPROVEMENT || GC.getImprovementInfo(eUpgrade).isOutsideBorders()) return 0;
	const char* szType = kImprovement.getType();
	bool bWanted = std::strcmp(szType, "IMPROVEMENT_MOON_COLONY1") == 0 || std::strcmp(szType, "IMPROVEMENT_MOON_COLONY2") == 0
		|| (isNative() && (std::strcmp(szType, "IMPROVEMENT_ALIEN_FERTILIZERS") == 0 || std::strcmp(szType, "IMPROVEMENT_ALIEN_BURROW") == 0));
	if (!bWanted) return 0;
	const std::vector<CivicTypes>& aeRestrictions = GC.getImprovementInfo(eUpgrade).getBuildTechnologyRestrictions();
	for (int i = 0; i < (int)aeRestrictions.size(); ++i)
	{
		CivicTypes eCivic = aeRestrictions[i];
		if (GC.getCivicInfo(eCivic).getAllowsBuildTypes(eUpgrade) > 0
			&& GET_PLAYER(getOwnerINLINE()).getIdeasResearched(eCivic) <= 0) return 0;
	}
	return 3000 + 1000 * pPlot->getUpgradeProgress() / std::max(1, GC.getGameINLINE().getImprovementUpgradeTime(pPlot->getImprovementType()));
}

int CvCityAI::AI_professionBasicOutput(ProfessionTypes eProfession, UnitTypes eUnit, const CvPlot* pPlot) const
{
	FAssert(NO_PROFESSION != eProfession);
	if (NO_PROFESSION == eProfession)
	{
		return 0;
	}

	CvProfessionInfo& kProfessionInfo = GC.getProfessionInfo(eProfession);
	// MultipleYieldsProduced Start by Aymerick 22/01/2010**
	YieldTypes eYieldProduced = (YieldTypes) kProfessionInfo.getYieldsProduced(0);
	// MultipleYieldsProduced End
	if (NO_YIELD == eYieldProduced)
	{
		return 0;
	}

	int iProfessionOutput = 0;
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	int iUnitModifier, iUnitChange, iUnitBonusChange;
	bool bOverrideYield = kOwner.AI_getUnitYieldBonuses(eUnit, eYieldProduced, iUnitModifier, iUnitChange, iUnitBonusChange);
	if (eUnit != NO_UNIT && std::strcmp(GC.getUnitInfo(eUnit).getType(), "UNIT_NATIVE") == 0)
	{
		bOverrideYield = false;
		iUnitModifier = GC.getUnitInfo(eUnit).getYieldModifier(eYieldProduced);
		iUnitChange = GC.getUnitInfo(eUnit).getYieldChange(eYieldProduced);
		iUnitBonusChange = GC.getUnitInfo(eUnit).getBonusYieldChange(eYieldProduced);
	}
	if (kProfessionInfo.isWorkPlot())
	{
		if (pPlot == NULL)
		{
			return 0;
		}
		//Kaszkaj - Reject land/water profession mismatches before applying AI worker bonuses.
		if (kProfessionInfo.isWater() != pPlot->isWater()) return 0;
		UnitTypes eYieldUnit = bOverrideYield ? NO_UNIT : eUnit;
		iProfessionOutput = pPlot->calculatePotentialYield(eYieldProduced, getOwnerINLINE(), pPlot->getImprovementType(), false, pPlot->getRouteType(), eYieldUnit, false);
		if (bOverrideYield)
		{
			if (iProfessionOutput > 0 && pPlot->isValidYieldChanges(eUnit))
			{
				iProfessionOutput += iUnitChange;
				if (pPlot->getBonusType() != NO_BONUS && GC.getBonusInfo(pPlot->getBonusType()).getYieldChange(eYieldProduced) > 0)
				{
					iProfessionOutput += iUnitBonusChange;
				}
			}
			iProfessionOutput = std::max(0, iProfessionOutput * (100 + iUnitModifier) / 100);
		}
	}
	else
	{

		SpecialBuildingTypes eSpecialBuilding = (SpecialBuildingTypes) kProfessionInfo.getSpecialBuilding();
		if (eSpecialBuilding == NO_SPECIALBUILDING)
		{
			return 0;
		}

		int iModifier = 100;
		int iExtra = 0;
		iModifier += iUnitModifier;
		iExtra += iUnitChange;

		const std::vector<BuildingTypes>& aeBuildings = GC.getSpecialBuildingInfo(eSpecialBuilding).getBuildingTypes();
		for (int i = 0; i < (int)aeBuildings.size(); ++i)
		{
			BuildingTypes eBuilding = aeBuildings[i];
			if (isHasBuilding(eBuilding))
			{
				int iBuildingOutput = (GC.getBuildingInfo(eBuilding).getProfessionOutput() + iExtra) * iModifier / 100;
				if (iBuildingOutput > iProfessionOutput) iProfessionOutput = iBuildingOutput;
			}
		}
	}


	return iProfessionOutput;
}


CvUnit* CvCityAI::AI_getWorstProfessionUnit(ProfessionTypes eProfession) const
{
	int iWorstOutput = MAX_INT;
	CvUnit* pWorstUnit = NULL;
	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pOldUnit = m_aPopulationUnits[i];
		if (pOldUnit != NULL && !pOldUnit->isColonistLocked() && pOldUnit->getProfession() == eProfession)
		{
			int iOutput = getProfessionOutput(eProfession, pOldUnit);
			if (iOutput < iWorstOutput)
			{
				iWorstOutput = iOutput;
				pWorstUnit = pOldUnit;
			}
		}
	}

	return pWorstUnit;
}

int CvCityAI::AI_unitJoinCityValue(CvUnit* pUnit, ProfessionTypes* peNewProfession) const
{
	bool bHumanSpecialist = GET_PLAYER(getOwnerINLINE()).AI_isNativeHumanSpecialist(pUnit->getUnitType());
	int iBestValue = 0;
	int iBestPlot = -1;
	ProfessionTypes eBestProfession = NO_PROFESSION;

	for (int i=0;i<GC.getNumProfessionInfos();i++)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes) i;
		if (GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession))
		{
			if (GC.getProfessionInfo(eLoopProfession).isCitizen())
			{
				if (pUnit->canHaveProfession(eLoopProfession, bHumanSpecialist, plot()))
				{
					if (GC.getProfessionInfo(eLoopProfession).isWorkPlot())
					{
						for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
						{
							if (iI != CITY_HOME_PLOT)
							{
								CvPlot* pLoopPlot = getCityIndexPlot(iI);

								if (pLoopPlot != NULL)
								{
									if (!isUnitWorkingPlot(iI) || bHumanSpecialist)
									{
										if (canWork(pLoopPlot))
										{
											CvUnit* pWorker = bHumanSpecialist ? getUnitWorkingPlot(iI) : NULL;
											if (pWorker != NULL && (pWorker == pUnit || pWorker->isColonistLocked())) continue;
											int iValue = AI_professionValue(eLoopProfession, pUnit, pLoopPlot, pWorker);
											if (bHumanSpecialist && iValue > 0)
											{
												iValue = AI_jobReplacementValue(eLoopProfession, pUnit, pLoopPlot)
													- (pWorker == NULL ? 0 : AI_jobReplacementValue(pWorker->getProfession(), pWorker, pLoopPlot));
											}

											if (iValue > iBestValue)
											{
												eBestProfession = eLoopProfession;
												iBestValue = iValue;
												iBestPlot = iI;
											}
										}
									}
								}
							}
						}
					}
					else
					{
						CvUnit* pWorker = NULL;
						if (bHumanSpecialist && !pUnit->canHaveProfession(eLoopProfession, false, plot()))
						{
							pWorker = AI_getWorstProfessionUnit(eLoopProfession);
							if (pWorker == NULL || pWorker == pUnit || pWorker->isColonistLocked()) continue;
						}
						int iValue = AI_professionValue(eLoopProfession, pUnit, NULL, pWorker);
						if (bHumanSpecialist && iValue > 0)
						{
							iValue = AI_jobReplacementValue(eLoopProfession, pUnit, NULL)
								- (pWorker == NULL ? 0 : AI_jobReplacementValue(eLoopProfession, pWorker, NULL));
						}
						if (iValue > iBestValue)
						{
							eBestProfession = eLoopProfession;
							iBestValue = iValue;
							iBestPlot = -1;
						}
					}
				}
			}
		}
	}

	int iFood = 0;
	for (int i = 0; i < NUM_CITY_PLOTS; ++i)
	{
		CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), i);
		if (pLoopPlot != NULL)
		{
			iFood += std::max(0, pLoopPlot->getYield(YIELD_FOOD) - GC.getFOOD_CONSUMPTION_PER_POPULATION());
		}
	}
	if (iFood < getPopulation())
	{
		iBestValue *= 6 + iFood;
		iBestValue /= 6 + getPopulation() * GC.getFOOD_CONSUMPTION_PER_POPULATION();
	}
	else if (iFood > getPopulation())
	{
		iBestValue *= 4 + iFood;
		iBestValue /= 4 + getPopulation() * GC.getFOOD_CONSUMPTION_PER_POPULATION();
	}

	if (peNewProfession != NULL)
	{
		*peNewProfession = eBestProfession;
	}

	return iBestValue;
}

int CvCityAI::AI_unitJoinReplaceValue(CvUnit* pUnit, CvUnit** pReplaceUnit) const
{
	int iBestValue = 0;
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (int i = 0; i < getPopulation(); ++i)
	{
		CvUnit* pLoopUnit = getPopulationUnitByIndex(i);
		FAssert(pLoopUnit != NULL);

		if (pLoopUnit->getProfession() != NO_PROFESSION)
		{


			CvPlot* pPlot = getPlotWorkedByUnit(pLoopUnit);
			if (pPlot == NULL)
			{
				pPlot = plot();
			}

			int iExistingValue = kOwner.AI_professionSuitability(pLoopUnit, pLoopUnit->getProfession(), pPlot);

			int iNewValue = kOwner.AI_professionSuitability(pUnit, pLoopUnit->getProfession(), pPlot);

			int iValue = iNewValue - iExistingValue;

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				if (pReplaceUnit != NULL)
				{
					*pReplaceUnit = pLoopUnit;
				}
			}
		}
	}

	return iBestValue;
}

ProfessionTypes CvCityAI::AI_bestPlotProfession(const CvUnit* pUnit, const CvPlot* pPlot) const
{
	FAssert(pUnit != NULL);
	FAssert(pPlot != NULL);

	ProfessionTypes eBestProfession = NO_PROFESSION;
	int iBestValue = -1;
	for (int iI = 0; iI < GC.getNumProfessionInfos(); iI++)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)iI;
		if (GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession))
		{
			if (GC.getProfessionInfo(eLoopProfession).isWorkPlot())
			{
				int iValue = AI_professionValue(eLoopProfession, pUnit, pPlot, NULL);
				if (iValue > iBestValue)
				{
					eBestProfession = eLoopProfession;
					iBestValue = iValue;
				}
			}
		}
	}

	return eBestProfession;
}

int CvCityAI::AI_bestProfessionPlot(ProfessionTypes eProfession, const CvUnit* pUnit) const
{
	FAssert(pUnit != NULL);
	FAssert(eProfession != NO_PROFESSION);

	int iBestValue = 0;
	int iBestPlot = -1;
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		if (iI != CITY_HOME_PLOT)
		{
			CvPlot* pLoopPlot = getCityIndexPlot(iI);

			if (pLoopPlot != NULL)
			{
				if (!isUnitWorkingPlot(iI) || (getUnitWorkingPlot(iI) == pUnit))
				{
					if (canWork(pLoopPlot))
					{
						int iValue = AI_professionValue(eProfession, pUnit, pLoopPlot, NULL);

						if (iValue > iBestValue)
						{
							iBestValue = iValue;
							iBestPlot = iI;
						}
					}
				}
			}
		}
	}

	return iBestPlot;
}


bool CvCityAI::AI_canMakeGift() const
{
	return (AI_getGiftTimer() <= 0);
}

int CvCityAI::AI_getGiftTimer() const
{
	return m_iGiftTimer;
}

void CvCityAI::AI_setGiftTimer(int iNewValue)
{
	m_iGiftTimer = iNewValue;
	FAssert(AI_getGiftTimer() >= 0);
}

void CvCityAI::AI_changeGiftTimer(int iChange)
{
	if (iChange != 0)
	{
		AI_setGiftTimer(AI_getGiftTimer() + iChange);
	}
}

int CvCityAI::AI_maxGoldTrade(PlayerTypes ePlayer) const
{
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
		return kOwner.AI_maxGoldTrade(ePlayer);
	}

int CvCityAI::AI_calculateAlarm(PlayerTypes eIndex) const
{
	FAssertMsg(eIndex >= 0, "eIndex expected to be >= 0");
	FAssertMsg(eIndex < MAX_PLAYERS, "eIndex expected to be < MAX_PLAYERS");

	AlarmTypes eAlarm = (AlarmTypes) GC.getLeaderHeadInfo(GET_PLAYER(getOwnerINLINE()).getLeaderType()).getAlarmType();
	if (eAlarm == NO_ALARM)
	{
		return 0;
	}

	CvAlarmInfo& kAlarm = GC.getAlarmInfo(eAlarm);

	int iPositiveAlarm = 0;
	int iNegativeAlarm = 0;

	int iRange = kAlarm.getRange();
	for (int iX = -iRange; iX <= iRange; iX++)
	{
		for (int iY = -iRange; iY <= iRange; iY++)
		{
			int iDistance = plotDistance(iX, iY, 0, 0);
			if (iDistance <= iRange)
			{
				CvPlot* pLoopPlot = plotXY(getX_INLINE(), getY_INLINE(), iX, iY);
				if (pLoopPlot != NULL)
				{
					int iPlotAlarm = 0;
					CvCity* pLoopCity = pLoopPlot->getPlotCity();
					if (pLoopCity != NULL)
					{
						if (pLoopCity->getOwner() == eIndex)
						{
							iPlotAlarm += kAlarm.getColony();
							iPlotAlarm += pLoopCity->getPopulation() * kAlarm.getPopulation();
						}
					}

					iPlotAlarm = iPlotAlarm * std::max(0, iRange - iDistance + 1) / std::max(1, iRange + 1);

					iPositiveAlarm += iPlotAlarm;
				}
			}
		}
	}

	//Religion
	if (getMissionaryPlayer() != NO_PLAYER)
	{
		if (GET_PLAYER(getMissionaryPlayer()).getCivilizationType() == GET_PLAYER(eIndex).getCivilizationType())
		{
			int iModifier = 100 + GET_PLAYER(eIndex).getMissionaryRateModifier() + GET_PLAYER(getOwnerINLINE()).getMissionaryRateModifier();
			iNegativeAlarm += getMissionaryRate() * kAlarm.getMissionary() * iModifier / 100;
		}
	}

	int iModifier = 100;
	iModifier += GET_PLAYER(eIndex).getNativeAngerModifier();
	iModifier = std::max(0, iModifier);

	iPositiveAlarm *= iModifier;
	iPositiveAlarm /= 100;

	return (iPositiveAlarm + iNegativeAlarm);
}

int CvCityAI::AI_estimateYieldValue(YieldTypes eYield, int iAmount) const
{
	int iValue = iAmount * GET_PLAYER(getOwnerINLINE()).AI_yieldValue(eYield);

	switch (eYield)
	{
		case YIELD_FOOD:
		case YIELD_LUMBER:
		case YIELD_SILVER:
		case YIELD_COTTON:
		case YIELD_FUR:
		case YIELD_SUGAR:
		case YIELD_TOBACCO:
		case YIELD_ORE:
		case YIELD_CLOTH:
		case YIELD_COATS:
		case YIELD_RUM:
		case YIELD_CIGARS:
		case YIELD_TOOLS:
		case YIELD_MUSKETS:
		case YIELD_HORSES:
		case YIELD_TRADE_GOODS:
		case YIELD_HAMMERS:
		 ///TKs Invention Core Mod v 1.0
		case YIELD_HYDROCARBONS:
		case YIELD_IDEAS:
            break;
		///TKe
		case YIELD_BELLS:
			break;
		case YIELD_CROSSES:
			break;
		case YIELD_EDUCATION:
			break;
		default:
			FAssert(false);
	}

	return iValue;
}

//Note that 0 means the camp should be disbanded in some way...
int CvCityAI::AI_getTargetSize() const
{
	return m_iTargetSize;
}

void CvCityAI::AI_setTargetSize(int iNewValue)
{
	m_iTargetSize = iNewValue;
}

//Yield inflow is the weight to put on delivering goods here
int CvCityAI::AI_getYieldOutputWeight(YieldTypes eYield) const
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");

	return m_aiYieldOutputWeight[eYield];
}

void CvCityAI::AI_setYieldOutputWeight(YieldTypes eYield, int iNewValue)
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	FAssertMsg(iNewValue >= 0, "Weight should be positive");

	m_aiYieldOutputWeight[eYield] = iNewValue;
}

int CvCityAI::AI_getNeededYield(YieldTypes eYield) const
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");

	return m_aiNeededYield[eYield];

}

void CvCityAI::AI_setNeededYield(YieldTypes eYield, int iNewValue)
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	FAssertMsg(iNewValue > 0, "Negative needed yield makes no sense");

	m_aiNeededYield[eYield] = iNewValue;
}


int CvCityAI::AI_getTradeBalance(YieldTypes eYield) const
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");

	int iAdjustment = 100 + 300 / (2 + YIELD_DISCOUNT_TURNS);

	return (m_aiTradeBalance[eYield] * iAdjustment) / (YIELD_DISCOUNT_TURNS * 100);
}

void CvCityAI::AI_changeTradeBalance(YieldTypes eYield, int iAmount)
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");

	m_aiTradeBalance[eYield] += iAmount;
}

int CvCityAI::AI_getYieldAdvantage(YieldTypes eYield) const
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	return m_aiYieldAdvantage[eYield];
}

void CvCityAI::AI_setYieldAdvantage(YieldTypes eYield, int iNewValue)
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	m_aiYieldAdvantage[eYield] = iNewValue;
}

void CvCityAI::AI_assignDesiredYield()
{
	YieldTypes eBestYield = NO_YIELD;
//orlanth natives
	if (isNative())
	{
		//Kaszkaj - Advertise missing construction materials and production inputs before optional trade goods.
		int iBestValue = 0;
		bool bBestNeeded = false;
		for (int i = 0; i < NUM_YIELD_TYPES; ++i)
		{
			YieldTypes eYield = (YieldTypes)i;
			const CvYieldInfo& kYield = GC.getYieldInfo(eYield);
			if (!kYield.isCargo() || kYield.getNativeBuyPrice() <= 0) continue;
			int iStored = getYieldStored(eYield);
			int iRequired = AI_getRequiredYieldLevel(eYield);
			int iInput = std::max(AI_getNeededYield(eYield), getRawYieldConsumed(eYield));
			int iBuffer = 4 * std::max(0, iInput - getRawYieldProduced(eYield));
			int iShortage = std::max(0, std::max(iRequired, iBuffer) - iStored);
			bool bNeeded = iShortage > 0;
			int iValue = 0;
			if (bNeeded)
			{
				int iTarget = std::max(1, std::max(getMaxYieldCapacity(), std::max(iRequired, iBuffer)));
				iValue = 100 * iShortage / iTarget + kYield.getNativeBuyPrice();
			}
			else if (iStored == 0 && !canProduceYield(eYield))
			{
				iValue = (10 + kYield.getNativeBuyPrice() + kYield.getNativeHappy())
					* (1 + GC.getGameINLINE().getSorenRandNum(100, "City Desired Yield"));
			}
			if (iValue > 0 && ((bNeeded && !bBestNeeded) || (bNeeded == bBestNeeded && iValue > iBestValue)))
			{
				iBestValue = iValue;
				bBestNeeded = bNeeded;
				eBestYield = eYield;
			}
		}
	}
//end orlanth natives
	if (m_eDesiredYield != eBestYield)
	{
		m_eDesiredYield = eBestYield;
		setBillboardDirty(true);

		if (eBestYield != NO_YIELD)
		{
			CvWString szMessage = gDLL->getText("TXT_KEY_DESIRED_YIELD_CHANGE", GET_PLAYER(getOwnerINLINE()).getCivilizationAdjectiveKey(), getNameKey(), GC.getYieldInfo(eBestYield).getTextKeyWide());
			for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
			{
				CvPlayer& kPlayer = GET_PLAYER((PlayerTypes) iPlayer);
				if (kPlayer.isAlive() && kPlayer.getID() != getOwnerINLINE())
				{
					if (isScoutVisited(kPlayer.getTeam()))
					{
						gDLL->getInterfaceIFace()->addMessage((PlayerTypes) iPlayer, false, GC.getEVENT_MESSAGE_TIME(), szMessage, "AS2D_POSITIVE_DINK", MESSAGE_TYPE_MINOR_EVENT, GC.getYieldInfo(eBestYield).getButton(), (ColorTypes)GC.getInfoTypeForString("COLOR_WHITE"), getX_INLINE(), getY_INLINE(), true, true);
					}
				}
			}
		}
	}
}

YieldTypes CvCityAI::AI_getDesiredYield() const
{
	return m_eDesiredYield;
}

void CvCityAI::AI_updateNeededYields()
{
	//This function has been updated to be invariant of the current workforce allocation.
	for (int i = 0; i < NUM_YIELD_TYPES; i++)
	{
		m_aiNeededYield[i] = 0;
	}

	const bool bCacheExperts = cityBuildingValueCacheSize() != 0 && !GC.getUSE_CAN_BUILD_CALLBACK()
		&& !GC.getUSE_UNIT_CANNOT_MOVE_INTO_CALLBACK() && !GC.getUSE_CAN_DECLARE_WAR_CALLBACK();
	std::map<UnitTypes, std::vector<ProfessionTypes> > aeExpertsByUnit;

	for (uint i = 0; i < m_aPopulationUnits.size(); ++i)
	{
		CvUnit* pLoopUnit = m_aPopulationUnits[i];
		if (pLoopUnit != NULL)
		{
			if (pLoopUnit->isColonistLocked())
			{
				if (pLoopUnit->getProfession() != NO_PROFESSION)
				{
					YieldTypes eConsumedYield = (YieldTypes)GC.getProfessionInfo(pLoopUnit->getProfession()).getYieldsConsumed(0, GET_PLAYER(getOwner()).getID());
					if (eConsumedYield != NO_YIELD)
					{
						m_aiNeededYield[eConsumedYield] += getProfessionInput(pLoopUnit->getProfession(), pLoopUnit);
					}
				}
			}
			else
			{
				int aiExpertInputs[NUM_YIELD_TYPES] = {0};
				const std::vector<ProfessionTypes>* pExperts = NULL;
				if (bCacheExperts)
				{
					UnitTypes eUnit = pLoopUnit->getUnitType();
					std::map<UnitTypes, std::vector<ProfessionTypes> >::iterator it = aeExpertsByUnit.find(eUnit);
					if (it == aeExpertsByUnit.end())
					{
						std::vector<ProfessionTypes>& aeExperts = aeExpertsByUnit[eUnit];
						for (int iProfession = 0; iProfession < GC.getNumProfessionInfos(); ++iProfession)
						{
							ProfessionTypes eProfession = (ProfessionTypes)iProfession;
							if (GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(eUnit, eProfession)) aeExperts.push_back(eProfession);
						}
						pExperts = &aeExperts;
					}
					else pExperts = &it->second;
				}
				int iProfessionCount = pExperts != NULL ? (int)pExperts->size() : GC.getNumProfessionInfos();
				for (int iProfession = 0; iProfession < iProfessionCount; ++iProfession)
				{
					ProfessionTypes eProfession = pExperts != NULL ? (*pExperts)[iProfession] : (ProfessionTypes)iProfession;
					if ((pExperts != NULL || GET_PLAYER(getOwnerINLINE()).AI_isProfessionExpert(pLoopUnit->getUnitType(), eProfession))
						&& pLoopUnit->canHaveProfession(eProfession, true, NULL))
					{
						YieldTypes eConsumedYield = (YieldTypes)GC.getProfessionInfo(eProfession).getYieldsConsumed(0, getOwnerINLINE());
						if (eConsumedYield >= 0 && eConsumedYield < NUM_YIELD_TYPES)
						{
							aiExpertInputs[eConsumedYield] = std::max(aiExpertInputs[eConsumedYield], getProfessionInput(eProfession, pLoopUnit));
						}
					}
				}
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
				{
					m_aiNeededYield[iYield] += aiExpertInputs[iYield];
				}
			}
		}
	}

	//Now, buildings.
	for (int iI = 0; iI < GC.getNumProfessionInfos(); iI++)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)iI;
		if (GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession))
		{
			CvProfessionInfo& kLoopProfession = GC.getProfessionInfo(eLoopProfession);
			if (kLoopProfession.isCitizen())
			{
				if (!kLoopProfession.isWorkPlot())
				{
					// MultipleYieldsProduced Start by Aymerick 22/01/2010**
					YieldTypes eYieldProduced = (YieldTypes)kLoopProfession.getYieldsProduced(0);
					YieldTypes eYieldConsumed = (YieldTypes)kLoopProfession.getYieldsConsumed(0, GET_PLAYER(getOwner()).getID());
					// MultipleYieldsProduced End
					if (eYieldConsumed != NO_YIELD)
					{
						if (eYieldProduced >= 0 && eYieldProduced < NUM_YIELD_TYPES && eYieldConsumed >= 0 && eYieldConsumed < NUM_YIELD_TYPES
							&& AI_getYieldAdvantage(eYieldProduced) == 100)
						{
							m_aiNeededYield[eYieldConsumed] = std::max(m_aiNeededYield[eYieldConsumed], getNumProfessionBuildingSlots(eLoopProfession) * getProfessionInput(eLoopProfession, NULL));
						}
					}
				}
			}
		}
	}
}

bool CvCityAI::AI_shouldImportYield(YieldTypes eYield) const
{
	if (AI_getNeededYield(eYield) > 0)
	{
		int iInput = getYieldStored(eYield) / 10;
		if (iInput < 10)
		{
			if ((iInput + getRawYieldProduced(eYield) - getRawYieldConsumed(eYield)) < AI_getNeededYield(eYield))
			{
				return true;
			}
		}
	}

	return false;
}

bool CvCityAI::AI_shouldExportYield(YieldTypes eYield) const
{
	if (GET_PLAYER(getOwnerINLINE()).AI_isYieldFinalProduct(eYield))
	{
		return true;
	}

	if ((GET_PLAYER(getOwnerINLINE()).AI_shouldBuyFromEurope(eYield)) || eYield == YIELD_LUMBER)
	{
		return false;
	}

	if ((getYieldStored(eYield) * 100) / GC.getGameINLINE().getCargoYieldCapacity() > 75)
	{
		if (AI_getNeededYield(eYield) == 0)
		{
			return true;
		}
		else
		{
			if (getRawYieldProduced(eYield) > getRawYieldConsumed(eYield))
			{
				return true;
			}
		}
	}
	return false;
}

int CvCityAI::AI_getTransitYield(YieldTypes eYield) const
{
	//This could(should?) be cached.
	int iTotal = 0;
	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
	CvUnit* pLoopUnit;
	int iLoop;
	for (pLoopUnit = kOwner.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = kOwner.nextUnit(&iLoop))
	{
		if (pLoopUnit->getYield() == eYield && pLoopUnit->getYieldStored() > 0)
		{
			if (AI_shouldImportYield(eYield))
			{
				FAssert(pLoopUnit->isCargo());
				CvUnit* pTransport = pLoopUnit->getTransportUnit();
				if (pTransport != NULL)
				{
					CvPlot* pMissionPlot = pTransport->getGroup()->AI_getMissionAIPlot();
					MissionAITypes eMissionAI = pTransport->getGroup()->AI_getMissionAIType();
					if ((eMissionAI == MISSIONAI_TRANSPORT) || (eMissionAI == MISSIONAI_TRANSPORT_SEA))
					{
						if (pMissionPlot == plot())
						{
							iTotal += pLoopUnit->getYieldStored();
						}
					}
				}
			}
		}
	}

	return iTotal;
}

int CvCityAI::AI_getFoodGatherable(int iPop, int iPlotFoodThreshold) const
{
	if (iPop == -1)
	{
		iPop = MAX_INT;
	}
	std::vector<int> yields;
	int iTotal = 0;
	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
		if (pLoopPlot != NULL)
		{
			if (iI == CITY_HOME_PLOT)
			{
				iTotal += pLoopPlot->getYield(YIELD_FOOD);
			}
			else
			{
				if ((pLoopPlot->getBonusType() == NO_BONUS) || GC.getBonusInfo(pLoopPlot->getBonusType()).getYieldChange(YIELD_FOOD) > 0)
				{
					if (canWork(pLoopPlot))
					{
						int iYield = pLoopPlot->getYield(YIELD_FOOD);
						yields.push_back(iYield);
					}
				}
			}
		}
	}

	std::sort(yields.begin(), yields.end(), std::greater<int>());
	for (int iI = 0; iI < (int)yields.size(); iI++)
	{
		if (iI > iPop)
		{
			break;
		}
		else
		{
			iTotal += yields[iI];
		}
	}
	return iTotal;
}

bool CvCityAI::AI_isPort() const
{
	return m_bPort;
}

void CvCityAI::AI_setPort(bool iNewValue)
{
	m_bPort = iNewValue;
}

bool CvCityAI::AI_potentialPlot(short* piYields) const
{
	int iNetFood = piYields[YIELD_FOOD] - GC.getFOOD_CONSUMPTION_PER_POPULATION();

	if (iNetFood < 0)
	{
 		if (piYields[YIELD_FOOD] == 0)
		{
			return false;
		}
	}

	return true;
}

int CvCityAI::AI_getFoundValue()
{
	return m_iFoundValue;
}

int CvCityAI::AI_getRequiredYieldLevel(YieldTypes eYield)
{
	FAssertMsg(eYield > NO_YIELD, "Index out of bounds");
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	return getMaintainLevel(eYield);
}

void CvCityAI::AI_updateRequiredYieldLevels()
{
	int aiLevels[NUM_YIELD_TYPES];
	for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
	{
		aiLevels[iI] = 0;
	}
	CvPlayerAI& kPlayer = GET_PLAYER(getOwner());

	//Kaszkaj - Let one waiting Intrepid Explorer use the free outfit allowance; reserve materials normally for combat equipment.
	bool bFreeScoutWaiting = kPlayer.AI_getScoutEquipmentCandidate() != NULL;
	int iBestValue = 0;
	ProfessionTypes eBestProfession = NO_PROFESSION;

	for (int iI = 0; iI < GC.getNumProfessionInfos(); ++iI)
	{
		ProfessionTypes eLoopProfession = (ProfessionTypes)iI;
		CvProfessionInfo& kProfession = GC.getProfessionInfo(eLoopProfession);
		if (bFreeScoutWaiting && kProfession.isScout())
		{
			continue;
		}

		if (!(kProfession.isCitizen() || kProfession.isWorkPlot()))
		{
			if (GC.getCivilizationInfo(getCivilizationType()).isValidProfession(eLoopProfession))
			{
				int iValue = kPlayer.AI_professionValue(eLoopProfession, UNITAI_DEFENSIVE);
				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					eBestProfession = eLoopProfession;
				}
				for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
				{
					YieldTypes eYield = (YieldTypes)iI;

					int iRequired = GET_PLAYER(getOwnerINLINE()).getYieldEquipmentAmount(eLoopProfession, eYield);
					int iPercent = 0;
					if (kPlayer.AI_isStrategy(STRATEGY_REVOLUTION_PREPARING))
					{
						if (eYield == YIELD_MUSKETS || eYield == YIELD_HORSES)
						{
							iPercent = 50;
							if (kPlayer.AI_isStrategy(STRATEGY_REVOLUTION_DECLARING))
							{
								iPercent = 75;
							}
						}
					}
					iRequired = std::max(iRequired, getMaxYieldCapacity() * iPercent / 100);
					iRequired = std::min(iRequired, getMaxYieldCapacity());
					aiLevels[eYield] = std::max(aiLevels[eYield], iRequired);
				}
			}
		}
	}

	if (eBestProfession != NO_PROFESSION)
	{
		int iNeeded = AI_neededDefenders();
		iNeeded -= AI_numDefenders(true, false);

		iNeeded = std::max(1, iNeeded);

		for (int iI = 0; iI < NUM_YIELD_TYPES; ++iI)
		{
			YieldTypes eYield = (YieldTypes)iI;

			int iRequired = GET_PLAYER(getOwnerINLINE()).getYieldEquipmentAmount(eBestProfession, eYield);

			aiLevels[eYield] = std::max(aiLevels[eYield], iRequired * iNeeded);
		}
	}

	for (int iPass = 0; iPass < 2; ++iPass)
	{
		BuildingTypes eBuilding = (iPass == 0) ? getProductionBuilding() : AI_bestBuildingIgnoreRequirements();
		if (eBuilding != NO_BUILDING)
		{
			CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
			for (int i = 0; i < NUM_YIELD_TYPES; ++i)
			{
				int iAmount = kBuilding.getYieldCost(i);
				aiLevels[i] = std::max(iAmount, aiLevels[i]);
			}
		}
	}

	if (AI_getTargetSize() > 3)
	{
		aiLevels[YIELD_LUMBER] = std::max(aiLevels[YIELD_LUMBER], getMaxYieldCapacity() / 2);
	}

	for (int i = 0; i < NUM_YIELD_TYPES; ++i)
	{
		YieldTypes eYield = (YieldTypes)i;
		if (eYield != YIELD_HAMMERS && GC.getYieldInfo(eYield).isCargo()) aiLevels[i] = std::max(aiLevels[i], kPlayer.AI_cityYieldTarget(this, eYield));
		//Kaszkaj - Release reserves after an order or shortage ends so transports can export the surplus.
		setMaintainLevel((YieldTypes)i, aiLevels[i]);
	}
}

bool CvCityAI::AI_foodAvailable(int iExtra) const
{
	PROFILE_FUNC();

	CvPlot* pLoopPlot;
	bool abPlotAvailable[NUM_CITY_PLOTS];
	int iFoodCount;
	int iPopulation;
	int iBestPlot;
	int iValue;
	int iBestValue;
	int iI;

	iFoodCount = 0;

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		abPlotAvailable[iI] = false;
	}

	for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = getCityIndexPlot(iI);

		if (pLoopPlot != NULL)
		{
			if (iI == CITY_HOME_PLOT)
			{
				iFoodCount += pLoopPlot->calculatePotentialYield(YIELD_FOOD, NULL, false);
			}
			else if ((pLoopPlot->getWorkingCity() == this) && AI_potentialPlot(pLoopPlot->getYield()))
			{
				abPlotAvailable[iI] = true;
			}
		}
	}

	iPopulation = (getPopulation() + iExtra);

	while (iPopulation > 0)
	{
		iBestValue = 0;
		iBestPlot = CITY_HOME_PLOT;

		for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
		{
			if (abPlotAvailable[iI])
			{
				iValue = getCityIndexPlot(iI)->calculatePotentialYield(YIELD_FOOD, NULL, false);

				if (iValue > iBestValue)
				{
					iBestValue = iValue;
					iBestPlot = iI;
				}
			}
		}

		if (iBestPlot != CITY_HOME_PLOT)
		{
			iFoodCount += iBestValue;
			abPlotAvailable[iBestPlot] = false;
		}
		else
		{
			break;
		}

		iPopulation--;
	}

	if (iFoodCount < foodConsumption(iExtra))
	{
		return false;
	}

	return true;
}


int CvCityAI::AI_yieldValue(short* piYields, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood, bool bIgnoreGrowth, bool bIgnoreStarvation, bool bWorkerOptimization) const
{
	int iValue = 0;

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		if (piYields[iI] != 0)
		{
			iValue +=  AI_estimateYieldValue((YieldTypes)iI, piYields[iI]);
		}
	}

	return iValue;
}


int CvCityAI::AI_plotValue(const CvPlot* pPlot, bool bAvoidGrowth, bool bRemove, bool bIgnoreFood, bool bIgnoreGrowth, bool bIgnoreStarvation) const
{
	PROFILE_FUNC();

	short aiYields[NUM_YIELD_TYPES];
	ImprovementTypes eCurrentImprovement;
	ImprovementTypes eFinalImprovement;
	int iYieldDiff;
	int iValue;
	int iI;
	int iTotalDiff;

	iValue = 0;
	iTotalDiff = 0;

	for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		aiYields[iI] = pPlot->calculatePotentialYield((YieldTypes)iI, NULL, false);
	}

	eCurrentImprovement = pPlot->getImprovementType();
	eFinalImprovement = NO_IMPROVEMENT;

	if (eCurrentImprovement != NO_IMPROVEMENT)
	{
		eFinalImprovement = finalImprovementUpgrade(eCurrentImprovement);
	}

	int iYieldValue = (AI_yieldValue(aiYields, bAvoidGrowth, bRemove, bIgnoreFood, bIgnoreGrowth, bIgnoreStarvation) * 100);
	if (eFinalImprovement != NO_IMPROVEMENT)
	{
		for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
		{
			iYieldDiff = (pPlot->calculateImprovementYieldChange(eFinalImprovement, ((YieldTypes)iI), getOwnerINLINE()) - pPlot->calculateImprovementYieldChange(eCurrentImprovement, ((YieldTypes)iI), getOwnerINLINE()));
			aiYields[iI] += iYieldDiff;
		}
		int iFinalYieldValue = (AI_yieldValue(aiYields, bAvoidGrowth, bRemove, bIgnoreFood, bIgnoreGrowth, bIgnoreStarvation) * 100);

		if (iFinalYieldValue > iYieldValue)
		{
			iYieldValue = (40 * iYieldValue + 60 * iFinalYieldValue) / 100;
		}
		else
		{
			iYieldValue = (60 * iYieldValue + 40 * iFinalYieldValue) / 100;
		}
	}
	// unless we are emph food (and also food not production)
	if (AI_getEmphasizeYieldCount(YIELD_FOOD) <= 0)
	{
		// if this plot is super bad (less than 2 food and less than combined 2 prod
		if (!AI_potentialPlot(aiYields))
		{
			// undervalue it even more!
			iYieldValue /= 16;
		}
	}
	iValue += iYieldValue;

	if (eCurrentImprovement != NO_IMPROVEMENT)
	{
		if (pPlot->getBonusType() == NO_BONUS) // XXX double-check CvGame::doFeature that the checks are the same...
		{
			for (iI = 0; iI < GC.getNumBonusInfos(); iI++)
			{
					if (GC.getImprovementInfo(eCurrentImprovement).getImprovementBonusDiscoverRand(iI) > 0)
					{
						iValue += 35;
					}
				}
			}
		}

	if ((eCurrentImprovement != NO_IMPROVEMENT) && (GC.getImprovementInfo(pPlot->getImprovementType()).getImprovementUpgrade() != NO_IMPROVEMENT))
	{
		iValue += 200;
		int iUpgradeTime = (GC.getGameINLINE().getImprovementUpgradeTime(eCurrentImprovement));
		if (iUpgradeTime > 0) //assert this?
		{
			int iUpgradePenalty = (100 * (iUpgradeTime - pPlot->getUpgradeProgress()));
			iUpgradePenalty *= (iTotalDiff * 5);
			iUpgradePenalty /= std::max(1, GC.getGameSpeedInfo(GC.getGame().getGameSpeedType()).getGrowthPercent());
			iValue -= iUpgradePenalty;
		}
	}

	return iValue;
}


int CvCityAI::AI_experienceWeight() const
{
	return ((getProductionExperience() + getDomainFreeExperience(DOMAIN_SEA)) * 2);
}


int CvCityAI::AI_plotYieldValue(const CvPlot* pPlot, int* piYields) const
{
	FAssert(piYields != NULL);
	int iValue = 0;

	int iBestValue = 0;

	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	for (int iJ = 0; iJ < NUM_YIELD_TYPES; iJ++)
	{
		YieldTypes eYield = (YieldTypes)iJ;

		if (piYields[eYield] > 0)
		{
			int iTempValue = (1 + piYields[eYield]) * kOwner.AI_yieldValue(eYield, true, 1, true);

			bool bImportant = false;

			if (pPlot->isBeingWorked())
			{
				if (pPlot->getYield(eYield) > 0)
				{
					iTempValue *= 2;
					bImportant = true;
				}
			}

			iValue += iTempValue;
			iBestValue = std::max(iBestValue, iTempValue);
		}
	}
	iValue += iBestValue * 2;

	return iValue;
}

// Improved worker AI provided by Blake - thank you!
void CvCityAI::AI_bestPlotBuild(const CvPlot* pPlot, int* piBestValue, BuildTypes* peBestBuild) const
{
	PROFILE_FUNC();
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());

	if (piBestValue != NULL)
	{
		*piBestValue = 0;
	}
	if (peBestBuild != NULL)
	{
		*peBestBuild = NO_BUILD;
	}

	if (pPlot->getWorkingCity() != this)
	{
		return;
	}

	FAssertMsg(pPlot->getOwnerINLINE() == getOwnerINLINE(), "pPlot must be owned by this city's owner");

	BuildTypes eForcedBuild = NO_BUILD;

	{	//If a worker is already building a build, force that Build.
		CLLNode<IDInfo>* pUnitNode;
		CvUnit* pLoopUnit;

		pUnitNode = pPlot->headUnitNode();

		while (pUnitNode != NULL)
		{
			pLoopUnit = ::getUnit(pUnitNode->m_data);
			pUnitNode = pPlot->nextUnitNode(pUnitNode);

			if (pLoopUnit->getBuildType() != NO_BUILD)
			{
				if (GC.getBuildInfo(pLoopUnit->getBuildType()).getImprovement() != NO_IMPROVEMENT)
				{
					eForcedBuild = pLoopUnit->getBuildType();
					break;
				}
			}
		}
	}

	int aiCurrentYields[NUM_YIELD_TYPES];
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		YieldTypes eYield = (YieldTypes)iI;
		aiCurrentYields[iI] = pPlot->calculateNatureYield(eYield, getTeam(), false);
		int iImprovementYieldChange = 0;
		if (pPlot->getImprovementType() != NO_IMPROVEMENT)
		{
			iImprovementYieldChange = pPlot->calculateImprovementYieldChange(pPlot->getImprovementType(), eYield, getOwnerINLINE(), false);
			aiCurrentYields[iI] += iImprovementYieldChange;
		}

		if (pPlot->getRouteType() != NO_ROUTE)
		{
			if (aiCurrentYields[iI] > 0)
			{
				aiCurrentYields[iI] += GC.getRouteInfo(pPlot->getRouteType()).getYieldChange(eYield);
			}
		}
		//Zero out particulary bad yields.
		if (aiCurrentYields[eYield] > 0)
		{
			if ((eYield == YIELD_FOOD) && aiCurrentYields[eYield] <= GC.getFOOD_CONSUMPTION_PER_POPULATION())
			{
				aiCurrentYields[eYield] = 0;
			}
			else if (eYield != YIELD_FOOD && eYield != YIELD_LUMBER && eYield != YIELD_FUR && eYield != YIELD_HYDROCARBONS
				&& (pPlot->getBonusType() == NO_BONUS || GC.getBonusInfo(pPlot->getBonusType()).getYieldChange(eYield) <= 0))
			{
				//Kaszkaj - Keep direct improvement production of advanced goods and Research even when the resource itself does not produce them.
				if (iImprovementYieldChange <= 0 && aiCurrentYields[eYield] <= (kOwner.AI_getBestPlotYield(eYield) / 2))
				{
					aiCurrentYields[eYield] = 0;
				}
			}
		}
	}

	int iCurrentValue = AI_plotYieldValue(pPlot, aiCurrentYields);

	int iBestValue = 0;
	BuildTypes eBestBuild = NO_BUILD;

	int aiFinalYields[NUM_YIELD_TYPES];
	FeatureTypes eFeature = (FeatureTypes)pPlot->getFeatureType();
	ImprovementTypes eImprovement = pPlot->getImprovementType();

	//Kaszkaj - Keep useful lunar and Alien upgrades growing instead of repeatedly rebuilding the same plot.
	bool bGrowingImprovement = AI_improvementUpgradeWorkValue(pPlot) > 0;
	const char* szOldImprovement = eImprovement == NO_IMPROVEMENT ? "" : GC.getImprovementInfo(eImprovement).getType();
	bool bReplaceAlienImprovement = !isHuman() && !isNative()
		&& (std::strcmp(szOldImprovement, "IMPROVEMENT_ALIEN_FERTILIZERS") == 0
			|| std::strcmp(szOldImprovement, "IMPROVEMENT_ALIEN_FUTUREFARM") == 0
			|| std::strcmp(szOldImprovement, "IMPROVEMENT_ALIEN_BURROW") == 0
			|| std::strcmp(szOldImprovement, "IMPROVEMENT_ALIEN_CAVERN") == 0);

	bool bIgnoreFeature = true;
	if (eFeature != NO_FEATURE)
	{
		CvFeatureInfo& kFeature = GC.getFeatureInfo(eFeature);
		for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
		{
			YieldTypes eYield = (YieldTypes)iYield;

			if (kFeature.getYieldChange(eYield) > 0)
			{
				int iYield = pPlot->calculateNatureYield(eYield, getTeam(), false);

				if (iYield > ((kOwner.AI_getBestPlotYield(eYield) * 2) / 3))
				{
					bIgnoreFeature = false;
					break;
				}
			}
		}

		if (pPlot->getBonusType() != NO_BONUS)
		{
			CvBonusInfo& kBonus = GC.getBonusInfo(pPlot->getBonusType());
			for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
			{
				YieldTypes eYield = (YieldTypes)iYield;

				if (kBonus.getYieldChange(eYield) > 0)
				{
					if (kFeature.getYieldChange(eYield) < 0)
					{
						bIgnoreFeature = true;
						break;
					}
					else if (kBonus.isFeature(eFeature))
					{
						bIgnoreFeature = false;
					}
				}
			}
		}
	}

	for (int iI = 0; iI < GC.getNumBuildInfos(); iI++)
	{
		BuildTypes eBuild = (BuildTypes)iI;

		CvBuildInfo& kBuild = GC.getBuildInfo(eBuild);
		bool bValid = GET_PLAYER(getOwnerINLINE()).canBuild(pPlot, eBuild, true, true);
		if (bGrowingImprovement && kBuild.getImprovement() != NO_IMPROVEMENT && kBuild.getImprovement() != eImprovement) bValid = false;

		if (bValid && eForcedBuild != NO_BUILD && eForcedBuild != eBuild)
		{
			CvBuildInfo& kForcedBuild = GC.getBuildInfo(eForcedBuild);

			if ((kBuild.getImprovement() != NO_IMPROVEMENT) && (kForcedBuild.getImprovement() != NO_IMPROVEMENT))
			{
				bValid = false;
			}
			else if ((kBuild.getRoute() != NO_ROUTE) && (kForcedBuild.getRoute() != NO_ROUTE))
			{
				bValid = false;
			}
			else if ((eFeature != NO_FEATURE) && kBuild.isFeatureRemove(eFeature) && kForcedBuild.isFeatureRemove(eFeature))
			{
				bValid = false;
			}
		}

		if (bValid)
		{
			bool bCaution = false;

			ImprovementTypes eImprovement = (ImprovementTypes)kBuild.getImprovement();
			ImprovementTypes eYieldImprovement = eImprovement;
			for (int iUpgrade = 0; eYieldImprovement != NO_IMPROVEMENT && iUpgrade < 2; ++iUpgrade)
			{
				ImprovementTypes eUpgrade = (ImprovementTypes)GC.getImprovementInfo(eYieldImprovement).getImprovementUpgrade();
				if (eUpgrade == NO_IMPROVEMENT) break;
				eYieldImprovement = eUpgrade;
			}
			RouteTypes eRoute = (RouteTypes)kBuild.getRoute();
			bool bRemoveFeature = false;
			if (eFeature != NO_FEATURE)
			{
				bRemoveFeature = kBuild.isFeatureRemove(eFeature);
			}

			for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
			{
				YieldTypes eYield = (YieldTypes)iYield;
				aiFinalYields[eYield] = pPlot->getYieldWithBuild(eBuild, eYield, true);

				//Zero out particulary bad yields.
				if (aiFinalYields[eYield] > 0)
				{
					if ((eYield == YIELD_FOOD) && aiFinalYields[eYield] <= GC.getFOOD_CONSUMPTION_PER_POPULATION())
					{
						aiFinalYields[eYield] = 0;
					}
					else if (eYield != YIELD_FOOD && eYield != YIELD_LUMBER && eYield != YIELD_FUR && eYield != YIELD_HYDROCARBONS
						&& (pPlot->getBonusType() == NO_BONUS || GC.getBonusInfo(pPlot->getBonusType()).getYieldChange(eYield) <= 0))
					{
						//Kaszkaj - Include direct production from the same improvement upgrade used to assess the completed Build.
						if (aiFinalYields[eYield] <= (kOwner.AI_getBestPlotYield(eYield) / 2)
							&& (eYieldImprovement == NO_IMPROVEMENT || pPlot->calculateImprovementYieldChange(eYieldImprovement, eYield, getOwnerINLINE(), false) <= 0))
						{
							aiFinalYields[eYield] = 0;
						}
					}
				}
			}

			int iValue = AI_plotYieldValue(pPlot, aiFinalYields);
			//Kaszkaj - Colonial workers prefer their own productive improvements over inherited Alien Fertilizers, Future Farms and Alien Burrows.
			if (bReplaceAlienImprovement && eImprovement != NO_IMPROVEMENT
				&& std::strncmp(GC.getImprovementInfo(eImprovement).getType(), "IMPROVEMENT_ALIEN_", 18) != 0)
			{
				int iProductiveYields = 0;
				for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield) iProductiveYields += aiFinalYields[iYield];
				if (iProductiveYields > 0 && iValue >= iCurrentValue * 3 / 4) iValue += 100 + iCurrentValue / 3;
			}

			if (kBuild.getRoute() != NO_ROUTE)
			{
				if (pPlot->getCrumbs() > 0)
				{
					bool bValid = true;
					for (int i = 0; i < NUM_CARDINALDIRECTION_TYPES; ++i)
					{
						CvPlot* pLoopPlot = ::plotCardinalDirection(pPlot->getX_INLINE(), pPlot->getY_INLINE(), (CardinalDirectionTypes)i);
						if (pLoopPlot != NULL && pLoopPlot->getCrumbs() > 2 * pPlot->getCrumbs())
						{
							bValid = false;
							break;
						}
					}
					if (bValid)
					{
						iValue += 100;
					}
				}
			}

			if ((eFeature != NO_FEATURE) && bIgnoreFeature && (eImprovement != NO_IMPROVEMENT))
			{
				CvImprovementInfo& kImprovement = GC.getImprovementInfo(eImprovement);
				if (kImprovement.getFeatureMakesValid(eFeature))
				{
					iValue /= 2;
				}
				else if (bRemoveFeature)
				{
					iValue *= 2;
					if (pPlot->getBonusType() != NO_BONUS)
					{
						iValue *= 2;
					}
				}
			}

			if (iValue > iCurrentValue)
			{
				if (bRemoveFeature)
				{
					bool bUnique = true;
					for (int i = 0; i < NUM_CITY_PLOTS; ++i)
					{
						CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), i);
						if (pLoopPlot != NULL && pLoopPlot != pPlot)
						{
							if (pLoopPlot->getFeatureType() == eFeature)
							{
								bUnique = false;
								break;
							}
						}
					}

					if (bUnique)
					{
						CvFeatureInfo& kFeature = GC.getFeatureInfo(eFeature);
						for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
						{
							YieldTypes eYield = (YieldTypes)iYield;
							if (kFeature.getYieldChange(eYield) > 0)
							{
								int iBestYield = 0;
								CvPlot* pBestYieldPlot = kOwner.AI_getBestWorkedYieldPlot(eYield);
								if (pBestYieldPlot != NULL)
								{
									iBestYield = std::max(iBestYield, pBestYieldPlot->calculateBestNatureYield(eYield, getTeam()));
								}

								pBestYieldPlot = kOwner.AI_getBestUnworkedYieldPlot(eYield);
								if (pBestYieldPlot != NULL)
								{
									iBestYield = std::max(iBestYield, pBestYieldPlot->calculateBestNatureYield(eYield, getTeam()));
								}

								if (pPlot->calculateBestNatureYield(eYield, getTeam()) >= iBestYield)
								{
									iValue *= 75;
									iValue /= 100;
								}
							}
						}
					}
				}

				if (iValue > iCurrentValue)
				{
					if (!isHuman())
					{
						if (eImprovement != NO_IMPROVEMENT)
						{
							iValue *= std::max(0, (GC.getLeaderHeadInfo(getPersonalityType()).getImprovementWeightModifier(eImprovement) + 100));
							iValue /= 100;
						}
					}

					if (eFeature != NO_FEATURE)
					{
						if (kBuild.isFeatureRemove(eFeature))
						{
							CvCity* pCity = NULL;
							for (int iYield = 0; iYield < NUM_YIELD_TYPES; ++iYield)
							{
								YieldTypes eYield = (YieldTypes) iYield;
								if (GC.getYieldInfo(eYield).isCargo())
								{
									iValue += pPlot->getFeatureYield(eBuild, eYield, getTeam(), &pCity) * 2;
								}
							}
							FAssert(pCity == this);
							//XXX update this once chops are saner (likely the chop yield type is defined)

							if (pPlot->getBonusType() != NO_BONUS)
							{
								iValue /= 2;
								//XXX Traditionally in Col, removing a feature destroys the bonus.
							}
						}
					}

					if (eBuild == eForcedBuild)
					{
						iValue *= 125;
						iValue /= 100;
					}

					if (iValue > iBestValue)
					{
						iBestValue = iValue;
						eBestBuild = eBuild;
					}
				}
			}
		}
	}

	if (eBestBuild != NO_BUILD)
	{
		CvBuildInfo& kBestBuild = GC.getBuildInfo(eBestBuild);
		if (eFeature != NO_FEATURE)
		{
			if (kBestBuild.isFeatureRemove(eFeature))
			{
				int iBestTime = kBestBuild.getTime();

				for (int iBuild = 0; iBuild < GC.getNumBuildInfos(); ++iBuild)
				{
					CvBuildInfo& kLoopBuild = GC.getBuildInfo((BuildTypes)iBuild);
					if (kLoopBuild.isFeatureRemove(eFeature))
					{
						if (kLoopBuild.getTime() < iBestTime)
						{
							eBestBuild = (BuildTypes)iBuild;
							iBestTime = kLoopBuild.getTime();
						}
					}
				}
			}
		}
	}

	if (eBestBuild != NO_BUILD)
	{
		FAssertMsg(iBestValue > 0, "iBestValue is expected to be greater than 0");

		if (piBestValue != NULL)
		{
			*piBestValue = iBestValue;
		}
		if (peBestBuild != NULL)
		{
			*peBestBuild = eBestBuild;
		}
	}
}

int CvCityAI::AI_cityValue() const
{

	AreaAITypes eAreaAI = area()->getAreaAIType(getTeam());
    if ((eAreaAI == AREAAI_OFFENSIVE) || (eAreaAI == AREAAI_MASSING) || (eAreaAI == AREAAI_DEFENSIVE))
    {
        return 0;
    }

	int iValue = 0;

	iValue += getYieldRate(YIELD_BELLS);
	iValue += getYieldRate(YIELD_HAMMERS);

	return iValue;
}

int CvCityAI::AI_calculateCulturePressure() const
{
    int iValue = 0;
    for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
		CvPlot* pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
		if (pLoopPlot != NULL)
		{
		    if (pLoopPlot->getOwnerINLINE() == NO_PLAYER)
		    {
		        iValue++;
		    }
		    else
		    {
				int iTempValue = pLoopPlot->calculateCulturePercent(getOwnerINLINE());
                if (iTempValue == 100)
                {
                    //do nothing
                }
                else if ((iTempValue == 0) || (iTempValue > 75))
                {
                    iValue++;
                }
                else
                {
                    iTempValue = (100 - iTempValue);
                    FAssert(iTempValue > 0);
                    FAssert(iTempValue <= 100);

                    if (iI != CITY_HOME_PLOT)
                    {
                        iTempValue *= 4;
                        iTempValue /= NUM_CITY_PLOTS;
                    }

                    if ((iTempValue > 80) && (pLoopPlot->getOwnerINLINE() == getID()))
                    {
                        //captured territory special case
                        iTempValue *= (100 - iTempValue);
                        iTempValue /= 100;
                    }

                    if (pLoopPlot->getTeam() == getTeam())
                    {
                        iTempValue /= 2;
                    }
                    else
                    {
                        iTempValue *= 2;
                    }

                    iValue += iTempValue;
                }
            }
		}
    }


    return iValue;
}

int CvCityAI::AI_calculateWaterWorldPercent() const
{
    int iI;
    int iWaterPercent = 0;
    int iTeamCityCount = 0;
	int iOtherCityCount = 0;
	for (iI = 0; iI < MAX_TEAMS; iI++)
	{
		if (GET_TEAM((TeamTypes)iI).isAlive())
		{
			if (iI == getTeam())
			{
				iTeamCityCount += GET_TEAM((TeamTypes)iI).countNumCitiesByArea(area());
			}
			else
			{
				iOtherCityCount += GET_TEAM((TeamTypes)iI).countNumCitiesByArea(area());
			}
		}
	}

    if (iOtherCityCount == 0)
    {
        iWaterPercent = 100;
    }
    else
    {
        iWaterPercent = 100 - ((iTeamCityCount + iOtherCityCount) * 100) / std::max(1, (GC.getGame().getNumCities()));
    }

    iWaterPercent *= 50;
    iWaterPercent /= 100;

    iWaterPercent += (50 * (2 + iTeamCityCount)) / (2 + iTeamCityCount + iOtherCityCount);

    iWaterPercent = std::max(1, iWaterPercent);


    return iWaterPercent;
}

//Please note, takes the yield multiplied by 100
int CvCityAI::AI_getYieldMagicValue(const int* piYieldsTimes100) const
{
	FAssert(piYieldsTimes100 != NULL);

    int iPopEats = GC.getFOOD_CONSUMPTION_PER_POPULATION();
    iPopEats *= 100;

    int iValue = (piYieldsTimes100[YIELD_FOOD] * 100 - iPopEats * 102);
    iValue /= 100;
    return iValue;
}

//The magic value is basically "Look at this plot, is it worth working"
//-50 or lower means the plot is worthless in a "workers kill yourself" kind of way.
//-50 to -1 means the plot isn't worth growing to work - might be okay with emphasize though.
//Between 0 and 50 means it is marginal.
//50-100 means it's okay.
//Above 100 means it's definitely decent - seriously question ever not working it.
//This function deliberately doesn't use emphasize settings.
int CvCityAI::AI_getPlotMagicValue(const CvPlot* pPlot, bool bWorkerOptimization) const
{
    int aiYields[NUM_YIELD_TYPES];
    ImprovementTypes eCurrentImprovement;
    ImprovementTypes eFinalImprovement;
    int iI;
    int iYieldDiff;

    FAssert(pPlot != NULL);

    for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
    {
    	if ((bWorkerOptimization) && (pPlot->getWorkingCity() == this) && (AI_getBestBuild(getCityPlotIndex(pPlot)) != NO_BUILD))
    	{
    		aiYields[iI] = pPlot->getYieldWithBuild(AI_getBestBuild(getCityPlotIndex(pPlot)), (YieldTypes)iI, true);
    	}
    	else
    	{
        	aiYields[iI] = pPlot->calculatePotentialYield((YieldTypes)iI, NULL, false) * 100;
    	}
    }

    eCurrentImprovement = pPlot->getImprovementType();

    if (eCurrentImprovement != NO_IMPROVEMENT)
    {
        eFinalImprovement = finalImprovementUpgrade(eCurrentImprovement);

        if ((eFinalImprovement != NO_IMPROVEMENT) && (eFinalImprovement != eCurrentImprovement))
        {
            for (iI = 0; iI < NUM_YIELD_TYPES; iI++)
            {
                iYieldDiff = 100 * pPlot->calculateImprovementYieldChange(eFinalImprovement, ((YieldTypes)iI), getOwnerINLINE());
                iYieldDiff -= 100 * pPlot->calculateImprovementYieldChange(eCurrentImprovement, ((YieldTypes)iI), getOwnerINLINE());
                aiYields[iI] += iYieldDiff / 2;
            }
        }
    }

    return AI_getYieldMagicValue(aiYields);
}

//useful for deciding whether or not to grow... or whether the city needs terrain
//improvement.
int CvCityAI::AI_countGoodTiles(bool bUnworkedOnly, int iThreshold, bool bWorkerOptimization) const
{
    CvPlot* pLoopPlot;
    int iI;
    int iCount;

    iCount = 0;
    for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
        pLoopPlot = plotCity(getX_INLINE(),getY_INLINE(), iI);
        if ((iI != CITY_HOME_PLOT) && (pLoopPlot != NULL))
        {
            if (pLoopPlot->getWorkingCity() == this)
            {
                if (!bUnworkedOnly || !(pLoopPlot->isBeingWorked()))
                {
                    if (AI_getPlotMagicValue(pLoopPlot) > iThreshold)
                    {
                        iCount++;
                    }
                }
            }
        }
    }
    return iCount;
}

int CvCityAI::AI_calculateTargetCulturePerTurn()
{
	return 1;
}

// +1/+3/+5 plot based on base food yield (1/2/3)
// +4 if being worked.
// +4 if a bonus.
// Unworked ocean ranks very lowly. Unworked lake ranks at 3. Worked lake at 7.
// Worked bonus in ocean ranks at like 11
int CvCityAI::AI_buildingSpecialYieldChangeValue(BuildingTypes eBuilding, YieldTypes eYield) const
{
    int iI;
    CvPlot* pLoopPlot;
    int iValue = 0;
    CvBuildingInfo& kBuilding = GC.getBuildingInfo(eBuilding);
    int iWorkedCount = 0;

    int iYieldChange = kBuilding.getSeaPlotYieldChange(eYield);
    if (iYieldChange > 0)
    {
        int iWaterCount = 0;
        for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
        {
            if (iI != CITY_HOME_PLOT)
            {
                pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);
                if ((pLoopPlot != NULL) && (pLoopPlot->getWorkingCity() == this))
                {
                    if (pLoopPlot->isWater())
                    {
                        iWaterCount++;
                        int iFood = pLoopPlot->calculatePotentialYield(YIELD_FOOD, NULL, false);
                        iFood += (eYield == YIELD_FOOD) ? iYieldChange : 0;

                        iValue += std::max(0, iFood * 2 - 1);
                        if (pLoopPlot->isBeingWorked())
                        {
                        	iValue += 4;
                        	iWorkedCount++;
                        }
                        iValue += ((pLoopPlot->getBonusType() != NO_BONUS) ? 8 : 0);
                    }
                }
            }
        }
    }
    if (iWorkedCount == 0)
    {
		if (getPopulation() > 2)
		{
			iValue /= 2;
		}
    }

    return iValue;
}

int CvCityAI::AI_countNumBonuses(BonusTypes eBonus, bool bIncludeOurs, bool bIncludeNeutral, int iOtherCultureThreshold, bool bLand, bool bWater) const
{
    CvPlot* pLoopPlot;
    BonusTypes eLoopBonus;
    int iI;
    int iCount = 0;
    for (iI = 0; iI < NUM_CITY_PLOTS; iI++)
    {
        pLoopPlot = plotCity(getX_INLINE(), getY_INLINE(), iI);

        if (pLoopPlot != NULL)
        {
        	if ((pLoopPlot->area() == area()) || (bWater && pLoopPlot->isWater()))
        	{
				eLoopBonus = pLoopPlot->getBonusType();
				if (eLoopBonus != NO_BONUS)
				{
					if ((eBonus == NO_BONUS) || (eBonus == eLoopBonus))
					{
						if (bIncludeOurs && (pLoopPlot->getOwnerINLINE() == getOwnerINLINE()) && (pLoopPlot->getWorkingCity() == this))
						{
							iCount++;
						}
						else if (bIncludeNeutral && (!pLoopPlot->isOwned()))
						{
							iCount++;
						}
						else if ((iOtherCultureThreshold > 0) && (pLoopPlot->isOwned() && (pLoopPlot->getOwnerINLINE() != getOwnerINLINE())))
						{
							if ((pLoopPlot->getCulture(pLoopPlot->getOwnerINLINE()) - pLoopPlot->getCulture(getOwnerINLINE())) < iOtherCultureThreshold)
							{
								iCount++;
							}
						}
					}
				}
        	}
        }
    }


    return iCount;

}

int CvCityAI::AI_playerCloseness(PlayerTypes eIndex, int iMaxDistance) const
{
	FAssert(GET_PLAYER(eIndex).isAlive());
	FAssert(eIndex != getID());

	if ((m_iCachePlayerClosenessTurn != GC.getGame().getGameTurn())
		|| (m_iCachePlayerClosenessDistance != iMaxDistance))
	{
		AI_cachePlayerCloseness(iMaxDistance);
	}

	return m_aiPlayerCloseness[eIndex];
}

void CvCityAI::AI_cachePlayerCloseness(int iMaxDistance) const
{
	PROFILE_FUNC();
	CvCity* pLoopCity;
	int iI;
	int iLoop;
	int iValue;
	int iTempValue;
	int iBestValue;

	for (iI = 0; iI < MAX_PLAYERS; iI++)
	{
		if (GET_PLAYER((PlayerTypes)iI).isAlive() &&
			((GET_TEAM(getTeam()).isHasMet(GET_PLAYER((PlayerTypes)iI).getTeam()))))
		{
			iValue = 0;
			iBestValue = 0;
			for (pLoopCity = GET_PLAYER((PlayerTypes)iI).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iI).nextCity(&iLoop))
			{
				int iDistance = stepDistance(getX_INLINE(), getY_INLINE(), pLoopCity->getX_INLINE(), pLoopCity->getY_INLINE());
				if (area() != pLoopCity->area())
				{
					iDistance += 1;
					iDistance /= 2;
				}
				if (iDistance <= iMaxDistance)
				{
					if (getArea() == pLoopCity->getArea())
					{
						int iPathDistance = GC.getMap().calculatePathDistance(plot(), pLoopCity->plot());
						if (iPathDistance > 0)
						{
							iDistance = iPathDistance;
						}
						else
						{

						}
					}
					if (iDistance <= iMaxDistance)
					{
						iTempValue = 20 + pLoopCity->getPopulation() * 2;
						iTempValue *= (1 + (iMaxDistance - iDistance));
						iTempValue /= (1 + iMaxDistance);

						//reduce for small islands.
						int iAreaCityCount = pLoopCity->area()->getNumCities();
						iTempValue *= std::min(iAreaCityCount, 5);
						iTempValue /= 5;
						if (iAreaCityCount < 3)
						{
							iTempValue /= 2;
						}

						iValue += iTempValue;
						iBestValue = std::max(iBestValue, iTempValue);
					}
				}
			}
			m_aiPlayerCloseness[iI] = (iBestValue + iValue / 4);
		}
	}

	m_iCachePlayerClosenessTurn = GC.getGame().getGameTurn();
	m_iCachePlayerClosenessDistance = iMaxDistance;
}

int CvCityAI::AI_cityThreat(bool bDangerPercent) const
{
	PROFILE_FUNC();
	int iValue = 0;

	for (int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		//Kaszkaj - Base Colony danger on nearby rivals, war plans and relations; ignore teammates.
		if (GET_PLAYER((PlayerTypes)iI).isAlive() && GET_PLAYER((PlayerTypes)iI).getTeam() != getTeam())
		{
			int iTempValue = AI_playerCloseness((PlayerTypes)iI, 5);

			// TAC - AI City Defense - koma13 - START
			if (iTempValue > 0)
			{
				if (atWar(getTeam(), GET_PLAYER((PlayerTypes)iI).getTeam()))
				{
					iTempValue *= 300;
				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                      01/04/09                                jdog5000      */
/*                                                                                              */
/* War tactics AI                                                                               */
/************************************************************************************************/
				// Beef up border security before starting war, but not too much
				else if ( GET_TEAM(getTeam()).AI_getWarPlan(GET_PLAYER((PlayerTypes)iI).getTeam()) != NO_WARPLAN )
				{
					iTempValue *= 180;
				}
				// Extra trust of Vassals, regardless of relations
//				else if ( GET_TEAM(GET_PLAYER((PlayerTypes)iI).getTeam()).isVassal(getTeam()) )
//				{
//					iTempValue *= 30;
//				}
/************************************************************************************************/
/* BETTER_BTS_AI_MOD                       END                                                  */
/************************************************************************************************/
				else
				{
					switch (GET_PLAYER(getOwnerINLINE()).AI_getAttitude((PlayerTypes)iI))
					{
					case ATTITUDE_FURIOUS:
						iTempValue *= 180;
						break;

					case ATTITUDE_ANNOYED:
						iTempValue *= 130;
						break;

					case ATTITUDE_CAUTIOUS:
						iTempValue *= 100;
						break;

					case ATTITUDE_PLEASED:
						iTempValue *= 50;
						break;

					case ATTITUDE_FRIENDLY:
						iTempValue *= 20;
						break;

					default:
						FAssert(false);
						break;
					}
				}

				iTempValue /= 100;
				iValue += iTempValue;
			}
			// TAC - AI City Defense - koma13 - END
		}
	}

	if (isCoastal(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
	{
		iValue += 6;
	}

	iValue += 2 * GET_PLAYER(getOwnerINLINE()).AI_getPlotDanger(plot(), 3, false);

	return iValue;
}

//Workers have/needed is not intended to be a strict
//target but rather an indication.
//if needed is at least 1 that means a worker
//will be doing something useful
int CvCityAI::AI_getWorkersHave() const
{
	return m_iWorkersHave;
}

int CvCityAI::AI_getWorkersNeeded() const
{
	return m_iWorkersNeeded;
}

void CvCityAI::AI_changeWorkersHave(int iChange)
{
	m_iWorkersHave += iChange;
	m_iWorkersHave = std::max(0, m_iWorkersHave);
}

//This needs to be serialized for human workers.
void CvCityAI::AI_updateWorkersNeededHere()
{
	CvPlot* pLoopPlot;

	int iWorkedUnimprovedCount = 0;
	int iUnimprovedBonusCount = 0;

	int iValue = 0;

	for (int iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = getCityIndexPlot(iI);

		if (NULL != pLoopPlot && pLoopPlot->getWorkingCity() == this)
		{
			if (pLoopPlot->getArea() == getArea())
			{
				if (iI != CITY_HOME_PLOT)
				{
					if (AI_getBestBuild(iI) != NO_BUILD)
					{
						if (pLoopPlot->isBeingWorked())
						{
							iValue += 40;
							if (pLoopPlot->getBonusType() != NO_BONUS)
							{
								iValue += 60;
							}
						}
						else
						{
							if (pLoopPlot->getBonusType() != NO_BONUS)
							{
								iValue += 50;
							}
						}
					}
				}
			}
		}
	}

	if (iValue == 0)
	{
		m_iWorkersNeeded = 0;
	}
	else
	{
		m_iWorkersNeeded = std::max(1, iValue / 100);
	}
}

BuildingTypes CvCityAI::AI_bestAdvancedStartBuilding(int iPass) const
{
	return AI_bestBuildingThreshold(0, 0, std::max(0, 20 - iPass * 5));
}

void CvCityAI::AI_educateStudent(int iUnitId)
{
	CvPlayerAI& kOwner = GET_PLAYER(getOwnerINLINE());
	//Kaszkaj - Alien AI graduates choose a useful teachable specialist for free.
	if (isNative() && !isHuman())
	{
		CvUnit* pStudent = getPopulationUnitById(iUnitId);
		if (pStudent == NULL || getEducationTurnsLeft(pStudent, pStudent->getProfession()) != 0)
		{
			return;
		}
		UnitTypes eBestSpecialist = NO_UNIT;
		int iBestSpecialistValue = 0;
		for (int i = 0; i < GC.getNumUnitInfos(); ++i)
		{
			UnitTypes eUnit = (UnitTypes)i;
			if (eUnit == pStudent->getUnitType() || getSpecialistTuition(eUnit) != 0)
			{
				continue;
			}
			int iValue = 100 * (1 + kOwner.AI_educationUnitValue(eUnit))
				+ GC.getGameINLINE().getSorenRandNum(100, "AI native educate unit");
			if (iValue > iBestSpecialistValue)
			{
				iBestSpecialistValue = iValue;
				eBestSpecialist = eUnit;
			}
		}
		if (eBestSpecialist != NO_UNIT)
		{
			educateStudent(iUnitId, eBestSpecialist);
		}
		return;
	}

	UnitTypes eBestUnit = NO_UNIT;
	int iBestValue = 0;
	for (int i = 0; i < GC.getNumUnitInfos(); ++i)
	{
		int iTuition = getSpecialistTuition((UnitTypes) i);
		if (GET_PLAYER(getOwnerINLINE()).getGold() >= iTuition && iTuition >= 0)
		{
			UnitTypes eLoopUnit = (UnitTypes)i;
			CvUnitInfo& kUnit = GC.getUnitInfo(eLoopUnit);

			int iValue = 50;

			int iNeeded = kOwner.AI_getNumCityUnitsNeeded(eLoopUnit);
			int iHave = kOwner.getUnitClassCount((UnitClassTypes)kUnit.getUnitClassType());

			if (iNeeded < iHave)
			{
				iValue += 100 + 50 * (iNeeded - iHave);

				iValue *= 100 + kOwner.AI_getUnitYieldModifier(eLoopUnit, YIELD_BELLS);
				iValue /= 100;

				iValue *= 100 + (3 * kOwner.AI_getUnitYieldModifier(eLoopUnit, YIELD_HAMMERS) / 2);
				iValue /= 100;
			}
			else
			{
				ProfessionTypes eIdealProfession = kOwner.AI_idealProfessionForUnit(eLoopUnit);
				if (eIdealProfession != NO_PROFESSION)
				{
					// MultipleYieldsProduced Start by Aymerick 22/01/2010**
					YieldTypes eYieldProducedType = (YieldTypes)GC.getProfessionInfo(eIdealProfession).getYieldsProduced(0);
					// MultipleYieldsProduced End
					if (eYieldProducedType != NO_YIELD)
					{
						if (eYieldProducedType == YIELD_FOOD)
						{
							iValue *= 200;
							iValue /= 100;
						}
					}
				}
			}

			iValue *= 40 + GC.getGameINLINE().getSorenRandNum(60, "AI best educate unit");

			if (iValue > iBestValue)
			{
				iBestValue = iValue;
				eBestUnit = eLoopUnit;
			}
		}
	}

	if (eBestUnit != NO_UNIT)
	{
		educateStudent(iUnitId, eBestUnit);
	}
}

//This suppresses certain checks to all the workforce allocation algorithm to run
//more smoothly (ignore time-consuming checks and swaps)
void CvCityAI::AI_setWorkforceHack(bool bNewValue)
{
	m_iWorkforceHack += (bNewValue ? 1 : -1);
}

bool CvCityAI::AI_isWorkforceHack()
{
	return (m_iWorkforceHack > 0);
}


bool CvCityAI::AI_isMajorCity() const
{
	for (int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
	{
		if (AI_getYieldAdvantage((YieldTypes)iYield) >= 100)
		{
			return true;
		}
	}

	CvPlayer& kOwner = GET_PLAYER(getOwnerINLINE());
	int iHigherPopulationCount = 0;

	int iLoop;
	for (CvCity* pLoopCity = kOwner.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kOwner.nextCity(&iLoop))
	{
		if (pLoopCity != this)
		{
			if (pLoopCity->getPopulation() > getPopulation())
			{
				iHigherPopulationCount++;
			}
			else if (pLoopCity->getPopulation() == getPopulation())
			{
				if (pLoopCity->getGameTurnAcquired() < getGameTurnAcquired())
				{
					iHigherPopulationCount++;
				}
			}
		}
	}

	if (100 * iHigherPopulationCount / kOwner.getNumCities() <= 20)
	{
		return true;
	}

	return false;
}


//
//
//
void CvCityAI::read(FDataStreamBase* pStream)
{
	CvCity::read(pStream);

	uint uiFlag=0;
	pStream->Read(&uiFlag);	// flags for expansion

	pStream->Read(&m_iGiftTimer);
	pStream->Read((int*)&m_eDesiredYield);
	pStream->Read(&m_iTargetSize);
	pStream->Read(&m_iFoundValue);

	pStream->Read(NUM_YIELD_TYPES, m_aiYieldOutputWeight);
	pStream->Read(NUM_YIELD_TYPES, m_aiNeededYield);
	pStream->Read(NUM_YIELD_TYPES, m_aiTradeBalance);
	pStream->Read(NUM_YIELD_TYPES, m_aiYieldAdvantage);

	pStream->Read(&m_iEmphasizeAvoidGrowthCount);

	pStream->Read(&m_bPort);
	pStream->Read(&m_bAssignWorkDirty);
	pStream->Read(&m_bChooseProductionDirty);

	m_routeToCity.read(pStream);

	pStream->Read(NUM_YIELD_TYPES, m_aiEmphasizeYieldCount);
	pStream->Read(&m_bForceEmphasizeCulture);
	pStream->Read(NUM_CITY_PLOTS, m_aiBestBuildValue);
	pStream->Read(NUM_CITY_PLOTS, (int*)m_aeBestBuild);
	pStream->Read(GC.getNumEmphasizeInfos(), m_abEmphasize);
	pStream->Read(&m_iCachePlayerClosenessTurn);
	pStream->Read(&m_iCachePlayerClosenessDistance);
	pStream->Read(MAX_PLAYERS, m_aiPlayerCloseness);
	pStream->Read(&m_iNeededFloatingDefenders);
	pStream->Read(&m_iNeededFloatingDefendersCacheTurn);
	pStream->Read(&m_iWorkersNeeded);
	pStream->Read(&m_iWorkersHave);
}

//
//
//
void CvCityAI::write(FDataStreamBase* pStream)
{
	CvCity::write(pStream);

	uint uiFlag=0;
	pStream->Write(uiFlag);		// flag for expansion

	pStream->Write(m_iGiftTimer);
	pStream->Write(m_eDesiredYield);
	pStream->Write(m_iTargetSize);
	pStream->Write(m_iFoundValue);

	pStream->Write(NUM_YIELD_TYPES, m_aiYieldOutputWeight);
	pStream->Write(NUM_YIELD_TYPES, m_aiNeededYield);
	pStream->Write(NUM_YIELD_TYPES, m_aiTradeBalance);
	pStream->Write(NUM_YIELD_TYPES, m_aiYieldAdvantage);

	pStream->Write(m_iEmphasizeAvoidGrowthCount);

	pStream->Write(m_bPort);
	pStream->Write(m_bAssignWorkDirty);
	pStream->Write(m_bChooseProductionDirty);

	m_routeToCity.write(pStream);

	pStream->Write(NUM_YIELD_TYPES, m_aiEmphasizeYieldCount);
	pStream->Write(m_bForceEmphasizeCulture);
	pStream->Write(NUM_CITY_PLOTS, m_aiBestBuildValue);
	pStream->Write(NUM_CITY_PLOTS, (int*)m_aeBestBuild);
	pStream->Write(GC.getNumEmphasizeInfos(), m_abEmphasize);
	pStream->Write(m_iCachePlayerClosenessTurn);
	pStream->Write(m_iCachePlayerClosenessDistance);
	pStream->Write(MAX_PLAYERS, m_aiPlayerCloseness);
	pStream->Write(m_iNeededFloatingDefenders);
	pStream->Write(m_iNeededFloatingDefendersCacheTurn);
	pStream->Write(m_iWorkersNeeded);
	pStream->Write(m_iWorkersHave);
}
