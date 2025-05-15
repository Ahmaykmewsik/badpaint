
#include "image.h"

#include "../includes/raylib/src/external/stb_image.h"
#include "../includes/raylib/src/external/stb_image_write.h"

#include <cstring>  //memcpy

u8 *LoadDataFromDisk(String fileName, unsigned int *bytesRead, Arena *arena)
{
	unsigned char *data = NULL;
	*bytesRead = 0;

	FILE *file = fopen(C_STRING_NULL_TERMINATED(fileName), "rb");

	if (file != NULL)
	{
		// WARNING: On binary streams SEEK_END could not be found,
		// using fseek() and ftell() could not work in some (rare) cases
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (size > 0)
		{
			data = ARENA_PUSH_ARRAY(arena, size, unsigned char);

			// NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
			unsigned int count = (unsigned int)fread(data, sizeof(unsigned char), size, file);
			*bytesRead = count;

			//TODO: (Ahmayk) Log this stuff ourselves
#if 0
			if (count != (u32) size)
				TRACELOG(LOG_WARNING, "FILEIO: [%s] File partially loaded", fileName);
			else
				TRACELOG(LOG_INFO, "FILEIO: [%s] File loaded successfully", fileName);
#endif
		}
		else
		{
			//TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to read file", fileName);
		}

		fclose(file);
	}
	else
	{
		//TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to open file", fileName);
	}

	return data;
}

void LoadDataSetPixel(u8 *dst, u32 i, v4 pixel)
{
	pixel = pixel * 255.0f;
	u32 index = i * 4; 
	dst[index + 0] = (u8)(pixel.x);
	dst[index + 1] = (u8)(pixel.y);
	dst[index + 2] = (u8)(pixel.z);
	dst[index + 3] = (u8)(pixel.w);
}

ImageRawRGBA32 LoadDataIntoRawImage(u8 *fileData, u32 fileSize, GameMemory *gameMemory)
{
	ImageRawRGBA32 result = {};

	int comp = 0;
	int width = 0;
	int height = 0;
	void *outputData = stbi_load_from_memory(fileData, fileSize, &width, &height, &comp, 0);

	if (outputData != NULL)
	{
		int dataSize = width * height * 4 * sizeof(unsigned char);
		ArenaFree(&gameMemory->rootImageArena);
		gameMemory->rootImageArena = ArenaInit(dataSize);

		if (gameMemory->rootImageArena.memory)
		{
			result.dim.x = width;
			result.dim.y = height;
			result.dataSize = dataSize;
			result.dataU8 = (u8*) ArenaPushSize(&gameMemory->rootImageArena, result.dataSize, {});

			if (comp != 4)
			{
				for (int i = 0, k = 0; i < result.dim.x * result.dim.y; i++)
				{
					v4 pixel;
					switch (comp)
					{
						case 1:
						{
							//NOTE: PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
							pixel.x = (f32)((u8*)outputData)[i] / 255.0f;
							pixel.y = (f32)((u8*)outputData)[i] / 255.0f;
							pixel.z = (f32)((u8*)outputData)[i] / 255.0f;
							pixel.w = 1.0f;
							LoadDataSetPixel(result.dataU8, i, pixel);
						} break;
						case 2:
						{
							//NOTE: PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA
							pixel.x = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.y = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.z = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.w = (f32)((u8*)outputData)[k + 1] / 255.0f;
							k += 2;
							LoadDataSetPixel(result.dataU8, i, pixel);
						} break;
						case 3:
						{
							//NOTE: PIXELFORMAT_UNCOMPRESSED_R8G8B8
							pixel.x = (f32)((u8*)outputData)[k] / 255.0f;
							pixel.y = (f32)((u8*)outputData)[k + 1] / 255.0f;
							pixel.z = (f32)((u8*)outputData)[k + 2] / 255.0f;
							pixel.w = 1.0f;
							k += 3;
							LoadDataSetPixel(result.dataU8, i, pixel);
						} break;
						InvalidDefaultCase
					}
				}
			}
			else
			{
				memcpy(result.dataU8, outputData, result.dataSize);
			}
		}
		else
		{
			String notification = STRING("Failed to allocate memory for the image! Too bad!");
			//TODO: (Ahmayk) Handle error reporting outside this function!
			//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
		}
		stbi_image_free(outputData);
	}
	else
	{
		String notification = STRING("I don't recognize that as an image. In the future you'll be able to load arbitrary data, but not yet.");
		//TODO: (Ahmayk) Handle error reporting outside this function!
		//InitNotificationMessage(notification, &gameMemory->circularNotificationBuffer);
	}

	return result;
}

bool ExportImage(Image image, String filepath)
{
	bool result = false;
	int channels = 4;

	//HACK: append png if there's no file path lol
	//TODO: the platform layer should handle this not the game
	//TODO: Do this ourselves so we don't have to play hot potato with string null termination
	String fileExtention = STRING(GetFileExtension(C_STRING_NULL_TERMINATED(filepath)));
	if (!fileExtention.length)
	{
		filepath += ".png";
	}

	fileExtention = STRING(GetFileExtension(C_STRING_NULL_TERMINATED(filepath)));

	char *filepathChars = C_STRING_NULL_TERMINATED(filepath);

	if (fileExtention == ".png")
	{
		int dataSize = 0;
		unsigned char *fileData = stbi_write_png_to_mem((const unsigned char *)image.data, image.width * channels, image.width, image.height, channels, &dataSize);
		result = SaveFileData(filepathChars, fileData, dataSize);
		RL_FREE(fileData);
	}
	else if (fileExtention == ".bmp")
	{
		result = stbi_write_bmp(filepathChars, image.width, image.height, channels, image.data);
	}
	else if (fileExtention == ".jpg" || fileExtention == ".jpeg")
	{
		result = stbi_write_jpg(filepathChars, image.width, image.height, channels, image.data, 100); // JPG quality: between 1 and 100
	}
	else
	{
		// Export raw pixel data (without header)
		// NOTE: It's up to the user to track image parameters
		result = SaveFileData(filepathChars, image.data, GetPixelDataSize(image.width, image.height, image.format));
	}

	return result;
}
