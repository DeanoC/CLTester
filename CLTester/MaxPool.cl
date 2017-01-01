
const sampler_t imgSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

#define filterWidth 2
#define filterHeight 2

__kernel void MaxPoolDownInt_F1_T1(
	int inputStride,
	__global float* input,
	int resultStride,
	__global float* result )
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float res = 0;
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int2 tc = (int2)((pixcoord.x * filterWidth) + x, (pixcoord.y * filterHeight) + y);
			float Y = input[(tc.y * inputStride) + tc.x];
			res = max(res, Y);
		}
	}

	result[(pixcoord.y * resultStride) + pixcoord.x] = res;

}

__kernel void MaxPoolDownInt_F1Image_T1(
	int dummy0,
	__read_only image2d_t image,
	int resultStride,
	__global float* result)
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float res = 0;
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int2 tc = (int2)((pixcoord.x * filterWidth) + x, (pixcoord.y * filterHeight) + y);
			float Y = read_imagef(image, imgSampler, tc).x;
			res = max(res, Y);
		}
	}

	result[(pixcoord.y * resultStride) + pixcoord.x] = res;

}

__kernel void MaxPoolDownInt_F1_T1Image(
	int inputStride,
	__global float* input,
	int dummy1,
	__write_only image2d_t outImage)
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float res = 0;
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int2 tc = (int2)((pixcoord.x * filterWidth) + x, (pixcoord.y * filterHeight) + y);
			float Y = input[(tc.y * inputStride) + tc.x];
			res = max(res, Y);
		}
	}

	write_imagef(outImage, pixcoord, (float4)(res));
}


__kernel void MaxPoolDownInt_F1Image_T1Image(
	int dummy0,
	__read_only image2d_t image,
	int resultStride,
	__write_only image2d_t outImage )
{
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));

	float res = 0;
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int2 tc = (int2)((pixcoord.x * filterWidth) + x, (pixcoord.y * filterHeight) + y);
			float Y = read_imagef(image, imgSampler, tc).x;
			res = max(res, Y);
		}
	}

	write_imagef(outImage, pixcoord, (float4)(res));
}
