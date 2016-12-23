#pragma once
/*******************************************************************************
* Copyright (c) 2008-2013 The Khronos Group Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and/or associated documentation files (the
* "Materials"), to deal in the Materials without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Materials, and to
* permit persons to whom the Materials are furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Materials.
*
* THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include <AvailabilityMacros.h>
#else
#include <CL/cl.h>
#endif

/*********************************
* cl_amd_device_memory_flags *
*********************************/
#define cl_amd_device_memory_flags 1
#define CL_MEM_USE_PERSISTENT_MEM_AMD       (1 << 6)        // Alloc from GPU's CPU visible heap

/* cl_device_info */
#define CL_DEVICE_MAX_ATOMIC_COUNTERS_EXT           0x4032

/*********************************
* cl_amd_device_attribute_query *
*********************************/
#define CL_DEVICE_PROFILING_TIMER_OFFSET_AMD        0x4036
#define CL_DEVICE_TOPOLOGY_AMD                      0x4037
#define CL_DEVICE_BOARD_NAME_AMD                    0x4038
#define CL_DEVICE_GLOBAL_FREE_MEMORY_AMD            0x4039
#define CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD         0x4040
#define CL_DEVICE_SIMD_WIDTH_AMD                    0x4041
#define CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD        0x4042
#define CL_DEVICE_WAVEFRONT_WIDTH_AMD               0x4043
#define CL_DEVICE_GLOBAL_MEM_CHANNELS_AMD           0x4044
#define CL_DEVICE_GLOBAL_MEM_CHANNEL_BANKS_AMD      0x4045
#define CL_DEVICE_GLOBAL_MEM_CHANNEL_BANK_WIDTH_AMD 0x4046
#define CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD   0x4047
#define CL_DEVICE_LOCAL_MEM_BANKS_AMD               0x4048
#define CL_DEVICE_THREAD_TRACE_SUPPORTED_AMD        0x4049
#define CL_DEVICE_GFXIP_MAJOR_AMD                   0x404A
#define CL_DEVICE_GFXIP_MINOR_AMD                   0x404B
#define CL_DEVICE_AVAILABLE_ASYNC_QUEUES_AMD        0x404C

typedef union
{
	struct { cl_uint type; cl_uint data[5]; } raw;
	struct { cl_uint type; cl_char unused[17]; cl_char bus; cl_char device; cl_char function; } pcie;
} cl_device_topology_amd;

#define CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD            1

/*************
* cl_amd_hsa *
**************/
#define CL_HSA_ENABLED_AMD                          (1ull << 62)
#define CL_HSA_DISABLED_AMD                         (1ull << 63)


/**************************
* cl_amd_offline_devices *
**************************/
#define CL_CONTEXT_OFFLINE_DEVICES_AMD              0x403F

#ifdef CL_VERSION_1_2
/********************************
* cl_amd_bus_addressable_memory *
********************************/

/* cl_mem flag - bitfield */
#define CL_MEM_BUS_ADDRESSABLE_AMD               (1<<30)
#define CL_MEM_EXTERNAL_PHYSICAL_AMD             (1<<31)

#define CL_COMMAND_WAIT_SIGNAL_AMD                0x4080
#define CL_COMMAND_WRITE_SIGNAL_AMD               0x4081
#define CL_COMMAND_MAKE_BUFFERS_RESIDENT_AMD      0x4082

typedef struct _cl_bus_address_amd
{
	cl_ulong surface_bus_address;
	cl_ulong marker_bus_address;
} cl_bus_address_amd;

typedef CL_API_ENTRY cl_int
(CL_API_CALL * clEnqueueWaitSignalAMD_fn)(cl_command_queue /*command_queue*/,
	cl_mem /*mem_object*/,
	cl_uint /*value*/,
	cl_uint /*num_events*/,
	const cl_event * /*event_wait_list*/,
	cl_event * /*event*/) CL_EXT_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int
(CL_API_CALL * clEnqueueWriteSignalAMD_fn)(cl_command_queue /*command_queue*/,
	cl_mem /*mem_object*/,
	cl_uint /*value*/,
	cl_ulong /*offset*/,
	cl_uint /*num_events*/,
	const cl_event * /*event_list*/,
	cl_event * /*event*/) CL_EXT_SUFFIX__VERSION_1_2;

typedef CL_API_ENTRY cl_int
(CL_API_CALL * clEnqueueMakeBuffersResidentAMD_fn)(cl_command_queue /*command_queue*/,
	cl_uint /*num_mem_objs*/,
	cl_mem * /*mem_objects*/,
	cl_bool /*blocking_make_resident*/,
	cl_bus_address_amd * /*bus_addresses*/,
	cl_uint /*num_events*/,
	const cl_event * /*event_list*/,
	cl_event * /*event*/) CL_EXT_SUFFIX__VERSION_1_2;
#endif /* CL_VERSION_1_2 */

#ifdef __cplusplus
}
#endif