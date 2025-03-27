from EMAN2 import *
from sparx import *

import os


if __name__ == "__main__":
	
	if len(sys.argv) != 5:
		sys.exit("Missuse detected")

	stack_file = sys.argv[1]
	image_name = sys.argv[2]
	ave_num = int(sys.argv[3])
	frame_folder_name = sys.argv[4]
	
	stack = get_image(stack_file)
	nx = stack.get_xsize()
	ny = stack.get_ysize()
	nz = stack.get_zsize()
	
	print ( "Doing splitting for:", stack_file )
	print ( "nz =", nz )
	



        #########################################
        # create frames 0, which is a central high-contrast frame of the first quarter of frames.
        #########################################

	folder_name = frame_folder_name + "/f0" 
	output_name = folder_name + "/" + image_name + "_0_raw.mrc"
	os.mkdir(folder_name)
	
        fat_num_from = 1;
        fat_num_to   = int(nz / 4);

	print ( "::Creating frame 0 from sub-frames ",fat_num_from," to ",fat_num_to,"." )

	for j in range(fat_num_from,fat_num_to):
	
		print ( "Creating frame 0 as", output_name, "by adding sub-frame", j )
			
		if j==fat_num_from:
			image = stack.get_clip(Region(0,0,j,nx,ny,1))
		else:
			image += stack.get_clip(Region(0,0,j,nx,ny,1))
		
	image.write_image(output_name)
	



        #########################################
        # create frames 1 to nz/ave_num:
        #########################################

	for i in range(0,int(nz/ave_num)):
		
		folder_name = frame_folder_name + "/f" + str(i+1)
		output_name = folder_name + "/" + image_name + "_" + str(i+1) + "_raw.mrc"
		os.mkdir(folder_name)
		
		for j in range(0,ave_num):
		
			print ( "Creating frame", i+1, "as", output_name, "by adding sub-frame", i*ave_num+j )
			
			# The following is to test the drift correction performance:
			# ioffx = 7*i/2-9
			# ioffy = 7*i-19
			#
			# if j==0:
			#	image1 = stack.get_clip(Region(0,0,i*ave_num+j,nx,ny,1))
			#	image =  cyclic_shift(image1,ioffx,ioffy)
			# else:
			#	image1 = stack.get_clip(Region(0,0,i*ave_num+j,nx,ny,1))
			#	image += cyclic_shift(image1,ioffx,ioffy)

			if j==0:
				image =  stack.get_clip(Region(0,0,i*ave_num+j,nx,ny,1))
			else:
				image += stack.get_clip(Region(0,0,i*ave_num+j,nx,ny,1))
			
		image.write_image(output_name)
	
	
