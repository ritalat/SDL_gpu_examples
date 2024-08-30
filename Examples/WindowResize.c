#include "Common.h"

typedef struct Resolution
{
    Uint32 x, y;
} Resolution;

static const Resolution Resolutions[] =
{
    { 640, 480 },
    { 1280, 720 },
    { 1024, 1024 },
    { 1600, 900 },
    { 1920, 1080 },
    { 3200, 1800 },
    { 3840, 2160 }
};
static Uint32 ResolutionCount = SDL_arraysize(Resolutions);

static SDL_GPUGraphicsPipeline* Pipeline;
static Sint32 ResolutionIndex;

static int Init(Context* context)
{
    int result = CommonInit(context, 0);
	if (result < 0)
	{
		return result;
	}

    ResolutionIndex = 0;

    SDL_GPUShader* vertexShader = LoadShader(context->Device, "RawTriangle.vert", 0, 0, 0, 0);
    if (vertexShader == NULL)
    {
        SDL_Log("Failed to create vertex shader!");
        return -1;
    }

    SDL_GPUShader* fragmentShader = LoadShader(context->Device, "SolidColor.frag", 0, 0, 0, 0);
    if (fragmentShader == NULL)
    {
        SDL_Log("Failed to create fragment shader!");
        return -1;
    }

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
		.attachmentInfo = {
			.colorAttachmentCount = 1,
			.colorAttachmentDescriptions = (SDL_GPUColorAttachmentDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(context->Device, context->Window),
				.blendState = {
					.blendEnable = SDL_TRUE,
					.alphaBlendOp = SDL_GPU_BLENDOP_ADD,
					.colorBlendOp = SDL_GPU_BLENDOP_ADD,
					.colorWriteMask = 0xF,
					.srcColorBlendFactor = SDL_GPU_BLENDFACTOR_ONE,
					.srcAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ONE,
					.dstColorBlendFactor = SDL_GPU_BLENDFACTOR_ZERO,
					.dstAlphaBlendFactor = SDL_GPU_BLENDFACTOR_ZERO
				}
			}},
		},
		.multisampleState.sampleMask = 0xFFFF,
		.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertexShader = vertexShader,
		.fragmentShader = fragmentShader,
        .rasterizerState.fillMode = SDL_GPU_FILLMODE_FILL
	};

    Pipeline = SDL_CreateGPUGraphicsPipeline(context->Device, &pipelineCreateInfo);
    if (Pipeline == NULL)
    {
        SDL_Log("Failed to create pipeline!");
        return -1;
    }

    SDL_ReleaseGPUShader(context->Device, vertexShader);
    SDL_ReleaseGPUShader(context->Device, fragmentShader);

    SDL_Log("Press Left and Right to resize the window!");

    return 0;
}

static int Update(Context* context)
{
    SDL_bool changeResolution = SDL_FALSE;

    if (context->RightPressed)
    {
        ResolutionIndex = (ResolutionIndex + 1) % ResolutionCount;
        changeResolution = SDL_TRUE;
    }

    if (context->LeftPressed)
    {
        ResolutionIndex -= 1;
        if (ResolutionIndex < 0)
        {
            ResolutionIndex = ResolutionCount - 1;
        }
        changeResolution = SDL_TRUE;
    }

    if (changeResolution)
    {
        Resolution currentResolution = Resolutions[ResolutionIndex];
        SDL_Log("Setting resolution to: %u, %u", currentResolution.x, currentResolution.y);
        SDL_SetWindowSize(context->Window, currentResolution.x, currentResolution.y);
        SDL_SetWindowPosition(context->Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_SyncWindow(context->Window);
    }

    return 0;
}

static int Draw(Context* context)
{
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(context->Device);
	if (cmdbuf == NULL)
	{
		SDL_Log("GPUAcquireCommandBuffer failed");
		return -1;
	}

	Uint32 w, h;
	SDL_GPUTexture* swapchainTexture = SDL_AcquireGPUSwapchainTexture(cmdbuf, context->Window, &w, &h);
	if (swapchainTexture != NULL)
	{
		SDL_GPUColorAttachmentInfo colorAttachmentInfo = { 0 };
		colorAttachmentInfo.texture = swapchainTexture;
		colorAttachmentInfo.clearColor = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f };
		colorAttachmentInfo.loadOp = SDL_GPU_LOADOP_CLEAR;
		colorAttachmentInfo.storeOp = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorAttachmentInfo, 1, NULL);
		SDL_BindGPUGraphicsPipeline(renderPass, Pipeline);
		SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
		SDL_EndGPURenderPass(renderPass);
	}

	SDL_SubmitGPUCommandBuffer(cmdbuf);

	return 0;
}

static void Quit(Context* context)
{
    SDL_ReleaseGPUGraphicsPipeline(context->Device, Pipeline);
    CommonQuit(context);
}

Example WindowResize_Example = { "WindowResize", Init, Update, Draw, Quit };
