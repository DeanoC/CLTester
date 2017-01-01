const sampler_t imgSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void ReLU_F1_T1(
	int inputStride,
	__global float* input,
	int resultStride,
	__global float* result)
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float res = max(0.0f, input[(pixcoord.y * inputStride) + pixcoord.x]);
	float Y = input[(pixcoord.y * inputStride) + pixcoord.x];
	result[(pixcoord.y * resultStride) + pixcoord.x] = max(0.0f, Y);
}

__kernel void ReLU_F1Image_T1(
	int dummy,
	__read_only image2d_t inImage,
	int resultStride,
	__global float* result )
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float Y = read_imagef(inImage, imgSampler, pixcoord).x;
	result[(pixcoord.y * resultStride) + pixcoord.x] = max( 0.0f, Y);
}

__kernel void ReLU_F1_T1Image(
	int inputStride,
	__global float* input,
	int dummy1,
	__write_only image2d_t outImage)
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float Y = input[(pixcoord.y * inputStride) + pixcoord.x];
	Y = max(Y, 0.0f);
	write_imagef(outImage, pixcoord, (float4)(Y));
}


__kernel void ReLU_F1Image_T1Image(
	int dummy0,
	__read_only image2d_t inImage,
	int dummy1,
	__write_only image2d_t outImage)
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float Y = read_imagef(inImage, imgSampler, pixcoord).x;
	Y = max(Y, 0.0f);
	write_imagef(outImage, pixcoord, (float4)(Y));
}
