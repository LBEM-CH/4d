#
#
# This is not an independent script.
#
# This should only be called from another script.
#
#
if ( ${ML_do_whiten} == 'y' ) then
  set ML_do_whiten_val = "1"
else
  set ML_do_whiten_val = "0"
endif
#
if ( ${ML_correct_CTF} == 'y' ) then
  set ML_correct_CTF_val = "1"
else
  set ML_correct_CTF_val = "0"
endif
#
if ( ${ctfrev} == "y" ) then
  set contrast_val = "-1"
else
  set contrast_val = "1"
endif
#
if ( ${ML_rotational_symmetry} == "0" ) then
  set ML_rotational_symmetry_val = 1
else if ( ${ML_rotational_symmetry} == "1" ) then
  set ML_rotational_symmetry_val = 2
else if( ${ML_rotational_symmetry} == "2" ) then
  set ML_rotational_symmetry_val = 3
else if( ${ML_rotational_symmetry} == "3" ) then
  set ML_rotational_symmetry_val = 4
else if( ${ML_rotational_symmetry} == "4" ) then
  set ML_rotational_symmetry_val = 6
endif
#
pwd
set date = `date`
echo date = ${date}
#
if ( ! -e ${nonmaskimagename}_profile.dat ) then
  if ( -e image_ctfcor_profile.dat ) then
    \mv -f image_ctfcor_profile.dat ${nonmaskimagename}_profile.dat
  endif 
  if ( ! -e ${nonmaskimagename}_profile.dat ) then
    ${proc_2dx}/linblock "${nonmaskimagename}_profile.dat not existing."
    ${proc_2dx}/protest "First run UNBEND II to create the PROFILE."
  endif
endif
#
echo "# IMAGE: APH/${imagename}_ctf.aph <APH: Unbending Amp&Phase File>" >> LOGS/${scriptname}.results
echo "# IMAGE: PS/${imagename}MAP-p1.ps <PS: Unbending MAP in p1>" >> LOGS/${scriptname}.results
echo "# IMAGE: ${imagename}-p1.mrc <Unbending MAP in p1>" >> LOGS/${scriptname}.results
#
if ( ! -e ${imagename}.mrc ) then
  ${proc_2dx}/protest "${imagename}.mrc not existing. No input image file ???"
endif
#
echo dummy > ML/ML_reference_1.mrc
\rm -f ML_result_noEnvelope.mrc
\rm -f ML_result_withEnvelope.mrc
\rm -f ML_result_ref_even.mrc
\rm -f ML_result_ref_odd.mrc
\rm -f ML/ML_reference_*.mrc
#
echo "<<@progress: 10>>"
#
#############################################################################
${proc_2dx}/linblock "2dx_ML - to run maximum likelihood processing"
#############################################################################   
#
${bin_2dx}/2dx_ML.exe << eot_ML1
${ML_doMLorCC}
LOGS/${scriptname}.results
eot_ML1
#
echo ${imagenumber} "           Maximum Likelihood 2D AMPS and PHS of the unit cell" > SCRATCH/ML_tmp.txt
cat APH/ML_result.aph >> SCRATCH/ML_tmp.txt
\mv -f SCRATCH/ML_tmp.txt APH/ML_result.aph
#
#############################################################################
#

