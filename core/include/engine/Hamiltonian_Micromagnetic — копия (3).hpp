#pragma once
#ifndef HAMILTONIAN_MICROMAGNETIC_H
#define HAMILTONIAN_MICROMAGNETIC_H

#include <vector>
#include <memory>

#include "Spirit_Defines.h"
#include <engine/Vectormath_Defines.hpp>
#include <engine/Hamiltonian.hpp>
#include <data/Geometry.hpp>
#include "FFT.hpp"
#include <vulkan/vulkan.h>
#include "VulkanInitializers.hpp"
#include <string.h>
#include <chrono>
#define VK_CHECK_RESULT(f) 																				\
{																										\
    VkResult res = (f);																					\
    if (res != VK_SUCCESS)																				\
    {																									\
        printf("Fatal : VkResult is %d in %s at line %d\n", res,  __FILE__, __LINE__); \
        assert(res == VK_SUCCESS);																		\
    }																									\
}
#define SUPPORTED_RADIX_LEVELS 1
#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))
#define __builtin_clz __lzcnt


namespace Engine
{
	/*enum class DDI_Method
    {
        FFT    = SPIRIT_DDI_METHOD_FFT,
        FMM    = SPIRIT_DDI_METHOD_FMM,
        Cutoff = SPIRIT_DDI_METHOD_CUTOFF,
        None   = SPIRIT_DDI_METHOD_NONE
    };
*/
    /*
        The Micromagnetic Hamiltonian
    */
    class Hamiltonian_Micromagnetic : public Hamiltonian
    {
    public:
    	/*
        Hamiltonian_Micromagnetic(
            scalar external_field_magnitude, Vector3 external_field_normal,
            int n_anisotropy, scalarfield anisotropy_magnitudes, vectorfield anisotropy_normals,
            scalar exchange_constant,
            scalar dmi_constant,
            std::shared_ptr<Data::Geometry> geometry,
            int spatial_gradient_order,
            intfield boundary_conditions,
            Vector3 cell_sizes,
            scalar Ms
        );
*/
        Hamiltonian_Micromagnetic(
			scalarfield external_field_magnitude, vectorfield external_field_normal,
			intfield n_anisotropies, std::vector<std::vector<scalar>>  anisotropy_magnitudes, std::vector<std::vector<Vector3>> anisotropy_normals,
			scalarfield  exchange_stiffness,
			scalarfield  dmi,
			std::shared_ptr<Data::Geometry> geometry,
			int spatial_gradient_order,
			intfield boundary_conditions,
			Vector3 cell_sizes,
			scalarfield Ms, int region_num
        );

        void Update_Interactions();

        void Update_Energy_Contributions() override;

        void Hessian(const vectorfield & spins, MatrixX & hessian) override;
        void Gradient(const vectorfield & spins, vectorfield & gradient) override;
        void Energy_Contributions_per_Spin(const vectorfield & spins, std::vector<std::pair<std::string, scalarfield>> & contributions) override;
		void Energy_Update(const vectorfield & spins, std::vector<std::pair<std::string, scalarfield>> & contributions, vectorfield & gradient);
        // Calculate the total energy for a single spin to be used in Monte Carlo.
        //      Note: therefore the energy of pairs is weighted x2 and of quadruplets x4.
        scalar Energy_Single_Spin(int ispin, const vectorfield & spins) override;

        // Hamiltonian name as string
        const std::string& Name() override;

        std::shared_ptr<Data::Geometry> geometry;
        scalarfield Ms;
        // ------------ ... ------------
        int spatial_gradient_order;

        // ------------ Single Spin Interactions ------------
        // External magnetic field across the sample
        scalarfield external_field_magnitude;
        vectorfield external_field_normal;
        Matrix3 anisotropy_tensor;
        scalar anisotropy_magnitude;
        intfield n_anisotropies;
        std::vector<std::vector<scalar>> anisotropy_magnitudes;//=scalarfield(256,0);
        std::vector<std::vector<Vector3>> anisotropy_normals;//=vectorfield(256, Vector3{0, 0, 1});
        scalarfield  exchange_stiffness;
        scalarfield  dmi;

        // ------------ Pair Interactions ------------
        // Exchange interaction
        Matrix3 exchange_tensor;
        scalar exchange_constant;
        // DMI
        Matrix3 dmi_tensor;
        scalar dmi_constant;
        //DDI_Method  ddi_method;
		intfield    ddi_n_periodic_images;
		Vector3 cell_sizes;
		//      ddi cutoff variables
		scalar      ddi_cutoff_radius;
		pairfield   ddi_pairs;
		scalarfield ddi_magnitudes;
		vectorfield ddi_normals;
		//#ifndef SPIRIT_LOW_MEMORY
			vectorfield mult_spins;
		//#endif
		#ifdef SPIRIT_LOW_MEMORY
			scalarfield temp_energies;
		#endif
		vectorfield external_field;
		pairfield neigh;
		field<Matrix3> spatial_gradient;

		bool A_is_nondiagonal=false;
		int region_num;
		intfield regions;
		std::vector<std::vector<scalar>> exchange_table;
		regionbook regions_book;
		scalar minMs;
		typedef struct {
			VkAllocationCallbacks* allocator;
			VkPhysicalDevice physicalDevice;
			VkPhysicalDeviceProperties physicalDeviceProperties;
			VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
			VkDevice device;
			VkQueue queue;
			VkCommandPool commandPool;
			VkFence fence;
			VkShaderModule shaderModules[SUPPORTED_RADIX_LEVELS];
			VkDeviceSize uboAlignment;
		} VulkanFFTContext;

		typedef struct {
			VulkanFFTContext* context;
			VkDeviceSize size;
			VkBuffer hostBuffer, deviceBuffer;
			VkDeviceMemory deviceMemory;
		} VulkanFFTTransfer;
		typedef struct {
			uint32_t sampleCount;
			uint32_t stageCount;
			uint32_t* stageRadix;
			VkDeviceSize uboSize;
			VkBuffer ubo;
			VkDeviceMemory uboDeviceMemory;
			VkDescriptorPool descriptorPool;
			VkDescriptorSetLayout* descriptorSetLayouts;
			VkDescriptorSet* descriptorSets;
			VkPipelineLayout pipelineLayout;
			VkPipeline* pipelines;
		}VulkanFFTAxis;

		typedef struct {
			VulkanFFTContext* context;
			bool inverse, resultInSwapBuffer;
			VulkanFFTAxis axes[3];
			VkDeviceSize bufferSize;
			VkBuffer* buffer;
			VkDeviceMemory* deviceMemory;
		} VulkanFFTPlan;
		typedef struct {
			uint32_t stride[3];
			uint32_t radixStride, stageSize;
			float directionFactor;
			float angleFactor;
			float normalizationFactor;
		} VulkanFFTUBO;
		
		typedef struct {
			uint32_t WIDTH;
			uint32_t HEIGHT;
			uint32_t DEPTH;
		} VulkanDimensions;
		class ComputeApplication {
		private:
			// The pixels of the rendered mandelbrot set are in this format:
			struct Pixel {
				float r, g, b, a;
			};
			int WIDTH = 64; // Size of rendered mandelbrot set.
			int HEIGHT = 64; // Size of renderered mandelbrot set.
			int DEPTH = 1; // Size of renderered mandelbrot set.
			int WORKGROUP_SIZE = 32; // Workgroup size in compute shader.
			/*
			In order to use Vulkan, you must create an instance.
			*/
			VkInstance instance;

			VkDebugReportCallbackEXT debugReportCallback;
			/*
			The physical device is some device on the system that supports usage of Vulkan.
			Often, it is simply a graphics card that supports Vulkan.
			*/
			VkPhysicalDevice physicalDevice;
			/*
			Then we have the logical device VkDevice, which basically allows
			us to interact with the physical device.
			*/
			VkDevice device;

			/*
			The pipeline specifies the pipeline that all graphics and compute commands pass though in Vulkan.

			We will be creating a simple compute pipeline in this application.
			*/
			VkPipeline pipeline_all, pipeline_convolution, pipeline_zeropadding, pipeline_zeropaddingremove, pipeline_convolutionhermitian, pipeline_fillzero;
			VkPipelineLayout pipelineLayout;
			VkPipelineLayout pipelineLayoutConvolution;
			VkPipelineLayout pipelineLayoutConvolutionHermitian;
			VkPipelineLayout pipelineLayoutZeroPadding;
			VkPipelineLayout pipelineLayoutZeroPaddingRemove;
			VkPipelineLayout pipelineLayoutFillZero;
			VkShaderModule computeShaderModule;
			VkShaderModule computeShaderModuleConvolution;
			VkShaderModule computeShaderModuleConvolutionHermitian;
			VkShaderModule computeShaderModuleZeroPadding;
			VkShaderModule computeShaderModuleZeroPaddingRemove;
			VkShaderModule computeShaderModuleFillZero;
			/*
			The command buffer is used to record commands, that will be submitted to a queue.

			To allocate such command buffers, we use a command pool.
			*/
			VkCommandPool commandPool;
			VkCommandBuffer commandBufferAll;
			VkCommandBuffer commandBufferConvolution;
			VkCommandBuffer commandBufferConvolutionHermitian;
			VkCommandBuffer commandBufferZeroPadding;
			VkCommandBuffer commandBufferZeroPaddingRemove;
			VkCommandBuffer commandBufferFillZero;
			VkCommandBuffer commandBufferFFT;
			VkCommandBuffer commandBufferiFFT;
			/*

			Descriptors represent resources in shaders. They allow us to use things like
			uniform buffers, storage buffers and images in GLSL.

			A single descriptor represents a single resource, and several descriptors are organized
			into descriptor sets, which are basically just collections of descriptors.
			*/
			VkDescriptorPool descriptorPool;
			VkDescriptorSet descriptorSet;
			VkDescriptorSetLayout descriptorSetLayout;
			VkDescriptorPool descriptorPoolConvolution;
			VkDescriptorSet descriptorSetConvolution;
			VkDescriptorSetLayout descriptorSetLayoutConvolution;
			VkDescriptorPool descriptorPoolConvolutionHermitian;
			VkDescriptorSet descriptorSetConvolutionHermitian;
			VkDescriptorSetLayout descriptorSetLayoutConvolutionHermitian;
			VkDescriptorPool descriptorPoolZeroPadding;
			VkDescriptorSet descriptorSetZeroPadding;
			VkDescriptorSetLayout descriptorSetLayoutZeroPadding;
			VkDescriptorPool descriptorPoolZeroPaddingRemove;
			VkDescriptorSet descriptorSetZeroPaddingRemove;
			VkDescriptorSetLayout descriptorSetLayoutZeroPaddingRemove;
			VkDescriptorPool descriptorPoolFillZero;
			VkDescriptorSet descriptorSetFillZero;
			VkDescriptorSetLayout descriptorSetLayoutFillZero;
			/*
			The mandelbrot set will be rendered to this buffer.

			The memory that backs the buffer is bufferMemory.
			*/
			VkBuffer bufferSpins;
			VkBuffer bufferGradient;
			VkBuffer bufferRegions_Book;
			VkBuffer bufferRegions;
			VkBuffer kernel;
			VkBuffer Spins_FFT;
			VkBuffer uboDimensions;
			VkDeviceMemory bufferMemorySpins;
			VkDeviceMemory bufferMemoryGradient;
			VkDeviceMemory bufferMemoryRegions_Book;
			VkDeviceMemory bufferMemoryRegions;
			VkDeviceMemory bufferMemoryKernel;
			VkDeviceMemory bufferMemorySpins_FFT;
			VkDeviceMemory uboMemoryDimensions;
			uint32_t bufferSizeSpins;
			uint32_t bufferSizeGradient;
			uint32_t bufferSizeRegions_Book;
			uint32_t bufferSizeRegions;
			uint32_t bufferSizeKernel;
			uint32_t bufferSizeSpins_FFT;
			uint32_t uboSizeDimensions;
			std::vector<const char*> enabledLayers;

			VkPipelineCache pipelineCache;

			// DDI FFT
			VulkanFFTContext context = {};
			VulkanFFTPlan vulkanFFTPlan = { &context };
			VulkanFFTPlan vulkaniFFTPlan = { &context };
			/*
			In order to execute commands on a device(GPU), the commands must be submitted
			to a queue. The commands are stored in a command buffer, and this command buffer
			is given to the queue.

			There will be different kinds of queues on the device. Not all queues support
			graphics operations, for instance. For this application, we at least want a queue
			that supports compute operations.
			*/
			VkQueue queue; // a queue supporting compute operations.

			/*
			Groups of queues that have the same capabilities(for instance, they all supports graphics and computer operations),
			are grouped into queue families.

			When submitting a command buffer, you must specify to which queue in the family you are submitting to.
			This variable keeps track of the index of that queue in its family.
			*/
			uint32_t queueFamilyIndex;
			struct UboFFTDynamic {
				VulkanFFTUBO* model = nullptr;
			} uboFFTDynamic;

		public:
			void init(regionbook regions_book, intfield regions, field<FFT::FFT_cpx_type> transformed_dipole_matrices, std::shared_ptr<Data::Geometry> geometry ) {
				WIDTH = geometry->n_cells[0];
				HEIGHT = geometry->n_cells[1];
				DEPTH = geometry->n_cells[2];
				// Buffer size of the storage buffer that will contain the rendered mandelbrot set.
				bufferSizeSpins = 3 * sizeof(float) * WIDTH *  HEIGHT * DEPTH;
				bufferSizeGradient = 3 * sizeof(float) * WIDTH * HEIGHT * DEPTH;
				bufferSizeRegions_Book = sizeof(Regionvalues) * 10;
				bufferSizeRegions = sizeof(int) * WIDTH * HEIGHT * DEPTH;
				bufferSizeKernel = 6 * 2 * sizeof(FFT::FFT_cpx_type) * (WIDTH+1) * HEIGHT * DEPTH;
				//bufferSizeSpins_FFT = 3 * 4 * sizeof(FFT::FFT_cpx_type) * WIDTH * HEIGHT * DEPTH;

				uboSizeDimensions = sizeof(VulkanDimensions);
				// Initialize vulkan:
				createInstance();
				findPhysicalDevice();
				createDevice();
				VkCommandPoolCreateInfo commandPoolCreateInfo = {};
				commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolCreateInfo.flags = 0;
				// the queue family of this command pool. All command buffers allocated from this command pool,
				// must be submitted to queues of this family ONLY. 
				commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
				vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool);
				context.commandPool = commandPool;
				createBuffer(&bufferSpins, &bufferMemorySpins, bufferSizeSpins);
				createBuffer(&bufferGradient, &bufferMemoryGradient, bufferSizeGradient);
				createBuffer(&bufferRegions_Book, &bufferMemoryRegions_Book, bufferSizeRegions_Book);
				createBuffer(&bufferRegions, &bufferMemoryRegions, bufferSizeRegions);
				createBuffer(&kernel, &bufferMemoryKernel, bufferSizeKernel);
				//createBuffer(&Spins_FFT, &bufferMemorySpins_FFT, bufferSizeSpins_FFT);
				createBufferUBO(&uboDimensions, &uboMemoryDimensions, uboSizeDimensions);

				/*createDescriptorSetLayout();
				createDescriptorSet();
				createComputePipeline();
				createCommandBuffer();
				createDescriptorSetLayoutConvolution();
				createDescriptorSetConvolution();
				createComputePipelineConvolution();
				createCommandBufferConvolution();*/
				vulkanFFTPlan.axes[0].sampleCount = 2*WIDTH;
				vulkanFFTPlan.axes[1].sampleCount = 2*HEIGHT;
				vulkanFFTPlan.axes[2].sampleCount = DEPTH;
				vulkaniFFTPlan.axes[0].sampleCount = 2 * WIDTH;
				vulkaniFFTPlan.axes[1].sampleCount = 2 * HEIGHT;
				vulkaniFFTPlan.axes[2].sampleCount = DEPTH;
				initVulkanFFTContext(&context);
				createVulkanFFT(&vulkanFFTPlan);
				commandBufferFFT = createCommandBufferFFT(&context, NULL);
				recordVulkanFFT(&vulkanFFTPlan, commandBufferFFT);
				vkEndCommandBuffer(commandBufferFFT);
				createFillZero(&vulkanFFTPlan);
				createZeroPadding(&vulkanFFTPlan);
				createConvolution(&vulkanFFTPlan);
				createConvolutionHermitian(&vulkanFFTPlan);
				createVulkaniFFT(&vulkaniFFTPlan, &vulkanFFTPlan);
				commandBufferiFFT = createCommandBufferFFT(&context, NULL);
				recordVulkanFFT(&vulkaniFFTPlan, commandBufferiFFT);
				vkEndCommandBuffer(commandBufferiFFT);
				createZeroPaddingRemove(&vulkaniFFTPlan);
				createComputeAll(&vulkanFFTPlan);

				
				

				void* mappedMemory = NULL;
				// Map the buffer memory, so that we can read from it on the CPU.
				vkMapMemory(device, bufferMemoryKernel, 0, bufferSizeKernel, 0, &mappedMemory);
				
				memcpy(mappedMemory, transformed_dipole_matrices.data(), 6 * 2 * sizeof(FFT::FFT_cpx_type) * (WIDTH + 1) * HEIGHT * DEPTH);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemoryKernel);

				VulkanFFTTransfer vulkanFFTTransfer;
				vulkanFFTTransfer.context = &context;
				vulkanFFTTransfer.size = sizeof(VulkanDimensions);
				vulkanFFTTransfer.deviceBuffer = uboDimensions;

				VulkanDimensions* ubo = (VulkanDimensions*)createVulkanFFTUpload(&vulkanFFTTransfer);
				ubo->WIDTH = WIDTH;
				ubo->HEIGHT = HEIGHT;
				ubo->DEPTH = DEPTH;
				freeVulkanFFTTransfer(&vulkanFFTTransfer);

				/*for (int i = 0; i < 6 * 2 * (WIDTH + 1) * HEIGHT; i++) {
					std::cout << i << " " << transformed_dipole_matrices[i][0] << " " << transformed_dipole_matrices[i][1] << " kernel\n";
					//std::cout << i << " " << real[i][1] << " " << imag[i][1] << " 1 fft\n";
					//std::cout << i << " " << real[i][2] << " " << imag[i][2] << " 2 fft\n";
					//std::cout << spins[16 + 32 * 64][index] << " " << real[16 + 32 * 64][index] << " " << imag[16 + 32 * 64][index] << " backward\n";
				}*/
				/*FFT::FFT_cpx_type tt[6];
				vkMapMemory(device, bufferMemoryKernel, 0, bufferSizeKernel, 0, &mappedMemory);

				memcpy(tt, mappedMemory, sizeof(FFT::FFT_cpx_type) * 6);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemoryKernel);
				std::cout << transformed_dipole_matrices[0][0] << " " << transformed_dipole_matrices[1][0] << " " << transformed_dipole_matrices[2][0] << "\n";
				std::cout << tt[0][0] << " " << tt[1][0] << " " << tt[2][0] << "\n";*/

				//std::cout << "createPipe\n";
				loadInput(regions_book, regions);
				//std::cout << "create\n";
				//std::cout << "save\n";
				// Clean up all vulkan resources.
				//cleanup();
			}


			void readDataStream(VulkanFFTPlan* vulkanFFTPlan, vectorfield& spins, vectorfield& imaginp, bool realinp){
				VulkanFFTTransfer vulkanFFTTransfer;
				vulkanFFTTransfer.context = &context;
				vulkanFFTTransfer.size = vulkanFFTPlan->bufferSize;
				vulkanFFTTransfer.deviceBuffer = vulkanFFTPlan->buffer[0];
				auto data = reinterpret_cast<std::complex<float>*>(createVulkanFFTUpload(&vulkanFFTTransfer));
				for (int i = 0; i < vulkanFFTPlan->axes[0].sampleCount * vulkanFFTPlan->axes[1].sampleCount * vulkanFFTPlan->axes[2].sampleCount; ++i) {
					float real, imag;
					for (int index=0; index<3; index++){
						real = spins[i][index];
						if (realinp == true) { imag = 0; }
						else { imag = imaginp[i][index]; }
						data[3*i+index] = std::complex<float>(real, imag);
					}
				}
				freeVulkanFFTTransfer(&vulkanFFTTransfer);
			}
			void writeDataStream(VulkanFFTPlan* vulkanFFTPlan, vectorfield& gradient, vectorfield& imagoutp) {
				VulkanFFTTransfer vulkanFFTTransfer;
				vulkanFFTTransfer.context = &context;
				vulkanFFTTransfer.size = vulkanFFTPlan->bufferSize;
				vulkanFFTTransfer.deviceBuffer = vulkanFFTPlan->buffer[vulkanFFTPlan->resultInSwapBuffer];
				auto data = reinterpret_cast<std::complex<float>*>(createVulkanFFTDownload(&vulkanFFTTransfer));
				for (uint32_t z = 0; z < vulkanFFTPlan->axes[2].sampleCount; ++z) {
					uint32_t zOffset = vulkanFFTPlan->axes[0].sampleCount * vulkanFFTPlan->axes[1].sampleCount * z;
						for (uint32_t y = 0; y < vulkanFFTPlan->axes[1].sampleCount; ++y) {
							uint32_t yzOffset = zOffset + vulkanFFTPlan->axes[0].sampleCount * y;
							for (uint32_t x = 0; x < vulkanFFTPlan->axes[0].sampleCount; ++x) {
								for (int index = 0; index < 3; index++) {
									gradient[(x + yzOffset)][index] = std::real(data[3 * (x + yzOffset) + index]);
									imagoutp[(x + yzOffset)][index] = std::imag(data[3 * (x + yzOffset) + index]);
								}
							}
					}
				}
				for (int i = 0; i < 10; i++) {
					//std::cout << i << " " << std::real(data[3 * i]) << " " << std::imag(data[3 * i]) << " load\n";
					//std::cout << i << " " << real[i][1] << " " << imag[i][1] << " ifft\n";
					//std::cout << i << " " << real[i][2] << " " << imag[i][2] << " ifft\n";
					//std::cout << spins[16 + 32 * 64][index] << " " << real[16 + 32 * 64][index] << " " << imag[16 + 32 * 64][index] << " backward\n";
				}
				freeVulkanFFTTransfer(&vulkanFFTTransfer);
				//std::cout << gradient[16+32*64][index] << " " << imagoutp[16 + 32 * 64][index] << " ss\n";
			}
			void initVulkanFFTContext(VulkanFFTContext* context) {
				vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties);
				vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &context->physicalDeviceMemoryProperties);
				
				VkFenceCreateInfo fenceCreateInfo = { };
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.flags = 0;
				vkCreateFence(context->device, &fenceCreateInfo, context->allocator, &context->fence);
				for (uint32_t i = 0; i < SUPPORTED_RADIX_LEVELS; ++i) {
					uint32_t filelength;
					// the code in comp.spv was created by running the command:
					// glslangValidator.exe -V shader.comp
					uint32_t* code = readFile(filelength, "fft.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &context->shaderModules[i]);
					delete[] code;
				}
				context->uboAlignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
			}

			void createVulkanFFT(VulkanFFTPlan* vulkanFFTPlan) {
				vulkanFFTPlan->resultInSwapBuffer = false;
				vulkanFFTPlan->bufferSize = 3*sizeof(float) * 2 * vulkanFFTPlan->axes[0].sampleCount * vulkanFFTPlan->axes[1].sampleCount * vulkanFFTPlan->axes[2].sampleCount;
				vulkanFFTPlan->buffer = (VkBuffer*)malloc(sizeof(VkBuffer) * 2);
				vulkanFFTPlan->deviceMemory = (VkDeviceMemory*)malloc(sizeof(VkDeviceMemory) * 2);
				for (uint32_t i = 0; i < 2; ++i) {
					createBufferFFT(vulkanFFTPlan->context, &vulkanFFTPlan->buffer[i], &vulkanFFTPlan->deviceMemory[i], VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, vulkanFFTPlan->bufferSize);
				}
				for (uint32_t i = 0; i < COUNT_OF(vulkanFFTPlan->axes); ++i) {
					if (vulkanFFTPlan->axes[i].sampleCount > 1)
						planVulkanFFTAxis(vulkanFFTPlan, i);

				}
			}

			void createVulkaniFFT(VulkanFFTPlan* vulkaniFFTPlan, VulkanFFTPlan* vulkanFFTPlan) {
				vulkaniFFTPlan->resultInSwapBuffer = vulkanFFTPlan->resultInSwapBuffer;
				vulkaniFFTPlan->bufferSize = 3 * sizeof(float) * 2 * vulkanFFTPlan->axes[0].sampleCount * vulkanFFTPlan->axes[1].sampleCount * vulkanFFTPlan->axes[2].sampleCount;
				//for (uint32_t i = 0; i < COUNT_OF(vulkaniFFTPlan->buffer); ++i) {
				vulkaniFFTPlan->buffer = vulkanFFTPlan->buffer;
				vulkaniFFTPlan->deviceMemory = vulkanFFTPlan->deviceMemory;
				//}
				vulkaniFFTPlan->inverse = true;
				for (uint32_t i = 0; i < COUNT_OF(vulkaniFFTPlan->axes); ++i) {
					if (vulkaniFFTPlan->axes[i].sampleCount > 1)
						planVulkanFFTAxis(vulkaniFFTPlan, i);
				}
			}
			void createBufferFFT(VulkanFFTContext* context, VkBuffer* buffer, VkDeviceMemory* deviceMemory, VkBufferUsageFlags usage, VkMemoryPropertyFlags propertyFlags, VkDeviceSize size) {
				uint32_t queueFamilyIndices[1] = {};
				VkBufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				bufferCreateInfo.queueFamilyIndexCount = COUNT_OF(queueFamilyIndices);
				bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
				bufferCreateInfo.size = size;
				bufferCreateInfo.usage = usage;
				vkCreateBuffer(context->device, &bufferCreateInfo, context->allocator, buffer) ;
				VkMemoryRequirements memoryRequirements;
				vkGetBufferMemoryRequirements(context->device, *buffer, &memoryRequirements) ;
				VkMemoryAllocateInfo memoryAllocateInfo = {};
				memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memoryAllocateInfo.allocationSize = memoryRequirements.size;
				memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);
				vkAllocateMemory(context->device, &memoryAllocateInfo, context->allocator, deviceMemory) ;
				vkBindBufferMemory(context->device, *buffer, *deviceMemory, 0) ;

			}

			void* createVulkanFFTUpload(VulkanFFTTransfer* vulkanFFTTransfer) {
				createBufferFFT(vulkanFFTTransfer->context, &vulkanFFTTransfer->hostBuffer, &vulkanFFTTransfer->deviceMemory, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkanFFTTransfer->size);
				void* map;
				vkMapMemory(device, vulkanFFTTransfer->deviceMemory, 0, vulkanFFTTransfer->size, 0, &map);
				return map;
			}

			void* createVulkanFFTDownload(VulkanFFTTransfer* vulkanFFTTransfer) {
				createBufferFFT(vulkanFFTTransfer->context, &vulkanFFTTransfer->hostBuffer, &vulkanFFTTransfer->deviceMemory, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vulkanFFTTransfer->size);
				bufferTransfer(vulkanFFTTransfer->context, vulkanFFTTransfer->hostBuffer, vulkanFFTTransfer->deviceBuffer, vulkanFFTTransfer->size);
				vulkanFFTTransfer->deviceBuffer = VK_NULL_HANDLE;
				void* map;
				vkMapMemory(vulkanFFTTransfer->context->device, vulkanFFTTransfer->deviceMemory, 0, vulkanFFTTransfer->size, 0, &map);
				return map;
			}
			VkCommandBuffer createCommandBufferFFT(VulkanFFTContext* context, VkCommandBufferUsageFlags usageFlags) {
				VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandPool = context->commandPool;
				commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				commandBufferAllocateInfo.commandBufferCount = 1;
				VkCommandBuffer commandBuffer;
				vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
				VkCommandBufferBeginInfo commandBufferBeginInfo = {};
				commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				commandBufferBeginInfo.flags = usageFlags;
				vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) ;
				return commandBuffer;
			}
			void bufferTransfer(VulkanFFTContext* context, VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size) {
				VkCommandBuffer commandBuffer = createCommandBufferFFT(context, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
				VkBufferCopy copyRegion = {};
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;
				copyRegion.size = size;
				vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion) ;
				vkEndCommandBuffer(commandBuffer);
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;
				vkQueueSubmit(context->queue, 1, &submitInfo, context->fence);
				vkWaitForFences(context->device, 1, &context->fence, VK_TRUE, 100000000000);
				vkResetFences(context->device, 1, &context->fence);
				vkFreeCommandBuffers(context->device, context->commandPool, 1, &commandBuffer);
			}
			void freeVulkanFFTTransfer(VulkanFFTTransfer* vulkanFFTTransfer) {
				if (vulkanFFTTransfer->deviceBuffer)
					bufferTransfer(vulkanFFTTransfer->context, vulkanFFTTransfer->deviceBuffer, vulkanFFTTransfer->hostBuffer, vulkanFFTTransfer->size);
				vkUnmapMemory(device, vulkanFFTTransfer->deviceMemory) ;
				vkDestroyBuffer(device, vulkanFFTTransfer->hostBuffer, vulkanFFTTransfer->context->allocator) ;
				vkFreeMemory(device, vulkanFFTTransfer->deviceMemory, vulkanFFTTransfer->context->allocator);
			}
			void recordVulkanFFT(VulkanFFTPlan* vulkanFFTPlan, VkCommandBuffer commandBuffer) {
				const uint32_t remap[3][3] = { {0, 1, 2} ,{1, 2, 0}, {2, 0, 1} };
				bool bit_swap = vulkanFFTPlan->resultInSwapBuffer;
				for (uint32_t i = 0; i < COUNT_OF(vulkanFFTPlan->axes); ++i) {

					if (vulkanFFTPlan->axes[remap[i][0]].sampleCount <= 1)
						continue;
					VulkanFFTAxis* vulkanFFTAxis = &vulkanFFTPlan->axes[remap[i][0]];
					for (uint32_t j = 0; j < vulkanFFTAxis->stageCount; ++j) {
						uint32_t dynamicOffset = vulkanFFTPlan->context->uboAlignment * j;
						vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanFFTAxis->pipelines[30 - __builtin_clz(vulkanFFTAxis->stageRadix[j])]);
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vulkanFFTAxis->pipelineLayout, 0, 1, &vulkanFFTAxis->descriptorSets[bit_swap], 1, &dynamicOffset);
						vkCmdDispatch(commandBuffer, vulkanFFTAxis->sampleCount / vulkanFFTAxis->stageRadix[j], vulkanFFTPlan->axes[remap[i][1]].sampleCount, vulkanFFTPlan->axes[remap[i][2]].sampleCount);
						bit_swap = !bit_swap;
					}
				}
				vulkanFFTPlan->resultInSwapBuffer = bit_swap;
			}

			void planVulkanFFTAxis(VulkanFFTPlan* vulkanFFTPlan, uint32_t axis) {
				VulkanFFTAxis* vulkanFFTAxis = &vulkanFFTPlan->axes[axis];
				{
					vulkanFFTAxis->stageCount = 31 - __builtin_clz(vulkanFFTAxis->sampleCount); // Logarithm of base 2
					vulkanFFTAxis->stageRadix = (uint32_t*)malloc(sizeof(uint32_t) * vulkanFFTAxis->stageCount);
					uint32_t stageSize = vulkanFFTAxis->sampleCount;
					vulkanFFTAxis->stageCount = 0;
					while (stageSize > 1) {
						uint32_t radixIndex = SUPPORTED_RADIX_LEVELS;
						do {
							assert(radixIndex > 0);
							--radixIndex;
							vulkanFFTAxis->stageRadix[vulkanFFTAxis->stageCount] = 2 << radixIndex;
						} while (stageSize % vulkanFFTAxis->stageRadix[vulkanFFTAxis->stageCount] > 0);
						stageSize /= vulkanFFTAxis->stageRadix[vulkanFFTAxis->stageCount];
						++vulkanFFTAxis->stageCount;
					}
				}

				{
					vulkanFFTAxis->uboSize = vulkanFFTPlan->context->uboAlignment * vulkanFFTAxis->stageCount;
					createBufferFFT(vulkanFFTPlan->context, &vulkanFFTAxis->ubo, &vulkanFFTAxis->uboDeviceMemory, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT, vulkanFFTAxis->uboSize);
					VulkanFFTTransfer vulkanFFTTransfer;
					vulkanFFTTransfer.context = vulkanFFTPlan->context;
					vulkanFFTTransfer.size = vulkanFFTAxis->uboSize;
					vulkanFFTTransfer.deviceBuffer = vulkanFFTAxis->ubo;
					//std::cout << vulkanFFTAxis->uboSize << " " << vulkanFFTPlan->context->uboAlignment << " " << vulkanFFTAxis->ubo << "uu\n";
					char* ubo = (char*)createVulkanFFTUpload(&vulkanFFTTransfer);
					const uint32_t remap[3][3] = { {0, 1, 2}, {1, 2, 0}, {2, 0, 1} };
					uint32_t strides[3] = { 1, vulkanFFTPlan->axes[0].sampleCount, vulkanFFTPlan->axes[0].sampleCount * vulkanFFTPlan->axes[1].sampleCount };
					uint32_t stageSize = 1;
					for (uint32_t j = 0; j < vulkanFFTAxis->stageCount; ++j) {
						VulkanFFTUBO* uboFrame = (VulkanFFTUBO*)&ubo[vulkanFFTPlan->context->uboAlignment * j];
						uboFrame->stride[0] = strides[remap[axis][0]];
						uboFrame->stride[1] = strides[remap[axis][1]];
						uboFrame->stride[2] = strides[remap[axis][2]];
						uboFrame->radixStride = vulkanFFTAxis->sampleCount / vulkanFFTAxis->stageRadix[j];
						uboFrame->stageSize = stageSize;
						uboFrame->directionFactor = (vulkanFFTPlan->inverse) ? -1.0F : 1.0F;
						uboFrame->angleFactor = uboFrame->directionFactor * (float)(3.14159265358979 / uboFrame->stageSize);
						uboFrame->normalizationFactor = (vulkanFFTPlan->inverse) ? 1.0F : 1.0F / vulkanFFTAxis->stageRadix[j];
						stageSize *= vulkanFFTAxis->stageRadix[j];
					}
					freeVulkanFFTTransfer(&vulkanFFTTransfer);
				}

				{
					VkDescriptorPoolSize descriptorPoolSize[2] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					descriptorPoolSize[0].descriptorCount = 2;
					descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[1].descriptorCount = 4;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 2;
					vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &vulkanFFTAxis->descriptorPool);
				}

				{
					const VkDescriptorType descriptorType[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
					vulkanFFTAxis->descriptorSetLayouts = (VkDescriptorSetLayout*)malloc(sizeof(VkDescriptorSetLayout) * 2);
					vulkanFFTAxis->descriptorSets = (VkDescriptorSet*)malloc(sizeof(VkDescriptorSet) * 2);
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &vulkanFFTAxis->descriptorSetLayouts[0]) ;
					vulkanFFTAxis->descriptorSetLayouts[1] = vulkanFFTAxis->descriptorSetLayouts[0];
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = vulkanFFTAxis->descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = 2;
					descriptorSetAllocateInfo.pSetLayouts = vulkanFFTAxis->descriptorSetLayouts;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, vulkanFFTAxis->descriptorSets);
					for (uint32_t j = 0; j < 2; ++j)
						for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
							VkDescriptorBufferInfo descriptorBufferInfo = { };
							VkWriteDescriptorSet writeDescriptorSet = { };
							if (i == 0) {
								descriptorBufferInfo.buffer = vulkanFFTAxis->ubo;
								//descriptorBufferInfo.offset = vulkanFFTPlan->context->uboAlignment * j;
								descriptorBufferInfo.offset = 0;
								descriptorBufferInfo.range = sizeof(VulkanFFTUBO);
							}
							else {
								//descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[1 - (vulkanFFTPlan->resultInSwapBuffer + i + j) % 2];
								descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[1 - (i + j) % 2];
								descriptorBufferInfo.offset = 0;
								descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
							}
							writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							writeDescriptorSet.dstSet = vulkanFFTAxis->descriptorSets[j];
							writeDescriptorSet.dstBinding = i;
							writeDescriptorSet.dstArrayElement = 0;
							writeDescriptorSet.descriptorType = descriptorType[i];
							writeDescriptorSet.descriptorCount = 1;
							writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
							vkUpdateDescriptorSets(vulkanFFTPlan->context->device, 1, &writeDescriptorSet, 0, NULL);
						}
				}

				{
					vulkanFFTAxis->pipelines = (VkPipeline*)malloc(sizeof(VkPipeline) * SUPPORTED_RADIX_LEVELS);
					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 2;
					pipelineLayoutCreateInfo.pSetLayouts = vulkanFFTAxis->descriptorSetLayouts;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &vulkanFFTAxis->pipelineLayout);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo[SUPPORTED_RADIX_LEVELS] = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo[SUPPORTED_RADIX_LEVELS] = { };

					for (uint32_t i = 0; i < SUPPORTED_RADIX_LEVELS; ++i) {
						pipelineShaderStageCreateInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
						pipelineShaderStageCreateInfo[i].stage = VK_SHADER_STAGE_COMPUTE_BIT;
						pipelineShaderStageCreateInfo[i].module = vulkanFFTPlan->context->shaderModules[i];
						pipelineShaderStageCreateInfo[i].pName = "main";
						//pipelineShaderStageCreateInfo[i].pSpecializationInfo = &specializationInfo;
						computePipelineCreateInfo[i].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
						computePipelineCreateInfo[i].stage = pipelineShaderStageCreateInfo[i];
						computePipelineCreateInfo[i].layout = vulkanFFTAxis->pipelineLayout;
						
					}
				
					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, SUPPORTED_RADIX_LEVELS, computePipelineCreateInfo, vulkanFFTPlan->context->allocator, vulkanFFTAxis->pipelines) ;
				}

			//	if (vulkanFFTAxis->stageCount %2 !=0)
				//	vulkanFFTPlan->resultInSwapBuffer = !vulkanFFTPlan->resultInSwapBuffer;
			}
			void run(const vectorfield& spins, vectorfield& gradient) {
				auto time0 = std::chrono::steady_clock::now();
				loadData(spins);
				//std::cout << "load\n";
				// Finally, run the recorded command buffer.
				//std::cout << sizeof(float) << " " << sizeof(Vector3) << " " << sizeof(FFT::FFT_cpx_type) << "run\n";
				//runCommandBuffer(commandBufferAll);
				//saveData(gradient);
				
				//vectorfield real = vectorfield(4*WIDTH * HEIGHT, { 1.0,1.0,1.0 });
				//vectorfield imag = vectorfield(4 * WIDTH * HEIGHT, { 1.0,1.0,1.0 });
				auto timeA = std::chrono::steady_clock::now();
				runCommandBuffer(commandBufferFillZero);
				//vulkanFFTPlan.resultInSwapBuffer = false;
				//vulkaniFFTPlan.resultInSwapBuffer = false;
				auto timeB = std::chrono::steady_clock::now();
				runCommandBuffer(commandBufferZeroPadding);
				auto timeC = std::chrono::steady_clock::now();
				runCommandBuffer(commandBufferFFT);
				auto timeD = std::chrono::steady_clock::now();
				//writeDataStream(&vulkanFFTPlan, real, imag);
				//for (int i = 0; i < 2 * WIDTH; i++) {
					//std::cout << i << " " << real[i + 1 * 128][0] << " " << real[i + 1 * 128][1] << " " << real[i + 1 * 128][2] << " 0rconvovulkan\n";
					//std::cout << i << " " << real[i + 127 * 128][0] << " " << real[i + 127 * 128][1] << " " << real[i + 127 * 128][2] << " 0rconvovulkan\n";
					//std::cout << i << " " << real[i + 80 * 128][0] << " " << real[i + 80 * 128][1] << " " << real[i + 80 * 128][2] << " 80rconvovulkan\n";
					//std::cout << i << " " << imag[i + 48 * 128][0] << " " << imag[i + 48 * 128][1] << " " << imag[i + 48 * 128][2] << " iconvovulkan\n";
					//std::cout << i << " " << real[32 + i * 128][0] << " " << real[32 + i * 128][1] << " " << real[32 + i * 128][2] << " 0rconvovulkan\n";
					//std::cout << i << " " << imag[0 + i * 128][0] << " " << imag[0 + i * 128][1] << " " << imag[0 + i * 128][2] << " iconvovulkan\n";

				//}
				//for (int i = 0; i < 2 * WIDTH; i++) {
					//std::cout << i << " " << real[i + 0 * 128][0] << " " << real[i + 0 * 128][1] << " " << real[i + 0 * 128][2] << " 0rconvovulkan\n";
					//std::cout << i << " " << real[i + 127 * 128][0] << " " << real[i + 127 * 128][1] << " " << real[i + 127 * 128][2] << " 127rconvovulkan\n";
					//std::cout << i << " " << real[i + 80 * 128][0] << " " << real[i + 80 * 128][1] << " " << real[i + 80 * 128][2] << " 80rconvovulkan\n";
					//std::cout << i << " " << imag[i + 48 * 128][0] << " " << imag[i + 48 * 128][1] << " " << imag[i + 48 * 128][2] << " iconvovulkan\n";
					//std::cout << i << " " << real[127 + i * 128][0] << " " << real[127 + i * 128][1] << " " << real[127 + i * 128][2] << " 127rconvovulkan\n";
					//std::cout << i << " " << imag[16 + i * 128][0] << " " << imag[16 + i * 128][1] << " " << imag[16 + i * 128][2] << " iconvovulkan\n";

				//}
				//bufferTransfer(&context, vulkanFFTPlan.buffer[!vulkanFFTPlan.resultInSwapBuffer], vulkanFFTPlan.buffer[vulkanFFTPlan.resultInSwapBuffer], vulkanFFTPlan.bufferSize);
				runCommandBuffer(commandBufferConvolution);
				auto timeE = std::chrono::steady_clock::now();
				runCommandBuffer(commandBufferConvolutionHermitian);
				auto timeF = std::chrono::steady_clock::now();
				//bufferTransfer(&context, vulkaniFFTPlan.buffer[0], Spins_FFT, bufferSizeSpins_FFT);
				//vulkanFFTPlan.resultInSwapBuffer = false;
				runCommandBuffer(commandBufferiFFT);
				auto timeG = std::chrono::steady_clock::now();
				runCommandBuffer(commandBufferZeroPaddingRemove);
				//writeDataStream(&vulkaniFFTPlan, real, imag);
				auto timeH = std::chrono::steady_clock::now();
				runCommandBuffer(commandBufferAll);
				auto timeK = std::chrono::steady_clock::now();
				saveData(gradient);
				auto timeL = std::chrono::steady_clock::now();
				
				/*printf("Setup: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeA - time0).count() * 0.001);
				printf("Fill0: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeB - timeA).count() * 0.001);
				printf("ZeroPad: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeC - timeB).count() * 0.001);
				printf( "FFT: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeD - timeC).count() * 0.001);
				printf( "Convo: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeE - timeD).count() * 0.001);
				printf( "ConvoHermit: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeF - timeE).count() * 0.001);
				printf("iFFT: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeG - timeF).count() * 0.001);
				printf("ZeroPadRemove: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeH - timeG).count() * 0.001);
				printf("all: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeK - timeH).count() * 0.001);
				printf("savedata: %.3f ms\n", std::chrono::duration_cast<std::chrono::microseconds>(timeL - timeK).count() * 0.001);*/
			}


			
			void loadInput(regionbook regions_book, intfield regions) {
				void* mappedMemory = NULL;
				// Map the buffer memory, so that we can read from it on the CPU.
				vkMapMemory(device, bufferMemoryRegions_Book, 0, bufferSizeRegions_Book, 0, &mappedMemory);
				Regionvalues* pmappedMemory2 = (Regionvalues*)mappedMemory;
				// Get the color data from the buffer, and cast it to bytes.
				// We save the data to a vector.
				memcpy(pmappedMemory2, regions_book.data(), sizeof(Regionvalues) * 2);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemoryRegions_Book);
				//std::cout << "2\n";
				mappedMemory = NULL;
				// Map the buffer memory, so that we can read from it on the CPU.
				vkMapMemory(device, bufferMemoryRegions, 0, bufferSizeRegions, 0, &mappedMemory);
				int* pmappedMemory3 = (int*)mappedMemory;
				// Get the color data from the buffer, and cast it to bytes.
				// We save the data to a vector.
				memcpy(pmappedMemory3, regions.data(), sizeof(int) * WIDTH * HEIGHT * DEPTH);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemoryRegions);
				//std::cout << "3\n";
				
			}
			void loadData(const vectorfield& spins) {
				void* mappedMemory = NULL;
				// Map the buffer memory, so that we can read from it on the CPU.
				vkMapMemory(device, bufferMemorySpins, 0, bufferSizeSpins, 0, &mappedMemory);
				Vector3* pmappedMemory0 = (Vector3*)mappedMemory;
				// Get the color data from the buffer, and cast it to bytes.
				// We save the data to a vector.
				memcpy(pmappedMemory0, spins.data(), 3 * sizeof(float) * WIDTH * HEIGHT * DEPTH);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemorySpins);
				//std::cout << "0\n";
				/*mappedMemory = NULL;
				// Map the buffer memory, so that we can read from it on the CPU.
				vkMapMemory(device, bufferMemoryGradient, 0, bufferSizeGradient, 0, &mappedMemory);
				Vector3* pmappedMemory1 = (Vector3*)mappedMemory;
				// Get the color data from the buffer, and cast it to bytes.
				// We save the data to a vector.
				memcpy(pmappedMemory1, gradient.data(), 3 * sizeof(float) * WIDTH * HEIGHT * DEPTH);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemoryGradient);*/
				
			}
			void saveData(vectorfield& gradient) {
				void* mappedMemory = NULL;
				// Map the buffer memory, so that we can read from it on the CPU.
				vkMapMemory(device, bufferMemoryGradient, 0, bufferSizeGradient, 0, &mappedMemory);
				Vector3* pmappedMemory1 = (Vector3*)mappedMemory;
				// Get the color data from the buffer, and cast it to bytes.
				// We save the data to a vector.
				memcpy(gradient.data(), pmappedMemory1, sizeof(Vector3) * WIDTH * HEIGHT * DEPTH);
				// Done reading, so unmap.
				vkUnmapMemory(device, bufferMemoryGradient);
			}

			static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallbackFn(
				VkDebugReportFlagsEXT                       flags,
				VkDebugReportObjectTypeEXT                  objectType,
				uint64_t                                    object,
				size_t                                      location,
				int32_t                                     messageCode,
				const char* pLayerPrefix,
				const char* pMessage,
				void* pUserData) {

				printf("Debug Report: %s: %s\n", pLayerPrefix, pMessage);

				return VK_FALSE;
			}

			void createInstance() {
				std::vector<const char*> enabledExtensions;
				
					/*f
					We get all supported layers with vkEnumerateInstanceLayerProperties.
					*/
					uint32_t layerCount;
					vkEnumerateInstanceLayerProperties(&layerCount, NULL);

					std::vector<VkLayerProperties> layerProperties(layerCount);
					vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

					/*
					And then we simply check if VK_LAYER_LUNARG_standard_validation is among the supported layers.
					*/
					bool foundLayer = false;
					for (VkLayerProperties prop : layerProperties) {

						if (strcmp("VK_LAYER_LUNARG_standard_validation", prop.layerName) == 0) {
							foundLayer = true;
							break;
						}

					}

					if (!foundLayer) {
						throw std::runtime_error("Layer VK_LAYER_LUNARG_standard_validation not supported\n");
					}
					enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation"); // Alright, we can use this layer.

					/*
					We need to enable an extension named VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
					in order to be able to print the warnings emitted by the validation layer.

					So again, we just check if the extension is among the supported extensions.
					*/

					uint32_t extensionCount;

					vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
					std::vector<VkExtensionProperties> extensionProperties(extensionCount);
					vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionProperties.data());

					bool foundExtension = false;
					for (VkExtensionProperties prop : extensionProperties) {
						if (strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, prop.extensionName) == 0) {
							foundExtension = true;
							break;
						}

					}

					if (!foundExtension) {
						throw std::runtime_error("Extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not supported\n");
					}
					enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
				


				/*
				Next, we actually create the instance.

				*/

				/*
				Contains application info. This is actually not that important.
				The only real important field is apiVersion.
				*/
				VkApplicationInfo applicationInfo = {};
				applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				applicationInfo.pApplicationName = "Hello world app";
				applicationInfo.applicationVersion = 0;
				applicationInfo.pEngineName = "awesomeengine";
				applicationInfo.engineVersion = 0;
				applicationInfo.apiVersion = VK_API_VERSION_1_0;;

				VkInstanceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				createInfo.flags = 0;
				createInfo.pApplicationInfo = &applicationInfo;

				// Give our desired layers and extensions to vulkan.
				createInfo.enabledLayerCount = enabledLayers.size();
				createInfo.ppEnabledLayerNames = enabledLayers.data();
				createInfo.enabledExtensionCount = enabledExtensions.size();
				createInfo.ppEnabledExtensionNames = enabledExtensions.data();

				/*
				Actually create the instance.
				Having created the instance, we can actually start using vulkan.
				*/
				VK_CHECK_RESULT(vkCreateInstance(
					&createInfo,
					NULL,
					&instance));

				/*
				Register a callback function for the extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME, so that warnings emitted from the validation
				layer are actually printed.
				*/
			
					VkDebugReportCallbackCreateInfoEXT createInfoD = {};
					createInfoD.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
					createInfoD.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
					createInfoD.pfnCallback = &debugReportCallbackFn;

					// We have to explicitly load this function.
					auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
					if (vkCreateDebugReportCallbackEXT == nullptr) {
						throw std::runtime_error("Could not load vkCreateDebugReportCallbackEXT");
					}

					// Create and register callback.
					VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(instance, &createInfoD, NULL, &debugReportCallback));
				

			}

			void findPhysicalDevice() {
				/*
				In this function, we find a physical device that can be used with Vulkan.
				*/

				/*
				So, first we will list all physical devices on the system with vkEnumeratePhysicalDevices .
				*/
				uint32_t deviceCount;
				vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
				if (deviceCount == 0) {
					throw std::runtime_error("could not find a device with vulkan support");
				}

				std::vector<VkPhysicalDevice> devices(deviceCount);
				vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

				/*
				Next, we choose a device that can be used for our purposes.

				With VkPhysicalDeviceFeatures(), we can retrieve a fine-grained list of physical features supported by the device.
				However, in this demo, we are simply launching a simple compute shader, and there are no
				special physical features demanded for this task.

				With VkPhysicalDeviceProperties(), we can obtain a list of physical device properties. Most importantly,
				we obtain a list of physical device limitations. For this application, we launch a compute shader,
				and the maximum size of the workgroups and total number of compute shader invocations is limited by the physical device,
				and we should ensure that the limitations named maxComputeWorkGroupCount, maxComputeWorkGroupInvocations and
				maxComputeWorkGroupSize are not exceeded by our application.  Moreover, we are using a storage buffer in the compute shader,
				and we should ensure that it is not larger than the device can handle, by checking the limitation maxStorageBufferRange.

				However, in our application, the workgroup size and total number of shader invocations is relatively small, and the storage buffer is
				not that large, and thus a vast majority of devices will be able to handle it. This can be verified by looking at some devices at_
				http://vulkan.gpuinfo.org/

				Therefore, to keep things simple and clean, we will not perform any such checks here, and just pick the first physical
				device in the list. But in a real and serious application, those limitations should certainly be taken into account.

				*/

				for (VkPhysicalDevice device : devices) {
					if (true) { // As above stated, we do no feature checks, so just accept.
						physicalDevice = device;
						context.physicalDevice = device;
						break;
					}
				}
			}

			// Returns the index of a queue family that supports compute operations. 
			uint32_t getComputeQueueFamilyIndex() {
				uint32_t queueFamilyCount;

				vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

				// Retrieve all queue families.
				std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

				// Now find a family that supports compute.
				uint32_t i = 0;
				for (; i < queueFamilies.size(); ++i) {
					VkQueueFamilyProperties props = queueFamilies[i];

					if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
						// found a queue with compute. We're done!
						break;
					}
				}

				if (i == queueFamilies.size()) {
					throw std::runtime_error("could not find a queue family that supports operations");
				}

				return i;
			}

			void createDevice() {
				/*
				We create the logical device in this function.
				*/

				/*
				When creating the device, we also specify what queues it has.
				*/
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueFamilyIndex = getComputeQueueFamilyIndex(); // find queue family with compute capability.
				queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
				queueCreateInfo.queueCount = 1; // create one queue in this family. We don't need more.
				float queuePriorities = 1.0;  // we only have one queue, so this is not that imporant. 
				queueCreateInfo.pQueuePriorities = &queuePriorities;

				/*
				Now we create the logical device. The logical device allows us to interact with the physical
				device.
				*/
				VkDeviceCreateInfo deviceCreateInfo = {};

				// Specify any desired device features here. We do not need any for this application, though.
				VkPhysicalDeviceFeatures deviceFeatures = {};

				deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				deviceCreateInfo.enabledLayerCount = enabledLayers.size();  // need to specify validation layers here as well.
				deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
				deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo; // when creating the logical device, we also specify what queues it has.
				deviceCreateInfo.queueCreateInfoCount = 1;
				deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

				VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device)); // create logical device.
				context.device = device;
				
				// Get a handle to the only member of the queue family.
				vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
				context.queue = queue;
			}

			// find memory type with desired properties.
			uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties) {
				VkPhysicalDeviceMemoryProperties memoryProperties;

				vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

				/*
				How does this search work?
				See the documentation of VkPhysicalDeviceMemoryProperties for a detailed description.
				*/
				for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
					if ((memoryTypeBits & (1 << i)) &&
						((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
						return i;
				}
				return -1;
			}

			void createBuffer(VkBuffer* buffer, VkDeviceMemory* bufferMemory, uint32_t bufferSize) {
				/*
				We will now create a buffer. We will render the mandelbrot set into this buffer
				in a computer shade later.
				*/

				VkBufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateInfo.size = bufferSize; // buffer size in bytes. 
				bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT| VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // buffer is used as a storage buffer.
				bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time. 

				VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, NULL, buffer)); // create buffer.

				/*
				But the buffer doesn't allocate memory for itself, so we must do that manually.
				*/

				/*
				First, we find the memory requirements for the buffer.
				*/
				VkMemoryRequirements memoryRequirements;
				vkGetBufferMemoryRequirements(device, buffer[0], &memoryRequirements);

				/*
				Now use obtained memory requirements info to allocate the memory for the buffer.
				*/
				VkMemoryAllocateInfo allocateInfo = {};
				allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocateInfo.allocationSize = memoryRequirements.size; // specify required memory.
				/*
				There are several types of memory that can be allocated, and we must choose a memory type that:

				1) Satisfies the memory requirements(memoryRequirements.memoryTypeBits).
				2) Satifies our own usage requirements. We want to be able to read the buffer memory from the GPU to the CPU
				   with vkMapMemory, so we set VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT.
				Also, by setting VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memory written by the device(GPU) will be easily
				visible to the host(CPU), without having to call any extra flushing commands. So mainly for convenience, we set
				this flag.
				*/
				allocateInfo.memoryTypeIndex = findMemoryType(
					memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				VK_CHECK_RESULT(vkAllocateMemory(device, &allocateInfo, NULL, bufferMemory)); // allocate memory on device.

				// Now associate that allocated memory with the buffer. With that, the buffer is backed by actual memory. 
				VK_CHECK_RESULT(vkBindBufferMemory(device, buffer[0], bufferMemory[0], 0));
			}
			void createBufferUBO(VkBuffer* buffer, VkDeviceMemory* bufferMemory, uint32_t bufferSize) {

				VkBufferCreateInfo bufferCreateInfo = {};
				bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateInfo.size = bufferSize; // buffer size in bytes. 
				bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT; // buffer is used as a storage buffer.
				bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // buffer is exclusive to a single queue family at a time. 

				VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, NULL, buffer)); // create buffer.
				VkMemoryRequirements memoryRequirements;
				vkGetBufferMemoryRequirements(device, buffer[0], &memoryRequirements);
				VkMemoryAllocateInfo allocateInfo = {};
				allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocateInfo.allocationSize = memoryRequirements.size; // specify required memory.
				allocateInfo.memoryTypeIndex = findMemoryType(
					memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				VK_CHECK_RESULT(vkAllocateMemory(device, &allocateInfo, NULL, bufferMemory)); // allocate memory on device.

				VK_CHECK_RESULT(vkBindBufferMemory(device, buffer[0], bufferMemory[0], 0));
			}
			void createDescriptorSetLayout() {
				/*
				Here we specify a descriptor set layout. This allows us to bind our descriptors to
				resources in the shader.

				*/

				/*
				Here we specify a binding of type VK_DESCRIPTOR_TYPE_STORAGE_BUFFER to the binding point
				0. This binds to

				  layout(std140, binding = 0) buffer buf

				in the compute shader.
				*/
				std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
					// Binding 0 : Particle position storage buffer
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						0),

					// Binding 1 : Particle position storage buffer
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						1),
					// Binding 2 : Particle position storage buffer
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						2),
					// Binding 3 : Particle position storage buffer
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						3),
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						4),
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						5),
				};

				VkDescriptorSetLayoutCreateInfo descriptorLayout =
					vks::initializers::descriptorSetLayoutCreateInfo(
						setLayoutBindings.data(),
						static_cast<uint32_t>(setLayoutBindings.size()));

				VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));


			}
			void createDescriptorSetLayoutConvolution() {
				/*
				Here we specify a descriptor set layout. This allows us to bind our descriptors to
				resources in the shader.

				*/

				/*
				Here we specify a binding of type VK_DESCRIPTOR_TYPE_STORAGE_BUFFER to the binding point
				0. This binds to

				  layout(std140, binding = 0) buffer buf

				in the compute shader.
				*/
				std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						0),

					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						1),


					vks::initializers::descriptorSetLayoutBinding(
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						VK_SHADER_STAGE_COMPUTE_BIT,
						2)					
				};

				VkDescriptorSetLayoutCreateInfo descriptorLayout =
					vks::initializers::descriptorSetLayoutCreateInfo(
						setLayoutBindings.data(),
						static_cast<uint32_t>(setLayoutBindings.size()));

				VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayoutConvolution));


			}
			
			void createDescriptorSet() {
				/*
				So we will allocate a descriptor set here.
				But we need to first create a descriptor pool to do that.
				*/

				/*
				Our descriptor pool can only allocate a single storage buffer.
				*/
				std::vector<VkDescriptorPoolSize> descriptorPoolSize =
				{
					vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6),
				};

				VkDescriptorPoolCreateInfo descriptorPoolInfo =
					vks::initializers::descriptorPoolCreateInfo(
						static_cast<uint32_t>(descriptorPoolSize.size()),
						descriptorPoolSize.data(),
						1);

				// create descriptor pool.
				VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, &descriptorPool));
				//std::cout << "createPool\n";
				/*
				With the pool allocated, we can now allocate the descriptor set.
				*/
				VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
				descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocateInfo.descriptorPool = descriptorPool; // pool to allocate from.
				descriptorSetAllocateInfo.descriptorSetCount = 1; // allocate a single descriptor set.
				descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

				// allocate descriptor set.
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));
				//std::cout << "allocate\n";
				/*
				Next, we need to connect our actual storage buffer with the descrptor.
				We use vkUpdateDescriptorSets() to update the descriptor set.
				*/

				// Specify the buffer to bind to the descriptor.
				VkDescriptorBufferInfo descriptorBufferInfoSpins = {};
				descriptorBufferInfoSpins.buffer = bufferSpins;
				descriptorBufferInfoSpins.offset = 0;
				descriptorBufferInfoSpins.range = bufferSizeSpins;
				VkDescriptorBufferInfo descriptorBufferInfoGradient = {};
				descriptorBufferInfoGradient.buffer = bufferGradient;
				descriptorBufferInfoGradient.offset = 0;
				descriptorBufferInfoGradient.range = bufferSizeGradient;
				VkDescriptorBufferInfo descriptorBufferInfoRegions_Book = {};
				descriptorBufferInfoRegions_Book.buffer = bufferRegions_Book;
				descriptorBufferInfoRegions_Book.offset = 0;
				descriptorBufferInfoRegions_Book.range = bufferSizeRegions_Book;
				VkDescriptorBufferInfo descriptorBufferInfoRegions = {};
				descriptorBufferInfoRegions.buffer = bufferRegions;
				descriptorBufferInfoRegions.offset = 0;
				descriptorBufferInfoRegions.range = bufferSizeRegions;
				VkDescriptorBufferInfo descriptorBufferInfoSpins_FFT = {};
				descriptorBufferInfoSpins_FFT.buffer = Spins_FFT;
				descriptorBufferInfoSpins_FFT.offset = 0;
				descriptorBufferInfoSpins_FFT.range = bufferSizeSpins_FFT;
				VkDescriptorBufferInfo descriptorBufferInfoKernel = {};
				descriptorBufferInfoKernel.buffer = kernel;
				descriptorBufferInfoKernel.offset = 0;
				descriptorBufferInfoKernel.range = bufferSizeKernel;
				std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
				{

					// Binding 0 : Storage buffer
					vks::initializers::writeDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						0,
						&descriptorBufferInfoSpins),
					// Binding 1 : Uniform buffer
					vks::initializers::writeDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						1,
						&descriptorBufferInfoGradient),
					// Binding 0 : Storage buffer
					vks::initializers::writeDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						2,
						&descriptorBufferInfoRegions_Book),
					// Binding 0 : Storage buffer

					vks::initializers::writeDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						3,
						&descriptorBufferInfoRegions),
				
					vks::initializers::writeDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						4,
						&descriptorBufferInfoKernel),
					vks::initializers::writeDescriptorSet(
						descriptorSet,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						5,
						&descriptorBufferInfoSpins_FFT)
				};
				////std::cout << computeWriteDescriptorSets.descriptorCount  <<"\n";
				vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, NULL);
				//std::cout << "write2\n";
			}
			void createDescriptorSetConvolution() {
				/*
				So we will allocate a descriptor set here.
				But we need to first create a descriptor pool to do that.
				*/

				/*
				Our descriptor pool can only allocate a single storage buffer.
				*/
				std::vector<VkDescriptorPoolSize> descriptorPoolSize =
				{
					vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2),
				};

				VkDescriptorPoolCreateInfo descriptorPoolInfo =
					vks::initializers::descriptorPoolCreateInfo(
						static_cast<uint32_t>(descriptorPoolSize.size()),
						descriptorPoolSize.data(),
						1);

				// create descriptor pool.
				VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, &descriptorPoolConvolution));
				//std::cout << "createPool\n";
				/*
				With the pool allocated, we can now allocate the descriptor set.
				*/
				VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
				descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocateInfo.descriptorPool = descriptorPoolConvolution; // pool to allocate from.
				descriptorSetAllocateInfo.descriptorSetCount = 1; // allocate a single descriptor set.
				descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayoutConvolution;

				// allocate descriptor set.
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSetConvolution));
				//std::cout << "allocate\n";
				/*
				Next, we need to connect our actual storage buffer with the descrptor.
				We use vkUpdateDescriptorSets() to update the descriptor set.
				*/

				// Specify the buffer to bind to the descriptor.
				VkDescriptorBufferInfo descriptorBufferInfoSpins_FFT = {};
				descriptorBufferInfoSpins_FFT.buffer = Spins_FFT;
				descriptorBufferInfoSpins_FFT.offset = 0;
				descriptorBufferInfoSpins_FFT.range = bufferSizeSpins_FFT;
				VkDescriptorBufferInfo descriptorBufferInfoKernel = {};
				descriptorBufferInfoKernel.buffer = kernel;
				descriptorBufferInfoKernel.offset = 0;
				descriptorBufferInfoKernel.range = bufferSizeKernel;
				std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
				{

					vks::initializers::writeDescriptorSet(
						descriptorSetConvolution,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						0,
						&descriptorBufferInfoKernel),
					vks::initializers::writeDescriptorSet(
						descriptorSetConvolution,
						VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						1,
						&descriptorBufferInfoSpins_FFT)
				};
				////std::cout << computeWriteDescriptorSets.descriptorCount  <<"\n";
				vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, NULL);
				//std::cout << "write2\n";
			}
			// Read file into array of bytes, and cast to uint32_t*, then return.
			// The data has been padded, so that it fits into an array uint32_t.
			uint32_t* readFile(uint32_t& length, const char* filename) {

				FILE* fp = fopen(filename, "rb");
				if (fp == NULL) {
					printf("Could not find or open file: %s\n", filename);
				}

				// get file size.
				fseek(fp, 0, SEEK_END);
				long filesize = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				long filesizepadded = long(ceil(filesize / 4.0)) * 4;

				// read file contents.
				char* str = new char[filesizepadded];
				fread(str, filesize, sizeof(char), fp);
				fclose(fp);

				// data padding. 
				for (int i = filesize; i < filesizepadded; i++) {
					str[i] = 0;
				}

				length = filesizepadded;
				return (uint32_t*)str;
			}

			void createComputePipeline() {
				/*
				We create a compute pipeline here.
				*/

				/*
				Create a shader module. A shader module basically just encapsulates some shader code.
				*/

				uint32_t filelength;
				// the code in comp.spv was created by running the command:
				// glslangValidator.exe -V shader.comp
				uint32_t* code = readFile(filelength, "comp2.spv");
				VkShaderModuleCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.pCode = code;
				createInfo.codeSize = filelength;

				VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModule));

				delete[] code;

				/*
				Now let us actually create the compute pipeline.
				A compute pipeline is very simple compared to a graphics pipeline.
				It only consists of a single stage with a compute shader.

				So first we specify the compute shader stage, and it's entry point(main).
				*/
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				shaderStageCreateInfo.module = computeShaderModule;
				shaderStageCreateInfo.pName = "main";
				// Prepare specialization info block for the shader stage
				struct SpecializationData {
					// Sets the lighting model used in the fragment "uber" shader
					uint32_t forceType;
				} specializationData;
				std::array<VkSpecializationMapEntry, 1> specializationMapEntries;
				specializationMapEntries[0].constantID = 0;
				specializationMapEntries[0].size = sizeof(specializationData.forceType);
				specializationMapEntries[0].offset = 0;
				
				VkSpecializationInfo specializationInfo{};
				specializationInfo.dataSize = sizeof(specializationData);
				specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
				specializationInfo.pMapEntries = specializationMapEntries.data();
				specializationInfo.pData = &specializationData;
				//shaderStageCreateInfo.pSpecializationInfo = &specializationInfo;
				/*
				The pipeline layout allows the pipeline to access descriptor sets.
				So we just specify the descriptor set layout we created earlier.
				*/
				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
				pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCreateInfo.setLayoutCount = 1;
				pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
				VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &pipelineLayout));

				VkComputePipelineCreateInfo pipelineCreateInfo = {};
				pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
				pipelineCreateInfo.stage = shaderStageCreateInfo;
				pipelineCreateInfo.layout = pipelineLayout;
			
				/*
				Now, we finally create the compute pipeline.
				*/
				VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
				pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
				VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
				//VkPipelineCache pipelineCache;
				specializationData.forceType = 0;
				VK_CHECK_RESULT(vkCreateComputePipelines(
					device, pipelineCache,
					1, &pipelineCreateInfo,
					NULL, &pipeline_all));
				specializationData.forceType = 2;
				//shaderStageCreateInfo.pSpecializationInfo = &specializationInfo;
				//pipelineCreateInfo.stage = shaderStageCreateInfo;

			}
			void createComputePipelineConvolution() {
				/*
				We create a compute pipeline here.
				*/

				/*
				Create a shader module. A shader module basically just encapsulates some shader code.
				*/

				uint32_t filelength;
				// the code in comp.spv was created by running the command:
				// glslangValidator.exe -V shader.comp
				uint32_t* code = readFile(filelength, "convolution.spv");
				VkShaderModuleCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.pCode = code;
				createInfo.codeSize = filelength;

				VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModuleConvolution));

				delete[] code;

				/*
				Now let us actually create the compute pipeline.
				A compute pipeline is very simple compared to a graphics pipeline.
				It only consists of a single stage with a compute shader.

				So first we specify the compute shader stage, and it's entry point(main).
				*/
				VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
				shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				shaderStageCreateInfo.module = computeShaderModuleConvolution;
				shaderStageCreateInfo.pName = "main";
				//shaderStageCreateInfo.pSpecializationInfo = &specializationInfo;
				/*
				The pipeline layout allows the pipeline to access descriptor sets.
				So we just specify the descriptor set layout we created earlier.
				*/
				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
				pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCreateInfo.setLayoutCount = 1;
				pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutConvolution;
				VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &pipelineLayoutConvolution));

				VkComputePipelineCreateInfo pipelineCreateInfo = {};
				pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
				pipelineCreateInfo.stage = shaderStageCreateInfo;
				pipelineCreateInfo.layout = pipelineLayoutConvolution;

				/*
				Now, we finally create the compute pipeline.
				
				VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
				pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
				VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));*/
				//VkPipelineCache pipelineCache;
				VK_CHECK_RESULT(vkCreateComputePipelines(
					device, NULL,
					1, &pipelineCreateInfo,
					NULL, &pipeline_convolution));

			}
			//void createCommandBuffer(VulkanFFTPlan* vulkanFFTPlan) {
			void createCommandBuffer() {
				/*
				We are getting closer to the end. In order to send commands to the device(GPU),
				we must first record commands into a command buffer.
				To allocate a command buffer, we must first create a command pool. So let us do that.
				*/
				VkCommandPoolCreateInfo commandPoolCreateInfo = {};
				commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolCreateInfo.flags = 0;
				// the queue family of this command pool. All command buffers allocated from this command pool,
				// must be submitted to queues of this family ONLY. 
				commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
				VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, NULL, &commandPool));
				context.commandPool= commandPool;
				/*
				Now allocate a command buffer from the command pool.
				*/
				VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandPool = commandPool; // specify the command pool to allocate from. 
				// if the command buffer is primary, it can be directly submitted to queues. 
				// A secondary buffer has to be called from some primary command buffer, and cannot be directly 
				// submitted to a queue. To keep things simple, we use a primary command buffer. 
				commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				commandBufferAllocateInfo.commandBufferCount = 1; // allocate a single command buffer. 
				VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferAll)); // allocate command buffer.

				/*
				Now we shall start recording commands into the newly allocated command buffer.
				*/
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // the buffer is only submitted and used once in this application.
				VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferAll, &beginInfo)); // start recording commands.

				/*
				We need to bind a pipeline, AND a descriptor set before we dispatch.

				The validation layer will NOT give warnings if you forget these, so be very careful not to forget them.
				*/
				//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
				vkCmdBindDescriptorSets(commandBufferAll, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
				vkCmdBindPipeline(commandBufferAll, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_all);
				vkCmdDispatch(commandBufferAll, (uint32_t)ceil(WIDTH / float(WORKGROUP_SIZE)), (uint32_t)ceil(HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);
				
				
				//recordVulkanFFT(vulkanFFTPlan, commandBufferAll);
				//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_ddi);
				/*
				Calling vkCmdDispatch basically starts the compute pipeline, and executes the compute shader.
				The number of workgroups is specified in the arguments.
				If you are already familiar with compute shaders from OpenGL, this should be nothing new to you.
				*/
				//vkCmdDispatch(commandBufferAll, (uint32_t)ceil(WIDTH / float(WORKGROUP_SIZE)), (uint32_t)ceil(HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);

				VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferAll)); // end recording commands.
			}
			void createCommandBufferConvolution() {
				
				/*
				Now allocate a command buffer from the command pool.
				*/
				VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
				commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocateInfo.commandPool = commandPool; // specify the command pool to allocate from. 
				// if the command buffer is primary, it can be directly submitted to queues. 
				// A secondary buffer has to be called from some primary command buffer, and cannot be directly 
				// submitted to a queue. To keep things simple, we use a primary command buffer. 
				commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				commandBufferAllocateInfo.commandBufferCount = 1; // allocate a single command buffer. 
				VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferConvolution)); // allocate command buffer.

				/*
				Now we shall start recording commands into the newly allocated command buffer.
				*/
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				//beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // the buffer is only submitted and used once in this application.
				VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferConvolution, &beginInfo)); // start recording commands.

				/*
				We need to bind a pipeline, AND a descriptor set before we dispatch.

				The validation layer will NOT give warnings if you forget these, so be very careful not to forget them.
				*/
				//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
				vkCmdBindDescriptorSets(commandBufferConvolution, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayoutConvolution, 0, 1, &descriptorSetConvolution, 0, NULL);
				vkCmdBindPipeline(commandBufferConvolution, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_convolution);
				vkCmdDispatch(commandBufferConvolution, (uint32_t)ceil((WIDTH+1) / float(WORKGROUP_SIZE)), (uint32_t)ceil(2 * HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);
				//vkCmdDispatch(commandBufferConvolution, (uint32_t)ceil(2 * WIDTH), (uint32_t)ceil(2 * HEIGHT ), (uint32_t)DEPTH);


				//recordVulkanFFT(vulkanFFTPlan, commandBufferAll);
				//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_ddi);
				/*
				Calling vkCmdDispatch basically starts the compute pipeline, and executes the compute shader.
				The number of workgroups is specified in the arguments.
				If you are already familiar with compute shaders from OpenGL, this should be nothing new to you.
				*/
				//vkCmdDispatch(commandBufferAll, (uint32_t)ceil(WIDTH / float(WORKGROUP_SIZE)), (uint32_t)ceil(HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);

				VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferConvolution)); // end recording commands.
			}

			void createComputeAll(VulkanFFTPlan* vulkanFFTPlan) {
				{
					VkDescriptorPoolSize descriptorPoolSize[2] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorPoolSize[0].descriptorCount = 1;
					descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[1].descriptorCount = 4;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 1;
					vkCreateDescriptorPool(vulkanFFTPlan->context->device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &descriptorPool);
				}

				{
					const VkDescriptorType descriptorType[] = {  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &descriptorSetLayout);
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, &descriptorSet);
					for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
						VkDescriptorBufferInfo descriptorBufferInfo = { };					
						if (i == 0) {
							descriptorBufferInfo.buffer = bufferSpins;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeSpins;
						}
						if (i == 1) {
							descriptorBufferInfo.buffer = bufferGradient;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeGradient;
						}
						if (i == 2) {
							descriptorBufferInfo.buffer = bufferRegions_Book;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeRegions_Book;
						}
						if (i == 3) {
							descriptorBufferInfo.buffer = bufferRegions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeRegions;
						}
						if (i == 4) {
							descriptorBufferInfo.buffer = uboDimensions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = sizeof(VulkanDimensions);
						}
						VkWriteDescriptorSet writeDescriptorSet = { };
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = descriptorSet;
						writeDescriptorSet.dstBinding = i;
						writeDescriptorSet.dstArrayElement = 0;
						writeDescriptorSet.descriptorType = descriptorType[i];
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
						vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
					}
				}

				{
					uint32_t filelength;
					uint32_t* code = readFile(filelength, "all.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModule);
					delete[] code;

					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 1;
					pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &pipelineLayout);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo = { };

					pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					pipelineShaderStageCreateInfo.module = computeShaderModule;
					pipelineShaderStageCreateInfo.pName = "main";
					computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
					computePipelineCreateInfo.layout = pipelineLayout;


					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, vulkanFFTPlan->context->allocator, &pipeline_all);
				}
				{
					VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
					commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocateInfo.commandPool = commandPool;
					commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocateInfo.commandBufferCount = 1;
					VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferAll)); // allocate command buffer.

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferAll, &beginInfo)); // start recording commands.
					vkCmdBindDescriptorSets(commandBufferAll, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
					vkCmdBindPipeline(commandBufferAll, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_all);
					vkCmdDispatch(commandBufferAll, (uint32_t)ceil((WIDTH) / float(WORKGROUP_SIZE)), (uint32_t)ceil(HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);
					VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferAll)); // end recording commands.
				}
			}
			void createConvolution(VulkanFFTPlan* vulkanFFTPlan)
 {
				{
					VkDescriptorPoolSize descriptorPoolSize[2] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorPoolSize[0].descriptorCount = 1;
					descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[1].descriptorCount = 2;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 1;
					vkCreateDescriptorPool(vulkanFFTPlan->context->device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &descriptorPoolConvolution);
				}

				{
					const VkDescriptorType descriptorType[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &descriptorSetLayoutConvolution);
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = descriptorPoolConvolution;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayoutConvolution;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, &descriptorSetConvolution);
					for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
						VkDescriptorBufferInfo descriptorBufferInfo = { };
						if (i == 0) {
							descriptorBufferInfo.buffer = uboDimensions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = sizeof(VulkanDimensions);
						}
						if (i == 1) {
							descriptorBufferInfo.buffer = kernel;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeKernel;
						}
						if (i == 2) {
							descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[vulkanFFTPlan->resultInSwapBuffer];
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
						}
						VkWriteDescriptorSet writeDescriptorSet = { };
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = descriptorSetConvolution;
						writeDescriptorSet.dstBinding = i;
						writeDescriptorSet.dstArrayElement = 0;
						writeDescriptorSet.descriptorType = descriptorType[i];
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
						vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
					}
				}

				{
					uint32_t filelength;
					uint32_t* code = readFile(filelength, "convolution.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModuleConvolution);
					delete[] code;

					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 1;
					pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutConvolution;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &pipelineLayoutConvolution);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo = { };

					pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					pipelineShaderStageCreateInfo.module = computeShaderModuleConvolution;
					pipelineShaderStageCreateInfo.pName = "main";
					computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
					computePipelineCreateInfo.layout = pipelineLayoutConvolution;


					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, vulkanFFTPlan->context->allocator, &pipeline_convolution);
				}
				{
					VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
					commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocateInfo.commandPool = commandPool;
					commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocateInfo.commandBufferCount = 1;
					VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferConvolution)); // allocate command buffer.

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferConvolution, &beginInfo)); // start recording commands.
					vkCmdBindDescriptorSets(commandBufferConvolution, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayoutConvolution, 0, 1, &descriptorSetConvolution, 0, NULL);
					vkCmdBindPipeline(commandBufferConvolution, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_convolution);
					vkCmdDispatch(commandBufferConvolution, (uint32_t)ceil((WIDTH+1) / float(WORKGROUP_SIZE)), (uint32_t)ceil(2 * HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);
					VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferConvolution)); // end recording commands.
				}
			}
			void createConvolutionHermitian(VulkanFFTPlan* vulkanFFTPlan) {
				{
					VkDescriptorPoolSize descriptorPoolSize[2] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorPoolSize[0].descriptorCount = 1;
					descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[1].descriptorCount = 1;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 1;
					vkCreateDescriptorPool(vulkanFFTPlan->context->device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &descriptorPoolConvolutionHermitian);
				}

				{
					const VkDescriptorType descriptorType[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &descriptorSetLayoutConvolutionHermitian);
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = descriptorPoolConvolutionHermitian;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayoutConvolutionHermitian;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, &descriptorSetConvolutionHermitian);
					for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
						VkDescriptorBufferInfo descriptorBufferInfo = { };
						if (i == 0) {
							descriptorBufferInfo.buffer = uboDimensions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = sizeof(VulkanDimensions);
						}
						if (i == 1) {
							descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[vulkanFFTPlan->resultInSwapBuffer];
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
						}
						VkWriteDescriptorSet writeDescriptorSet = { };
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = descriptorSetConvolutionHermitian;
						writeDescriptorSet.dstBinding = i;
						writeDescriptorSet.dstArrayElement = 0;
						writeDescriptorSet.descriptorType = descriptorType[i];
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
						vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
					}
				}

				{
					uint32_t filelength;
					uint32_t* code = readFile(filelength, "convolutionhermitian.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModuleConvolutionHermitian);
					delete[] code;

					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 1;
					pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutConvolutionHermitian;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &pipelineLayoutConvolutionHermitian);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo = { };

					pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					pipelineShaderStageCreateInfo.module = computeShaderModuleConvolutionHermitian;
					pipelineShaderStageCreateInfo.pName = "main";
					computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
					computePipelineCreateInfo.layout = pipelineLayoutConvolutionHermitian;


					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, vulkanFFTPlan->context->allocator, &pipeline_convolutionhermitian);
				}
				{
					VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
					commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocateInfo.commandPool = commandPool;
					commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocateInfo.commandBufferCount = 1;
					VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferConvolutionHermitian)); // allocate command buffer.

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferConvolutionHermitian, &beginInfo)); // start recording commands.
					vkCmdBindDescriptorSets(commandBufferConvolutionHermitian, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayoutConvolutionHermitian, 0, 1, &descriptorSetConvolutionHermitian, 0, NULL);
					vkCmdBindPipeline(commandBufferConvolutionHermitian, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_convolutionhermitian);
					vkCmdDispatch(commandBufferConvolutionHermitian, (uint32_t)ceil((WIDTH - 1) / float(WORKGROUP_SIZE)), (uint32_t)ceil((2 * HEIGHT) / float(WORKGROUP_SIZE)), (uint32_t)(DEPTH));
					VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferConvolutionHermitian)); // end recording commands.
				}
			}
			void createZeroPadding(VulkanFFTPlan* vulkanFFTPlan) {
				{
					VkDescriptorPoolSize descriptorPoolSize[2] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorPoolSize[0].descriptorCount = 1;
					descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[1].descriptorCount = 4;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 1;
					vkCreateDescriptorPool(vulkanFFTPlan->context->device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &descriptorPoolZeroPadding);
				}

				{
					const VkDescriptorType descriptorType[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &descriptorSetLayoutZeroPadding);
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = descriptorPoolZeroPadding;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayoutZeroPadding;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, &descriptorSetZeroPadding);
					for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
						VkDescriptorBufferInfo descriptorBufferInfo = { };
						if (i == 0) {
							descriptorBufferInfo.buffer = uboDimensions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = sizeof(VulkanDimensions);
						}
						if (i == 1) {
							descriptorBufferInfo.buffer = bufferSpins;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeSpins;
						}
						if (i == 2) {
							descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[0];
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
						}
						if (i == 3) {
							descriptorBufferInfo.buffer = bufferRegions_Book;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeRegions_Book;
						}
						if (i == 4) {
							descriptorBufferInfo.buffer = bufferRegions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeRegions;
						}
						VkWriteDescriptorSet writeDescriptorSet = { };
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = descriptorSetZeroPadding;
						writeDescriptorSet.dstBinding = i;
						writeDescriptorSet.dstArrayElement = 0;
						writeDescriptorSet.descriptorType = descriptorType[i];
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
						vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
					}
				}

				{
					uint32_t filelength;
					uint32_t* code = readFile(filelength, "zeropad.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModuleZeroPadding);
					delete[] code;

					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 1;
					pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutZeroPadding;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &pipelineLayoutZeroPadding);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo = { };

					pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					pipelineShaderStageCreateInfo.module = computeShaderModuleZeroPadding;
					pipelineShaderStageCreateInfo.pName = "main";
					computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
					computePipelineCreateInfo.layout = pipelineLayoutZeroPadding;


					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, vulkanFFTPlan->context->allocator, &pipeline_zeropadding);
				}
				{
					VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
					commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocateInfo.commandPool = commandPool;  
					commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocateInfo.commandBufferCount = 1; 
					VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferZeroPadding)); // allocate command buffer.

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferZeroPadding, &beginInfo)); // start recording commands.
					vkCmdBindDescriptorSets(commandBufferZeroPadding, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayoutZeroPadding, 0, 1, &descriptorSetZeroPadding, 0, NULL);
					vkCmdBindPipeline(commandBufferZeroPadding, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_zeropadding);
					vkCmdDispatch(commandBufferZeroPadding, (uint32_t)ceil((WIDTH ) / float(WORKGROUP_SIZE)), (uint32_t)ceil(HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);
					VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferZeroPadding)); // end recording commands.
				}
			}
			void createZeroPaddingRemove(VulkanFFTPlan* vulkanFFTPlan) {
				{
					VkDescriptorPoolSize descriptorPoolSize[2] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorPoolSize[0].descriptorCount = 1;
					descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[1].descriptorCount = 4;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 1;
					vkCreateDescriptorPool(vulkanFFTPlan->context->device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &descriptorPoolZeroPaddingRemove);
				}

				{
					const VkDescriptorType descriptorType[] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &descriptorSetLayoutZeroPaddingRemove);
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = descriptorPoolZeroPaddingRemove;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayoutZeroPaddingRemove;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, &descriptorSetZeroPaddingRemove);
					for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
						VkDescriptorBufferInfo descriptorBufferInfo = { };
						if (i == 0) {
							descriptorBufferInfo.buffer = uboDimensions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = sizeof(VulkanDimensions);
						}
						if (i == 1) {
							descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[vulkanFFTPlan->resultInSwapBuffer];
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
						}
						if (i == 2) {
							descriptorBufferInfo.buffer = bufferGradient;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeGradient;
						}
						if (i ==3) {
							descriptorBufferInfo.buffer = bufferRegions_Book;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeRegions_Book;
						}
						if (i == 4) {
							descriptorBufferInfo.buffer = bufferRegions;
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = bufferSizeRegions;
						}
						VkWriteDescriptorSet writeDescriptorSet = { };
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = descriptorSetZeroPaddingRemove;
						writeDescriptorSet.dstBinding = i;
						writeDescriptorSet.dstArrayElement = 0;
						writeDescriptorSet.descriptorType = descriptorType[i];
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
						vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
					}
				}

				{
					uint32_t filelength;
					uint32_t* code = readFile(filelength, "zeropadremove.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModuleZeroPaddingRemove);
					delete[] code;

					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 1;
					pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutZeroPaddingRemove;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &pipelineLayoutZeroPaddingRemove);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo = { };

					pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					pipelineShaderStageCreateInfo.module = computeShaderModuleZeroPaddingRemove;
					pipelineShaderStageCreateInfo.pName = "main";
					computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
					computePipelineCreateInfo.layout = pipelineLayoutZeroPaddingRemove;


					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, vulkanFFTPlan->context->allocator, &pipeline_zeropaddingremove);
				}
				{
					VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
					commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocateInfo.commandPool = commandPool;
					commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocateInfo.commandBufferCount = 1;
					VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferZeroPaddingRemove)); // allocate command buffer.

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferZeroPaddingRemove, &beginInfo)); // start recording commands.
					vkCmdBindDescriptorSets(commandBufferZeroPaddingRemove, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayoutZeroPaddingRemove, 0, 1, &descriptorSetZeroPaddingRemove, 0, NULL);
					vkCmdBindPipeline(commandBufferZeroPaddingRemove, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_zeropaddingremove);
					vkCmdDispatch(commandBufferZeroPaddingRemove, (uint32_t)ceil(( WIDTH) / float(WORKGROUP_SIZE)), (uint32_t)ceil( HEIGHT / float(WORKGROUP_SIZE)), (uint32_t)DEPTH);
					VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferZeroPaddingRemove)); // end recording commands.
				}
			}
			void createFillZero(VulkanFFTPlan* vulkanFFTPlan) {
				{
					VkDescriptorPoolSize descriptorPoolSize[1] = { };
					descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					descriptorPoolSize[0].descriptorCount = 2;
					VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
					descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					descriptorPoolCreateInfo.poolSizeCount = COUNT_OF(descriptorPoolSize);
					descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize;
					descriptorPoolCreateInfo.maxSets = 1;
					vkCreateDescriptorPool(vulkanFFTPlan->context->device, &descriptorPoolCreateInfo, vulkanFFTPlan->context->allocator, &descriptorPoolFillZero);
				}

				{
					const VkDescriptorType descriptorType[] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
					VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[COUNT_OF(descriptorType)];
					for (uint32_t i = 0; i < COUNT_OF(descriptorSetLayoutBindings); ++i) {
						descriptorSetLayoutBindings[i].binding = i;
						descriptorSetLayoutBindings[i].descriptorType = descriptorType[i];
						descriptorSetLayoutBindings[i].descriptorCount = 1;
						descriptorSetLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					}
					VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
					descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
					descriptorSetLayoutCreateInfo.bindingCount = COUNT_OF(descriptorSetLayoutBindings);
					descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
					vkCreateDescriptorSetLayout(vulkanFFTPlan->context->device, &descriptorSetLayoutCreateInfo, vulkanFFTPlan->context->allocator, &descriptorSetLayoutFillZero);
					VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { };
					descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					descriptorSetAllocateInfo.descriptorPool = descriptorPoolFillZero;
					descriptorSetAllocateInfo.descriptorSetCount = 1;
					descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayoutFillZero;
					vkAllocateDescriptorSets(vulkanFFTPlan->context->device, &descriptorSetAllocateInfo, &descriptorSetFillZero);
					for (uint32_t i = 0; i < COUNT_OF(descriptorType); ++i) {
						VkDescriptorBufferInfo descriptorBufferInfo = { };
						if (i == 0) {
							descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[0];
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
						}
						if (i == 1) {
							descriptorBufferInfo.buffer = vulkanFFTPlan->buffer[1];
							descriptorBufferInfo.offset = 0;
							descriptorBufferInfo.range = vulkanFFTPlan->bufferSize;
						}
						VkWriteDescriptorSet writeDescriptorSet = { };
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.dstSet = descriptorSetFillZero;
						writeDescriptorSet.dstBinding = i;
						writeDescriptorSet.dstArrayElement = 0;
						writeDescriptorSet.descriptorType = descriptorType[i];
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;
						vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
					}
				}

				{
					uint32_t filelength;
					uint32_t* code = readFile(filelength, "fillzero.spv");
					VkShaderModuleCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.pCode = code;
					createInfo.codeSize = filelength;
					vkCreateShaderModule(device, &createInfo, NULL, &computeShaderModuleFillZero);
					delete[] code;

					VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
					pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutCreateInfo.setLayoutCount = 1;
					pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutFillZero;
					vkCreatePipelineLayout(vulkanFFTPlan->context->device, &pipelineLayoutCreateInfo, vulkanFFTPlan->context->allocator, &pipelineLayoutFillZero);
					VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = { };
					VkComputePipelineCreateInfo computePipelineCreateInfo = { };

					pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
					pipelineShaderStageCreateInfo.module = computeShaderModuleFillZero;
					pipelineShaderStageCreateInfo.pName = "main";
					computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
					computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
					computePipelineCreateInfo.layout = pipelineLayoutFillZero;


					vkCreateComputePipelines(vulkanFFTPlan->context->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, vulkanFFTPlan->context->allocator, &pipeline_fillzero);
				}
				{
					VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
					commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocateInfo.commandPool = commandPool;
					commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocateInfo.commandBufferCount = 1;
					VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBufferFillZero)); // allocate command buffer.

					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(commandBufferFillZero, &beginInfo)); // start recording commands.
					vkCmdBindDescriptorSets(commandBufferFillZero, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayoutFillZero, 0, 1, &descriptorSetFillZero, 0, NULL);
					vkCmdBindPipeline(commandBufferFillZero, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_fillzero);
					vkCmdDispatch(commandBufferFillZero, (uint32_t) ceil(3* vulkanFFTPlan->axes[0].sampleCount * vulkanFFTPlan->axes[1].sampleCount * vulkanFFTPlan->axes[2].sampleCount/1024.0f), 1, 1);
					VK_CHECK_RESULT(vkEndCommandBuffer(commandBufferFillZero)); // end recording commands.
				}
			}
			void runCommandBuffer(VkCommandBuffer commandBuffer) {
				/*
				Now we shall finally submit the recorded command buffer to a queue.
				*/

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1; // submit a single command buffer
				submitInfo.pCommandBuffers = &commandBuffer; // the command buffer to submit.
				vkQueueSubmit(context.queue, 1, &submitInfo, context.fence);
				vkWaitForFences(context.device, 1, &context.fence, VK_TRUE, 100000000000) ;
				vkResetFences(context.device, 1, &context.fence);

				/*
				We submit the command buffer on the queue, at the same time giving a fence.
				*/
				//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
				/*
				The command will not have finished executing until the fence is signalled.
				So we wait here.
				We will directly after this read our buffer from the GPU,
				and we will not be sure that the command has finished executing unless we wait for the fence.
				Hence, we use a fence here.
				*/
				//VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000));

				//vkDestroyFence(device, fence, NULL);
			}

			void cleanup() {
				/*
				Clean up all Vulkan Resources.
				*/

				vkFreeMemory(device, bufferMemorySpins, NULL);
				vkFreeMemory(device, bufferMemoryGradient, NULL);
				vkFreeMemory(device, bufferMemoryRegions_Book, NULL);
				vkFreeMemory(device, bufferMemoryRegions, NULL);
				vkDestroyBuffer(device, bufferSpins, NULL);
				vkDestroyBuffer(device, bufferGradient, NULL);
				vkDestroyBuffer(device, bufferRegions_Book, NULL);
				vkDestroyBuffer(device, bufferRegions, NULL);
				vkDestroyShaderModule(device, computeShaderModule, NULL);
				vkDestroyDescriptorPool(device, descriptorPool, NULL);
				vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
				vkDestroyPipelineLayout(device, pipelineLayout, NULL);
				vkDestroyPipeline(device, pipeline_all, NULL);
				//vkDestroyPipeline(device, pipeline_ddi, NULL);
				vkDestroyCommandPool(device, commandPool, NULL);
				vkDestroyDevice(device, NULL);
				vkDestroyInstance(instance, NULL);
			}
		};
		ComputeApplication app;
    private:
        // ------------ Effective Field Functions ------------
        // Calculate the Zeeman effective field of a single Spin
        void Gradient_Zeeman(vectorfield & gradient);
        // Calculate the Anisotropy effective field of a single Spin
        void Gradient_Anisotropy(const vectorfield & spins, vectorfield & gradient);
        // Calculate the exchange interaction effective field of a Spin Pair
        void Gradient_Exchange(const vectorfield & spins, vectorfield & gradient);
        // Calculate the DMI effective field of a Spin Pair
        void Gradient_DMI(const vectorfield & spins, vectorfield & gradient);
		void Spatial_Gradient(const vectorfield & spins);
		// Calculates the Dipole-Dipole contribution to the effective field of spin ispin within system s
		void Gradient_DDI(const vectorfield& spins, vectorfield & gradient);
		void Gradient_DDI_Cutoff(const vectorfield& spins, vectorfield & gradient);
		void Gradient_DDI_Direct(const vectorfield& spins, vectorfield & gradient);
		void Gradient_DDI_FFT(const vectorfield& spins, vectorfield & gradient);
		void ComputeShader(const vectorfield& spins, vectorfield& gradient);
        // ------------ Energy Functions ------------
        // Indices for Energy vector
        //int idx_zeeman, idx_anisotropy, idx_exchange, idx_dmi, idx_ddi;
		void Energy_Set(const vectorfield & spins, scalarfield & Energy, vectorfield & gradient);
		#ifdef SPIRIT_LOW_MEMORY
			scalar Energy_Low_Memory(const vectorfield & spins, vectorfield & gradient);
		#endif
		void E_Update(const vectorfield & spins, scalarfield & Energy, vectorfield & gradient);
        // Calculate the Zeeman energy of a Spin System
        /*void E_Zeeman(const vectorfield & spins, scalarfield & Energy, vectorfield & gradient);
        // Calculate the Anisotropy energy of a Spin System
        void E_Anisotropy(const vectorfield & spins, scalarfield & Energy, vectorfield & gradient);
        // Calculate the exchange interaction energy of a Spin System
        void E_Exchange(const vectorfield & spins, scalarfield & Energy, vectorfield & gradient);
        // Calculate the DMI energy of a Spin System
        void E_DMI(const vectorfield & spins, scalarfield & Energy, vectorfield & gradient);
		
        // Dipolar interactions
		void E_DDI(const vectorfield& spins, scalarfield & Energy, vectorfield & gradient);*/
		//void E_DDI_Direct(const vectorfield& spins, scalarfield & Energy);
		//void E_DDI_Cutoff(const vectorfield& spins, scalarfield & Energy);
		//void E_DDI_FFT(const vectorfield& spins, scalarfield & Energy);
		void set_mult_spins(const vectorfield & spins, vectorfield & mult_spins);
		// Quadruplet
		//void E_Quadruplet(const vectorfield & spins, scalarfield & Energy);

		// Preparations for DDI-Convolution Algorithm
		void Prepare_DDI();
		// Preparations for Exchange regions constant
		void Prepare_Exchange();
		void Clean_DDI();

		// Plans for FT / rFT
		FFT::FFT_Plan fft_plan_spins;
		FFT::FFT_Plan fft_plan_reverse;

		field<FFT::FFT_cpx_type> transformed_dipole_matrices;

		bool save_dipole_matrices = true;
		field<Matrix3> dipole_matrices;

		matrixfield exchange_tensors;
		// Number of inter-sublattice contributions
		int n_inter_sublattice;
		// At which index to look up the inter-sublattice D-matrices
		field<int> inter_sublattice_lookup;

		// Lengths of padded system
		field<int> n_cells_padded;
		// Total number of padded spins per sublattice
		int sublattice_size;

		FFT::StrideContainer spin_stride;
		FFT::StrideContainer dipole_stride;

		//Calculate the FT of the padded D matriess
		void FFT_Dipole_Matrices(FFT::FFT_Plan & fft_plan_dipole, int img_a, int img_b, int img_c);
		//Calculate the FT of the padded spins
		void FFT_Spins(const vectorfield & spins);

		//Bounds for nested for loops. Only important for the CUDA version
		field<int> it_bounds_pointwise_mult;
		field<int> it_bounds_write_gradients;
		field<int> it_bounds_write_spins;
		field<int> it_bounds_write_dipole;

		int prev_position=0;
		int iteration_num=0;
    };


}
#endif
