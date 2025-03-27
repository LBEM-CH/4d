from EMAN2 import *
from sparx import *

import os


if __name__ == "__main__":
	
	if len(sys.argv) != 8:
		sys.exit("Missuse detected")

	stack_file = sys.argv[1]
	image_name = sys.argv[2]
	folder_name = sys.argv[3]
	skip_num = int(sys.argv[4])
	nx = int(sys.argv[5])	
	ny = int(sys.argv[6])	
	nz = int(sys.argv[7])	

	stack = get_image(stack_file)
	
	print ( ":Doing splitting for:", stack_file )
	print ( ":nx = ", nx, "  ny = ", ny, "  nz =", nz )
	


        #########################################
        # create frames 1 to nz+1:
        #########################################

	for i in range(0,nz-skip_num):
						
		output_name = folder_name + "/f" + str(i+1) + ".mrc"
				
		print ( "Creating frame", i+1, "out of", stack_file, "as", output_name )
		
                ioffx = 0
                ioffy = 0
		
		# The following is to test the drift correction performance:
		# ioffx = i/2-9
		# ioffy = i-19

		image = stack.get_clip(Region(ioffx,ioffy,i+skip_num,nx,ny,1))

		image.write_image(output_name)
	
	
