## Sid Meier's Civilization 4
## Copyright Firaxis Games 2006
##
## CvEventManager
## This class is passed an argsList from CvAppInterface.onEvent
## The argsList can contain anything from mouse location to key info
## The EVENTLIST that are being notified can be found

from CvPythonExtensions import *
import CvUtil
import CvScreensInterface
import CvDebugTools
import CvWBPopups
import CvCameraControls
import sys
import CvWorldBuilderScreen
import CvAdvisorUtils

gc = CyGlobalContext()
localText = CyTranslator()

# globals
###################################################
class CvEventManager:
	def __init__(self):
		#################### ON EVENT MAP ######################
		self.bCtrl = False
		self.bShift = False
		self.bAlt = False
		self.bAllowCheats = False

		# OnEvent Enums
		self.EventLButtonDown=1
		self.EventLcButtonDblClick=2
		self.EventRButtonDown=3
		self.EventBack=4
		self.EventForward=5
		self.EventKeyDown=6
		self.EventKeyUp=7

		self.__LOG_MOVEMENT = 0
		self.__LOG_BUILDING = 0
		self.__LOG_COMBAT = 0
		self.__LOG_CONTACT = 0
		self.__LOG_IMPROVEMENT =0
		self.__LOG_CITYLOST = 0
		self.__LOG_CITYBUILDING = 0
		self.__LOG_UNITBUILD = 0
		self.__LOG_UNITKILLED = 1
		self.__LOG_UNITLOST = 0
		self.__LOG_UNITPROMOTED = 0
		self.__LOG_UNITSELECTED = 0
		self.__LOG_UNITPILLAGE = 0
		self.__LOG_GOODYRECEIVED = 0
		self.__LOG_WARPEACE = 0
		self.__LOG_PUSH_MISSION = 0

		## EVENTLIST
		self.EventHandlerMap = {
			'mouseEvent'			: self.onMouseEvent,
			'kbdEvent' 				: self.onKbdEvent,
			'ModNetMessage'			: self.onModNetMessage,
			'Init'					: self.onInit,
			'Update'				: self.onUpdate,
			'UnInit'				: self.onUnInit,
			'OnSave'				: self.onSaveGame,
			'OnPreSave'				: self.onPreSave,
			'OnLoad'				: self.onLoadGame,
			'GameStart'				: self.onGameStart,
			'GameEnd'				: self.onGameEnd,
			'plotRevealed' 			: self.onPlotRevealed,
			'plotFeatureRemoved' 	: self.onPlotFeatureRemoved,
			'plotPicked'			: self.onPlotPicked,
			'gotoPlotSet'			: self.onGotoPlotSet,
			'BeginGameTurn'			: self.onBeginGameTurn,
			'EndGameTurn'			: self.onEndGameTurn,
			'BeginPlayerTurn'		: self.onBeginPlayerTurn,
			'EndPlayerTurn'			: self.onEndPlayerTurn,
			'endTurnReady'			: self.onEndTurnReady,
			'combatResult' 			: self.onCombatResult,
			'combatLogCalc'	 		: self.onCombatLogCalc,
			'combatLogHit'			: self.onCombatLogHit,
			'improvementBuilt' 		: self.onImprovementBuilt,
			'improvementDestroyed' 	: self.onImprovementDestroyed,
			'routeBuilt' 			: self.onRouteBuilt,
			'firstContact' 			: self.onFirstContact,
			'cityBuilt' 			: self.onCityBuilt,
			'cityRazed'				: self.onCityRazed,
			'cityAcquired' 			: self.onCityAcquired,
			'cityAcquiredAndKept' 	: self.onCityAcquiredAndKept,
			'cityLost'				: self.onCityLost,
			'cultureExpansion' 		: self.onCultureExpansion,
			'cityGrowth' 			: self.onCityGrowth,
			'cityDoTurn' 			: self.onCityDoTurn,
			'cityBuildingUnit'		: self.onCityBuildingUnit,
			'cityBuildingBuilding'	: self.onCityBuildingBuilding,
			'cityRename'			: self.onCityRename,
			'createTradeRoute'		: self.onCreateTradeRoute,
			'editTradeRoute'		: self.onEditTradeRoute,
			'cityHurry'				: self.onCityHurry,
			'selectionGroupPushMission'		: self.onSelectionGroupPushMission,
			'unitMove' 				: self.onUnitMove,
			'unitSetXY' 			: self.onUnitSetXY,
			'unitCreated' 			: self.onUnitCreated,
			'unitBuilt' 			: self.onUnitBuilt,
			'unitKilled'			: self.onUnitKilled,
			'unitLost'				: self.onUnitLost,
			'unitPromoted'			: self.onUnitPromoted,
			'unitSelected'			: self.onUnitSelected,
			'missionaryConvertedUnit' : self.onMissionaryConvertedUnit,
			'UnitRename'			: self.onUnitRename,
			'unitPillage'			: self.onUnitPillage,
			'unitGifted'			: self.onUnitGifted,
			'unitBuildImprovement'	: self.onUnitBuildImprovement,
			'goodyReceived'        	: self.onGoodyReceived,
			'buildingBuilt' 		: self.onBuildingBuilt,
			'chat' 					: self.onChat,
			'victory'				: self.onVictory,
			'yieldSoldToEurope'		: self.onYieldSoldToEurope,
			'yieldBoughtFromEurope'	: self.onYieldBoughtFromEurope,
			'unitBoughtFromEurope'	: self.onUnitBoughtFromEurope,
			'unitTravelStateChanged'	: self.onUnitTravelStateChanged,
			'emmigrantAtDocks'		: self.onEmmigrantAtDocks,
			'populationJoined'		: self.onPopulationJoined,
			'populationUnjoined'	: self.onPopulationUnjoined,
			'unitLearned'			: self.onUnitLearned,
			'yieldProduced'			: self.onYieldProduced,
			'changeWar'				: self.onChangeWar,
			'setPlayerAlive'		: self.onSetPlayerAlive,
			'playerGoldTrade'		: self.onPlayerGoldTrade,
			'windowActivation'		: self.onWindowActivation,
			'cityScreenOpen'		: self.onCityScreenOpen,
			'gameUpdate'			: self.onGameUpdate,		# sample generic event
		}

		################## Events List ###############################
		#
		# Dictionary of Events, indexed by EventID (also used at popup context id)
		#   entries have name, beginFunction, applyFunction [, randomization weight...]
		#
		# Normal events first, random events after
		#
		################## Events List ###############################
		self.Events={
			CvUtil.EventEditCityName : ('EditCityName', self.__eventEditCityNameApply, self.__eventEditCityNameBegin),
			CvUtil.EventEditCity : ('EditCity', self.__eventEditCityApply, self.__eventEditCityBegin),
			CvUtil.EventPlaceObject : ('PlaceObject', self.__eventPlaceObjectApply, self.__eventPlaceObjectBegin),
			CvUtil.EventAwardGold: ('AwardGold', self.__EventAwardGoldApply, self.__EventAwardGoldBegin),
			CvUtil.EventEditUnitName : ('EditUnitName', self.__eventEditUnitNameApply, self.__eventEditUnitNameBegin),
			CvUtil.EventWBAllPlotsPopup : ('WBAllPlotsPopup', self.__eventWBAllPlotsPopupApply, self.__eventWBAllPlotsPopupBegin),
			CvUtil.EventWBLandmarkPopup : ('WBLandmarkPopup', self.__eventWBLandmarkPopupApply, self.__eventWBLandmarkPopupBegin),
			CvUtil.EventWBScriptPopup : ('WBScriptPopup', self.__eventWBScriptPopupApply, self.__eventWBScriptPopupBegin),
			CvUtil.EventWBStartYearPopup : ('WBStartYearPopup', self.__eventWBStartYearPopupApply, self.__eventWBStartYearPopupBegin),
			CvUtil.EventShowWonder: ('ShowWonder', self.__eventShowWonderApply, self.__eventShowWonderBegin),
			CvUtil.EventCreateTradeRoute: ('CreateTradeRoute', self.__eventCreateTradeRouteApply, self.__eventCreateTradeRouteBegin),
			CvUtil.EventEditTradeRoute: ('EditTradeRoute', self.__eventEditTradeRouteApply, self.__eventEditTradeRouteBegin),

		}
#################### EVENT STARTERS ######################
	def handleEvent(self, argsList):
		'EventMgr entry point'
		# extract the last 6 args in the list, the first arg has already been consumed
		self.origArgsList = argsList	# point to original
		tag = argsList[0]				# event type string
		idx = len(argsList)-6
		bDummy = false
		self.bDbg, bDummy, self.bAlt, self.bCtrl, self.bShift, self.bAllowCheats = argsList[idx:]
		ret = 0
		if self.EventHandlerMap.has_key(tag):
			fxn = self.EventHandlerMap[tag]
			ret = fxn(argsList[1:idx])
		return ret

#################### EVENT APPLY ######################
	def beginEvent( self, context, argsList=-1 ):
		'Begin Event'
		entry = self.Events[context]
		return entry[2]( argsList )

	def applyEvent( self, argsList ):
		'Apply the effects of an event '
		context, playerID, netUserData, popupReturn = argsList

		if context == CvUtil.PopupTypeEffectViewer:
			return CvDebugTools.g_CvDebugTools.applyEffectViewer( playerID, netUserData, popupReturn )

		entry = self.Events[context]

		if ( context not in CvUtil.SilentEvents ):
			self.reportEvent(entry, context, (playerID, netUserData, popupReturn) )
		return entry[1]( playerID, netUserData, popupReturn )   # the apply function

	def reportEvent(self, entry, context, argsList):
		'Report an Event to Events.log '
		if (gc.getGame().getActivePlayer() != -1):
			message = "DEBUG Event: %s (%s)" %(entry[0], gc.getActivePlayer().getName())
			CyInterface().addImmediateMessage(message,"")
			CvUtil.pyPrint(message)
		return 0

#################### ON EVENTS ######################
	def onKbdEvent(self, argsList):
		'keypress handler - return 1 if the event was consumed'

		eventType,key,mx,my,px,py = argsList
		game = gc.getGame()

		if (self.bAllowCheats):
			# notify debug tools of input to allow it to override the control
			argsList = (eventType,key,self.bCtrl,self.bShift,self.bAlt,mx,my,px,py,gc.getGame().isNetworkMultiPlayer())
			if ( CvDebugTools.g_CvDebugTools.notifyInput(argsList) ):
				return 0

		if ( eventType == self.EventKeyDown ):
			theKey=int(key)

			#Custom Camera Controls

			if (theKey == int(InputTypes.KB_LEFT)):
				if self.bCtrl:
						CyCamera().SetBaseTurn(CyCamera().GetBaseTurn() - 45.0)
						return 1
				elif self.bShift:
						CyCamera().SetBaseTurn(CyCamera().GetBaseTurn() - 15.0)
						return 1
			
			elif (theKey == int(InputTypes.KB_RIGHT)):
					if self.bCtrl:
							CyCamera().SetBaseTurn(CyCamera().GetBaseTurn() + 45.0)
							return 1
					elif self.bShift:
							CyCamera().SetBaseTurn(CyCamera().GetBaseTurn() + 15.0)
							return 1

			elif (theKey == int(InputTypes.KB_UP)):
					if (self.bCtrl or self.bShift) and CyCamera().GetBasePitch() > -45:
						CyCamera().SetBasePitch(CyCamera().GetBasePitch() - 5.0)
						return 1

			elif (theKey == int(InputTypes.KB_DOWN)):
					if (self.bCtrl or self.bShift) and CyCamera().GetBasePitch() < 20:
						CyCamera().SetBasePitch(CyCamera().GetBasePitch() + 5.0)
						return 1

			elif (theKey == int(InputTypes.KB_HOME) and self.bCtrl):
						CyCamera().SetBaseTurn(0)
						CyCamera().SetBasePitch(0)
						return 1

			#End Custom Camera Controls

			CvCameraControls.g_CameraControls.handleInput( theKey )

			if (self.bAllowCheats):
				# Shift - T (Debug - No MP)
				if (theKey == int(InputTypes.KB_T)):
					if ( self.bShift ):
						self.beginEvent(CvUtil.EventAwardGold)
						#self.beginEvent(CvUtil.EventCameraControlPopup)
						return 1

				elif (theKey == int(InputTypes.KB_W)):
					if ( self.bShift and self.bCtrl):
						self.beginEvent(CvUtil.EventShowWonder)
						return 1

				# Shift - ] (Debug - currently mouse-overd unit, health += 10
				elif (theKey == int(InputTypes.KB_LBRACKET) and self.bShift ):
					unit = CyMap().plot(px, py).getUnit(0)
					if ( not unit.isNone() ):
						d = min( unit.maxHitPoints()-1, unit.getDamage() + 10 )
						unit.setDamage( d )

				# Shift - [ (Debug - currently mouse-overd unit, health -= 10
				elif (theKey == int(InputTypes.KB_RBRACKET) and self.bShift ):
					unit = CyMap().plot(px, py).getUnit(0)
					if ( not unit.isNone() ):
						d = max( 0, unit.getDamage() - 10 )
						unit.setDamage( d )

				elif (theKey == int(InputTypes.KB_F1)):
					if ( self.bShift ):
						CvScreensInterface.replayScreen.showScreen(False)
						return 1
					# don't return 1 unless you want the input consumed


		return 0

	def onModNetMessage(self, argsList):
		'Called whenever CyMessageControl().sendModNetMessage() is called - this is all for you modders!'

		iData1, iData2, iData3, iData4, iData5 = argsList

		print("Modder's net message!")

		CvUtil.pyPrint( 'onModNetMessage' )

	def onInit(self, argsList):
		'Called when Civ starts up'
		CvUtil.pyPrint( 'OnInit' )

	def onUpdate(self, argsList):
		'Called every frame'
		fDeltaTime = argsList[0]

		# allow camera to be updated
		CvCameraControls.g_CameraControls.onUpdate( fDeltaTime )

	def onWindowActivation(self, argsList):
		'Called when the game window activates or deactivates'
		bActive = argsList[0]

	def onCityScreenOpen(self, argsList):
		'Called when the game window activates or deactivates'
		iPlayer = argsList[0]
		iCityId = argsList[1]
		CvAdvisorUtils.cityScreenFeats(iPlayer, iCityId)

	def onUnInit(self, argsList):
		'Called when Civ shuts down'
		CvUtil.pyPrint('OnUnInit')

	def onPreSave(self, argsList):
		"called before a game is actually saved"
		CvUtil.pyPrint('OnPreSave')

	def onSaveGame(self, argsList):
		"return the string to be saved - Must be a string"
		return ""

	def onLoadGame(self, argsList):
		return 0

	def onGameStart(self, argsList):

# Place random stuff on map START
		if (gc.getGame().getGameTurnYear() == gc.getDefineINT("START_YEAR")):
				CyMapGenerator().eraseBonuses()
				CyMapGenerator().eraseGoodies()
				CyMapGenerator().addBonuses()
				CyMapGenerator().addGoodies()
# Place random stuff on map END

		'Called at the start of the game'
		if (gc.getGame().getGameTurnYear() == gc.getDefineINT("START_YEAR") and not gc.getGame().isOption(GameOptionTypes.GAMEOPTION_ADVANCED_START)):
			for iPlayer in range(gc.getMAX_PLAYERS()):
				player = gc.getPlayer(iPlayer)
				if (player.isAlive() and player.isHuman()):
					popupInfo = CyPopupInfo()
					popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_PYTHON_SCREEN)
					popupInfo.setText(u"showDawnOfMan")
					popupInfo.addPopup(iPlayer)
					if player.isNative():
			    			iRnd = CyGame().getSorenRandNum(CyMap().numPlots(), "Saucer")
						pPlot = CyMap().plotByIndex(iRnd)
	        				iNewProfession = CvUtil.findInfoTypeNum('NONE')
						player.initUnit(CvUtil.findInfoTypeNum('UNIT_SAUCER'), iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
		else:
			CyInterface().setSoundSelectionReady(true)

		if gc.getGame().isPbem():
			for iPlayer in range(gc.getMAX_PLAYERS()):
				player = gc.getPlayer(iPlayer)
				if (player.isAlive() and player.isHuman()):
					popupInfo = CyPopupInfo()
					popupInfo.setButtonPopupType(ButtonPopupTypes.BUTTONPOPUP_DETAILS)
					popupInfo.setOption1(true)
					popupInfo.addPopup(iPlayer)
# Orlanth Killbot setup
		game = CyGame()
	        iCoast = gc.getInfoTypeForString('TERRAIN_COAST')
	        iPeak = gc.getInfoTypeForString('TERRAIN_PEAK')
	        iTemple = gc.getInfoTypeForString('BONUS_TEMPLE')
	        iZig = gc.getInfoTypeForString('BONUS_GRANDZIGGURAT')
	        iHatch = gc.getInfoTypeForString('BONUS_HATCHERY')
	        iSpider = gc.getInfoTypeForString('BONUS_SPIDERLAIR')
	        iMetro = gc.getInfoTypeForString('BONUS_METROPOLIS')
	        iMound = gc.getInfoTypeForString('FEATURE_MOUND')

	        iNewUnit = CvUtil.findInfoTypeNum('UNIT_KILLBOT')
	        iNewProfession = ProfessionTypes.NO_PROFESSION
	        iOuterGodsLeader = gc.getInfoTypeForString('LEADER_INVASION_OUTER_GODS')
	        for iPlayer in range(gc.getMAX_PLAYERS()):
	            pPlayer = gc.getPlayer(iPlayer)
	            if pPlayer.getLeaderType() == iOuterGodsLeader:
	                bPlayer = pPlayer
	        for i in range(CyMap().numPlots()):
	            pPlot = CyMap().plotByIndex(i)
	            iBonus = pPlot.getBonusType()
	            iFeat = pPlot.getFeatureType()
	            iTerrain = pPlot.getTerrainType()
	            if pPlot.isPeak():
			    iMetroRnd = game.getSorenRandNum(21, "PeakForts")
			    if iMetroRnd > 18:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_CITADEL'))
	                    	bPlayer.initUnit(iNewUnit, iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
			    	if iMetroRnd > 19:
					pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_FORCEFIELD'),1)
	                    		bPlayer.initUnit(CvUtil.findInfoTypeNum('UNIT_PROGENITORAI'), iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
			    elif iMetroRnd > 16:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_KEEP'))
	                    	bPlayer.initUnit(iNewUnit, iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
	            if pPlot.getEurope() == (gc.getInfoTypeForString('EUROPE_EAST')) or pPlot.getEurope() == (gc.getInfoTypeForString('EUROPE_WEST')):
			    iOdd = (pPlot.getX())%2 + (pPlot.getY())%2			
			    if iOdd == 1:
			    	iFlareRnd = game.getSorenRandNum(20, "Flare")
			    	if iFlareRnd > 13:
	                    		pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_NOVA'),1)
#Moon setup
	            if iBonus == gc.getInfoTypeForString('BONUS_VERDANTMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_TOXIC'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
	            elif iBonus == gc.getInfoTypeForString('BONUS_ARTIFACTMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_TOXIC'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
	            elif iBonus == gc.getInfoTypeForString('BONUS_VOLCANICMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_TOXIC'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
	            elif iBonus == gc.getInfoTypeForString('BONUS_BARRENMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_TOXIC'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
	            elif iBonus == gc.getInfoTypeForString('BONUS_TOXICMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_TOXIC'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
	            elif iBonus == gc.getInfoTypeForString('BONUS_ARCTICMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
	            elif iBonus == gc.getInfoTypeForString('BONUS_AQUATICMOON'):
			    iRnd = game.getSorenRandNum(20, "Moon")
			    if iRnd > 14:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_EARTHLIKE'),1)
			    elif iRnd > 10:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_METHANOGEN'),1)
			    elif iRnd > 8:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_INFECTED'),1)
			    elif iRnd > 6:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    elif iRnd > 4:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_RADIOACTIVE'),1)
			    elif iRnd > 2:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_DUSTSTORM'),1)
			    else:
	                    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_ATMOS_CORROSIVE'),1)
#end Moon setup	            
	            if iBonus == iSpider:
			    bPlayer.initUnit(CvUtil.findInfoTypeNum('UNIT_SPIDER'), iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
	            if iBonus == iMetro:
			    iMetroRnd = game.getSorenRandNum(20, "Mounds")
			    if iMetroRnd > 8:
			    	pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_MOUND'),1)
			    elif iMetroRnd > 3:
	                    	bPlayer.initUnit(iNewUnit, iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
			    elif iMetroRnd > 2:
	                    	bPlayer.initUnit(CvUtil.findInfoTypeNum('UNIT_PROGENITORAI'), iNewProfession, pPlot.getX(), pPlot.getY(), UnitAITypes.NO_UNITAI, DirectionTypes.NO_DIRECTION, 0)
			    iMetroRnd = game.getSorenRandNum(20, "Improvements")
			    if iMetroRnd > 18:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_INSTALLATION'))
			    elif iMetroRnd > 16:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_LIBRARY'))
			    elif iMetroRnd > 14:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_KEEP'))
			    elif iMetroRnd > 13:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_CITADEL'))
			    elif iMetroRnd > 12:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_BUNKER'))
			    elif iMetroRnd > 9:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_REACTOR'))
			    elif iMetroRnd > 7:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_PROIN1'))
			    elif iMetroRnd > 5:
	                    	pPlot.setImprovementType(gc.getInfoTypeForString('IMPROVEMENT_PROIN2'))
# end Orlanth Killbot setup

	def onGameEnd(self, argsList):
		'Called at the End of the game'
		print("Game is ending")
		return

	def onBeginGameTurn(self, argsList):
		'Called at the beginning of the end of each turn'
		iGameTurn = argsList[0]
# start Orlanth native features
#		game = CyGame()
#		if iGameTurn < 5:
#			for i in range(CyMap().numPlots()):
#				pPlot = CyMap().plotByIndex(i)
#				pOwner = gc.getPlayer(pPlot.getOwner())
#				if (pPlot.isCity() and (pOwner.getCivilizationType() == gc.getInfoTypeForString('CIVILIZATION_TUPI') or pOwner.getCivilizationType() == gc.getInfoTypeForString('CIVILIZATION_IROQUOIS'))):
#					iLakeRnd = game.getSorenRandNum(20, "Lake")
#					if iLakeRnd > 11:
#						pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_LAKE', 0)
#					elif iLakeRnd > 2:
#						pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_LAGOON', 0)
#				elif (pPlot.isCity() and pOwner.getCivilizationType() == gc.getInfoTypeForString('CIVILIZATION_ARAWAK')):
#					pPlot.setFeatureType(gc.getInfoTypeForString('FEATURE_GEYSER', 0)
# end Orlanth native features

	def onEndGameTurn(self, argsList):
		'Called at the end of the end of each turn'
		iGameTurn = argsList[0]

	def onBeginPlayerTurn(self, argsList):
		'Called at the beginning of a players turn'
		iGameTurn, iPlayer = argsList

	def onEndPlayerTurn(self, argsList):
		'Called at the end of a players turn'
		iGameTurn, iPlayer = argsList

		CvAdvisorUtils.endTurnNags(iPlayer)
		CvAdvisorUtils.endTurnFeats(iPlayer)

	def onEndTurnReady(self, argsList):
		iGameTurn = argsList[0]

	def onFirstContact(self, argsList):
		'Contact'
		iTeamX,iHasMetTeamY = argsList
		if (not self.__LOG_CONTACT):
			return
		CvUtil.pyPrint('Team %d has met Team %d' %(iTeamX, iHasMetTeamY))

	def onCombatResult(self, argsList):
		'Combat Result'
		pWinner,pLoser = argsList
		playerX = gc.getPlayer(pWinner.getOwner())
		unitX = gc.getUnitInfo(pWinner.getUnitType())
		playerY = gc.getPlayer(pLoser.getOwner())
		unitY = gc.getUnitInfo(pLoser.getUnitType())
		if (not self.__LOG_COMBAT):
			return
		if playerX and playerX and unitX and playerY:
			CvUtil.pyPrint('Player %d Civilization %s Unit %s has defeated Player %d Civilization %s Unit %s'
				%(playerX.getID(), playerX.getCivilizationDescription(0), unitX.getDescription(),
				playerY.getID(), playerY.getCivilizationDescription(0), unitY.getDescription()))

	def onCombatLogCalc(self, argsList):
		'Combat Result'
		genericArgs = argsList[0][0]
		cdAttacker = genericArgs[0]
		cdDefender = genericArgs[1]
		iCombatOdds = genericArgs[2]
		CvUtil.combatMessageBuilder(cdAttacker, cdDefender, iCombatOdds)

	def onCombatLogHit(self, argsList):
		'Combat Message'
		global gCombatMessages, gCombatLog
		genericArgs = argsList[0][0]
		cdAttacker = genericArgs[0]
		cdDefender = genericArgs[1]
		iIsAttacker = genericArgs[2]
		iDamage = genericArgs[3]

		if cdDefender.eOwner == cdDefender.eVisualOwner:
			szDefenderName = gc.getPlayer(cdDefender.eOwner).getNameKey()
		else:
			szDefenderName = localText.getText("TXT_KEY_TRAIT_PLAYER_UNKNOWN", ())
		if cdAttacker.eOwner == cdAttacker.eVisualOwner:
			szAttackerName = gc.getPlayer(cdAttacker.eOwner).getNameKey()
		else:
			szAttackerName = localText.getText("TXT_KEY_TRAIT_PLAYER_UNKNOWN", ())

		if (iIsAttacker == 0):
			combatMessage = localText.getText("TXT_KEY_COMBAT_MESSAGE_HIT", (szDefenderName, cdDefender.sUnitName, iDamage, cdDefender.iCurrHitPoints, cdDefender.iMaxHitPoints))
			CyInterface().addCombatMessage(cdAttacker.eOwner,combatMessage)
			CyInterface().addCombatMessage(cdDefender.eOwner,combatMessage)
			if (cdDefender.iCurrHitPoints <= 0):
				combatMessage = localText.getText("TXT_KEY_COMBAT_MESSAGE_DEFEATED", (szAttackerName, cdAttacker.sUnitName, szDefenderName, cdDefender.sUnitName))
				CyInterface().addCombatMessage(cdAttacker.eOwner,combatMessage)
				CyInterface().addCombatMessage(cdDefender.eOwner,combatMessage)
		elif (iIsAttacker == 1):
			combatMessage = localText.getText("TXT_KEY_COMBAT_MESSAGE_HIT", (szAttackerName, cdAttacker.sUnitName, iDamage, cdAttacker.iCurrHitPoints, cdAttacker.iMaxHitPoints))
			CyInterface().addCombatMessage(cdAttacker.eOwner,combatMessage)
			CyInterface().addCombatMessage(cdDefender.eOwner,combatMessage)
			if (cdAttacker.iCurrHitPoints <= 0):
				combatMessage = localText.getText("TXT_KEY_COMBAT_MESSAGE_DEFEATED", (szDefenderName, cdDefender.sUnitName, szAttackerName, cdAttacker.sUnitName))
				CyInterface().addCombatMessage(cdAttacker.eOwner,combatMessage)
				CyInterface().addCombatMessage(cdDefender.eOwner,combatMessage)

	def onImprovementBuilt(self, argsList):
		'Improvement Built'
		iImprovement, iX, iY = argsList
		if (not self.__LOG_IMPROVEMENT):
			return
		CvUtil.pyPrint('Improvement %s was built at %d, %d'
			%(gc.getImprovementInfo(iImprovement).getDescription(), iX, iY))


	def onImprovementDestroyed(self, argsList):
		'Improvement Destroyed'
		iImprovement, iOwner, iX, iY = argsList
		if (not self.__LOG_IMPROVEMENT):
			return
		CvUtil.pyPrint('Improvement %s was Destroyed at %d, %d'
			%(gc.getImprovementInfo(iImprovement).getDescription(), iX, iY))


	def onRouteBuilt(self, argsList):
		'Route Built'
		iRoute, iX, iY = argsList
		if (not self.__LOG_IMPROVEMENT):
			return
		CvUtil.pyPrint('Route %s was built at %d, %d'
			%(gc.getRouteInfo(iRoute).getDescription(), iX, iY))

	def onPlotRevealed(self, argsList):
		'Plot Revealed'
		pPlot = argsList[0]
		iTeam = argsList[1]

	def onPlotFeatureRemoved(self, argsList):
		'Plot Revealed'
		pPlot = argsList[0]
		iFeatureType = argsList[1]
		pCity = argsList[2] # This can be null

	def onPlotPicked(self, argsList):
		'Plot Picked'
		pPlot = argsList[0]
		CvUtil.pyPrint('Plot was picked at %d, %d'
			%(pPlot.getX(), pPlot.getY()))

	def onGotoPlotSet(self, argsList):
		'Goto Plot'
		pPlot, iPlayer = argsList

	def onBuildingBuilt(self, argsList):
		'Building Completed'
		pCity, iBuildingType = argsList

		CvAdvisorUtils.buildingBuiltFeats(pCity, iBuildingType)

		if (not self.__LOG_BUILDING):
			return
		CvUtil.pyPrint('%s was finished by Player %d Civilization %s'
			%(gc.getBuildingInfo(iBuildingType).getDescription(), pCity.getOwner(), gc.getPlayer(pCity.getOwner()).getCivilizationDescription(0)))


	def onSelectionGroupPushMission(self, argsList):
		'selection group mission'
		eOwner = argsList[0]
		eMission = argsList[1]
		iNumUnits = argsList[2]
		listUnitIds = argsList[3]

		if (not self.__LOG_PUSH_MISSION):
			return
		if pHeadUnit:
			CvUtil.pyPrint("Selection Group pushed mission %d" %(eMission))

	def onUnitMove(self, argsList):
		'unit move'
		pPlot,pUnit,pOldPlot = argsList
		player = gc.getPlayer(pUnit.getOwner())
		unitInfo = gc.getUnitInfo(pUnit.getUnitType())
		CvAdvisorUtils.unitMoveFeats(pUnit, pPlot, pOldPlot)
		if (not self.__LOG_MOVEMENT):
			return
		if player and unitInfo:
			CvUtil.pyPrint('Player %d Civilization %s unit %s is moving to %d, %d'
				%(player.getID(), player.getCivilizationDescription(0), unitInfo.getDescription(),
				pUnit.getX(), pUnit.getY()))

	def onUnitSetXY(self, argsList):
		'units xy coords set manually'
		pPlot,pUnit = argsList
		if (not self.__LOG_MOVEMENT):
			return
			
	def onUnitCreated(self, argsList):
		'Unit Completed'
		unit = argsList[0]
		if (not self.__LOG_UNITBUILD):
			return

	def onUnitBuilt(self, argsList):
		'Unit Completed'
		city = argsList[0]
		unit = argsList[1]
		player = gc.getPlayer(city.getOwner())

		CvAdvisorUtils.unitBuiltFeats(city, unit)

		if (not self.__LOG_UNITBUILD):
			return
		CvUtil.pyPrint('%s was finished by Player %d Civilization %s'
			%(gc.getUnitInfo(unit.getUnitType()).getDescription(), player.getID(), player.getCivilizationDescription(0)))

	def onUnitKilled(self, argsList):
		'Unit Killed'
		unit, iAttacker = argsList
		player = gc.getPlayer(unit.getOwner())
		attacker = gc.getPlayer(iAttacker)
		if (not self.__LOG_UNITKILLED):
			return
		CvUtil.pyPrint('Player %d Civilization %s Unit %s was killed by Player %d'
			%(player.getID(), player.getCivilizationDescription(0), gc.getUnitInfo(unit.getUnitType()).getDescription(), attacker.getID()))

	def onUnitLost(self, argsList):
		'Unit Lost'
		unit = argsList[0]
		player = gc.getPlayer(unit.getOwner())
		if (not self.__LOG_UNITLOST):
			return
		CvUtil.pyPrint('%s was lost by Player %d Civilization %s'
			%(gc.getUnitInfo(unit.getUnitType()).getDescription(), player.getID(), player.getCivilizationDescription(0)))

	def onUnitPromoted(self, argsList):
		'Unit Promoted'
		pUnit, iPromotion = argsList
		player = gc.getPlayer(pUnit.getOwner())
		if (not self.__LOG_UNITPROMOTED):
			return
		CvUtil.pyPrint('Unit Promotion Event: %s - %s' %(player.getCivilizationDescription(0), pUnit.getName(),))

	def onUnitRename(self, argsList):
		'Unit is renamed'
		pUnit = argsList[0]
		if (pUnit.getOwner() == gc.getGame().getActivePlayer()):
			self.__eventEditUnitNameBegin(pUnit)

	def onUnitPillage(self, argsList):
		'Unit pillages a plot'
		pUnit, iImprovement, iRoute, iOwner = argsList
		iPlotX = pUnit.getX()
		iPlotY = pUnit.getY()
		pPlot = CyMap().plot(iPlotX, iPlotY)

		if (not self.__LOG_UNITPILLAGE):
			return
		CvUtil.pyPrint("Player %d's %s pillaged improvement %d and route %d at plot at (%d, %d)"
			%(iOwner, gc.getUnitInfo(pUnit.getUnitType()).getDescription(), iImprovement, iRoute, iPlotX, iPlotY))

	def onUnitGifted(self, argsList):
		'Unit is gifted from one player to another'
		pUnit, iGiftingPlayer, pPlotLocation = argsList

	def onUnitBuildImprovement(self, argsList):
		'Unit begins enacting a Build (building an Improvement or Route)'
		pUnit, iBuild, bFinished = argsList

	def onUnitSelected(self, argsList):
		pUnit = argsList[0]
		CvAdvisorUtils.unitSelectedFeats(pUnit)
		
	def onMissionaryConvertedUnit(self, argsList):
		pUnit = argsList[0]
		CvAdvisorUtils.addUnitToNagList(pUnit)

	def onGoodyReceived(self, argsList):
		'Goody received'
		iPlayer, pPlot, pUnit, iGoodyType = argsList
		if (not self.__LOG_GOODYRECEIVED):
			return
		CvUtil.pyPrint('%s received a goody' %(gc.getPlayer(iPlayer).getCivilizationDescription(0)),)

	def onChangeWar(self, argsList):
		'War Status Changes'
		bIsWar = argsList[0]
		iTeam = argsList[1]
		iRivalTeam = argsList[2]
		if (not self.__LOG_WARPEACE):
			return
		if (bIsWar):
			strStatus = "declared war"
		else:
			strStatus = "declared peace"
		CvUtil.pyPrint('Team %d has %s on Team %d'
			%(iTeam, strStatus, iRivalTeam))

	def onChat(self, argsList):
		'Chat Message Event'
		chatMessage = "%s" %(argsList[0],)

	def onSetPlayerAlive(self, argsList):
		'Set Player Alive Event'
		iPlayerID = argsList[0]
		bNewValue = argsList[1]
		CvUtil.pyPrint("Player %d's alive status set to: %d" %(iPlayerID, int(bNewValue)))

	def onPlayerGoldTrade(self, argsList):
		'Player Trades gold to another player'
		iFromPlayer, iToPlayer, iGoldAmount = argsList

	def onCityBuilt(self, argsList):
		'City Built'
		city = argsList[0]
		if ((city.getOwner() == gc.getGame().getActivePlayer()) and gc.getPlayer(city.getOwner()).isHuman()):
			self.__eventEditCityNameBegin(city, False)
		CvUtil.pyPrint('City Built Event: %s' %(city.getName()))

	def onCityRazed(self, argsList):
		'City Razed'
		city, iPlayer = argsList
		iOwner = city.findHighestCulture()

		CvUtil.pyPrint("City Razed Event: %s" %(city.getName(),))

	def onCityAcquired(self, argsList):
		'City Acquired'
		iPreviousOwner,iNewOwner,pCity,bConquest,bTrade = argsList
		CvUtil.pyPrint('City Acquired Event: %s' %(pCity.getName()))

	def onCityAcquiredAndKept(self, argsList):
		'City Acquired and Kept'
		iOwner,pCity = argsList
		CvUtil.pyPrint('City Acquired and Kept Event: %s' %(pCity.getName()))

	def onCityLost(self, argsList):
		'City Lost'
		city = argsList[0]
		player = gc.getPlayer(city.getOwner())
		if (not self.__LOG_CITYLOST):
			return
		CvUtil.pyPrint('City %s was lost by Player %d Civilization %s'
			%(city.getName(), player.getID(), player.getCivilizationDescription(0)))

	def onCultureExpansion(self, argsList):
		'City Culture Expansion'
		pCity = argsList[0]
		iPlayer = argsList[1]
		CvUtil.pyPrint("City %s's culture has expanded" %(pCity.getName(),))

	def onCityGrowth(self, argsList):
		'City Population Growth'
		pCity = argsList[0]
		iPlayer = argsList[1]
		CvUtil.pyPrint("%s has grown" %(pCity.getName(),))

	def onCityDoTurn(self, argsList):
		'City Production'
		pCity = argsList[0]
		iPlayer = argsList[1]

		CvAdvisorUtils.cityAdvise(pCity, iPlayer)

	def onCityBuildingUnit(self, argsList):
		'City begins building a unit'
		pCity = argsList[0]
		iUnitType = argsList[1]
		if (not self.__LOG_CITYBUILDING):
			return
		CvUtil.pyPrint("%s has begun building a %s" %(pCity.getName(),gc.getUnitInfo(iUnitType).getDescription()))

	def onCityBuildingBuilding(self, argsList):
		'City begins building a Building'
		pCity = argsList[0]
		iBuildingType = argsList[1]
		if (not self.__LOG_CITYBUILDING):
			return
		CvUtil.pyPrint("%s has begun building a %s" %(pCity.getName(),gc.getBuildingInfo(iBuildingType).getDescription()))

	def onCityRename(self, argsList):
		'City is renamed'
		pCity = argsList[0]
		if (pCity.getOwner() == gc.getGame().getActivePlayer()):
			self.__eventEditCityNameBegin(pCity, True)

	def onCreateTradeRoute(self, argsList):
		'Trade Route is Created'
		PlayerID = argsList[0]
		self.__eventCreateTradeRouteBegin(PlayerID)

	def onEditTradeRoute(self, argsList):
		'Trade Route is Modified'
		PlayerID = argsList[0]
		iRouteID = argsList[1]
		self.__eventEditTradeRouteBegin(PlayerID, iRouteID)

	def onCityHurry(self, argsList):
		'City is renamed'
		pCity = argsList[0]
		iHurryType = argsList[1]

	def onVictory(self, argsList):
		'Victory'
		iTeam, iVictory = argsList
		if (iVictory >= 0 and iVictory < gc.getNumVictoryInfos()):
			victoryInfo = gc.getVictoryInfo(int(iVictory))
			CvUtil.pyPrint("Victory!  Team %d achieves a %s victory"
				%(iTeam, victoryInfo.getDescription()))

	def onYieldSoldToEurope(self, argsList):
		'Yield Sold To Europe'
		iPlayer, iYield, iAmount = argsList

	def onYieldBoughtFromEurope(self, argsList):
		'Yield Bought From Europe'
		iPlayer, iYield, iAmount = argsList

	def onUnitBoughtFromEurope(self, argsList):
		'Unit Bought From Europe'
		iPlayer, iUnitId = argsList

	def onUnitTravelStateChanged(self, argsList):
		'Ship Arrived in Europe or America'
		iPlayer, iUnitTravelState, iUnitId = argsList

	def onEmmigrantAtDocks(self, argsList):
		'Emmigrant At Docks'
		iPlayer, iUnitId = argsList

	def onPopulationJoined(self, argsList):
		'Population Joined'
		iPlayer, iCityId, iUnitId = argsList

	def onPopulationUnjoined(self, argsList):
		'Population Unjoined'
		iPlayer, iCityId, iUnitId = argsList

	def onUnitLearned(self, argsList):
		'Unit Learned'
		iPlayer, iUnitId = argsList

	def onYieldProduced(self, argsList):
		'Yield Produced'
		iPlayer, iCityId, iYield = argsList

	def onGameUpdate(self, argsList):
		'sample generic event, called on each game turn slice'
		genericArgs = argsList[0][0]	# tuple of tuple of my args
		turnSlice = genericArgs[0]

	def onMouseEvent(self, argsList):
		'mouse handler - returns 1 if the event was consumed'
		eventType,mx,my,px,py,interfaceConsumed,screens = argsList
		if ( px!=-1 and py!=-1 ):
			if ( eventType == self.EventLButtonDown ):
				if (self.bAllowCheats and self.bCtrl and self.bAlt and CyMap().plot(px,py).isCity() and not interfaceConsumed):
					# Launch Edit City Event
					self.beginEvent( CvUtil.EventEditCity, (px,py) )
					return 1

				elif (self.bAllowCheats and self.bCtrl and self.bShift and not interfaceConsumed):
					# Launch Place Object Event
					self.beginEvent( CvUtil.EventPlaceObject, (px, py) )
					return 1

		if ( eventType == self.EventBack ):
			return CvScreensInterface.handleBack(screens)
		elif ( eventType == self.EventForward ):
			return CvScreensInterface.handleForward(screens)

		return 0

#shipname modcomp            
	def NameGen(self, unit):
		pPlayer = gc.getPlayer(unit.getOwner())
		pCiv = pPlayer.getCivilizationType()
		sFull = "test"
		lName = ["test1","test2"]
		if pCiv == gc.getInfoTypeForString('CIVILIZATION_SPAIN') or pCiv == gc.getInfoTypeForString('CIVILIZATION_SPAIN_EUROPE'):
			lName = ["Nina","Pinta","Santa Maria","San Miguel","San Joseph","San Pedro y San Pablo","Santa Ana","Real Mazi","San Felipe","Santa Rosa Palermo","Conde de Tolosa","Incendio","Retiro","Constante","Fuerte","Andalucia","Constante","Africa","Europa","Asia","America","Dragon","Castilla","Invencible","Glorioso","Bizarro","Invencible","Conquistador","Africa","Vencedor","Tigre","Princesa","Galicia","Infante","Oriente","Eolo","Neptuno","Magnanimo","Aquilon","Gallardo","Brillante","Hector","Soberano","Astuto","America","San Telmo","San Felipe","Santiago el Mayor","San Martin","San Juan Baptista","Santiago","San Juan Bautista","San Agustin","Gusana","Pingue Volante","San Andres","Santa Susana","Santa Teresa","Jupiter","Nuestra SeA켹ra de Atocha","La Griega","San Esteban Apedreado","La Catalina","Santa Catalina","El Jardin de Triton","Santiago","Neptuno","Burlandin","El Burlando","Gusana","Concepcion","Nuestra SeA켹ra de Aranzazu","Nuestra SeA켹ra del Rosario","Santa Susana","Neptuno","Santa Barbara","La Chata","La SeA켹ra Sara","San Francisco Javier","Victoria","Galera Victoria","Galera Victoria","San Cayetano","Concepcion","Hermione","Flora","Triunfo","San Cristobal","Astrea","Santa Barbara","Santa Barbara","Estrella","Aguila","Aurora","Concepcion","Flora","Santa Rosalia","La Galga","Perla","Flecha","Pena","Venganza","Jupiter","Aguila","Dorada","Esmeralda","Ventura","Venturera","Liebre","Santa Cristina","Venus","Santa Brigida","Victoria","Industria","Juno","Santa Cecilia","Palas","Santa Yrene","Astra","Concepcion","Tetis","Guadalupe","Fenix","Santa Barbara","Soledad","Santa Rosa","Santa Rosalia","Santa Catalina","Santa Teresa","Santa Gertrudis","Santa Barbara","Santa Lucia"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_ENGLAND') or pCiv == gc.getInfoTypeForString('CIVILIZATION_ENGLAND_EUROPE'):
			lName = ["Royal Prince","Charles","St Andrew","London","Prince","Royal James","Royal Charles","Royal James","Royal Katherine","Royal Oak","Loyal London","Victory","French Ruby","St Michael","Clove Tree","House of Sweeds","Golden Phoenix","Slothany","Helverson","Cambridge","Warspite","Defiance","Rupert","Resolution","Monmouth","Edgar","Swiftsure","Harwich","Royal Oak","Defiance","Arms of Rotterdam","Montagu","Bonaventure","West Friesland","Seven Oaks","Charles V","Guilder de Ruyter","Maria Sancta","Mars","Delfe","St Paul","Hope","Black Spread Eagle","Golden Lion","Zealand","Unity","Young Prince","Black Bull","Constant Warwick","St Patrick","Greenwich","St David","Stathouse van Harlem","Stavoreen","Arms of Terver","Oxford","Woolwich","Kingfisher","Britannia","Vanguard","Windsor Castle","Sandwich","Duchess","Albemarle","Neptune","Duke","Ossory","Coronation","Lenox","Hampton Court","Anne","Captain","Restoration","Berwick","Burford","Eagle","Expedition","Grafton","Pendennis","Northumberland","Essex","Kent","Exeter","Suffolk","Hope","Elizabeth","Stirling Castle","Breda","Devonshire","Cornwall","Boyne","Russell","Norfolk","Humber","Sussex","Torbay","Lancaster","Dorsetshire","Cambridge","Chichester","Newark","Shrewsbury","Cumberland","Ranelagh","Somerset","Bredah","Ipswich","Yarmouth","Medway","Carlisle","Winchester","Canterbury","Sunderland","Pembroke","Gloucester","Windsor","Kingston","Exeter","Bedford","Orford","Nassau","Revenge","Dreadnought","Association","Barfleur","Namur","Triumph","Chatham","Centurion","Chester","Norwich","Weymouth","Falmouth","Rochester","Portland","Southampton","Norwich","Dartmouth","Anglesea","Colchester","Romney","Lichfield","Lincoln","Coventry","Severn","Burlington","Harwich","Pendennis","Blackwall","Guernsey","Nonsuch","Warwick","Hampshire","Winchester","Salisbury","Worcester","Dartmouth","Jersey","Carlisle","Tilbury","Falkland","Royal William","Queen","Victory","Royal Oak","Defiance","Swiftsure","Crown","Dragon","Newcastle","Bristol","Dover","Northumberland","Stirling Castle","Resolution","Nassau","Elizabeth","Restoration","Nottingham","Mary","York","Swallow","Antelope","Leopard","Panther","Newcastle","Reserve","Saint Albans","Colchester","Prince George","St George","Royal Katherine","Union","Devonshire","Chichester","Cornwall","Resolution","Burford","Eagle","Expedition","Kent","Stirling Castle","Suffolk","Berwick","Edgar","Essex","Grafton","Hampton Court","Lenox","Northumberland","Restoration","Elizabeth","Monmouth","Warspite","Rupert","Defiance","Montague","Monck","Dunkirk","Plymouth","Dreadnought","Advice","Assistance","Bonaventure","Greenwich","Kingfisher","Deptford","Southampton","Reserve","Tiger","Falkland","Crown","Ruby","Marlborough","Blenheim","Neptune","Vanguard","Princess","Sandwich","Barfleur","Boyne","Humber","Russell","Cumberland","Devonshire","Dorsetshire","Shrewsbury","Cambridge","Torbay","Newark","Resolution","Captain","Grafton","Hampton Court","Edgar","Yarmouth","Orford","Royal Oak","Expedition","Monmouth","Revenge","Suffolk","Plymouth","Lion","Gloucester","Rippon","Montague","Medway","Kingston","Nottingham","Salisbury","Dragon","Falmouth","Pembroke","Ruby","Chester","Romney","Bonaventure","Bristol","Warwick","Ormonde","Assistance","Gloucester","Advice","Strafford","Worcester","Panther","Dartmouth","Rochester","Nonsuch","Salisbury","Winchester","St Albans","Guernsey","Norwich","Deptford","Tiger","Weymouth","Swallow","Royal Sovereign","Prince George","Union","Namur","Neptune","Marlborough","Lancaster","Princess Amelia","Cornwall","Norfolk","Somerset","Princess Caroline","Russell","Edinburgh","Northumberland","Burford","Captain","Stirling Castle","Berwick","Lenox","Kent","Grafton","Ipswich","Buckingham","Prince of Orange","Canterbury","Plymouth","Sunderland","Windsor","Deptford","Swallow","Tilbury","Warwick","Pembroke","Dunkirk","Falkland","Chatham","Colchester","Leopard","Argyll","Portland","Assistance","Romney","Salisbury","Oxford","Falmouth","Lichfield","Greenwich","Newcastle","Victory","Duke","St George","Boyne","Cumberland","Elizabeth","Suffolk","Essex","Nassau","Prince Frederick","Bedford","Royal Oak","Stirling Castle","Monmouth","Revenge","Captain","Berwick","Weymouth","Strafford","Worcester","Augusta","Dragon","Jersey","Superb","Lion","Kingston","Rupert","Dreadnought","Medway","Princess Mary","Exeter","Nottingham","Gloucester","St Albans","Severn","Guernsey","Hampshire","Leopard","Nonsuch","Sutherland","Antelope","Dartmouth","Woolwich","Preston","Hector","Anglesea","Diamond","Mary Galley","Ludlow","Pearl","Kinsale","Lark","Adventure","Roebuck","Torrington","Princess Louisa","Southsea Castle","Dover","Folkestone","Faversham","Lynn","Gosport","Sapphire","Hastings","Liverpool","Kinsale","Adventure","Diamond","Launceston","Looe","Ramillies","Prince","Newark","Devonshire","Lancaster","Culloden","Somerset","Northumberland","Edinburgh","Hampton Court","Kent","Yarmouth","Princess Louisa","Defiance","Canterbury","Sunderland","Tilbury","Eagle","Windsor","Chester","Harwich","Winchester","Maidstone","Colchester","Portland","Falkland","Salisbury","Advice","Gloucester","Norwich","Ruby","Colchester","Lichfield","Panther","Bristol","Rochester","Royal George","Britannia","Princess Amelia","Vanguard","Somerset","Orford","Grafton","Swiftsure","Northumberland","Buckingham","St Albans","Anson","Tiger","Weymouth","York","Medway","Assistance","Greenwich","Tavistock","Falmouth","Newcastle","Dartmouth","Severn","Woolwich","Namur","Union","Neptune","Cambridge","Chichester","Dunkirk","Achilles","America","Montagu","Preston","Pembroke","Rippon","Chatham","Burford","Dorsetshire","Boyne","Temple","Conqueror","Victory","Royal Sovereign","Royal George","Queen Charlotte","Queen Charlotte","Sandwich","Blenheim","Ocean","London","Barfleur","Prince George","Princess Royal","Formidable","Queen","Duke","St George","Glory","Atlas","Prince","Impregnable","Windsor Castle","Boyne","Prince of Wales","Caesar","Dublin","Norfolk","Lenox","Mars","Shrewsbury","Warspite","Resolution","Fame","Hero","Hercules","Thunderer","Bellona","Dragon","Superb","Kent","Defence","Valiant","Triumph","Arrogant","Cornwall","Edgar","Goliath","Zealous","Audacious","Elephant","Bellerophon","Saturn","Vanguard","Excellent","Illustrious","Canada","Majestic","Orion","Captain","Albion","Grafton","Alcide","Fortitude","Irresistible","Ramillies","Monarch","Magnificent","Marlborough","Suffolk","Terrible","Russell","Invincible","Robust","Prince of Wales","Ajax","Royal Oak","Conqueror","Bedford","Hector","Vengeance","Sultan","Egmont","Elizabeth","Resolution","Cumberland","Berwick","Bombay Castle","Powerful","Defiance","Swiftsure","Culloden","Thunderer","Venerable","Victorious","Ramillies","Terrible","Hannibal","Theseus","Alfred","Alexander","Warrior","Montagu","Ganges","Culloden","Tremendous","Invincible","Minden","Minotaur","Leviathan","Carnatic","Colossus","Minotaur","Asia","Essex","Africa","St Albans","Augusta","Director","Exeter","Europa","Trident","Prudent","Ardent","Raisonnable","Agamemnon","Belliqueux","Stately","Nassau","Indefatigable","Worcester","Lion","Stirling Castle","Intrepid","Monmouth","Defiance","Nonsuch","Ruby","Vigilant","Eagle","America","Anson","Polyphemus","Magnanime","Sampson","Repulse","Diadem","Standard","Inflexible","Africa","Dictator","Sceptre","Crown","Ardent","Scipio","Veteran","Edgar","Panther","Firm","Warwick","Romney","Salisbury","Centurion","Portland","Bristol","Renown","Isis","Leopard","Hannibal","Jupiter","Leander","Adamant","Assistance","Europa","Experiment","Medusa","Grampus","Cato","Trusty","Caledonia","Britannia","Prince Regent","Royal George","Nelson","Saint Vincent","Howe","Saint George","Royal William","Neptune","Waterloo","Trafalgar","Ville de Paris","Hibernia","Ocean","Impregnable","Trafalgar","Princess Charlotte","Royal Adelaide","Dreadnought","Neptune","Temeraire","Boyne","Union","Rodney","Nile","London","Formidable","Ganges","Asia","Bombay","Calcutta","Monarch","Vengeance","Thunderer","Powerful","Clarence","Foudroyant","Rochfort","Sandwich","Waterloo","Cambridge","Indus","Hindostan","Brunswick","Mars","Centaur","Courageux","Plantagenet","Bulwark","Valiant","Ajax","Kent","Conqueror","Dragon","Northumberland","Renown","Spencer","Achille","Superb","Revenge","Milford","Princess Amelia","Colossus","Warspite","Fame","Albion","Hero","Illustrious","Marlborough","York","Hannibal","Sultan","Royal Oak","Aboukir","Bombay","Swiftsure","Victorious","Repulse","Eagle","Sceptre","Magnificent","Valiant","Elizabeth","Cumberland","Venerable","Talavera","Belleisle","Malabar","Blake","Santo Domingo","Armada","Cressy","Vigo","Vengeur","Ajax","Conquestador","Poictiers","Berwick","Egmont","Clarence","Edinburgh","America","Scarborough","Asia","Mulgrave","Anson","Gloucester","Rodney","Hogue","Dublin","Barham","Benbow","Stirling Castle","Vindictive","Blenheim","Duncan","Rippon","Medway","Cornwall","Pembroke","Indus","Redoubtable","Devonshire","Defence","Hercules","Agincourt","Pitt","Wellington","Russell","Akbar","Cornwallis","Wellesley","Carnatic","Black Prince","Melville","Hawke","Chatham","Hastings","Augusta","Imaun","Antelope","Diomede","Grampus","Jupiter","Salisbury","Romney","Isis","Brave","Alexandre","Duquesne","Implacable","Mont Blanc","Scipion","Brave","Maida","Marengo","Abercrombie","Genoa","Rivoli","Royal Albert","Windsor Castle","Marlborough","Royal Sovereign","Prince of Wales","Queen","Victoria","Frederick William","Algiers","Royal Sovereign","Albion","Aboukir","Exmouth","Saint Jean D'Acre","Hannibal","Princess Royal","Hannibal","Algiers","Caesar","Vanguard","Goliath","Superb","Meeanee","Collingwood","Centurion","Mars","Lion","Majestic","Colossus","Irresistible","Brunswick","Orion","Hood","Edgar","Sans Pareil","Boscawen","Cumberland","Duke of Wellington","Marlborough","Royal Sovereign","Prince of Wales","Royal Albert","Windsor Castle","Orion","Hood","Edgar","Caesar","Algiers","Princess Royal","Hannibal","Rodney","Nile","London","Nelson","Prince Regent","Royal George","St George","Royal William","Neptune","Waterloo","Trafalgar","Albion","Aboukir","Exmouth","Queen","Frederick William","Cressy","Goliath","Collingwood","Centurion","Mars","Lion","Majestic","Meeanee","Colossus","Brunswick","Irresistible","Bombay","Sans Pareil","Ajax","Blenheim","Edinburgh","Hogue","Cornwallis","Hastings","Hawke","Pembroke","Russell","Victoria","Howe","St Jean d'Acre","Conqueror","Donegal","Duncan","Gibraltar","Agamemnon","James Watt","Victor Emanuel","Edgar","Hero","Revenge","Renown","Atlas","Anson","Defiance","Bulwark","Robust","Repulse","Zealous","Royal Alfred","Royal Oak","Triumph","Ocean","Caledonia","Blake","Kent","Pitt","Warrior","Black Prince","Defence","Resistance","Hector","Valiant","Minotaur","Agincourt","Northumberland","Prince Consort","Caledonia","Ocean","Lord Clyde","Lord Warden","Captain","Audacious","Invincible","Iron Duke","Vanguard","Swiftsure","Triumph","Devastation","Thunderer","Superb","Agamemnon","Ajax","Scorpion","Wivern","Cerberus","Magdala","Belleisle","Orion","Conqueror","Hero"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_FRANCE') or pCiv == gc.getInfoTypeForString('CIVILIZATION_FRANCE_EUROPE'):
			lName = ["Galathee","Mutine","Emeraude","Fine","Sirene","Renomee","Amphitrite","Megere","Topaze","Thetis","Heroine","Comete","Fleur de Lys","Licorne","Sauvage","Hermine","Opale","Minerve","Oiseau","Blonde","Brune","Aigrette","Vestale","Felicite","Alcmene","Aimable","Infidele","Legere","Sincere","Inconstante","Blanche","Enjouee","Dedaigneuse","Belle Poule","Amphitrite","Tourterelle","Indiscrete","Sensible","Pourvoyeuse","Consolante","Nymphe","Andromaque","Astree","Sibylle","Diane","Nereide","Fine","Emeraude","Charmante","Junon","Gracieuse","Inconstante","Helene","Concorde","Courageuse","Hermione","Iphigenie","Surveillante","Resolue","Gentille","Amazone","Prudente","Gloire","Bellone","Medee","Magicienne","Precieuse","Vestale","Alceste","Iris","Reunion","Modeste","Topaze","Ceres","Fee","Galathee","Railleuse","Fleur de Lys","Charente Inferieure","Capricieuse","Friponne","Capricieuse","Prudente","Venus","CleopA쥁re","Felicite","Calypso","Fidele","Fortunee","Semillante","Insurgente","Charente Inferieure","Republique Francaise","Decade Francaise","Cocarde Nationale","Bravoure","Patriote","Fidele","Dedaigneuse","Themis","Heureuse","Chiffonne","Venus","Hebe","Dryade","Proserpine","Sibylle","Carmagnole","Danae","Meduse","Didon","Nymphe","Thetis","Cybele","Concorde","Minerve","Junon","Imperieuse","Melpomene","Minerve","Valeureuse","Infatigable","Carrere","Muiron","Seine","Revolutionnaire","Spartiate","Indienne","Furieuse","Virginie","Courageuse","Harmonie","Volontaire","Cornelie","Didon","Rhin","Belle Poule","Surveillante","Atalante","Preneuse","Africaine","Armide","Minerve","Penelope","Flore","Amphitrite","Niemen","Saale","Saale","Alcmene","Circe","Antigone","Cleopatre","Magicienne","Gloire","President","Topaze","Venus","Junon","Calypso","Amazone","Consolante","Piemontaise","Italienne","Danae","Bellone","Nereide","Illyrienne","Galatee","Milanaise","Vistule","Oder","Oder","Perle","Hortense","Hermione","Pomone","Manche","Caroline","Pauline","Corona","Pallas","Elbe","Amelie","Clorinde","Renommee","Elisa","Favorita","Astree","Fidele","Adrienne","Aurore","Nymphe","Iphigenie","Meduse","Pregel","Ariane","Medee","Andromaque","Yssel","Carolina","Principessa di Bologna","Gloire","Meuse","Terpsichore","Erigone","Arethuse","Jahde","Trave","Weser","Melpomene","Rubis","Ems","Atalante","Ceres","Piave","Dryade","Dryade","Sultane","Etoile","Rancune","Amphitrite","Cybele","Duchesse de Berry","Constance","Thetis","Astree","Armide","Proserpine","Iphigenie","Nereide","Ceylon","Vengeance","Resistance","Forte","Egyptienne","Romaine","Immortalite","Impatiente","Incorruptible","Revanche","Libre","Comete","Desiree","Poursuivante","Jeanne d'Arc","Clorinde","Amazone","Vestale","Venus","Ceres","Syrene","Atalante","Artemise","Andromede","Gloire","Poursuivante","Virginie","Cleopatre","Danae","Nereide","Zenobie","Alceste","Pandore","Sibylle","Reine Blanche","Surveillante","Iphigenie","Terpsichore","Dryade","Belle Gabrielle","Herminie","Melpomene","Didon","Uranie","Belle Poule","Semillante","Andromaque","Forte","Renommee","Perseverante","Vengeance","Entreprenante","Erigone","Africaine","Jeanne d'Arc","Penelope","Chartre","Psyche","Clorinde","Heliopolis","Algerie","Isis"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_DUTCH') or pCiv == gc.getInfoTypeForString('CIVILIZATION_DUTCH_EUROPE'):
			lName = ["Zuiderzee","Dolfijn","Eenhoorn","Salamander","Gelderland","Fredrik Hendrik","Eendracht","Zon","Groningen","Ter Goes","Prinses Roijael","Utrecht","Gewapende Ruyter","Maan","Zeven Provincien","Haarlem","Brederode","Huis van Nassau","Graaf Willem","Kameel","Postiljon van Smyrna","Vrede","Jaarsveld","Vrijheid","Mercurius","Louisa Hendrika","Vogelstruis","Witte Lam","Groote Liefde","Campen","Sint Matheeus","Rosenkrans","Zeelandia","Vlissingen","Middelburg","Kampveere","Prins Willem","Gelderland","Eendracht","Prinses Amalia","Dordrecht","Sint Matheeus","Caleb","Jupiter","Drie Helden Davids","Willem van Nieuhoff","Westergo","Stad en Lande","Prinses Albertina","Oostergo","Huis te Swieten","Huis te Cruiningen","Delfland","Amsterdam","Landman","Huis te Oosterwijk","Beurs van Amsterdam","Jozua","Duivenvoorde","Doesburg","Carolus Quintus","Zeelandia","Westvriesland","Prinses Maria","Admiraal Generaal","Kapitein Generaal","Hollandia","Akerboom","Prince Friso","Prins Willem","Kasteel van Medemblik","Koning William","Keurvorst van Brandenburg","Keurvorstin van Brandenburg","Keurvorst van Saksen","Beschermer","Beschermer","Unie","Gouda","Reigersberg","Zeven Provincien","Vrijheid","Zeelandt","te Veere","Nassauw","Middelburg","Eendragt","Matenes","Hardenbroek","Overwinnaer","Buis","Wolfswinkel","Starrenburg","Schonauwen","Huis te Nek","Loosdregt","Boetzelaer","Amsterdam","Ter Meer","Duinveld","de Purmer","Leyden","Gelderland","Roozendaal","Gouda","Souterwoude","Haarlem","Kasteel van Egmont","Polaanen","Damiaten","Tholen","Heemstede","Kasteel van Medenblick","Valkenburg","Beemsterlust","Twikkelo","Maas","Proventie Utrecht","Ramhorst","Prins Friso","Boekenroode","Delft","Brederode","Vrijheid","Moriaanshoofd","Watervlied","Zierikzee","ter Goes","Vlissingen","Assendelft","Haarlem","Delft"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_RUSSIA') or pCiv == gc.getInfoTypeForString('CIVILIZATION_RUSSIA_EUROPE'):
			lName = ["Apostol Pavel","Prorochestvo","Sviatogo Dukha","Sviatoi Ilya","Shlisselburg","Kronshlot","Peterburg","Triumph","Derpt","Narva","Mikhail Arkhangel","Ivan-gorod","Olifant","Dumkrat","Sviatoi Piotr","Sviatoi Pavel","Samson","Sviatoi Yakov","Esperans","Sviatoi Nikolai","Lansdou","Richmond","Sviatoi Ilya","Endracht","Kreyser","Rossiya","Vakhmeister","Mitau","Printsessa Anna","Gektor","Voin","Kavalier","Merkurius","Apollon","Yagudiil","Arkhangel Mikhail","Kreyser","Vakhtmeister","Rossiya","Sviatoi Mikhail","Sviatoi Sergii","Gremiaschii","Nadezhda","Afrika","Sviatoi Fiodor","Vestovoi","Nadezhda Blagopoluchiya","Sviatoi Aleksandr","Vtoraya Ekaterina","Pochtalyon","Paros","Pobeda","Sviatoi Nikolai","Sviatoi Pavel","Slava","Konstantsiya","Pomoschnyi","Ungaria","Bohemia","Pavel","Astafii","Nataliya","Liogkii","Stchastlivyi","Sviatoi Mikhail","Pospeshnyi","Aleksandr","Voin","Mariya","Patrikii","Simion","Nadezhda","Slava","Vozamislav","Podrazhislav","Blagopoluchiya","Gektor","Mstislavets","Yaroslavets","Riga","Premislav","Briachislav","Arkhangel Gavriil","Pomoschnyi","Kronstadt","Arkhipelag","Narva","Revela","Riga","Arkhangel Mikhail","Rafail","Stchastlivyi","Emmanuil","Emprenabla","Pospeshnyi","Tikhvenskaya Bogoroditsa","Feodosii Totemskii","Speshnyi","Argus","Bystryi","Merkurii","Patrikii","Liogkii","Patrikii","Merkurii","Provornyi","Vestovoi","Konstantin","Aleksandra","Mariya","Olga","Kniaginia Lovitch","Elisaveta","Ekaterina","Anna","Prints Oranskii","Neva","Bellona","Yunona","Pomona","Tserera","Kastor","Amfitrida","Prozerpina","Diana","Avrora","Melapomena","Konstantin","Liogkii","Neva","Geroi","Amfitrida","Avtroil","Arkhipelag","Argus","Diana","Avtroil","Liogkiy","Kastor","Poluks","Venera","Sveaborg","Poluks","Rossiya","Neva","Pomona","Pomoschnyi","Provornyi","Pospeshnyi","Gektor","Kreyser","Aleksandr Nevskii","Kastor","Elena","Aleksandr Nevskii","Pallada","Diana","Narva","Borodino","Vilagosh","Sysoi Velikii","Pervyi","Vtoroi","Tretiy","Chetviortyi","Piatyi","Shestoi","Sedamoi","Vosamoi","Deviatyi","Desiatyi","Odinnadtsatyi","Dvenadtsatyi","Trinadtsatyi","Chetyrnadtsatyi","Piatnadtsatyi","Shestnadtsatyii","Vestnik","Grigorii Velikiya Armenii","Sergii Chudotvorets","Nikolai Chudotvorets","Sviatoi Georgii Pobedonosets","Taganrog","Kinburn","Berislav","Fanagoriya","Apostol Andrei","Aleksandr Nevskii","Piotr Apostol","Ioann Bogoslov","Tsara Konstantin","Fiodor Stratilat","Kazanskaya Bogoroditsa","Nikolai Belomorskii","Sviatoi Matvei","Sviatoi Nikolai","Grigorii Velikiya Armenii","Ioann Zlatoust","Pospeshnyi","Stchastlivyi","Liogkii","Mikhail","Nazaret","Krepkii","Voin","Afrika","Minerva","Vezul","Speshnyi","Evstafii","Flora","Pospeshnyi","Shtandart","Rafail","Tenedos","Erivana","Arkhipelag","Varna","Enos","Burgas","Agatopola","Brailov","Flora","Mesemvriya","Sizopola","Midiya","Kagul","Kovarna","Kulevtchi","Kavkaz","Astrakhana","Kizliar","Tsaritsyn","Provornyi","Ekaterina","Aleksandr","Aleksandra","Elena","Konstantin","Mariya","Nikolai","Pavel","Aleksandr","Ekaterina","Elizaveta","Mariya","Konstantin","Nikolai","Bogoyavleniye Gospodne","Emmanuil","Vifleem","Petergof","Bodryi","Neva","Sveaborg","Torneo","Mirnyi","Nadezhda","Malyi","Uraniya","Rossiya","Nadezhda","Otvazhnosta","Postoyanstvo","Vernosta","Uspekh","Nadezhda","Karlskron-Vapen","Venker","Dansk-Ern","Kisken","Star Feniks","Brilyant","Ulriksdal","Arkhipelag","Naktsiya","Minerva","Avtroil","Oden","Gelgomar"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_PORTUGAL') or pCiv == gc.getInfoTypeForString('CIVILIZATION_PORTUGAL_EUROPE'):
			lName = ["Nuno Tristao","Diogo Gomes","Diogo Cao","Corte Real","Pero Escobar","Alvares Cabral","Pacheco Pereira","Dom Francisco","Vasco da Gama","Almirante Pereira","Almirante Gago Coutinho","Almirante Magalhaes","Joao Belo","Comandante Roberto Ivens","Hermenegildo","Sacadura Cabral","Vasco da Gama","Alvares Cabral","Corte Real","Bartolomeu Dias","Almeida","Cacine","Cunene","Mandovi","Rovuma","Cuanza","Geba","Zaire","Zambeze","Limpopo","Save","Espadarte","Foca","Golfinho","Hidra","Delfim","Espadarte","Golfinho","Neptuno","Narval","Nautilo","Cachalote","Albacora","Barracuda","Delfim II","Tridente","Arpao"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_SWEDEN') or pCiv == gc.getInfoTypeForString('CIVILIZATION_SWEDEN_EUROPE') or gc.getInfoTypeForString('CIVILIZATION_DENMARK') or pCiv == gc.getInfoTypeForString('CIVILIZATION_DENMARK_EUROPE'):
			lName = ["Elefant","Finska Svan","Svenska Hektor","St Christopher","Engel","St Erik","Mars","Enharning","Stockholms Hjort","Fargylta Dufva","Hjort","Rada Hund","Bramare","Lilla Svan","Lilia","Lilla Hjort","Skotska Pincka","Hector","Svenska Morian","Tranheje","Bruna Lejon","Memnon","Jonas von Emden","Hollands Galej","Kalmar Bark","Rada Drake","Roda Lejon","Finska Memnon","Trekronor","Vasa","Applet","Applet","Applet","Kristina","Jupiter","Mars","Krona Ark","Gata Ark","Scepter","Patentia","Oldenburg","Tre Lejon","Andromeda","Vestervik","Cesar","Maria","Sankt Anna","Wismar","Herkules","Carolus","Merkurius","Falk","Amarant","Apollo","Drake","Mane","Gateborg","Viktoria","Pelikan","Ulven","Fenix","Andromeda","Monikendam","Apple","SvA쨝d","Saturnus","Wrangel","Nyckel","Jupiter","Spes","Mars","Sol","Venus","Krona","Svenska Lejon","Wismar","Merkurius","Neptunus","Sankt Hieronymus","Wrangels Pallats","Lax","Kalmar","Carolus XI","Carolus IX","Drottning Ulrika","Gota","Drottning Hedvig","Karlskrona","Wrangel","Upland","Hercules","Drottning Ulrika","Wachtmeister","Prins Carl","Halland","Gotland","Lifland","Bleking","Estland","Osel","Konung Karl","Carolus IX","Mane","Carolus","Princessa Hedvig","Wenden","Viktoria","Pommern","Ulrika Eleanora","Sadermanland","Kalmar","Stettin","Enighet","Westmanland","Goteborg","SkA쩸e","Wrede","Frederika Amalia","Norrkoping","Wismar","Warberg","Elfsborg","Gatha Lejon","Nordstjerna","Prins Carl Fredrik","Bremen","Aland","Tre Kroner","Werden","Riga","Stockholm","Prinsessa Fredrika","Halmstad","Prins Carl","Kronskepp","Ulrika Eleonora","Greve Sparre","Sophia Charlotta","Fred","Frihet","Drottningholm","Hessen Cassel","Enighet","Sverige","Prins Wilhelm","Finland","Adolf Fredrik","Sparre","Uppland","Sadermanland","Gotha","Fredrik Rex","Prins Carl","Prins Gustaf","(Prinsessin) Sophia Albertina","Sofia Magdalena","Fredrik Adolph","Adolph Fredrik","Gustaf III","Kronprins Gustaf","Wladislaff","Louisa Ulrika","Konung Gustaf IV","Carl XIII","Karl XIV Johan","Prins Oscar","Gustav den Store","Stockholm"]
		elif pCiv == gc.getInfoTypeForString('CIVILIZATION_AUSTRIA'):
			lName = ["Viribus Unitis","Tegetthoff","Prinz Eugen","Szent Istvan","Habsburg","Arpad","Babenberg","Erzherzog Karl","Erzherzog Friedrich","Erzherzog Ferdinand Max","Franz Ferdinand","Radetzky","Zrinyi","Budapest","Monarch","Wien","Kronprinz Rudolf","Kronprinzessin","Maria Theresia","Kaiser Karl VI","Sankt Georg","Venus","Bellona","Novara","Schwarzenberg","Minerva","Diana","Carolina","Pylades","Pola","Montecuccoli","Hussar","Arthemisia","Arethusa","Saida","Dromedar","Bravo","Fido","Camaeleon","Roma","Jupiter","Gorzkowski","Messagiere","Achilles","Taurus","Vulkan","Custoza","Curtatone","Santa Lucia","Volta","Prinz Eugen","Kaiserin Elisabeth","Radetzky","Adria","Donau","Novara","Schwarzenberg","Laudon","Erzherzog Friedrich","Dandolo","Donau","Saida","Donau","Helgoland","Fasana","Zrinyi","Aurora","Frundsberg","Zara","Spalato","Sebenico","Lussin","Panther","Leopard","Tiger","Franz Joseph I","Kaiserin Elisabeth","Aspern","Szigetvar","Zenta","Admiral Spaun","Saida","Helgoland","Novara","Boa","Warasinder","Drache","Salamander","Kaiser Max","Prinz Eugen","Ferdinand Max","Habsburg","Lissa","Maros","Leitha","Kaiser","Custozza","Erzherzog Albrecht","Kaiser Max","Prinz Eugen","Tegetthoff","Temes and SMS Bodrog","Enns","Bosna","Novara","Kaiser","Meteor","Blitz","Komet","Planet","Trabant","Satellit","Magnet"]
		if pCiv == gc.getInfoTypeForString('CIVILIZATION_ENGLAND') or pCiv == gc.getInfoTypeForString('CIVILIZATION_ENGLAND_EUROPE'):
			sFull = "HMS "+lName[CyGame().getSorenRandNum(len(lName), "Name Gen")-1]
		else:
			sFull = lName[CyGame().getSorenRandNum(len(lName), "Name Gen")-1]
		# trim name length
		if len(sFull) > 25:
			sFull = sFull[:25]        
		return sFull
#end shipname modcomp

#################### TRIGGERED EVENTS ##################

	def __eventEditCityNameBegin(self, city, bRename):
		popup = CyPopup(CvUtil.EventEditCityName, EventContextTypes.EVENTCONTEXT_ALL, True)
		popup.setUserData((city.getID(), bRename))
		popup.setHeaderString(localText.getText("TXT_KEY_NAME_CITY", ()), CvUtil.FONT_CENTER_JUSTIFY)
		popup.setBodyString(localText.getText("TXT_KEY_SETTLE_NEW_CITY_NAME", ()), CvUtil.FONT_CENTER_JUSTIFY)
		popup.createEditBox(city.getName(), 0)
		popup.setEditBoxMaxCharCount( 15, 32, 0 )
		popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

	def __eventEditCityNameApply(self, playerID, userData, popupReturn):

		'Edit City Name Event'
		iCityID = userData[0]
		bRename = userData[1]
		player = gc.getPlayer(playerID)
		city = player.getCity(iCityID)
		cityName = popupReturn.getEditBoxString(0)
		if (len(cityName) > 30):
			cityName = cityName[:30]
		city.setName(cityName, not bRename)

	def __eventCreateTradeRouteBegin(self, PlayerID):
		popup = CyPopup(CvUtil.EventCreateTradeRoute, EventContextTypes.EVENTCONTEXT_ALL, 1)
		popup.setHeaderString(localText.getText("TXT_KEY_CREATE_TRADE_ROUTE", ()), CvUtil.FONT_LEFT_JUSTIFY)

		popup.setBodyString(localText.getText("TXT_KEY_SOURCE", ()), CvUtil.FONT_LEFT_JUSTIFY)
		popup.createPullDown(0)
		popup.addPullDownString(localText.getText("TXT_KEY_NO_SOURCE", ()), -1, 0)
		player = gc.getPlayer(PlayerID)
		for iPlayer in range(gc.getMAX_PLAYERS()):
			loopPlayer = gc.getPlayer(iPlayer)
			if (loopPlayer.isAlive() and player.canLoadYield(iPlayer)):
				(pCity, iter) = loopPlayer.firstCity(false)
				while (pCity):
					iId = gc.getMAX_PLAYERS() * pCity.getID() + pCity.getOwner()
					popup.addPullDownString(pCity.getName(), iId, 0)
					(pCity, iter) = loopPlayer.nextCity(iter, false)

		popup.setBodyString(localText.getText("TXT_KEY_DESTINATION", ()), CvUtil.FONT_LEFT_JUSTIFY)
		popup.createPullDown(1)
		popup.addPullDownString(localText.getText("TXT_KEY_NO_DESTINATION", ()), -1, 1)
		for iPlayer in range(gc.getMAX_PLAYERS()):
			loopPlayer = gc.getPlayer(iPlayer)
			if (loopPlayer.isAlive() and player.canUnloadYield(iPlayer)):
				(pCity, iter) = loopPlayer.firstCity(false)
				while (pCity):
					iId = gc.getMAX_PLAYERS() * pCity.getID() + pCity.getOwner()
					popup.addPullDownString(pCity.getName(), iId, 1)
					(pCity, iter) = loopPlayer.nextCity(iter, false)

		popup.setBodyString(localText.getText("TXT_KEY_YIELD", ()), CvUtil.FONT_LEFT_JUSTIFY)
		popup.createPullDown(2)
		popup.addPullDownString(localText.getText("TXT_KEY_NO_YIELD", ()), -1, 2)
		for i in range( YieldTypes.NUM_YIELD_TYPES ):
			if (gc.getYieldInfo(i).isCargo()):
				popup.addPullDownString(gc.getYieldInfo(i).getDescription(), i, 2)

		popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

	def __eventCreateTradeRouteApply(self, playerID, userData, popupReturn):
		'Create Trade Route Event'
		if (popupReturn.getSelectedPullDownValue(0) != -1 and popupReturn.getSelectedPullDownValue(1) != -1 and popupReturn.getSelectedPullDownValue(2) != -1):
			iSourceCityID = popupReturn.getSelectedPullDownValue( 0 ) / gc.getMAX_PLAYERS()
			iSourceCityPlayer = popupReturn.getSelectedPullDownValue( 0 ) % gc.getMAX_PLAYERS()
			iDestinationCityID = popupReturn.getSelectedPullDownValue( 1 ) / gc.getMAX_PLAYERS()
			iDestinationCityPlayer = popupReturn.getSelectedPullDownValue( 1 ) % gc.getMAX_PLAYERS()
			iYieldType = popupReturn.getSelectedPullDownValue( 2 )

			player = gc.getPlayer(playerID)
			gc.getPlayer(playerID).addTradeRoute(iSourceCityPlayer, iSourceCityID, iDestinationCityPlayer, iDestinationCityID, iYieldType)

	def __eventEditTradeRouteBegin(self, playerID, iRouteID):
		popup = CyPopup(CvUtil.EventEditTradeRoute, EventContextTypes.EVENTCONTEXT_ALL, 1)
		popup.setHeaderString(localText.getText("TXT_KEY_EDIT_TRADE_ROUTE", ()), CvUtil.FONT_LEFT_JUSTIFY)

		player = gc.getPlayer(playerID)
		pRoute = player.getTradeRoute(iRouteID)
		popup.setUserData((iRouteID,))

		popup.setBodyString(localText.getText("TXT_KEY_SOURCE", ()), CvUtil.FONT_LEFT_JUSTIFY)
		popup.createPullDown(0)
		for iPlayer in range(gc.getMAX_PLAYERS()):
			loopPlayer = gc.getPlayer(iPlayer)
			if (loopPlayer.isAlive() and player.canLoadYield(iPlayer)):
				(pCity, iter) = loopPlayer.firstCity(false)
				while (pCity):
					iId = gc.getMAX_PLAYERS() * pCity.getID() + pCity.getOwner()
					popup.addPullDownString(pCity.getName(), iId, 0)
					if (pRoute.getSourceCity().iID == pCity.getID() and pRoute.getSourceCity().eOwner == pCity.getOwner()):
						popup.setSelectedPulldownID(iId, 0);
					(pCity, iter) = loopPlayer.nextCity(iter, false)

		popup.setBodyString(localText.getText("TXT_KEY_DESTINATION", ()), CvUtil.FONT_LEFT_JUSTIFY)
		popup.createPullDown(1)
		for iPlayer in range(gc.getMAX_PLAYERS()):
			loopPlayer = gc.getPlayer(iPlayer)
			if (loopPlayer.isAlive() and player.canUnloadYield(iPlayer)):
				(pCity, iter) = loopPlayer.firstCity(false)
				while (pCity):
					iId = gc.getMAX_PLAYERS() * pCity.getID() + pCity.getOwner()
					popup.addPullDownString(pCity.getName(), iId, 1)
					if (pRoute.getDestinationCity().iID == pCity.getID() and pRoute.getDestinationCity().eOwner == pCity.getOwner()):
						popup.setSelectedPulldownID(iId, 1);
					(pCity, iter) = loopPlayer.nextCity(iter, false)

				if player.canTradeWithEurope():
					popup.addPullDownString(localText.getText("TXT_KEY_CONCEPT_EUROPE", ()), -1, 1)

				if (pRoute.getDestinationCity().iID == -1 and pRoute.getDestinationCity().eOwner == playerID):
					popup.setSelectedPulldownID(-1, 1);


		popup.setBodyString(localText.getText("TXT_KEY_YIELD", ()), CvUtil.FONT_LEFT_JUSTIFY)
		popup.createPullDown(2)
		for i in range( YieldTypes.NUM_YIELD_TYPES ):
			if (gc.getYieldInfo(i).isCargo()):
				popup.addPullDownString(gc.getYieldInfo(i).getDescription(), i, 2)
		popup.setSelectedPulldownID(pRoute.getYield(), 2);

		popup.createCheckBoxes( 1, 3 )
		popup.setCheckBoxText( 0, localText.getText("TXT_KEY_DELETE_TRADE_ROUTE", ()), 3 )

		popup.createCheckBoxes( 1, 4 )
		popup.setCheckBoxText( 0, localText.getText("TXT_KEY_CREATE_TRADE_ROUTE", ()), 4 )

		popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

	def __eventEditTradeRouteApply(self, PlayerID, userData, popupReturn):
		'Edit Trade Route Event'
		iSourceCityID = popupReturn.getSelectedPullDownValue( 0 ) / gc.getMAX_PLAYERS()
		iSourceCityPlayer = popupReturn.getSelectedPullDownValue( 0 ) % gc.getMAX_PLAYERS()
		iDestinationCityID = popupReturn.getSelectedPullDownValue( 1 ) / gc.getMAX_PLAYERS()
		iDestinationCityPlayer = popupReturn.getSelectedPullDownValue( 1 ) % gc.getMAX_PLAYERS()
		iYieldType = popupReturn.getSelectedPullDownValue( 2 )

		iRouteID = userData[0]
		player = gc.getPlayer(PlayerID)

		if (popupReturn.getCheckboxBitfield(3)):
			player.removeTradeRoute(iRouteID)
		elif (popupReturn.getCheckboxBitfield(4)):
			player.addTradeRoute(iSourceCityPlayer, iSourceCityID, iDestinationCityPlayer, iDestinationCityID, iYieldType)
		else:
			player.editTradeRoute(iRouteID, iSourceCityPlayer, iSourceCityID, iDestinationCityPlayer, iDestinationCityID, iYieldType)

	def __eventEditCityBegin(self, argsList):
		'Edit City Event'
		px,py = argsList
		CvWBPopups.CvWBPopups().initEditCity(argsList)

	def __eventEditCityApply(self, playerID, userData, popupReturn):
		'Edit City Event Apply'
		if (getChtLvl() > 0):
			CvWBPopups.CvWBPopups().applyEditCity( (popupReturn, userData) )

	def __eventPlaceObjectBegin(self, argsList):
		'Place Object Event'
		CvDebugTools.CvDebugTools().initUnitPicker(argsList)

	def __eventPlaceObjectApply(self, playerID, userData, popupReturn):
		'Place Object Event Apply'
		if (getChtLvl() > 0):
			CvDebugTools.CvDebugTools().applyUnitPicker( (popupReturn, userData) )
	def __EventAwardGoldBegin(self, argsList):
		'Award Gold Event'
		CvDebugTools.CvDebugTools().cheatGold()
	def __EventAwardGoldApply(self, playerID, netUserData, popupReturn):
		'Award Gold Event Apply'

		if (getChtLvl() > 0):
			CvDebugTools.CvDebugTools().applyGoldCheat( (popupReturn) )

	def __eventShowWonderBegin(self, argsList):
		'Show Wonder Event'
		CvDebugTools.CvDebugTools().wonderMovie()

	def __eventShowWonderApply(self, playerID, netUserData, popupReturn):
		'Wonder Movie Apply'
		if (getChtLvl() > 0):
			CvDebugTools.CvDebugTools().applyWonderMovie( (popupReturn) )

	def __eventEditUnitNameBegin(self, argsList):
		pUnit = argsList
		popup = CyPopup(CvUtil.EventEditUnitName, EventContextTypes.EVENTCONTEXT_ALL, True)
		popup.setUserData((pUnit.getID(),))
		popup.setBodyString(localText.getText("TXT_KEY_RENAME_UNIT", ()), CvUtil.FONT_CENTER_JUSTIFY)
		popup.createEditBox(pUnit.getNameNoDesc(), 0)
		popup.setEditBoxMaxCharCount(20, 20, 0)
		popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

	def __eventEditUnitNameApply(self, playerID, userData, popupReturn):

		'Edit Unit Name Event'
		iUnitID = userData[0]
		unit = gc.getPlayer(playerID).getUnit(iUnitID)
		newName = popupReturn.getEditBoxString(0)
		if (len(newName) > 25):
			newName = newName[:25]
		unit.setName(newName)

	def __eventWBAllPlotsPopupBegin(self, argsList):
		CvScreensInterface.getWorldBuilderScreen().allPlotsCB()
		return
	def __eventWBAllPlotsPopupApply(self, playerID, userData, popupReturn):
		if (popupReturn.getButtonClicked() >= 0):
			CvScreensInterface.getWorldBuilderScreen().handleAllPlotsCB(popupReturn)
		return

	def __eventWBLandmarkPopupBegin(self, argsList):
		CvScreensInterface.getWorldBuilderScreen().setLandmarkCB("")
		return

	def __eventWBLandmarkPopupApply(self, playerID, userData, popupReturn):
		if (popupReturn.getEditBoxString(0)):
			szLandmark = popupReturn.getEditBoxString(0)
			if (len(szLandmark)):
				CvScreensInterface.getWorldBuilderScreen().setLandmarkCB(szLandmark)
		return

	def __eventWBScriptPopupBegin(self, argsList):
		popup = CyPopup(CvUtil.EventWBScriptPopup, EventContextTypes.EVENTCONTEXT_ALL, True)
		popup.setHeaderString(localText.getText("TXT_KEY_WB_SCRIPT", ()), CvUtil.FONT_CENTER_JUSTIFY)
		popup.createEditBox(CvScreensInterface.getWorldBuilderScreen().getCurrentScript(), 0)
		popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

	def __eventWBScriptPopupApply(self, playerID, userData, popupReturn):
		if (popupReturn.getEditBoxString(0)):
			szScriptName = popupReturn.getEditBoxString(0)
			CvScreensInterface.getWorldBuilderScreen().setScriptCB(szScriptName)

	def __eventWBStartYearPopupBegin(self, argsList):
		popup = CyPopup(CvUtil.EventWBStartYearPopup, EventContextTypes.EVENTCONTEXT_ALL, True)
		popup.createSpinBox(0, "", gc.getGame().getStartYear(), 1, 5000, -5000)
		popup.launch(true, PopupStates.POPUPSTATE_IMMEDIATE)

	def __eventWBStartYearPopupApply(self, playerID, userData, popupReturn):
		iStartYear = popupReturn.getSpinnerWidgetValue(int(0))
		CvScreensInterface.getWorldBuilderScreen().setStartYearCB(iStartYear)
		
