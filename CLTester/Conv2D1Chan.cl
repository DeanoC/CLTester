
const sampler_t imgSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

#define filterWidth 3
#define filterHeight 3

__constant float filter[filterWidth * filterHeight] = 
{
	0,0,0,
	0,1,0,
	0,0,0,
};

__kernel void Conv2D1Chan(
	image2d_t image,
	int resultStride,
	__global float* result )
{

	int2 pixcoord = (int2)( get_global_id(0), get_global_id(1) );
	if (pixcoord.x*pixcoord.y == 0) return;

	float res = 0;
	for (short y = -filterHeight / 2; y < filterHeight / 2; ++y)
	{
		for (short x = -filterWidth / 2; x < filterWidth / 2; ++x)
		{
			float pix = read_imagef(image, imgSampler, pixcoord + (int2)(x, y)).x;
			res += pix *filter[(y*filterWidth) + x];
		}
	}

	result[(pixcoord.y * resultStride) + pixcoord.x] = res;

}