// CLTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <list>
#include <algorithm>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <iso646.h>
#include <array>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cldevice.h"
#include "consthash.h"

namespace {
	//! \param size in bytes
	//! \param alignment in bits (note bits not bytes!)
	void* aligned_malloc(size_t size, size_t alignment)
	{
		// TODO this is windows specific, add other OS aligned mallocs
		return _aligned_malloc(size, alignment >> 3);
	}

	void aligned_free(void* ptr)
	{
		return _aligned_free(ptr);
	}

	// convert the kernel file into a string
	bool convertToString(const char *filename, std::string& s)
	{
		size_t size;
		char*  str;
		std::fstream f(filename, (std::fstream::in | std::fstream::binary));

		if (f.is_open())
		{
			size_t fileSize;
			f.seekg(0, std::fstream::end);
			size = fileSize = (size_t)f.tellg();
			f.seekg(0, std::fstream::beg);
			str = new char[size + 1];
			if (!str)
			{
				f.close();
				return false;
			}

			f.read(str, fileSize);
			f.close();
			str[size] = '\0';
			s = str;
			delete[] str;
			return true;
		}
		std::cout << "Error: failed to open file\n:" << filename << std::endl;
		return false;
	}

	const char *getOpenCLErrorString(cl_int error)
	{
		switch (error) {
			// run-time and JIT compiler errors
		case 0: return "CL_SUCCESS";
		case -1: return "CL_DEVICE_NOT_FOUND";
		case -2: return "CL_DEVICE_NOT_AVAILABLE";
		case -3: return "CL_COMPILER_NOT_AVAILABLE";
		case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
		case -5: return "CL_OUT_OF_RESOURCES";
		case -6: return "CL_OUT_OF_HOST_MEMORY";
		case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
		case -8: return "CL_MEM_COPY_OVERLAP";
		case -9: return "CL_IMAGE_FORMAT_MISMATCH";
		case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
		case -11: return "CL_BUILD_PROGRAM_FAILURE";
		case -12: return "CL_MAP_FAILURE";
		case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
		case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
		case -15: return "CL_COMPILE_PROGRAM_FAILURE";
		case -16: return "CL_LINKER_NOT_AVAILABLE";
		case -17: return "CL_LINK_PROGRAM_FAILURE";
		case -18: return "CL_DEVICE_PARTITION_FAILED";
		case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

			// compile-time errors
		case -30: return "CL_INVALID_VALUE";
		case -31: return "CL_INVALID_DEVICE_TYPE";
		case -32: return "CL_INVALID_PLATFORM";
		case -33: return "CL_INVALID_DEVICE";
		case -34: return "CL_INVALID_CONTEXT";
		case -35: return "CL_INVALID_QUEUE_PROPERTIES";
		case -36: return "CL_INVALID_COMMAND_QUEUE";
		case -37: return "CL_INVALID_HOST_PTR";
		case -38: return "CL_INVALID_MEM_OBJECT";
		case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
		case -40: return "CL_INVALID_IMAGE_SIZE";
		case -41: return "CL_INVALID_SAMPLER";
		case -42: return "CL_INVALID_BINARY";
		case -43: return "CL_INVALID_BUILD_OPTIONS";
		case -44: return "CL_INVALID_PROGRAM";
		case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
		case -46: return "CL_INVALID_KERNEL_NAME";
		case -47: return "CL_INVALID_KERNEL_DEFINITION";
		case -48: return "CL_INVALID_KERNEL";
		case -49: return "CL_INVALID_ARG_INDEX";
		case -50: return "CL_INVALID_ARG_VALUE";
		case -51: return "CL_INVALID_ARG_SIZE";
		case -52: return "CL_INVALID_KERNEL_ARGS";
		case -53: return "CL_INVALID_WORK_DIMENSION";
		case -54: return "CL_INVALID_WORK_GROUP_SIZE";
		case -55: return "CL_INVALID_WORK_ITEM_SIZE";
		case -56: return "CL_INVALID_GLOBAL_OFFSET";
		case -57: return "CL_INVALID_EVENT_WAIT_LIST";
		case -58: return "CL_INVALID_EVENT";
		case -59: return "CL_INVALID_OPERATION";
		case -60: return "CL_INVALID_GL_OBJECT";
		case -61: return "CL_INVALID_BUFFER_SIZE";
		case -62: return "CL_INVALID_MIP_LEVEL";
		case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
		case -64: return "CL_INVALID_PROPERTY";
		case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
		case -66: return "CL_INVALID_COMPILER_OPTIONS";
		case -67: return "CL_INVALID_LINKER_OPTIONS";
		case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

			// extension errors
		case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
		case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
		case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
		case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
		case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
		case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
		default: return "Unknown OpenCL error";
		}
	}

#define CHK_OCL(sts) if((sts) != CL_SUCCESS) { std::cerr << "OpenCL Error:" << getOpenCLErrorString((sts)) << std::endl; }

	void DecodeExtensionString(OpenCL::Device& cld)
	{
		size_t stringLength = 0;
		clGetDeviceInfo(cld.deviceId, CL_DEVICE_EXTENSIONS, 0, nullptr, &stringLength);
		if (stringLength == 0) return;
		std::unique_ptr<char[]> tempCharArray(new char[stringLength]);

		clGetDeviceInfo(cld.deviceId, CL_DEVICE_EXTENSIONS, stringLength, tempCharArray.get(), nullptr);

		std::stringstream extensionsStream(tempCharArray.get());
		std::vector<std::string> extensionStrings;
		while( !extensionsStream.eof() )
		{
			std::string tmp;
			extensionsStream >> tmp;
			extensionStrings.push_back(tmp);
		}

		cld.extensionsSupported.reset();

		for (const std::string& extension: extensionStrings)
		{
			uint64_t inHash = RuntimeHash<uint64_t>(extension);
			switch( inHash )
			{
			case Hash<uint64_t>("cl_khr_fp16") :
				cld.extensionsSupported[(int)OpenCL::Extensions::CL_KHR_FP16] = true; break;
			case Hash<uint64_t>("cl_khr_fp64") :
				cld.extensionsSupported[(int)OpenCL::Extensions::CL_KHR_FP64] = true; break;
			case Hash<uint64_t>("cl_nv_device_attribute_query") :
				cld.extensionsSupported[(int)OpenCL::Extensions::CL_NV_DEVICE_ATTRIBUTE_QUERY] = true; break;
			case Hash<uint64_t>("cl_amd_device_attribute_query"):
				cld.extensionsSupported[(int)OpenCL::Extensions::CL_AMD_DEVICE_ATTRIBUTE_QUERY] = true; break;
			case Hash<uint64_t>("cl_amd_media_ops") :
				cld.extensionsSupported[(int)OpenCL::Extensions::CL_AMD_MEDIA_OPS] = true; break;
			case Hash<uint64_t>("cl_amd_media_ops2") :
				cld.extensionsSupported[(int)OpenCL::Extensions::CL_AMD_MEDIA_OPS2] = true; break;
			default:
				break;
			}
		}

		// misc flags
		cl_bool flag = false;
		clGetDeviceInfo(cld.deviceId, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &flag, nullptr);
		if (flag) cld.flags = (OpenCL::DeviceFlags) (OpenCL::SHARED_HOST_DEVICE_MEMORY | cld.flags);
		
	}


	void EstimateHMFLOPs(std::list<OpenCL::Device>& cldevices)
	{

		using namespace OpenCL;
		for (Device& cld : cldevices)
		{
			if( cld.extensionsSupported[(int)Extensions::CL_KHR_FP16] == false )
			{
				cld.estimatedHMFLOPs = 0;
				continue;
			}
			cl_uint cuCount = 0;
			cl_uint nativeHalfSimdWidth = 0;
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &cuCount, nullptr);
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, sizeof(cl_uint), &nativeHalfSimdWidth, nullptr);

			unsigned int alus = cuCount * nativeHalfSimdWidth; // probably not true but its generic start

			// the hard bit of this estimate is how many flops per 'compute unit'
			// some vendors have extended query tools that will provide us with more info
			// others we have to 'guess'. As such the number produced is dubious but still useful
			// as a first look see at the power of each device.

			if (cld.vendor == Vendor::NVIDIA &&
				cld.extensionsSupported[(int)Extensions::CL_NV_DEVICE_ATTRIBUTE_QUERY])
			{
				cl_uint warpSize = 0;
				cl_uint cudaMajor = 1;
				cl_uint cudaMinor = 0;
				cl_uint regsPerWorkgroup = 0;
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_WARP_SIZE_NV, sizeof(cl_uint), &warpSize, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(cl_uint), &cudaMajor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(cl_uint), &cudaMinor, nullptr);

				clGetDeviceInfo(cld.deviceId, CL_DEVICE_REGISTERS_PER_BLOCK_NV, sizeof(cl_uint), &regsPerWorkgroup, nullptr);

				// TODO add more known HW configs. Is this corrrect?
				switch (cudaMajor)
				{
				case 5:
				case 6:
					if (cudaMajor == 6 && cudaMinor == 0)
					{
						alus = cuCount * warpSize * 4 * 2; // GP100 has x2 half units and 64 core per SM (warp * 2)
					} else
					{
						alus = cuCount * warpSize * 4; // Maxwell GPUs have no special half units and 128 cores per SM (warp * 4)
					}
					break;
				}
			}
			else if (cld.vendor == Vendor::AMD &&
				cld.extensionsSupported[(int)Extensions::CL_AMD_DEVICE_ATTRIBUTE_QUERY])
			{
				cl_uint simdPerCU = 0;
				cl_uint simdWidth = 0;
				cl_uint gfxIPMajor = 0;
				cl_uint gfxIPMinor = 0;
				cl_uint localMemPerCU = 0;
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD, sizeof(cl_uint), &simdPerCU, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_SIMD_WIDTH_AMD, sizeof(cl_uint), &simdWidth, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_GFXIP_MAJOR_AMD, sizeof(cl_uint), &gfxIPMajor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_GFXIP_MINOR_AMD, sizeof(cl_uint), &gfxIPMinor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD, sizeof(cl_uint), &localMemPerCU, nullptr);

				alus = cuCount * simdPerCU * simdWidth;
				if( gfxIPMajor == 9)
				{
					alus = alus * 2; // VEGA has x2 half units
				}
			}
			else if (cld.vendor == Vendor::Intel)
			{
				if (cld.type == CL_DEVICE_TYPE_GPU) {
					// not sure how to tell which genX is installed so assuming gen 9 at the mo (cos thats what I've got)
					alus = cuCount * 4 * 2 * 2;  // two FPUs with 4 way float SIMD per EU (AKA CU), double rate halfs!
				}
			}

			cl_uint maxClock = 0;
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &maxClock, nullptr);
			cld.estimatedHMFLOPs = alus * maxClock * 2;
		}
	}

	void EstimateSMFLOPs(std::list<OpenCL::Device>& cldevices)
	{

		using namespace OpenCL;
		for (Device& cld : cldevices)
		{
			cl_uint cuCount = 0;
			cl_uint nativeFloatSimdWidth = 0;
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &cuCount, nullptr);
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &nativeFloatSimdWidth, nullptr);

			unsigned int alus = cuCount * nativeFloatSimdWidth; // probably not true but its generic start

			// the hard bit of this estimate is how many flops per 'compute unit'
			// some vendors have extended query tools that will provide us with more info
			// others we have to 'guess'. As such the number produced is dubious but still useful
			// as a first look see at the power of each device.

			if (cld.vendor == Vendor::NVIDIA &&
				cld.extensionsSupported[(int)Extensions::CL_NV_DEVICE_ATTRIBUTE_QUERY])
			{
				cl_uint warpSize = 0;
				cl_uint cudaMajor = 1;
				cl_uint cudaMinor = 0;
				cl_uint regsPerWorkgroup = 0;
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_WARP_SIZE_NV, sizeof(cl_uint), &warpSize, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(cl_uint), &cudaMajor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(cl_uint), &cudaMinor, nullptr);

				clGetDeviceInfo(cld.deviceId, CL_DEVICE_REGISTERS_PER_BLOCK_NV, sizeof(cl_uint), &regsPerWorkgroup, nullptr);

				// TODO add more known HW configs. Is this corrrect?
				switch( cudaMajor )
				{
				case 5:
				case 6:
					if (cudaMajor ==6 && cudaMinor == 0)
					{
						alus = cuCount * warpSize * 2; // GP100 has 64 core per SM (warp * 2)
					}
					else
					{
						alus = cuCount * warpSize * 4; // Maxwell based GPU have 128 cores per SM (warp * 4)
					}
					break;
				}
			}
			else if (cld.vendor == Vendor::AMD &&
				cld.extensionsSupported[(int)Extensions::CL_AMD_DEVICE_ATTRIBUTE_QUERY])
			{
				cl_uint simdPerCU = 0;
				cl_uint simdWidth = 0;
				cl_uint gfxIPMajor = 0;
				cl_uint gfxIPMinor = 0;
				cl_uint localMemPerCU = 0;
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD, sizeof(cl_uint), &simdPerCU, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_SIMD_WIDTH_AMD, sizeof(cl_uint), &simdWidth, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_GFXIP_MAJOR_AMD, sizeof(cl_uint), &gfxIPMajor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_GFXIP_MINOR_AMD, sizeof(cl_uint), &gfxIPMinor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD, sizeof(cl_uint), &localMemPerCU, nullptr);
				
				alus = cuCount * simdPerCU * simdWidth;
			} else if( cld.vendor == Vendor::Intel )
			{
				if (cld.type == CL_DEVICE_TYPE_GPU) {
					// not sure how to tell which genX is installed so assuming gen 9 at the mo (cos thats what I've got)
					alus = cuCount * 4 * 2;  // two FPUs with 4 way float SIMD per EU (AKA CU)
				}
			}

			cl_uint maxClock = 0;
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &maxClock, nullptr);
			cld.estimatedSMFLOPs = alus * maxClock * 2;
		}
	}

	void EstimateDMFLOPs(std::list<OpenCL::Device>& cldevices)
	{

		using namespace OpenCL;
		for (Device& cld : cldevices)
		{
			if (cld.extensionsSupported[(int)Extensions::CL_KHR_FP64] == false)
			{
				cld.estimatedDMFLOPs = 0;
				continue;
			}

			cl_uint cuCount = 0;
			cl_uint nativeDoubleSimdWidth = 0;
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &cuCount, nullptr);
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &nativeDoubleSimdWidth, nullptr);

			unsigned int alus = cuCount * nativeDoubleSimdWidth; // probably not true but its generic start

			// the hard bit of this estimate is how many flops per 'compute unit'
			// some vendors have extended query tools that will provide us with more info
			// others we have to 'guess'. As such the number produced is dubious but still useful
			// as a first look see at the power of each device.
			if (cld.vendor == Vendor::NVIDIA &&
				cld.extensionsSupported[(int)Extensions::CL_NV_DEVICE_ATTRIBUTE_QUERY])
			{
				cl_uint warpSize = 0;
				cl_uint cudaMajor = 1;
				cl_uint cudaMinor = 0;
				cl_uint regsPerWorkgroup = 0;
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_WARP_SIZE_NV, sizeof(cl_uint), &warpSize, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(cl_uint), &cudaMajor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(cl_uint), &cudaMinor, nullptr);

				clGetDeviceInfo(cld.deviceId, CL_DEVICE_REGISTERS_PER_BLOCK_NV, sizeof(cl_uint), &regsPerWorkgroup, nullptr);

				switch (cudaMajor)
				{
				case 5:
				case 6:
					if (cudaMajor == 6 && cudaMinor == 0)
					{
						alus = cuCount * warpSize * 2 / 2; // GP100 has 64 core per SM (warp * 2). 1/2 rate DP
					}
					else
					{
						alus = cuCount * warpSize / 8; // Maxwell based GPU have 4 double ALUs per SM (warp / 8)
					}
					break;
				}
			}
			else if (cld.vendor == Vendor::AMD &&
				cld.extensionsSupported[(int)Extensions::CL_AMD_DEVICE_ATTRIBUTE_QUERY])
			{
				cl_uint simdPerCU = 0;
				cl_uint simdWidth = 0;
				cl_uint gfxIPMajor = 0;
				cl_uint gfxIPMinor = 0;
				cl_uint localMemPerCU = 0;
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD, sizeof(cl_uint), &simdPerCU, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_SIMD_WIDTH_AMD, sizeof(cl_uint), &simdWidth, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_GFXIP_MAJOR_AMD, sizeof(cl_uint), &gfxIPMajor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_GFXIP_MINOR_AMD, sizeof(cl_uint), &gfxIPMinor, nullptr);
				clGetDeviceInfo(cld.deviceId, CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD, sizeof(cl_uint), &localMemPerCU, nullptr);

				// TODO double performance varys from 1/16 of single, need to figure out how
				alus = cuCount * 4; // 1/16th of single performance
			}
			else if (cld.vendor == Vendor::Intel)
			{
				if (cld.type == CL_DEVICE_TYPE_GPU) {
					// not sure how to tell which genX is installed so assuming gen 9 at the mo (cos thats what I've got)
					alus = cuCount * (4/2);  // 1 FPU at half rate
				}
			}

			cl_uint maxClock = 0;
			clGetDeviceInfo(cld.deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &maxClock, nullptr);
			cld.estimatedDMFLOPs = alus * maxClock * 2;
		}
	}

	bool CreateCLDevices(std::list<OpenCL::Device>& cldevices)
	{
		using namespace OpenCL;

		cl_uint numPlatforms;	//the NO. of platforms
		cl_int	status = clGetPlatformIDs(0, nullptr, &numPlatforms);
		if (status != CL_SUCCESS)
		{
			std::cout << "Error: Getting platforms!" << std::endl;
			return false;
		}

		std::vector<cl_platform_id> platforms;
		// each vendors GPU will be a different platform.
		// CPU devices will likely by Intel or AMDs platform (depending on installs)
		if (numPlatforms > 0)
		{
			platforms.resize(numPlatforms);
			status = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
			if (status != CL_SUCCESS)
			{
				std::cout << "Error: Getting platform IDs!" << std::endl;
				return false;
			}
		}

		// for some reason i'm getting duplicate platforms...so lets fix that
		std::sort(platforms.begin(), platforms.end());
		platforms.erase(std::unique(platforms.begin(), platforms.end()), platforms.end());


		std::list<Device>::iterator preferredCPUDevice = cldevices.end();

		for (cl_platform_id platform : platforms)
		{
			// A platform may have a number of devices, each a different type.
			// A platform AND device id are a unique key to each device
			cl_uint	numDevices = 0;
			clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
			if (numDevices == 0)	//no devices available.
			{
				continue;
			}

			std::vector<cl_device_id> devices;
			devices.resize(numDevices);
			status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices.data(), nullptr);

			Vendor vendor(Vendor::Unknown);
			APIVersion version(APIVersion::CL1_0);
			static const size_t tempCharArraySize = 2048;
			char tempCharArray[tempCharArraySize];

			clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, tempCharArraySize, tempCharArray, nullptr);
			std::string platformVendor(tempCharArray);

			if (platformVendor == "NVIDIA Corporation") vendor = Vendor::NVIDIA;
			if (platformVendor == "Intel(R) Corporation") vendor = Vendor::Intel;
			if (platformVendor == "Advanced Micro Devices, Inc.") vendor = Vendor::AMD;

			// version is always OpenCL X.Y ZZZZZZZZZZ (Z is optional and vendor depedent)
			clGetPlatformInfo(platform, CL_PLATFORM_VERSION, tempCharArraySize, tempCharArray, nullptr);
			std::stringstream pvstrstream(tempCharArray);
			std::string dummy, platformVersion;
			pvstrstream >> dummy >> platformVersion;
			std::string platformMajorVersion = platformVersion.substr(0, 1);
			pvstrstream.str(platformMajorVersion);
			int pmv = 0;
			pvstrstream >> pmv;
			switch (pmv)
			{
			case 1:
				version = APIVersion::CL1_X;
				if (platformVersion == "1.0") version = APIVersion::CL1_0;
				if (platformVersion == "1.1") version = APIVersion::CL1_1;
				if (platformVersion == "1.2") version = APIVersion::CL1_2;
				break;
			case 2:
				version = APIVersion::CL2_X;
				if (platformVersion == "2.0") version = APIVersion::CL2_0;
				if (platformVersion == "2.1") version = APIVersion::CL2_1;
				break;
			case 3: case 4: case 5: case 6: case 7: case 8: case 9:
				version = APIVersion::CL3_PLUS; break;
			default:
				version = APIVersion::CL1_0; break;
			}

			for (cl_device_id device : devices)
			{
				bool deviceOk;
				Device cld = { vendor, version, platform, device };
				clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &cld.type, nullptr);
				deviceOk = (cld.type != CL_DEVICE_TYPE_CUSTOM); // we don't support CUSTOMs

				cl_bool available = false;
				clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, sizeof(cl_bool), &available, nullptr);
				deviceOk |= !!available;

				DecodeExtensionString(cld);

				if (deviceOk)
				{
					auto inserted = cldevices.insert(cldevices.end(), 1, cld);

					// we can have multiple platform providing a CPU device
					// where possible pick the platform vendor that matchs the CPU vendor
					if (cld.type == CL_DEVICE_TYPE_CPU)
					{
						uint32_t vendorId = 0;
						clGetDeviceInfo(device, CL_DEVICE_NAME, tempCharArraySize, tempCharArray, nullptr);
						std::string deviceName(tempCharArray);

						if (vendor == Vendor::Intel && (deviceName.find("Intel") != std::string::npos))
						{
							preferredCPUDevice = inserted;
						}
						else if (vendor == Vendor::AMD && (deviceName.find("AMD") != std::string::npos))
						{
							preferredCPUDevice = inserted;
						}
					}
				}
			}
		}

		// if we have a preferredCPUDevice remove others, if not just use the first
		for (auto it = cldevices.begin(); it != cldevices.end(); )
		{
			if (it->type == CL_DEVICE_TYPE_CPU)
			{
				if (preferredCPUDevice == cldevices.end())
				{
					preferredCPUDevice = it;
				}

				if (it != preferredCPUDevice)
				{
					it = cldevices.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}

		//
		// When we get here we should a list of actual physical devices
		// We now want to create a context per platform, so devices in a platform
		// can more easily share data (cross platform/context is generally a host 
		// round trip always)
		std::vector<std::vector<cl_device_id>> devicesPerPlatform;
		for (cl_platform_id platform_id : platforms)
		{
			size_t platIdx = devicesPerPlatform.size();
			devicesPerPlatform.resize(platIdx + 1);
			for (const Device& cldevice : cldevices)
			{
				if (cldevice.platformId == platform_id)
				{
					devicesPerPlatform[platIdx].push_back(cldevice.deviceId);
				}
			}
		}
		std::vector<cl_context> contexts;
		for (size_t i = 0; i < platforms.size(); ++i)
		{
			cl_platform_id platform_id = platforms[i];

			auto& devices = devicesPerPlatform[i];
			const intptr_t contextProperties[] =
			{
				CL_CONTEXT_PLATFORM, (intptr_t)platform_id,
				0, 0
			};
			if (devices.size() == 0)
			{
				contexts.push_back(nullptr);
				continue;
			}

			cl_context context = clCreateContext(contextProperties, devices.size(), devices.data(), NULL, NULL, NULL);
			contexts.push_back(context);

			for (Device& cldevice : cldevices)
			{
				if (cldevice.platformId == platform_id)
				{
					cldevice.context = context;
				}
			}
		}
		return true;
	}


	enum class ProgramRef
	{
		Conv2D1Chan,
	};

	struct KernelAssets
	{
		int			index;
		std::string	name;
	};

	static const std::array<KernelAssets,1> kernelAssets =
	{
		{ (int)ProgramRef::Conv2D1Chan, "Conv2D1Chan" }
	};

	struct PerContext
	{
		std::vector<cl_program> programs;
		std::vector<cl_kernel>	kernels;
		cl_mem inImage;
	};

	struct PerDevice
	{
		const OpenCL::Device* cld;
		cl_command_queue	queue;
		// put the device name here, to make it easier when debugging
		char const *		name;

		cl_mem outBuffer;
	};

	struct App
	{
		std::list<OpenCL::Device> cldevices;
		std::unordered_map<cl_context, PerContext> ctxs;
		std::vector<PerDevice> devices;

	};

}

void CreatePerContexts(const std::list<OpenCL::Device>& cldevices, std::unordered_map<cl_context, PerContext>& ctxs)
{
	std::vector<cl_context> contexts;
	for (const OpenCL::Device& cld : cldevices)
	{
		contexts.push_back(cld.context);
	}
	std::sort(contexts.begin(), contexts.end());
	contexts.erase(std::unique(contexts.begin(), contexts.end()), contexts.end());

	for (cl_context context : contexts)
	{
		PerContext ctx;
		ctx.inImage = nullptr;
		for (const KernelAssets& asset : kernelAssets)
		{
			std::string sourceStr;
			const std::string filename = asset.name + ".cl";

			convertToString(filename.c_str(), sourceStr);
			const char *source = sourceStr.c_str();
			size_t sourceSize[] = { strlen(source) };

			cl_program prg = clCreateProgramWithSource(context, 1, &source, sourceSize, nullptr);
			ctx.programs.push_back(prg);

		}
		ctxs[context] = ctx;
	}
}

bool CreatePerDevices(const std::list<OpenCL::Device>& cldevices, std::unordered_map<cl_context, PerContext>& ctxs, std::vector<PerDevice>& devices)
{
	// now run the back end per device compile and create PerDevice structures
	for (const OpenCL::Device& cld : cldevices)
	{
		const auto& ctx = ctxs[cld.context];

		for (cl_program prg : ctx.programs)
		{
			cl_int status = clBuildProgram(prg, 1, &cld.deviceId, "-cl-std=CL1.2", nullptr, nullptr);
			CHK_OCL(status);

			cl_build_status build_status;
			clGetProgramBuildInfo(prg, cld.deviceId, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, nullptr);
			if (build_status == CL_BUILD_ERROR)
			{
				size_t buildLogSize = 0;
				clGetProgramBuildInfo(prg, cld.deviceId, CL_PROGRAM_BUILD_LOG, sizeof(size_t), nullptr, &buildLogSize);
				std::unique_ptr<char[]> buildLog(new char[buildLogSize]);
				clGetProgramBuildInfo(prg, cld.deviceId, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog.get(), nullptr);
				std::cerr << buildLog.get();

				return true;
			}
		}

		cl_command_queue commandQueue = clCreateCommandQueue(cld.context, cld.deviceId, 0, nullptr);

		char const * name = new char[1024];
		clGetDeviceInfo(cld.deviceId, CL_DEVICE_NAME, 1024, (void*)name, nullptr);


		devices.emplace_back<PerDevice>({
			&cld,
			commandQueue,
			name,
			nullptr
		});
	}

	for (auto& ctx : ctxs)
	{
		for (const KernelAssets& asset : kernelAssets)
		{
			// only now we can create the kernels (both contexts and devices setup has occured)
			cl_kernel kernel = clCreateKernel(ctx.second.programs[asset.index], asset.name.c_str(), nullptr);
			ctx.second.kernels.push_back(kernel);
		}
	}

	return false;
}

void CleanupPerContexts(std::unordered_map<cl_context, PerContext>& ctxs)
{
	for (auto& pc : ctxs)
	{
		clReleaseMemObject(pc.second.inImage);

		for (cl_kernel kernel : pc.second.kernels) {
			clReleaseKernel(kernel);
		}
		for (cl_program prg : pc.second.programs) {
			clReleaseProgram(prg);
		}
		clReleaseContext(pc.first);
	}
	ctxs.clear();
}

void CleanupPerDevices(std::vector<PerDevice>& devices)
{
	for (auto& dev : devices)
	{
		clReleaseMemObject(dev.outBuffer);
		delete[] dev.name;
		clReleaseCommandQueue(dev.queue);
	}
	devices.clear();
}


bool SetupOpenCLForApp(App& app)
{
	std::list<OpenCL::Device>& cldevices = app.cldevices;

	std::unordered_map<cl_context, PerContext>& ctxs = app.ctxs;
	std::vector<PerDevice>& devices = app.devices;

	if (!CreateCLDevices(cldevices))
	{
		return false;
	}

	EstimateHMFLOPs(cldevices);
	EstimateSMFLOPs(cldevices);
	EstimateDMFLOPs(cldevices);

	// we have to do things (create programs) in terms of unique contexts
	CreatePerContexts(cldevices, ctxs);
	if (CreatePerDevices(cldevices, ctxs, devices))
	{
		return false;
	}
	
	return true;
}

void CleanupOpenCLForApp(App& app)
{
	std::unordered_map<cl_context, PerContext>& ctxs = app.ctxs;
	std::vector<PerDevice>& devices = app.devices;

	// ---- They think its all over... It is now!
	// destroy everything (included shared contexts)

	CleanupPerDevices(devices);
	CleanupPerContexts(ctxs);
}

int main(int argc, char* argv[])
{
	static const int MAIN_SUCCESS_CODE = 0;
	static const int MAIN_FAIL_CODE = 10;

	App app;
	cl_int status;

	if (!SetupOpenCLForApp(app)) { return MAIN_FAIL_CODE; }

	// TODO use aligned alloc to avoid host -> host copy for integrated devices
	// 4 = 4 channel but 'n' will always be the number that it would have been if you said 0 (whats in the file)
	int w = 0, h = 0, n = 0;
	unsigned char * loaddata = stbi_load("radpro256x256.jpg", &w, &h, &n, 4);
	if (loaddata == nullptr || w == 0 || h == 0 || n == 0)
	{
		CleanupOpenCLForApp(app);
		return MAIN_FAIL_CODE;
	}
	uint8_t* imagedata = (uint8_t*)aligned_malloc(w * h * 4, 4096);
	if( imagedata == nullptr )
	{
		CleanupOpenCLForApp(app);
		return MAIN_FAIL_CODE;
	}
	memcpy(imagedata, loaddata, w * h * 4);
	stbi_image_free(loaddata);

	// ... process data if not NULL ...
	const cl_image_format imgFmt = { CL_RGBA, CL_UNORM_INT8 };
	const cl_image_desc imgDesc = { CL_MEM_OBJECT_IMAGE2D, w, h, 1, 1, w*4, 0, 0, 0, nullptr };

	// we can share read buffer between all devices on a context (its upto the driver to decide)
	for (auto& ctx : app.ctxs)
	{
		ctx.second.inImage = clCreateImage(ctx.first, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS, &imgFmt, &imgDesc, imagedata, &status);
		CHK_OCL(status);
	}
	// however outputs have to be device specific
	for (auto& device : app.devices)
	{
		if( device.cld->flags & OpenCL::SHARED_HOST_DEVICE_MEMORY)
		{
			device.outBuffer = clCreateBuffer(device.cld->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, w * h * sizeof(float), nullptr, &status);
		} else
		{			
			device.outBuffer = clCreateBuffer(device.cld->context, CL_MEM_WRITE_ONLY, w * h * sizeof(float), nullptr, &status);
		}
		CHK_OCL(status);
	}


	//-----------
	// main app body goes here
	//-----------
	for (const PerDevice& device : app.devices)
	{
		const size_t global_work_offset[2] = { 1, 1 };
		const size_t global_work_size[2] = { w-1, h-1 };
		const cl_int resultStride = w;

		const PerContext& ctx = app.ctxs[device.cld->context];
		cl_kernel kernel = ctx.kernels[(int)ProgramRef::Conv2D1Chan];

		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&ctx.inImage);
		CHK_OCL(status);
		status = clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&resultStride);
		CHK_OCL(status);
		status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&device.outBuffer);
		CHK_OCL(status);

		status = clEnqueueNDRangeKernel(	
								device.queue,
								kernel,
								2, global_work_offset, global_work_size, 
								nullptr, 
								0, nullptr, nullptr );
		CHK_OCL(status);

		std::unique_ptr<char[]> tmp;
		char* data;
		if( device.cld->flags & OpenCL::SHARED_HOST_DEVICE_MEMORY )
		{
			data = (char*) clEnqueueMapBuffer(device.queue, device.outBuffer, CL_TRUE, CL_MAP_READ, 0, w * h * sizeof(float), 0, nullptr, nullptr, &status);
			CHK_OCL(status);
		}
		else
		{
			tmp.reset(new char[w * h * sizeof(float)]);
			data = tmp.get();
			status = clEnqueueReadBuffer(device.queue, device.outBuffer, CL_TRUE, 0, w * h * sizeof(float), tmp.get(), 0, nullptr, nullptr);
			CHK_OCL(status);
		}

		static int counter = 0;
		char filename[256];
		sprintf_s(filename, 256, "dump%i.bin", counter++);
		FILE* fh;
		fopen_s(&fh, filename, "wb");
		fwrite(data, 64 * 64 * 4, 1, fh);
		fclose(fh);
	}

	aligned_free(imagedata);
	CleanupOpenCLForApp(app);

	return MAIN_SUCCESS_CODE;
}
