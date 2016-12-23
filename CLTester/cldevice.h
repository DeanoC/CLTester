#pragma once
#include <CL/cl.h>
#include <bitset>

namespace OpenCL
{
	enum class Vendor
	{
		Unknown,
		NVIDIA,
		AMD,
		Intel
	};

	enum class APIVersion
	{
		CL1_0,
		CL1_1,
		CL1_2,
		CL1_X,
		CL2_0,
		CL2_1,
		CL2_X,
		CL3_PLUS,
	};

	// this isn't a full list of extensions (as that list is potentially unbounded...)
	// its just the ones we care about. We convert the extension string to a bit array for
	// easy checking in code. DecodeExtensionString switch should match this enum
	enum class Extensions
	{
		CL_KHR_FP16,
		CL_KHR_FP64,
		CL_NV_DEVICE_ATTRIBUTE_QUERY,
		CL_AMD_DEVICE_ATTRIBUTE_QUERY,
		CL_AMD_MEDIA_OPS,
		CL_AMD_MEDIA_OPS2,

		Count 
	};

	struct Device
	{
		Vendor			vendor;
		APIVersion		version;
		cl_platform_id	platformId;
		cl_device_id	deviceId;
		cl_device_type	type;
		cl_context		context;

		// rough idea of floating point (half,single,double) operation per second from hw stats
		// 0 if no native support for half or double
		uint64_t		estimatedHMFLOPs;
		uint64_t		estimatedSMFLOPs;
		uint64_t		estimatedDMFLOPs;

		std::bitset<(size_t)Extensions::Count> extensionsSupported;

	};
}
