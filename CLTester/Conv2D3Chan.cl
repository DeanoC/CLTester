
const sampler_t imgSampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

#define filterWidth 3
#define filterHeight 3

__constant float filter[filterWidth * filterHeight] = 
{
	+0.0, -1.0,	+0.0,
	-1.0, +5.0	-1.0,
	+0.0, -1.0,	+0.0,
};

__kernel void Conv2D3Chan(
	image2d_t image,
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