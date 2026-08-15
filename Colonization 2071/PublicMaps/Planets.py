#
#	FILE:	 Donut.py
#	AUTHOR:  Bob Thomas (Sirian)
#	PURPOSE: Global map script - Circular continent with center region.
#-----------------------------------------------------------------------------
#	Copyright (c) 2007 Firaxis Games, Inc. All rights reserved.
#-----------------------------------------------------------------------------
#

from CvPythonExtensions import *
import CvUtil
import CvMapGeneratorUtil
from CvMapGeneratorUtil import TerrainGenerator
from math import sqrt
import random
import sys

latitudes = []

def getDescription():
	return "Several planets to colonize."

def getWrapX():
	return false
	
def getWrapY():
	return false

def isAdvancedMap():
	"This map should not show up in simple mode"
	return 0

def isClimateMap():
	return 0

def isSeaLevelMap():
	return 0

def getGridSize(argsList):
	"Override Grid Size function to make the maps square."
	grid_sizes = {
		WorldSizeTypes.WORLDSIZE_TINY:      (8*4,8*4),
		WorldSizeTypes.WORLDSIZE_SMALL:     (10*4,10*4),
		WorldSizeTypes.WORLDSIZE_STANDARD:  (13*4,13*4),
		WorldSizeTypes.WORLDSIZE_LARGE:     (16*4,16*4),
		WorldSizeTypes.WORLDSIZE_HUGE:      (20*4,20*4)
	}

	if (argsList[0] == -1): # (-1,) is passed to function on loads
		return []
	[eWorldSize] = argsList
	return grid_sizes[eWorldSize]

def minStartingDistanceModifier():
	return -12

class DonutFractalWorld(CvMapGeneratorUtil.FractalWorld):
	def generatePlotTypes(self, water_percent=78, shift_plot_types=True, grain_amount=3):
		gc = CyGlobalContext()		#Why don't they put this at the top of ALL files?
	
		self.hillsFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, grain_amount, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.peaksFrac.fracInit(self.iNumPlotsX, self.iNumPlotsY, grain_amount+1, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		iHillsBottom1 = self.hillsFrac.getHeightFromPercent(max((self.hillGroupOneBase - self.hillGroupOneRange), 0))
		iHillsTop1 = self.hillsFrac.getHeightFromPercent(min((self.hillGroupOneBase + self.hillGroupOneRange), 100))
		iHillsBottom2 = self.hillsFrac.getHeightFromPercent(max((self.hillGroupTwoBase - self.hillGroupTwoRange), 0))
		iHillsTop2 = self.hillsFrac.getHeightFromPercent(min((self.hillGroupTwoBase + self.hillGroupTwoRange), 100))
		iPeakThreshold = self.peaksFrac.getHeightFromPercent(self.peakPercent)
		
		#Planet at center
		iCenterX = int(self.iNumPlotsX / 2)
		iCenterY = int(self.iNumPlotsY / 2)
		iRadiusMod = round(iCenterX * 2 / 3)
		if CyMap().getWorldSize() == gc.getInfoTypeForString('WORLDSIZE_TINY'):
			iRadiusMod = iCenterX / 2
		iRadius = min((iCenterX - iRadiusMod), (iCenterY - iRadiusMod))
		iHoleRadius = int(iRadius / 2)

		for x in range(self.iNumPlotsX):
			for y in range(self.iNumPlotsY):
				i = y*self.iNumPlotsX + x
				if x == iCenterX and y == iCenterY:
					fDistance = 0
				else:
					fDistance = sqrt(((x - iCenterX) ** 2) + ((y - iCenterY) ** 2))
				if fDistance > iRadius:
					self.plotTypes[i] = PlotTypes.PLOT_OCEAN
				else:
					hillVal = self.hillsFrac.getHeight(x,y)
					if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
						peakVal = self.peaksFrac.getHeight(x,y)
						if (peakVal <= iPeakThreshold):
							self.plotTypes[i] = PlotTypes.PLOT_PEAK
						else:
							self.plotTypes[i] = PlotTypes.PLOT_HILLS
					else:
						self.plotTypes[i] = PlotTypes.PLOT_LAND

		#Other planets
		iCoreX = int(self.iNumPlotsX / 2)
		iCoreY = int(self.iNumPlotsY / 2)
		iNumPlanets = (gc.getMap().getWorldSize() * 2)
		
		for i in range(iNumPlanets):
			iAttemptsLeft = 200
			while iAttemptsLeft > 0:
				iCenterX = CyGame().getSorenRandNum(gc.getMap().getGridWidth(), "Center X")
				iCenterY = CyGame().getSorenRandNum(gc.getMap().getGridHeight(), "Center Y")
				iRadiusMod = self.getRadius(iCoreX)
				iRadius = min((iCoreX - iRadiusMod), (iCoreY - iRadiusMod))
				iHoleRadius = int(iRadius / 2)
				
				if not (self.isValidPlanet(iCenterX, iCenterY, iRadius)):
					iAttemptsLeft -= 1
					continue

				for x in range(self.iNumPlotsX):
					for y in range(self.iNumPlotsY):
						i = y*self.iNumPlotsX + x
						if x == iCenterX and y == iCenterY:
							fDistance = 0
						else:
							fDistance = sqrt(((x - iCenterX) ** 2) + ((y - iCenterY) ** 2))
						if fDistance < iRadius:
							hillVal = self.hillsFrac.getHeight(x,y)
							if ((hillVal >= iHillsBottom1 and hillVal <= iHillsTop1) or (hillVal >= iHillsBottom2 and hillVal <= iHillsTop2)):
								peakVal = self.peaksFrac.getHeight(x,y)
								if (peakVal <= iPeakThreshold):
									self.plotTypes[i] = PlotTypes.PLOT_PEAK
								else:
									self.plotTypes[i] = PlotTypes.PLOT_HILLS
							else:
								self.plotTypes[i] = PlotTypes.PLOT_LAND
				break

		if shift_plot_types:
			self.shiftPlotTypes()

		return self.plotTypes

	def isValidPlanet(self, iCenterX, iCenterY, iRadius):
		if ((iCenterX - iRadius < 1) or (iCenterY - iRadius < 1) or (iCenterX + iRadius + 1 > self.iNumPlotsX) or (iCenterY + iRadius + 1 > self.iNumPlotsY)):
			return false
	
		for x in range(self.iNumPlotsX):
			for y in range(self.iNumPlotsY):
				i = y*self.iNumPlotsX + x
				if x == iCenterX and y == iCenterY:
					fDistance = 0
				else:
					fDistance = sqrt(((x - iCenterX) ** 2) + ((y - iCenterY) ** 2))
				if fDistance < iRadius+2:
					if self.plotTypes[i] != PlotTypes.PLOT_OCEAN:
						return false

		return true

	def getRadius(self, iCoreX):
		bValid = false
		for i in range(2000):
			iRadiusMod = (iCoreX * (CyGame().getSorenRandNum(3, "Radius")) / 2)
			if round(iCoreX * 3 / 4) < iRadiusMod < iCoreX:
				bValid = true
				break

		if bValid == false:
			iRadiusMod = round(iCoreX * 3 / 4)

		return iRadiusMod

def generatePlotTypes():
	NiTextOut("Setting Plot Types (Python Donut) ...")
	fractal_world = DonutFractalWorld()
	return fractal_world.generatePlotTypes()

def generateTerrainTypes():
	NiTextOut("Generating Terrain (Python Donut) ...")
	terraingen = TerrainGenerator()
	terrainTypes = terraingen.generateTerrain()
	return terrainTypes

class DonutFeatureGenerator(CvMapGeneratorUtil.FeatureGenerator):
	def __init__(self, iJunglePercent=70, iForestPercent=75, iLightForestPercent=45,
	             jungle_grain=5, forest_grain=6,
	             fracXExp=-1, fracYExp=-1):

		self.gc = CyGlobalContext()
		self.map = CyMap()
		self.mapRand = self.gc.getGame().getMapRand()
		self.jungles = CyFractal()
		self.forests = CyFractal()
		self.lightForests = CyFractal()

		self.iFlags = 0  # Disallow FRAC_POLAR flag, to prevent "zero row" problems.
		if self.map.isWrapX(): self.iFlags += CyFractal.FracVals.FRAC_WRAP_X
		if self.map.isWrapY(): self.iFlags += CyFractal.FracVals.FRAC_WRAP_Y

		self.iGridW = self.map.getGridWidth()
		self.iGridH = self.map.getGridHeight()

		self.iJunglePercent = iJunglePercent
		self.iForestPercent = iForestPercent
		self.iLightForestPercent = iLightForestPercent

		jungle_grain += self.gc.getWorldInfo(self.map.getWorldSize()).getFeatureGrainChange()
		forest_grain += self.gc.getWorldInfo(self.map.getWorldSize()).getFeatureGrainChange()

		self.jungle_grain = jungle_grain
		self.forest_grain = forest_grain

		self.fracXExp = fracXExp
		self.fracYExp = fracYExp

		self.__initFractals()
		self.__initFeatureTypes()

	def __initFractals(self):
		self.jungles.fracInit(self.iGridW, self.iGridH, self.jungle_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.forests.fracInit(self.iGridW, self.iGridH, self.forest_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)
		self.lightForests.fracInit(self.iGridW, self.iGridH, self.forest_grain, self.mapRand, self.iFlags, self.fracXExp, self.fracYExp)

		self.iJungleBottom = self.jungles.getHeightFromPercent(50 - (self.iJunglePercent / 2))
		self.iJungleTop = self.jungles.getHeightFromPercent(50 + (self.iJunglePercent / 2))
		self.iForestLevel = self.forests.getHeightFromPercent(100 - self.iForestPercent)
		self.iLightForestLevel = self.lightForests.getHeightFromPercent(100 - self.iLightForestPercent)

	def __initFeatureTypes(self):
		self.featureIce = self.gc.getInfoTypeForString("FEATURE_ICE")
		self.featureJungle = self.gc.getInfoTypeForString("FEATURE_JUNGLE")
		self.featureForest = self.gc.getInfoTypeForString("FEATURE_FOREST")
		self.featureLightForest = self.gc.getInfoTypeForString("FEATURE_LIGHT_FOREST")
		self.featureFungalForest = self.gc.getInfoTypeForString("FEATURE_FUNGAL_FOREST")
		self.featureCycadForest = self.gc.getInfoTypeForString("FEATURE_CYCAD_FOREST")
		self.featureLuminousForest = self.gc.getInfoTypeForString("FEATURE_LUMINOUS_FOREST")
		self.featurePetrifiedForest = self.gc.getInfoTypeForString("FEATURE_PETRIFIED_FOREST")
		self.featureScrub = self.gc.getInfoTypeForString("FEATURE_SCRUB")
		self.featureScrub = self.gc.getInfoTypeForString("FEATURE_LAKE")
		self.featureScrub = self.gc.getInfoTypeForString("FEATURE_LAGOON")
		self.featureScrub = self.gc.getInfoTypeForString("FEATURE_VOLCANO")

	def addFeatures(self):
		"adds features to all plots as appropriate"
		for iX in range(self.iGridW):
			for iY in range(self.iGridH):
				self.addFeaturesAtPlot(iX, iY)

	def getLatitudeAtPlot(self, iX, iY):
		"returns a value in the range of 0.0 (tropical) to 1.0 (polar)"
		return abs((self.iGridH/2) - iY)/float(self.iGridH/2) # 0.0 = equator, 1.0 = pole

	def addFeaturesAtPlot(self, iX, iY):
		"adds any appropriate features at the plot (iX, iY) where (0,0) is in the SW"
		lat = self.getLatitudeAtPlot(iX, iY)
		pPlot = self.map.sPlot(iX, iY)

		for iI in range(self.gc.getNumFeatureInfos()):
			if pPlot.canHaveFeature(iI):
				if self.mapRand.get(10000, "Add Feature PYTHON") < self.gc.getFeatureInfo(iI).getAppearanceProbability():
					pPlot.setFeatureType(iI, -1)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addIceAtPlot(pPlot, iX, iY, lat)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addJunglesAtPlot(pPlot, iX, iY, lat)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addLightForestsAtPlot(pPlot, iX, iY, lat)

		if (pPlot.getFeatureType() == FeatureTypes.NO_FEATURE):
			self.addForestsAtPlot(pPlot, iX, iY, lat)

	def addIceAtPlot(self, pPlot, iX, iY, lat):
		if pPlot.canHaveFeature(self.featureIce):
			if (self.map.isWrapX() and not self.map.isWrapY()) and (iY == 0 or iY == self.iGridH - 1):
				pPlot.setFeatureType(self.featureIce, -1)
			elif (self.map.isWrapY() and not self.map.isWrapX()) and (iX == 0 or iX == self.iGridW - 1):
				pPlot.setFeatureType(self.featureIce, -1)
			elif (not self.map.isWrapY()):
				rand = self.mapRand.get(100, "Add Ice PYTHON")/100.0
				if rand < 8 * (lat - (1.0 - (self.gc.getClimateInfo(self.map.getClimate()).getRandIceLatitude() / 2.0))):
					pPlot.setFeatureType(self.featureIce, -1)
				elif rand < 4 * (lat - (1.0 - self.gc.getClimateInfo(self.map.getClimate()).getRandIceLatitude())):
					pPlot.setFeatureType(self.featureIce, -1)

	def addJunglesAtPlot(self, pPlot, iX, iY, lat):
		if pPlot.canHaveFeature(self.featureJungle):
			iJungleHeight = self.jungles.getHeight(iX, iY)
			if self.iJungleTop >= iJungleHeight >= self.iJungleBottom + (self.iJungleTop - self.iJungleBottom)*self.gc.getClimateInfo(self.map.getClimate()).getJungleLatitude()*lat:
				game = CyGame()
				iForestRnd = game.getSorenRandNum(20, "Cycad Forests")
				if iForestRnd >= 12 and pPlot.canHaveFeature(self.featureCycadForest):
					pPlot.setFeatureType(self.featureCycadForest, -1)
				elif iForestRnd >= 7 and pPlot.canHaveFeature(self.featurePetrifiedForest):
					pPlot.setFeatureType(self.featurePetrifiedForest, -1)
				elif iForestRnd >= 2 and pPlot.canHaveFeature(self.featureFungalForest):
					pPlot.setFeatureType(self.featureFungalForest, -1)
				else:
					pPlot.setFeatureType(self.featureJungle, -1)

	def addLightForestsAtPlot(self, pPlot, iX, iY, lat):
		if pPlot.canHaveFeature(self.featureLightForest):
			if self.lightForests.getHeight(iX, iY) >= self.iLightForestLevel:
				game = CyGame()
				iForestRnd = game.getSorenRandNum(20, "Light Forests")
				if iForestRnd >= 15 and pPlot.canHaveFeature(self.featureFungalForest):
					pPlot.setFeatureType(self.featureFungalForest, -1)
				elif iForestRnd >= 10 and pPlot.canHaveFeature(self.featureLuminousForest):
					pPlot.setFeatureType(self.featureLuminousForest, -1)
				elif iForestRnd >= 5 and pPlot.canHaveFeature(self.featurePetrifiedForest):
					pPlot.setFeatureType(self.featurePetrifiedForest, -1)
				else:
					pPlot.setFeatureType(self.featureLightForest, -1)

	def addForestsAtPlot(self, pPlot, iX, iY, lat):
		if pPlot.canHaveFeature(self.featureForest):
			if self.forests.getHeight(iX, iY) >= self.iForestLevel:
				game = CyGame()
				iForestRnd = game.getSorenRandNum(20, "Light Forests")
				if iForestRnd >= 14:
					self.addLightForestsAtPlot(pPlot, iX, iY, lat)
				else:
					pPlot.setFeatureType(self.featureForest, -1)
	
def addFeatures():
	NiTextOut("Adding Features (Python Donut) ...")
	featuregen = DonutFeatureGenerator()
	featuregen.addFeatures()
	return 0
