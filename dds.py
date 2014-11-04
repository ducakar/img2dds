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

# The following regex patterens must match the beginning of a texture path (after `GameData/` to
# work.

# Excluded texture patterns. These textures will be left intact.
EXCLUDE = [
  'BoulderCo/',
  'CommunityResourcePack/',
  'NavyFish/Plugins/PluginData/',
  'RCSBuildAid/Textures/iconAppLauncher\.png$',
  'SCANsat/Icons/',
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
  'ASET/'
  'ASET_Props/',
  'FASA/',
  'JSI/RasterPropMonitor/Library/Components/MFD40x20v2/',
  'KAS/Textures/',
  'KSO/RPM/KSO_PROP/',
  'KSO/RPM/KSO_Laptop_1/KSOS_Laptop\.',
  'KSO/RPM/KSO_Laptop_1/KSOS_Laptop_emis\.',
  'KSO/RPM/KSO_Laptop_1/KSOS_Laptop_norm_NRM\.',
  'Lionhead_Aerospace_Inc/',
  'Part Revamp Extras/',
  'ProceduralFairings/',
  'Space Factory Ind/',
  'Squad/SPP/',
  'TextureReplacer/'
]

# Some non-model textures may match the previous model patterns. Exclude them from models.
NOT_MODEL = [
  '.*/Agencies/.*',
  '.*/Flags/.*',
  '.*/Icons/.*',
  'ASET_Props/MFDs/',
  'HOME2/Props/.*\.png'
  'Space Factory Ind/.*/JSI/.*\.png'
]

####################################################################################################

import os, re, sys

EXCLUDE   = [re.compile(e) for e in EXCLUDE]
MODEL     = [re.compile(m) for m in MODEL]
NOT_MODEL = [re.compile(m) for m in NOT_MODEL]
IMAGE     = re.compile('.*\.(png|jpg|tga|mbm)$')
PREFIX    = re.compile('.*GameData/')
SYSTEM    = 'linux64' if sys.maxsize > 2**32 else 'linux32'
SYSTEM    = 'win32' if sys.platform == 'win32' else SYSTEM
IMG2DDS   = './img2dds/' + SYSTEM + '/img2dds'
IMG2DDS   = 'img2dds\win32\img2dds.exe' if SYSTEM == 'win32' else IMG2DDS
DIR       = sys.argv[1] if len(sys.argv) == 2 else './GameData'

for (dirPath, dirNames, fileNames) in os.walk(DIR):
  dirPath = PREFIX.sub('', dirPath.replace('\\', '/'))
  images  = {(dirPath + '/' + name) for name in fileNames if IMAGE.match(name)}
  images -= {i for i in images for e in EXCLUDE if e.match(i)}
  models  = {i for i in images for m in MODEL if m.match(i)}
  models -= {i for i in images for m in NOT_MODEL if m.match(i)}

  for i in images:
    options = '-vcmNs' if i in models else '-vc'
    path = DIR + '/' + i

    if os.system(IMG2DDS + ' ' + options + ' "' + path + '"') == 0:
      os.remove(path)
    else:
      print('FAILED to convert ' + path)

if SYSTEM == 'win32' and not sys.stdin.closed:
  print('Finished. Press Enter to continue ...')
  sys.stdin.readline()
