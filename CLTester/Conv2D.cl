
const sampler_t imgSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

#define filterWidth 3
#define filterHeight 3

__constant float filter[filterWidth * filterHeight] = 
{
	0.0f,		0.0f,		0.0f,
	0.0f,		1.0f,		0.0f,
	0.0f,		0.0f,		0.0f,
};

__kernel void Conv2D_F1Image_T1(
	int dummy,
	__read_only image2d_t image,
	int resultStride,
	__global float* result )
{
	const int2 imageMaxCoords = (int2)(get_image_width(image), get_image_height(image));
	const int2 pixcoord = (int2)(get_global_id(0), get_global_id(1));
	if (any(pixcoord >= imageMaxCoords))
		return;

	float res = 0;	
	for (short y = 0; y < filterHeight; ++y)
	{
		for (short x = 0; x < filterWidth; ++x)
		{
			const int filterOffset = (y*filterWidth) + x;
			// offset so the filter kernel is centered on this pixel
			const int2 tc = (int2)(pixcoord.x + x - (filterWidth / 2), pixcoord.y + y - (filterHeight / 2));
			float pix = read_imagef(image, imgSampler, tc).x;
			res += pix * filter[filterOffset];
		}
	}

	result[(pixcoord.y * resultStride) + pixcoord.x] = res;
}

__kernel void  Conv2D_F3Image_T3(
	int dummy,
	__read_only image2d_t image,
	int resultStride,
	__global float* result )
{
	const int2 imageMaxCoords = (int2)(get_image_width(image), get_image_height(image));
	const int2 pixcoord = (int2)( get_global_id(0), get_global_id(1) );
	if ( any(pixcoord >=  imageMaxCoords) )
		return;

	float3 res = 0;	
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int filterOffset = (y*filterWidth) + x;
			const float fv = filter[filterOffset];

			// offset so the filter kernel is centered on this pixel
			const int2 tc = (int2)( pixcoord.x + x - (filterWidth/2), pixcoord.y + y - (filterHeight/2) );
			float3 pix3 = read_imagef(image, imgSampler, tc).xyz;
			res += pix3 * (float3)(fv, fv, fv);
		}
	}
	
	result[(pixcoord.y * resultStride) + (pixcoord.x * 3) + 0] = clamp(res.x, 0.0f, 1.0f);
	result[(pixcoord.y * resultStride) + (pixcoord.x * 3) + 1] = clamp(res.y, 0.0f, 1.0f);
	result[(pixcoord.y * resultStride) + (pixcoord.x * 3) + 2] = clamp(res.z, 0.0f, 1.0f);
}

__kernel void Conv2D_F3Image_T1(
	int dummy,
	__read_only image2d_t image,
	int resultStride,
	__global float* result )
{
	const int2 imageMaxCoords = (int2)(get_image_width(image), get_image_height(image));
	const int2 pixcoord = (int2)( get_global_id(0), get_global_id(1) );
	if ( any(pixcoord >=  imageMaxCoords) )
		return;

	float3 res = 0;	
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int filterOffset = (y*filterWidth) + x;
			const float fv = filter[filterOffset];

			// offset so the filter kernel is centered on this pixel
			const int2 tc = (int2)( pixcoord.x + x - (filterWidth/2), pixcoord.y + y - (filterHeight/2) );
			float3 pix3 = read_imagef(image, imgSampler, tc).xyz;
			res += pix3 * (float3)(fv, fv, fv);
		}
	}	

	float Y = (0.257f * res.x) + (0.504f * res.y) + (0.098f * res.z) + (16.0f/256.0f);

	result[(pixcoord.y * resultStride) + pixcoord.x] = Y;
}

__kernel void Conv2D_F3Image_T1Image(
	int dummy0,
	__read_only image2d_t inImage,
	int dummy1,
	__write_only image2d_t outImage )
{
	const int2 imageMaxCoords = (int2)(get_image_width(inImage), get_image_height(inImage));
	const int2 pixcoord = (int2)( get_global_id(0), get_global_id(1) );
	if ( any(pixcoord >=  imageMaxCoords) )
		return;

	float3 res = 0;	
	for (int y = 0; y < filterHeight; ++y)
	{
		for (int x = 0; x < filterWidth; ++x)
		{
			const int filterOffset = (y*filterWidth) + x;
			const float fv = filter[filterOffset];

			// offset so the filter kernel is centered on this pixel
			const int2 tc = (int2)( pixcoord.x + x - (filterWidth/2), pixcoord.y + y - (filterHeight/2) );
			float3 pix3 = read_imagef(inImage, imgSampler, tc).xyz;
			res += pix3 * (float3)(fv, fv, fv);
		}
	}	

	float Y = (0.257f * res.x) + (0.504f * res.y) + (0.098f * res.z) + (16.0f/256.0f);

	write_imagef(outImage, pixcoord, (float4)(Y));
}