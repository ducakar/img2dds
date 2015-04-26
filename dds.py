#!/usr/bin/python

# Usage:
#
#   ./dds.py /path/to/GameData
#
#  or just
#
#   ./dds.py
#
# in case this is located inside KSP directory.

# The following regex patterens must match the beginning of a texture path (after `GameData/`).

# Excluded texture patterns. These textures will be left intact.
EXCLUDE = [
  'CommunityResourcePack/',
  'CustomBiomes/PluginData/CustomBiomes/',
  'KittopiaSpace/',
  'Kopernicus',
  'OPM/',
  'NavyFish/Plugins/PluginData/',
  'NearFuture.*/(Icons|PluginData)/',
  'RCSBuildAid/Textures/iconAppLauncher\.png$',
  'RealChute/Plugins/PluginData/',
  'RealSolarSystem/Plugins/PluginData/',
  'SCANsat/Icons/',
  'Squad/Contracts/Icons/',
  'TextureReplacer/EnvMap/',
  'TriggerTech/.*/(Textures|ToolbarIcons)/'
]

# Model texture patterns (mipmaps are generated, checked for normal maps).
MODEL = [
  '.*/FX/',
  '.*/Part/',
  '.*/Parts/',
  '.*/part/',
  '.*/parts/',
  '.*/Props/',
  '.*/Spaces/',
  'ART/',
  'ASET/'
  'ASET_Props/',
  'BobCatind/JoolV/',
  'BoulderCo/',
  'BoxSat alpha/',
  'FASA/',
  'JSI/RasterPropMonitor/Library/Components/MFD40x20v2/',
  'KAS/Textures/',
  'KerbalScienceFoundation/Mk3Cockpit/',
  'Kopernicus',
  'KSO/RPM/KSO_PROP/',
  'KSO/RPM/KSO_Laptop_1/KSOS_Laptop\.',
  'KSO/RPM/KSO_Laptop_1/KSOS_Laptop_emis\.',
  'KSO/RPM/KSO_Laptop_1/KSOS_Laptop_norm_NRM\.',
  'Lionhead_Aerospace_Inc/',
  'Part Revamp Extras/',
  'ProceduralFairings/',
  'Regolith/Assets/',
  'RetroFuture/',
  'SnacksPartsByWhyren/',
  'Space Factory Ind/',
  'Squad/SPP/',
  'TantaresLV/',
  'TextureReplacer/',
  'UmbraSpaceIndustries/',
  'WildBlueIndustries/MCM/MultiModule/'
]

# Some non-model textures may match the previous model patterns. Exclude them from models.
NOT_MODEL = [
  '.*/Agencies/.*',
  '.*/Flags/.*',
  '.*/Icons/.*',
  'ASET_Props/MFDs/',
  'AxialAerospace/Props/.*\.png$',
  'B9_Aerospace/Props/B9_MFD/images/',
  'HOME2/Props/.*\.png',
  'KIS/Parts/guide/page.*\.png$',
  'Kopernicus.*height\.png',
  'Space Factory Ind/.*/JSI/.*\.png$',
  'TextureReplacer/Default/(HUD|IVA)NavBall',
  'TextureReplacer/Plugins',
  'UmbraSpaceIndustries/Kolonization/MKS/Assets/(OrbLogisticsIcon|StationManager)'
]

# Scale for model textures.
MODEL_SCALE = 1.0

# Scale for model normal maps.
MODEL_NORMALS_SCALE = 1.0

# Uncomment to shrink normal maps to ~ 1/2 size; i.e. 1024 -> 720, 512 -> 360, 256 -> 180 etc.
MODEL_NORMALS_SCALE = 0.703125

####################################################################################################

import os, re, sys

EXCLUDE   = [re.compile(e) for e in EXCLUDE]
MODEL     = [re.compile(m) for m in MODEL]
NOT_MODEL = [re.compile(m) for m in NOT_MODEL]
IMAGE     = re.compile('.*\.(png|PNG|jpg|JPG|tga|TGA|mbm|MBM)$')
NORMAL    = re.compile('.*(NRM|_nm|_normal)\....$')
PREFIX    = re.compile('.*GameData/')
SYSTEM    = 'linux64' if sys.maxsize > 2**32 else 'linux32'
SYSTEM    = 'win32' if sys.platform == 'win32' else SYSTEM
SYSTEM    = 'osx64' if sys.platform == 'darwin' else SYSTEM
IMG2DDS   = './img2dds/' + SYSTEM + '/img2dds'
IMG2DDS   = 'img2dds\win32\img2dds.exe' if SYSTEM == 'win32' else IMG2DDS
DIR       = sys.argv[1] if len(sys.argv) == 2 else './GameData'
BASEDIR   = re.compile('^(.*GameData).*').sub('\\1', DIR)

for (dirPath, dirNames, fileNames) in os.walk(DIR):
  dirPath = PREFIX.sub('', dirPath.replace('\\', '/'))
  images  = {(dirPath + '/' + name) for name in fileNames if IMAGE.match(name)}
  images -= {i for i in images for e in EXCLUDE if e.match(i)}
  models  = {i for i in images for m in MODEL if m.match(i)}
  models -= {i for i in images for m in NOT_MODEL if m.match(i)}
  normals = {i for i in models if NORMAL.match(i)}

  for i in images:
    path     = BASEDIR + '/' + i
    isNormal = i in normals or os.system(IMG2DDS + ' -N "' + path + '"') == 0
    options  = '-vc'

    if i in models:
      options += 'mnsr ' + str(MODEL_NORMALS_SCALE) if isNormal else 'mr ' + str(MODEL_SCALE)

    if os.system(IMG2DDS + ' ' + options + ' "' + path + '"') == 0:
      os.remove(path)
    else:
      print('FAILED to convert ' + path)

if SYSTEM == 'win32' and not sys.stdin.closed:
  print('Finished. Press Enter to continue ...')
  sys.stdin.readline()
