#!/usr/bin/env python
# This script converts a FREALIGN .par file into a RELION .star file.
from __future__ import print_function
import numpy as np
import sys
import os.path
from optparse import OptionParser

def main():

	progname = os.path.basename(sys.argv[0])
	usage = progname + """ <input .par file> <output .star file> [options] 

	This script converts a FREALIGN .par file into a RELION .star file.

	"""

	parser = OptionParser(usage)

	parser.add_option("--angpix", metavar=1.0, default=1.0, type="float", help="Pixel size in Angstroems")
	parser.add_option("--ampcon", metavar=0.07, default=0.07, type="float", help="Amplitude contrast, between 0.0 and 1.0.")
	parser.add_option("--kv", metavar=300.0, default=300.0, type="float", help="Microscope voltage.")
	parser.add_option("--cs", metavar=2.7, default=2.7, type="float", help="Spherical Aberration (Cs).")
	parser.add_option("--stack", metavar="particles.mrcs", type="string", default='particles.mrcs', help="The .mrcs stack file for RELION.")
	parser.add_option("--crystal", action="store_true", help="Ensures that particles belonging to the same 2D crystal are assigned to the same half-set to avoid inflation of the FSC.", default=False)

	(options, args) = parser.parse_args()

	command = ' '.join(sys.argv)

	# par_file = '/scratch/data/MloK1/cAMP-2015-November/frealign/C4_134/1_class/new2/MloK1_1_r1.par'
	# par_file = '/mnt/raid0/data/Aqp2/2dx/SPR/stacks/particles_ctfcor_1_r1.par'
	# star_file = '/scratch/data/MloK1/cAMP-2015-November/relion/1_class/MloK1.star'
	# star_file = '/mnt/raid0/data/Aqp2/2dx/SPR/stacks/particles_ctfcor_1_r1.star'
	# stack_rootname = 'particles_ctfcor'
	# apix = 1.23732
	# AmpContrast = 0.07
	# volt = 200.0
	# cs = 2.0

	# particles_stack = stack_rootname.split('/')[-1]+'.mrcs'
	par_file = args[0]
	star_file = args[1]
	particles_stack = options.stack
	apix = options.angpix
	AmpContrast = options.ampcon
	volt = options.kv
	cs = options.cs

	par = np.loadtxt(par_file, comments='C')
	par[:,4:6] = par[:,4:6] / apix

	N = np.shape(par)[0]

	mic = 0
	f = open(star_file, 'w+')
	print( 'data_', file=f)
	print( 'loop_', file=f)
	print( '_rlnImageName #1', file=f)
	print( '_rlnMicrographName #2', file=f)
	print( '_rlnDefocusU #2', file=f)
	print( '_rlnDefocusV #3', file=f)
	print( '_rlnDefocusAngle #4', file=f)
	print( '_rlnVoltage #5', file=f)
	print( '_rlnSphericalAberration #6', file=f)
	print( '_rlnAmplitudeContrast #7', file=f)
	print( '_rlnAngleRot #8', file=f)
	print( '_rlnAngleTilt #9', file=f)
	print( '_rlnAnglePsi #10', file=f) 
	print( '_rlnOriginX #11', file=f) 
	print( '_rlnOriginY #12', file=f)
	print( '_rlnRandomSubset #13', file=f)

	if options.crystal:

		print( '_rlnHelicalTubeID #14', file=f)

		for i in range(N):

			num = i+1
			phi = par[i,3]
			theta = par[i,2]
			psi = par[i,1]
			shx = par[i,4]
			shy = par[i,5]
			df1 = par[i,8]
			df2 = par[i,9]
			ang = par[i,10]
			rnd = i % 2 + 1

			film = str(int(par[i,7]))

			print( '%.6d@' % num+particles_stack,' mic'+film,' %.2f' % df1,' %.2f' % df2,' %.2f' % ang,' %.1f' % volt,' %.1f' % cs,' ',' %.2f' % AmpContrast,' %.2f' % phi,' %.2f' % theta,' %.2f' % psi,' %.2f' % shx,' %.2f' % shy,' %d' % rnd,' %d' % par[i,7], file=f)
			# print 'Wrote image %d/%d.\r' % (i+1, N),

	else:

		for i in range(N):

			num = i+1
			phi = par[i,3]
			theta = par[i,2]
			psi = par[i,1]
			shx = par[i,4]
			shy = par[i,5]
			df1 = par[i,8]
			df2 = par[i,9]
			ang = par[i,10]
			rnd = i % 2 + 1

			film = str(int(par[i,7]))

			print( '%.6d@' % num+particles_stack,' mic'+film,' %.2f' % df1,' %.2f' % df2,' %.2f' % ang,' %.1f' % volt,' %.1f' % cs,' ',' %.2f' % AmpContrast,' %.2f' % phi,' %.2f' % theta,' %.2f' % psi,' %.2f' % shx,' %.2f' % shy,' %d' % rnd, file=f)
			# print 'Wrote image %d/%d.\r' % (i+1, N),


	print('Done!')
	f.close()

if __name__ == "__main__":
	main()