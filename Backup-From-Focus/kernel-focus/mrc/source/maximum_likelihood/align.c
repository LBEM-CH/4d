/*   Align two images 

input:   
refer: the first image
temp_image: the second image which is aligned to the first one
Ouput:
pa[0]: rotation
pa[1]: translation in x
pa[2]: translation in y
pa[3]: flip in x or y

 */
#include <common.h>


int   align(int nx, int ny, float *refer, float *temp_image,  int *pa)
{    


	int i,j,k4,k3,m;
	float   *temp_image1, *refer1,  *RTimage,t,  max=-1.0e20, *corr, devi, mean;

	refer1=(float *)calloc(nx*ny,sizeof(float));
	temp_image1=(float *)calloc(nx*ny,sizeof(float));
	RTimage=(float *)calloc(nx*ny,sizeof(float));
	corr=(float *)calloc(nx*ny,sizeof(float));

	//	low_pass(nx,ny,refer,0.3,2);
	//        low_pass(nx,ny,temp_image,0.3,2); 


	for(i=0;i<nx;i++)
		for(j=0;j<ny;j++)
			refer1[IDX(i,j,nx,ny)]=refer[IDX(i,j,nx,ny)];

	mean=0;
	for(i=0; i<nx;i++)
		for(j=0;j<ny;j++)
			mean+=refer1[IDX(i,j,nx,ny)];
	mean/=(nx*ny);

	for(i=0; i<nx;i++)
		for(j=0;j<ny;j++)
			refer1[IDX(i,j,nx,ny)]-=mean;


	pa[0]=0;pa[1]=0;

	for(k4=0; k4<3;k4++)
	{     


		if(k4==0)
			for(i=0;i<nx;i++)
				for(j=0;j<ny;j++)
					temp_image1[IDX(i,j,nx,ny)]=temp_image[IDX(i,j,nx,ny)];
		else  if(k4==1)
			for(i=0;i<nx;i++)
				for(j=0;j<ny;j++)
					temp_image1[IDX(i,j,nx,ny)]=temp_image[IDX(i,ny-j-1,nx,ny)];
		else             
			for(i=0;i<nx;i++)
				for(j=0;j<ny;j++)
					temp_image1[IDX(i,j,nx,ny)]=temp_image[IDX(nx-i-1,j,nx,ny)];

		mask(120,120,nx,ny,temp_image1);        

		mean=0;
		for(i=0; i<nx;i++)
			for(j=0;j<ny;j++)
				mean+=temp_image1[IDX(i,j,nx,ny)];
		mean/=(nx*ny);

		for(i=0; i<nx;i++)
			for(j=0;j<ny;j++)
				temp_image1[IDX(i,j,nx,ny)]-=mean;      


		for(k3=0; k3<360; k3++)
		{          
			rotate(nx,ny,k3*1.0,temp_image1,RTimage);   

			devi=0.0;
			for(i=0;i<nx;i++)
				for(j=0;j<ny;j++)
					devi=powf(RTimage[IDX(i,j,nx,ny)],2.0);
			devi=sqrtf(devi/(nx*ny));

			for(i=0;i<nx;i++)
				for(j=0;j<ny;j++)

					RTimage[IDX(i,j,nx,ny)]/=sqrt(devi);         


			cross_corr(nx,ny,1, refer, RTimage, corr);

			for(i=0; i<nx; i++)   
				for(j=0; j<ny; j++)
					if(corr[IDX(i,j,nx,ny)]>max)
					{         max=corr[IDX(i,j,nx,ny)];

						pa[0]=k3;
						pa[1]=i;
						pa[2]=j;
						pa[3]=k4;
					}                      
		}  

	}   



	if(pa[1]>nx/2)  pa[1]=pa[1]-nx;
	if(pa[2]>ny/2)  pa[2]=pa[2]-ny;  



	//      printf(" &&&&&&&&&&&&  alpha=%d      phaori=%d %d             flip=%d  \n", pa[0],pa[1],pa[2],pa[3]); 
	//      fflush(stdout);

	free(RTimage);  
	free(temp_image1);
	free(corr);
	free(refer1);


}
