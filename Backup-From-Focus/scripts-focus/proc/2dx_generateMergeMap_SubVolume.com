#
#
# 2dx_genergeMergeMap_SubVolume.com
#
#   -----------------------------------------------------------------------------------
#   ... This is not an independent script. It should only be called from other scripts.
#   -----------------------------------------------------------------------------------
#
#
# This calculates sub-volumes from the raw volume SCRATCH/scratch1.map.
#
#
#
#
#
#############################################################################
${proc_2dx}/linblock "SubVolume generation Script called, with option ${calculate_subvolume}"
#############################################################################
#
if ( ${calculate_subvolume}x == "1x" ) then  
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to adjust dimensions"
  #############################################################################
  #
  set ALAT2 = `echo ${ALAT} | awk '{ s = $1 / 2.0 } END { print s }'`
  #
  \rm -f SCRATCH/scratch1b.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1.map wrkout SCRATCH/scratch1b.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA MATRIX   1.000 0.000 0.000      0.000 1.000 0.000    0.000 0.000 1.000
TRANS  0.0 -20.0 ${ALAT2}
eot
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to rotate volue for sub-volume preparation"
  #############################################################################
  #
  \rm -f SCRATCH/rot_volume.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1b.map wrkout SCRATCH/rot_volume.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA POLAR 0.0 0.0 45.0
TRANS  0.0 0.0 0.0
eot
  #
  # echo "# IMAGE: SCRATCH/rot_volume.map <MAP: rotated 3D Volume>" >> LOGS/${scriptname}.results
  #
  echo ":mask_subvolume_PDB = ${mask_subvolume_PDB}"
  \rm -f SCRATCH/rot_volume_tmp.map
  if ( ${mask_subvolume_PDB} == 'y' ) then
    #############################################################################
    ${proc_2dx}/linblock "mapmask - to mask with PDB file"
    #############################################################################
    #
    \rm -f NCSMask.map
    ${bin_ccp4}/ncsmask xyzin ${mask_subvolume_PDB_file} mskout NCSMask.map << eof
RADIUS 3.0
END
eof
    echo "# IMAGE: NCSMask.map <MAP: Mask map from PDB file>" >> LOGS/${scriptname}.results
    #
    ${bin_ccp4}/mapmask mapin SCRATCH/rot_volume.map mapout MSKIN2 NCSMask.map MAPOUT SCRATCH/rot_volume_tmp.map << eot
MODE MAPIN MSKIN2
END
eot
    #
    mv -f SCRATCH/rot_volume_tmp.map volume_masked.map
    echo "# IMAGE-IMPORTANT: volume_masked.map <MAP: PDB-file masked 3D Sub-Volume>" >> LOGS/${scriptname}.results
  endif
  #

  #############################################################################
  ${proc_2dx}/linblock "mapmask - to cut sub-volume"
  #############################################################################
  #   
  # 0.7071 = 1/sqrt(2)
  set middlex = "0.45"
  set middley = "0.45"
  set diam = "0.82"
  #
  set limxmin = `echo ${middlex} ${diam} | awk '{ s = $1 - ( $2 / 2.0 ) } END { print s }'`
  set limxmax = `echo ${middlex} ${diam} | awk '{ s = $1 + ( $2 / 2.0 ) } END { print s }'`
  set limymin = `echo ${middley} ${diam} | awk '{ s = $1 - ( $2 / 2.0 ) } END { print s }'`
  set limymax = `echo ${middley} ${diam} | awk '{ s = $1 + ( $2 / 2.0 ) } END { print s }'`
  #
  echo ":Limits are ${limxmin} to ${limxmax}, ${limymin} to ${limymax}"
  #
  \rm -f volume_sub.map
  ${bin_ccp4}/mapmask mapin SCRATCH/rot_volume.map mapout volume_sub.map << eof
AXIS X,Y,Z
scale factor 1
xyzlim ${limxmin} ${limxmax} ${limymin} ${limymax} 0.0 1.0
pad -100
SYMM 1
END
eof
  #
  echo "# IMAGE-IMPORTANT: volume_sub.map <MAP: Final 3D Volume (Sub Volume)>" >> LOGS/${scriptname}.results
  #
endif
#
#
#
#
#
#############################################################################
#############################################################################
#############################################################################
if ( ${calculate_subvolume}x == "2x" ) then  
  #############################################################################
  #############################################################################
  #############################################################################
  #
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to rotate volue for sub-volume preparation"
  #############################################################################
  #
  \rm -f SCRATCH/scratch1c.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1.map wrkout SCRATCH/scratch1c.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA POLAR 0.0 0.0 22.5
TRANS  18.0 0.0 0.0
eot
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to adjust dimensions"
  #############################################################################
  #
  \rm -f SCRATCH/rot_volume.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1c.map wrkout SCRATCH/rot_volume.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA MATRIX 1.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 1.0
TRANS  0.0 0.0 0.0
eot
  #
  # echo "# IMAGE: SCRATCH/rot_volume.map <MAP: rotated 3D Volume>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "mapmask - to cut sub-volume"
  #############################################################################
  #   
  # 0.7071 = 1/sqrt(2)
  set limxmin = "0.03"
  set limxmax = "0.97"
  set limymin = "0.1"
  set limymax = "0.625"
  #
  \rm -f volume_sub.map
  ${bin_ccp4}/mapmask mapin SCRATCH/rot_volume.map mapout volume_sub.map << eof
AXIS X,Y,Z
scale factor 1
xyzlim ${limxmin} ${limxmax} ${limymin} ${limymax} -0.5 0.5
END
eof
  #
  echo "# IMAGE-IMPORTANT: volume_sub.map <MAP: Final 3D Volume (Sub Volume)>" >> LOGS/${scriptname}.results
  #
  #
endif
#
#
#
#
#
#
#############################################################################
#############################################################################
#############################################################################
if ( ${calculate_subvolume}x == "3x" ) then  
  #############################################################################
  #############################################################################
  #############################################################################
  #
  #############################################################################
  ${proc_2dx}/linblock "Not yet implemented"
  #############################################################################
  #
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to rotate volue for sub-volume preparation"
  #############################################################################
  #
  \rm -f SCRATCH/scratch1c.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1.map wrkout SCRATCH/scratch1c.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA POLAR 0.0 0.0 22.5
TRANS  18.0 0.0 0.0
eot
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to adjust dimensions"
  #############################################################################
  #
  \rm -f SCRATCH/rot_volume.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1c.map wrkout SCRATCH/rot_volume.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA MATRIX 1.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 1.0
TRANS  0.0 0.0 0.0
eot
  #
  # echo "# IMAGE: SCRATCH/rot_volume.map <MAP: rotated 3D Volume>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "mapmask - to cut sub-volume"
  #############################################################################
  #   
  # 0.7071 = 1/sqrt(2)
  set limxmin = "0.01"
  set limxmax = "0.99"
  set limymin = "0.09"
  set limymax = "0.65"
  set limzmin = "-0.540"
  set limzmax = "0.539"
  #
  \rm -f volume_sub.map
  ${bin_ccp4}/mapmask mapin SCRATCH/rot_volume.map mapout volume_sub.map << eof
AXIS X,Y,Z
scale factor 1
xyzlim ${limxmin} ${limxmax} ${limymin} ${limymax} ${limzmin} ${limzmax}
END
eof
  #
  echo "# IMAGE-IMPORTANT: volume_sub.map <MAP: Final 3D Volume (Sub Volume)>" >> LOGS/${scriptname}.results
  #
endif
#
#
#
#
#
#
#############################################################################
#############################################################################
#############################################################################
if ( ${calculate_subvolume}x == "4x" ) then  
  #############################################################################
  #############################################################################
  #############################################################################
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to rotate volue for sub-volume preparation"
  #############################################################################
  #
  \rm -f SCRATCH/scratch1b.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1.map wrkout SCRATCH/scratch1b.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA POLAR 0.0 0.0 45.0
TRANS  0.0 0.0 0.0
eot
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to adjust dimensions"
  #############################################################################
  #
  \rm -f SCRATCH/rot_volume.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1b.map wrkout SCRATCH/rot_volume.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA MATRIX 1.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 1.0
TRANS  0.0 0.0 0.0
eot
  #
  # echo "# IMAGE: SCRATCH/rot_volume.map <MAP: rotated 3D Volume>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "mapmask - to cut sub-volume"
  #############################################################################
  #   
  set limxmin = "0.4"
  set limxmax = "0.99"
  set limymin = "0.4"
  set limymax = "0.99"
  #
  \rm -f volume_sub.map
  ${bin_ccp4}/mapmask mapin SCRATCH/rot_volume.map mapout volume_sub.map << eof
AXIS X,Y,Z
scale factor 1
xyzlim ${limxmin} ${limxmax} ${limymin} ${limymax} -0.5 0.5
END
eof
  #
  echo "# IMAGE-IMPORTANT: volume_sub.map <MAP: Final 3D Volume (Sub Volume)>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  #
endif
#
#
#
#
#
#
#############################################################################
#############################################################################
#############################################################################
if ( ${calculate_subvolume}x == "5x" ) then  
  #############################################################################
  #############################################################################
  #############################################################################
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to rotate volue for sub-volume preparation"
  #############################################################################
  #
  \rm -f SCRATCH/scratch1c.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1.map wrkout SCRATCH/scratch1c.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA POLAR 0.0 0.0 52.5
TRANS  70.0 -10.0 0.0
eot
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to adjust dimensions"
  #############################################################################
  #
  \rm -f SCRATCH/rot_volume.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1c.map wrkout SCRATCH/rot_volume.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA MATRIX 1.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 1.0
TRANS  0.0 0.0 0.0
eot
  #
  # echo "# IMAGE: SCRATCH/rot_volume.map <MAP: rotated 3D Volume>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "mapmask - to cut sub-volume"
  #############################################################################
  #   
  # 0.7071 = 1/sqrt(2)
  set limxmin = "0.17"
  set limxmax = "0.71"
  set limymin = "0.05"
  set limymax = "0.92"
  #
  \rm -f volume_sub.map
  ${bin_ccp4}/mapmask mapin SCRATCH/rot_volume.map mapout volume_sub.map << eof
AXIS X,Y,Z
scale factor 1
xyzlim ${limxmin} ${limxmax} ${limymin} ${limymax} -0.5 0.5
END
eof
  #
  echo "# IMAGE-IMPORTANT: volume_sub.map <MAP: Final 3D Volume (Sub Volume)>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  ${proc_2dx}/linblock "Sub-volume cropped"
  #############################################################################
  #
endif
#
#
#
#
#
#############################################################################
#############################################################################
#############################################################################
if ( ${calculate_subvolume}x == "6x" ) then  
  #############################################################################
  #############################################################################
  #
  #
  #############################################################################
  ${proc_2dx}/linblock "maprot - to adjust dimensions"
  #############################################################################
  #
  \rm -f SCRATCH/scratch2.map
  ${bin_ccp4}/maprot mapin SCRATCH/scratch1.map wrkout SCRATCH/scratch2.map << eot
MODE FROM
CELL WORK ${realcell} ${ALAT} 90.0 90.0 ${realang}
GRID WORK ${cellx} ${celly} ${ALAT}
XYZLIM 0 ${cellxm1} 0 ${cellym1} 0 ${ALATm1}
SYMM WORK 1
AVER
ROTA MATRIX 1.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0 1.0
TRANS  0.0 0.0 0.0
eot
  #
  # echo "# IMAGE: SCRATCH/scratch2.map <MAP: flipped volume >" >> LOGS/${scriptname}.results
  #
  #
  #############################################################################
  ${proc_2dx}/linblock "mapmask - to cut sub-volume"
  #############################################################################
  #   
  #
  \rm -f volume_sub.map
  ${bin_ccp4}/mapmask mapin SCRATCH/scratch2.map mapout volume_sub.map << eof
AXIS X,Y,Z
scale factor 1
xyzlim -0.25 1.0 -1.0 0.25 -0.25 0.07
SYMM 1
END
eof
  #
  echo "# IMAGE-IMPORTANT: volume_sub.map <MAP: Final 3D Volume (Sub Volume)>" >> LOGS/${scriptname}.results
  #
  #############################################################################
  #
endif
#
#
#
#
#
#############################################################################
#############################################################################
#############################################################################
if ( ${calculate_subvolume}x == "7x" ) then  
  #############################################################################
  #############################################################################
  #############################################################################
  #
  #############################################################################
  ${proc_2dx}/linblock "not yet implemented"
  #############################################################################
  #
endif
#
#
#
