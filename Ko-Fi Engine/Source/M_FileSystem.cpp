#include "M_FileSystem.h"

#include "GameObject.h"
#include "Engine.h"
#include "SceneIntro.h"
#include "Log.h"
#include "ImGuiAppLog.h"
#include "SDL_assert.h"
#include "SDL.h"
#include <fstream>
#include <filesystem>
#include "JsonHandler.h"

#include "md5.h"

#include "optick.h"

M_FileSystem::M_FileSystem(KoFiEngine* engine)
{
	name = "FileSystem";

	std::string rootPathString = SDL_GetBasePath();
	std::replace(rootPathString.begin(), rootPathString.end(), '\\', '/');
	rootPath = rootPathString;

	// Comment this for release path and uncomment when developing...
	rootPath = rootPath.parent_path().parent_path();
	// Comment this for release path and uncomment when developing...
	AddPath("/Ko-Fi Engine/Ko-Fi");
	this->engine = engine;

	CONSOLE_LOG("Filesystem: %s", rootPath.string());
	appLog->AddLog("Filesystem: %s\n", rootPath.string());
}

M_FileSystem::~M_FileSystem()
{

}

bool M_FileSystem::Awake(Json configModule)
{
	bool ret = true;

	CONSOLE_LOG("Turning on M_FileSystem debugger...");
	appLog->AddLog("Turning on M_FileSystem debugger...\n");

	//Prepare filesystem
	//std::filesystem::directory_entry().assign(addPath);
	// Stream log messages to Debug window

	ret = LoadConfiguration(configModule);

	return ret;
}

bool M_FileSystem::Start()
{
	CONSOLE_LOG("Starting M_FileSystem...");
	appLog->AddLog("Starting M_FileSystem...\n");

	return true;
}

bool M_FileSystem::PreUpdate(float dt)
{
	return true;
}

bool M_FileSystem::Update(float dt)
{
	return true;
}

bool M_FileSystem::PostUpdate(float dt)
{
	return true;
}

bool M_FileSystem::CleanUp()
{
	CONSOLE_LOG("Cleaning M_FileSystem up...");
	appLog->AddLog("Cleaning M_FileSystem up...\n");

	// detach log stream

	return true;
}

// Method to receive and manage events
void M_FileSystem::OnNotify(const Event& event)
{
	// Manage events
}

bool M_FileSystem::SaveConfiguration(Json& configModule) const
{
	return true;
}

bool M_FileSystem::LoadConfiguration(Json& configModule)
{
	return true;
}

bool M_FileSystem::InspectorDraw()
{
	return true;
}

std::string M_FileSystem::OpenFile(const char* path) const
{
	std::string fileText;

	SDL_assert(path != nullptr);
	std::ifstream stream(path);
	if (stream.is_open())
	{
		std::string line;

		while (std::getline(stream, line))
			fileText.append(line + "\n");
	}
	stream.close();
	return fileText;
}

std::string M_FileSystem::OpenFileBinary(const char* path) const
{
	std::string fileText;

	SDL_assert(path != nullptr);
	std::ifstream stream(path, std::ios::binary);
	if (stream.is_open())
	{
		std::string line;

		while (std::getline(stream, line))
			fileText.append(line + "\n");
	}
	stream.close();
	return fileText;
}

bool M_FileSystem::SaveFile(const char* path, std::string text) const
{
	bool ret = true;
	SDL_assert(path != nullptr);
	std::ofstream stream(path);

	if (stream.is_open())
		stream.write(text.c_str(), text.size());

	stream.close();
	return ret;
}

void M_FileSystem::EnumerateFiles(const char* path, std::vector<std::string>& files, std::vector<std::string>& dirs)
{
	std::string p = rootPath.string() + "/" + path;
	for (const auto& file : std::filesystem::directory_iterator(p))
	{
		if (std::filesystem::is_directory(file.path()))
			dirs.push_back(file.path().filename().string());
		else
			files.push_back(file.path().filename().string());
	}
}

void M_FileSystem::DiscoverAllFiles(const char* directory, std::vector<std::string>& files)
{
	if (directory == nullptr)
	{
		KOFI_ERROR(" Resource Manager: discovering files, directory string was nullptr.");
		return;
	}
	if (!std::filesystem::exists(directory))
	{
		KOFI_ERROR(" Resource Manager: discovering files, directory %s doesn't exist or not found.", directory);
		return;
	}

	std::vector<std::string> directories;
	EnumerateFiles(directory, files, directories);

	for (const auto dir : directories)
	{
		std::string path = directory + dir + std::string("/");
		DiscoverAllFiles(path.c_str(), files);
	}

	directories.clear();
	directories.shrink_to_fit();
}

void M_FileSystem::DiscoverAllFilesFiltered(const char* directory, std::vector<std::string>& files, std::vector<std::string>& filteredFiles, const char* filter)
{
	if (directory == nullptr)
	{
		KOFI_ERROR(" Resource Manager: discovering files, directory string was nullptr.");
		return;
	}
	if (!std::filesystem::exists(directory))
	{
		KOFI_ERROR(" Resource Manager: discovering files, directory %s doesn't exist or not found.", directory);
		return;
	}
	if (filter == nullptr)
	{
		KOFI_ERROR(" Resource Manager: Filter was nullptr.");
		return;
	}

	std::vector<std::string> directories;
	std::vector<std::string> allFiles;
	EnumerateFiles(directory, allFiles, directories);

	for (auto & file : allFiles)
	{
		std::filesystem::path fileTmp = file;
		if (!fileTmp.extension().empty())
		{
			if (StringCompare(fileTmp.extension().string().c_str(), filter) == 0)
				filteredFiles.push_back(directory + file);
			else
				files.push_back(directory + file);
		}
	}

	for (const auto dir : directories)
	{
		std::string path = directory + dir + std::string("/");
		DiscoverAllFilesFiltered(path.c_str(), files, filteredFiles, filter);
	}

	directories.clear();
	directories.shrink_to_fit();
}

bool M_FileSystem::CheckDirectory(const char* path)
{
	if (path == nullptr)
	{
		LOG_BOTH("[ERROR] Filesystem: directory path was nullptr.");
		return false;
	}

	std::filesystem::path directoryPath = path;

	if (!std::filesystem::exists(path))
	{
		if (directoryPath.extension() != "")
		{
			LOG_BOTH("[WARNING] Filesystem: directory path isn't a directory.");
			return false;
		}

		bool ret = std::filesystem::create_directory(directoryPath);
		if (ret)
			return true;
		else
		{
			LOG_BOTH("[WARNING] Filesystem: directory path couldn't be created.");
			return false;
		}
	}
	else
		return true;

	return true;
}

int M_FileSystem::GetLastModTime(const char* path)
{
	auto fTime = std::filesystem::last_write_time(path);

	int ret = std::chrono::time_point_cast<std::chrono::seconds>(fTime).time_since_epoch().count();
	return ret;
}

std::string M_FileSystem::GetFileHash(const char* path)
{
	if (path == nullptr)
	{
		CONSOLE_LOG("[ERROR] Filesystem: generating MD5 hash, file path was nullptr.");
		return "";
	}

	MD5 digestMd5;

	std::ifstream file;
	std::istream* input = NULL;

	file.open(path, std::ios::in | std::ios::binary);
	if (!file)
	{
		CONSOLE_LOG("[ERROR] Filesystem: generating MD5 hash, file %s couldn't be accessed.", path);
		return "";
	}
	else
		input = &file;

	// each cycle processes about 1 MByte
	const size_t bufferSize = 144 * 7 * 1024;
	char* buffer = new char[bufferSize];

	while (*input)
	{
		input->read(buffer, bufferSize);
		std::size_t bytesRead = size_t(input->gcount());

		digestMd5.add(buffer, bytesRead);
	}

	file.close();
	delete[] buffer;
	buffer = nullptr;

	return digestMd5.getHash();
}

bool M_FileSystem::CopyFileTo(const char* sourcePath, const char* destinationPath)
{
	if (!std::filesystem::exists(sourcePath))
		return false;

	std::filesystem::path source = sourcePath;
	std::filesystem::path dest = destinationPath;

	if (std::filesystem::is_directory(source) || std::filesystem::is_directory(dest))
		return false;

	if (source.extension() != dest.extension())
		return false;
	else if (CheckDirectory(dest.parent_path().string().c_str()))
	{
		try
		{
			std::filesystem::copy_file(source, dest, std::filesystem::copy_options::update_existing);
		}
		catch (std::filesystem::filesystem_error& e)
		{
			KOFI_ERROR(" Filesystem: couldn't copy &s: &s", sourcePath, e.what());
			return false;
		}
	}
	return true;
}

std::string M_FileSystem::GetFileName(const char* path) const
{
	std::string fileName = path;
	fileName = fileName.substr(fileName.find_last_of("/") + 1, fileName.size());
	return fileName.c_str();
}

std::string M_FileSystem::GetNameFromPath(std::string path)
{
	std::string name = path;
	name = name.substr(name.find_last_of("/\\") + 1);
	std::string::size_type const p(name.find_last_of('.'));
	name = name.substr(0, p);
	return name;
}

int M_FileSystem::StringCompare(const char* a, const char* b) {
	int ca, cb;
	do
	{
		ca = *(unsigned char*)a;
		cb = *(unsigned char*)b;
		ca = tolower(toupper(ca));
		cb = tolower(toupper(cb));
		a++;
		b++;
	} while (ca == cb && ca != '\0');
	return ca - cb;
}

void M_FileSystem::AddPath(const char* path)
{
	rootPath += path;
}

void M_FileSystem::CreateMaterial(const char* path, const char* filename, const char* texturePath)
{
	JsonHandler jsonHandler;
	auto materialJson = R"(
	{
		"name" : "",
		"path" : "",
		"textures" : [""],
		"uniforms" :
		{
			"albedoTint" :
			{
				"x":1.0,
				"y":1.0,
				"z":1.0,
				"w":1.0
			}
		}
	}
	)"_json;

	materialJson["name"] = filename;

	auto texturesArray = json::array();
	texturesArray.push_back(texturePath);

	materialJson["textures"] = texturesArray;

	jsonHandler.SaveJson(materialJson, path);
}

void M_FileSystem::CreateMaterial(const char* path)
{
	JsonHandler jsonHandler;
	auto materialJson = R"(
	{
		"name" : "",
		"path" : "",
		"textures" : [""],
		"uniforms" : 
		{
			"albedoTint" :
			{
				"x":1.0,
				"y":1.0,
				"z":1.0,
				"w":1.0
			}
		}
	}
	)"_json;

	materialJson["name"] = "default";
	materialJson["path"] = path;

	jsonHandler.SaveJson(materialJson, path);
}

void M_FileSystem::CreateShader(const char* path)
{
	const char* text = R"(
	#shader vertex
	#version 330 core

	layout (location = 0) in vec3 position;
	layout (location = 1) in vec3 normals;
	layout (location = 2) in vec2 texCoord;

	out vec4 ourColor;
	out vec2 TexCoord;

	uniform mat4 model_matrix;
	uniform mat4 view;
	uniform mat4 projection;
	uniform vec4 albedoTint;

	void main()
	{
		gl_Position = projection * view * model_matrix * vec4(position, 1.0f);
		ourColor = albedoTint;
		TexCoord = texCoord;
	}

	#shader fragment
	#version 330 core

	in vec4 ourColor;
	in vec2 TexCoord;

	out vec4 color;

	uniform sampler2D ourTexture;

	void main()
	{
		color = texture(ourTexture, TexCoord)*ourColor;
	}
	)";

	SDL_assert(path != nullptr);

	std::ofstream stream(path, std::ios::trunc);
	SDL_assert(stream.is_open());
	try
	{
		stream << std::setw(4) << text << std::endl;

	}
	catch (Json::parse_error& e)
	{
		CONSOLE_LOG("Error while Saving File: %c", e.what());
		appLog->AddLog("Error while Saving File: %c\n", e.what());
	}
	stream.close();
}

void M_FileSystem::CreateScene(const char* path,const char* sceneName)
{
	Json sceneJson;
	std::string name = GetNameFromPath(sceneName);
	sceneJson[name];
	sceneJson[name]["active"] = true;
	sceneJson[name]["game_objects_amount"] = 0;
	sceneJson[name]["game_objects_list"] = json::array();
	sceneJson[name]["name"] = name;

	SDL_assert(path != nullptr);

	JsonHandler handler;
	handler.SaveJson(sceneJson, path);
}
