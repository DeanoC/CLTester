// CLTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "cldevice.h"
#include "consthash.h"

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

namespace {

	/* convert the kernel file into a string */
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
		switch(pmv)
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
			version = APIVersion::CL3_PLUS;break;
		default:
			version = APIVersion::CL1_0; break;
		}

		for (cl_device_id device : devices)
		{
			bool deviceOk;
			Device cld = { vendor, version, platform, device };
			clGetDeviceInfo( device, CL_DEVICE_TYPE, sizeof(cl_device_type), &cld.type, nullptr);
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
					} else if (vendor == Vendor::AMD && (deviceName.find("AMD") != std::string::npos))
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
		if( it->type == CL_DEVICE_TYPE_CPU)
		{
			if (preferredCPUDevice == cldevices.end())
			{
				preferredCPUDevice = it;
			}

			if (it != preferredCPUDevice)
			{
				it = cldevices.erase(it);
			} else
			{
				++it;
			}
		} else
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
			CL_CONTEXT_PLATFORM, (intptr_t) platform_id,
			0, 0 
		};
		if( devices.size() == 0)
		{
			contexts.push_back(nullptr);
			continue;
		}

		cl_context context = clCreateContext(contextProperties, devices.size(), devices.data(), NULL, NULL, NULL);
		contexts.push_back(context);

		for (Device& cldevice : cldevices)
		{
			if(cldevice.platformId == platform_id)
			{
				cldevice.context = context;
			}
		}
	}
	return true;
}



int main(int argc, char* argv[])
{
	std::list<OpenCL::Device> cldevices;

	if (!CreateCLDevices(cldevices)) return 10;

	EstimateHMFLOPs(cldevices);
	EstimateSMFLOPs(cldevices);
	EstimateDMFLOPs(cldevices);

	// lets now do something kernelish

	// we have to do things (create programs) in terms of unique contexts
	std::vector<cl_context> contexts;

	for (OpenCL::Device& cld : cldevices)
	{
		contexts.push_back(cld.context);
	}
	std::sort(contexts.begin(), contexts.end());
	contexts.erase(std::unique(contexts.begin(), contexts.end()), contexts.end());

	std::unordered_map<cl_context, cl_program> programs; // 1 program per context

	// load/parsing/compiler ahoy ahoy
	const char *filename = "Conv2D1Chan.cl";
	std::string sourceStr;
	convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };

	for (cl_context context : contexts)
	{
		programs[context] = clCreateProgramWithSource(context, 1, &source, sourceSize, nullptr);
	}

	for (OpenCL::Device& cld : cldevices)
	{
		cl_program prg = programs[cld.context];
		cl_int status = clBuildProgram(prg, 1, &cld.deviceId, "-cl-std=CL1.2", nullptr, nullptr);

		cl_build_status build_status;
		clGetProgramBuildInfo(programs[cld.context], cld.deviceId, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &build_status, NULL);
		if (build_status == CL_BUILD_ERROR)
		{
			size_t buildLogSize = 0;
			clGetProgramBuildInfo(programs[cld.context], cld.deviceId, CL_PROGRAM_BUILD_LOG, sizeof(size_t), nullptr, &buildLogSize);
			std::unique_ptr<char[]> buildLog(new char[buildLogSize]);
			clGetProgramBuildInfo(programs[cld.context], cld.deviceId, CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog.get(), nullptr);
			std::cerr << buildLog.get();

			return 10;
		}
	}

	// ---- They think its all over... It is now!
	// destroy everything (included shared contexts)
	for (std::pair<cl_context const, cl_program> programpair : programs)
	{
		clReleaseProgram(programpair.second);
	}
	programs.clear();

	for (cl_context context : contexts)
	{
		clReleaseContext(context);
	}
	contexts.clear();
	cldevices.clear();
	/*
	//Step 4: Creating command queue associate with the context.
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

	//Step 7: Initial input,output for the host and create memory objects for the kernel
	const char* input = "GdkknVnqkc";
	size_t strlength = strlen(input);
	cout << "input string:" << endl;
	cout << input << endl;
	char *output = (char*)malloc(strlength + 1);

	cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (strlength + 1) * sizeof(char), (void *)input, NULL);
	cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, (strlength + 1) * sizeof(char), NULL, NULL);

	//Step 8: Create kernel object
	cl_kernel kernel = clCreateKernel(program, "helloworld", NULL);

	//Step 9: Sets Kernel arguments.
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outputBuffer);

	//Step 10: Running the kernel.
	size_t global_work_size[1] = { strlength };
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

	//Step 11: Read the cout put back to host memory.
	status = clEnqueueReadBuffer(commandQueue, outputBuffer, CL_TRUE, 0, strlength * sizeof(char), output, 0, NULL, NULL);

	output[strlength] = '\0';	//Add the terminal character to the end of output.
	cout << "\noutput string:" << endl;
	cout << output << endl;

	//Step 12: Clean the resources.
	status = clReleaseKernel(kernel);				//Release kernel.
	status = clReleaseProgram(program);				//Release the program object.
	status = clReleaseMemObject(inputBuffer);		//Release mem object.
	status = clReleaseMemObject(outputBuffer);
	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	status = clReleaseContext(context);				//Release context.
	*/
	std::cout << "Passed!\n";
	return 0;
}
