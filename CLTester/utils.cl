
const sampler_t imgSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void Identity_F1Image_T1(
	int dummy,
	__read_only image2d_t inImage,
	int resultStride,
	__global float* result )
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	result[(pixcoord.y * resultStride) + pixcoord.x] = read_imagef(inImage, imgSampler, pixcoord).x;
}
