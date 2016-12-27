
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
	if (pixcoord.x >= get_image_width(image) || pixcoord.y >= get_image_height(image))
		return;

	float res = 0;	
	for (short y = 0; y < filterHeight; ++y)
	{
		for (short x = 0; x < filterWidth; ++x)
		{
			const int filterOffset = (y*filterWidth) + x;
			float pix = read_imagef(image, imgSampler, pixcoord + (int2)(x, y)).x;
			res += pix * filter[filterOffset];
		}
	}

	result[(pixcoord.y * resultStride) + pixcoord.x] = res;
}