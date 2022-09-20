#include "I_Texture.h"
#include "R_Texture.h"
#include "Engine.h"
#include "M_FileSystem.h"
#include "FSDefs.h"
#include "Log.h"
#include "ImGuiAppLog.h"
#include "I_Scene.h"

#include "glew.h"
#include "stb_image.h"

#include <string>
#include <fstream>

#define CHECKERS_SIZE 64

I_Texture::I_Texture(KoFiEngine* engine) : engine(engine), checkerTexture(nullptr)
{}

I_Texture::~I_Texture()
{}

bool I_Texture::Import(const char* path, R_Texture* texture, bool cleanAfterImport)
{
	if (texture == nullptr || path == nullptr)
	{
		KOFI_ERROR(" Importer: Could not Import Texture! Error: R_Texture or Path was nullptr.");
		return false;
	}

	texture->data = stbi_load(path, &texture->width, &texture->height, &texture->nrChannels, 0);

	texture->imageSizeBytes = texture->GetTextureWidth() * texture->GetTextureHeight() * texture->GetNrChannels() * sizeof(unsigned char);

	texture->SetUpTexture();

	if (cleanAfterImport) {
		if (texture->data != nullptr)
		{
			stbi_image_free(texture->data);
			texture->data = nullptr;
		}
	}

	return true;
}

bool I_Texture::Save(R_Texture* texture, const char* path)
{
	if (engine->GetFileSystem()->CheckDirectory(TEXTURES_DIR))
	{
		std::ofstream file;
		file.open(path, std::ios::in | std::ios::trunc | std::ios::binary);
		if (file.is_open())
		{
			// HEADER
			file.write((char*)&texture->width, sizeof(int));
			file.write((char*)&texture->height, sizeof(int));
			file.write((char*)&texture->nrChannels, sizeof(int));

			// BODY
			file.write((char*)texture->data, texture->imageSizeBytes);

			file.close();

			return true;
		}
	}
	else
		KOFI_ERROR(" Texture Save: directory %s couldn't be accessed.", TEXTURES_DIR);

	if (texture->data != nullptr)
	{
		stbi_image_free(texture->data);
		texture->data = nullptr;
	}

	return false;
}

bool I_Texture::Load(const char* path, R_Texture* texture)
{
	if (engine->GetFileSystem()->CheckDirectory(TEXTURES_DIR))
	{
		std::ifstream file;
		file.open(path, std::ios::binary);
		if (file.is_open())
		{
			// HEADER
			file.read((char*)&texture->width, sizeof(int));
			file.read((char*)&texture->height, sizeof(int));
			file.read((char*)&texture->nrChannels, sizeof(int));

			// BODY
			texture->imageSizeBytes = texture->width * texture->height * texture->nrChannels * sizeof(unsigned char);
			texture->data = (unsigned char*)malloc(texture->imageSizeBytes);
			file.read((char*)texture->data, texture->imageSizeBytes);

			file.close();

			texture->SetUpTexture();

			return true;
		}
	}
	else
		KOFI_ERROR(" Texture Load: directory %s couldn't be accessed.", TEXTURES_DIR);

	return false;
}

R_Texture* I_Texture::GetCheckerTexture()
{
	if (checkerTexture == nullptr)
	{
		checkerTexture = new R_Texture();

		GLubyte checkerImage[CHECKERS_SIZE][CHECKERS_SIZE][4];
		for (int i = 0; i < CHECKERS_SIZE; i++)
		{
			for (int j = 0; j < CHECKERS_SIZE; j++)
			{
				int c = ((((i & 0x8) == 0) ^ (((j & 0x8)) == 0))) * 255;
				checkerImage[i][j][0] = (GLubyte)c;
				checkerImage[i][j][1] = (GLubyte)c;
				checkerImage[i][j][2] = (GLubyte)c;
				checkerImage[i][j][3] = (GLubyte)255;
			}
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(1, &checkerTexture->textureID);

		glBindTexture(GL_TEXTURE_2D, checkerTexture->textureID);

		// Set the texture wrapping/filtering options (on the currently bound texture object)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // 03/05/2022 was GL_LINEAR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKERS_SIZE, CHECKERS_SIZE,
			0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);

		checkerTexture->width = CHECKERS_SIZE;
		checkerTexture->height = CHECKERS_SIZE;
		checkerTexture->nrChannels = 4;
		checkerTexture->imageSizeBytes = CHECKERS_SIZE * CHECKERS_SIZE * 4 * sizeof(unsigned char);
	}
	return checkerTexture;
}