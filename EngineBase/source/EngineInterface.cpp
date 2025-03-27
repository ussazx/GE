#define INTERFACE_IMPLEMENT
#include "EngineInterface.h"
#include "Generic/LuaWrapper/LuaUtility.h"
#include <fstream>
#include <codecvt>

namespace Engine
{
	class StreamInput
	{
	public:
		virtual ~StreamInput() {};

		virtual bool IsValid() = 0;

		virtual int64_t GetSize() = 0;

		virtual int64_t GetOffset() = 0;

		virtual void* GetData() = 0;

		virtual int64_t Seek(int64_t offset, SeekPos start = CUR) = 0;
		virtual int64_t Read(void* buff, int64_t size) = 0;

		Lua_wrap_cpp_class(StreamInput, Lua_abstract, Lua_mf(IsValid), Lua_mf(GetSize), Lua_mf(GetOffset), Lua_mf(GetData));
	};
	Lua_global_add_cpp_class(StreamInput);

	class StreamOutput
	{
	public:
		virtual ~StreamOutput() {};
		virtual bool IsValid() = 0;
		virtual bool OutputUtf8(const char* str) = 0;
		Lua_wrap_cpp_class(StreamOutput, Lua_abstract, Lua_mf(IsValid), Lua_mf(OutputUtf8))
	};
	Lua_global_add_cpp_class(StreamOutput);

	class FileInput : public StreamInput
	{
	public:
		~FileInput()
		{
			Close();
		}

		bool Open(LString fileName, bool isBinary)
		{
			m_size = 0;
			m_data.clear();
			if (m_ifs.is_open())
				m_ifs.close();
			m_ifs.open(fileName.c_str(), isBinary ? std::ios::binary : 1);
			return m_ifs.is_open();
		}

		void Close()
		{
			if (m_ifs.is_open())
				m_ifs.close();
		}

		bool IsValid() override
		{
			return m_ifs.is_open();
		}
		int64_t GetSize() override
		{
			if (m_size > 0)
				return m_size;
			int64_t begin = m_ifs.tellg();
			m_ifs.seekg(0, std::ios::end);
			m_size = m_ifs.tellg();
			m_ifs.seekg(begin, std::ios::beg);
			return m_size;
		}
		int64_t GetOffset() override
		{
			return m_ifs.tellg();
		}
		int64_t Seek(int64_t offset, SeekPos start = CUR) override
		{
			static std::ios_base::seekdir type[] = { std::ios::cur, std::ios::beg,  std::ios::end };
			return m_ifs.seekg(offset, type[start]).tellg();
		}
		int64_t Read(void* buff, int64_t len) override
		{
			return m_ifs.read((char*)buff, len).gcount();
		}
		void* GetData() override
		{
			if (m_data.size() > 0)
				return m_data.data();
			m_data.resize(GetSize());
			int64_t offset = m_ifs.tellg();
			m_ifs.seekg(0, std::ios::beg);
			m_ifs.read((char*)m_data.data(), m_size);
			m_ifs.seekg(offset, std::ios::beg);
			return m_data.data();
		}

		Lua_wrap_cpp_class_derived(StreamInput, FileInput, Lua_abstract, Lua_mf(Open), Lua_mf(Close));
	private:
		std::vector<char> m_data;
		uint64_t m_size{};
		std::ifstream m_ifs;
	};
	Lua_global_add_cpp_class(FileInput);

#ifdef ANDROID
	class AssetReader : public FileInput
	{
	public:
		AssetReader(const char* fileName, bool isBinary)
		{

		}
		Lua_cpp_class_derived(StreamReader, AssetReader, Lua_abstract);

		AAsset* asset{};
		static AAssetManager* assetManager;
	};
#endif

	LuacObjNew<FileInput> CNewFileInput(bool isAsset)
	{
#ifdef ANDROID
		if (isAsset)
			return new CAssetReader(fileName, isBinary);
#endif
		return new FileInput;
	}
	Lua_global_add_cfunc(CNewFileInput);

	class FileOutput : public StreamOutput
	{
	public:
		~FileOutput()
		{
			Close();
		}

		bool Open(LString fileName, bool isBinary)
		{
			m_size = 0;
			m_data.clear();
			if (m_ofs.is_open())
				m_ofs.close();
			m_ofs.open(fileName.c_str(), (isBinary ? std::ios::binary : 0) | std::ios::out);
			return m_ofs.is_open();
		}

		void Close()
		{
			if (m_ofs.is_open())
				m_ofs.close();
		}

		bool IsValid() override
		{
			return m_ofs.is_open();
		}
		bool OutputUtf8(const char* buff) override
		{
			return m_ofs.write(buff, strlen(buff)).good();
		}

		Lua_wrap_cpp_class_derived(StreamOutput, FileOutput, Lua_abstract, Lua_mf(Open), Lua_mf(Close));
	private:
		std::vector<char> m_data;
		uint64_t m_size{};
		std::ofstream m_ofs;
	};
	Lua_global_add_cpp_class(FileOutput);

	LuacObjNew<FileOutput> CNewFileOutput()
	{
		return new FileOutput;
	}
	Lua_global_add_cfunc(CNewFileOutput)
};