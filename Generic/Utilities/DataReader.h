#pragma once
#include <fstream>
#include <memory>

class DataReader
{
public:
	enum SeekStart
	{
		CUR,
		BEG,
		END
	};
	DataReader() {}
	virtual ~DataReader() {}
	template<class T>
	__int64 Read(T& out, __int64 len = sizeof(T), bool outRangeFail = true)
	{
		return Readv(&out, len, outRangeFail);
	}
	virtual const void* GetDataOnMemory() = 0;
	virtual __int64 GetSize() = 0;
	virtual __int64 Readv(void* buff, __int64 len, bool outRangeFail = false) = 0;
	virtual __int64 GetOffset() = 0;
	virtual __int64 Seek(__int64 offset, SeekStart start = CUR) = 0;
};

class BufferReader : public DataReader
{
public:
	BufferReader(const void* buffer = nullptr, __int64 size = 0)
	{
		if (size > 0)
		{
			m_begin = (const char*)buffer;
			m_cur = m_begin;
			m_end = m_begin + size - 1;
		}
	}
	void SetBuffer(const void* buffer, __int64 size)
	{
		if (size > 0)
		{
			m_begin = (const char*)buffer;
			m_cur = m_begin;
			m_end = m_begin + size - 1;
		}
	}
	const void* GetDataOnMemory() override
	{
		return (void*)m_begin;
	}
	__int64 GetSize() override
	{
		return m_end == nullptr ? 0 : m_end - m_begin + 1;
	}
	__int64 Readv(void* buff, __int64 len, bool outRangeFail = false) override
	{
		if (!m_cur)
			return 0;

		__int64 remain = m_end - m_cur + 1;
		bool outRange = len > remain;
		if (outRangeFail && outRange)
			return 0;

		if (outRange)
			len = remain;
		memcpy(buff, m_cur, len);
		m_cur += len;
		return len;
	}
	__int64 GetOffset() override
	{
		return m_cur - m_begin;
	}
	__int64 Seek(__int64 offset, SeekStart start = CUR) override
	{
		if (!m_cur)
			return 0;

		if (start == BEG)
			m_cur = m_begin;
		else if (start == END)
			m_cur = m_end;
		m_cur += offset;
		if (m_cur < m_begin)
			m_cur = m_begin;
		else if (m_cur > m_end)
			m_cur = m_end;
		return m_cur - m_begin;
	}
private:
	const char* m_begin;
	const char* m_cur;
	const char* m_end;
};

class FileReader : public DataReader
{
public:
	FileReader() { m_size = 0; }
	FileReader(const char* fileName, bool isBinary)
	{
		m_size = 0;
		m_ifs.open(fileName, isBinary ? std::ios::binary : 1);
	}
	FileReader(const wchar_t* fileName, bool isBinary)
	{
		m_ifs.open(fileName, isBinary ? std::ios::binary : 1);
	}
	FileReader(const FileReader&) = delete;
	const FileReader& operator = (const FileReader&) = delete;
	~FileReader()
	{
		Close();
	}
	bool Open(const char* fileName, bool isBinary)
	{
		m_size = 0;
		if (m_ifs.is_open())
			m_ifs.close();
		m_ifs.open(fileName, isBinary ? std::ios::binary : 1);
		return m_ifs.is_open();
	}
	bool Open(const wchar_t* fileName, bool isBinary)
	{
		m_size = 0;
		if (m_ifs.is_open())
			m_ifs.close();
		m_ifs.open(fileName, isBinary ? std::ios::binary : 1);
		return m_ifs.is_open();
	}
	bool IsOpen()
	{
		return m_ifs.is_open();
	}
	const void* GetDataOnMemory() override
	{
		return nullptr;
	}
	__int64 GetSize() override
	{
		if (m_size > 0)
			return m_size;
		__int64 begin = m_ifs.tellg();
		m_ifs.seekg(0, std::ios::end);
		m_size = m_ifs.tellg();
		m_ifs.seekg(begin, std::ios::beg);
		return m_size;
	}
	__int64 GetOffset() override
	{
		return m_ifs.tellg();
	}
	__int64 Readv(void* buff, __int64 len, bool outRangeFail = false) override
	{
		return outRangeFail && GetSize() - m_ifs.tellg() < len ? 0 : m_ifs.read((char*)buff, len).gcount();
	}
	__int64 Seek(__int64 offset, SeekStart start = CUR) override
	{
		static std::ios_base::seekdir type[] = { std::ios::cur, std::ios::beg,  std::ios::end };
		return m_ifs.seekg(offset, type[start]).tellg();
	}
	void Close()
	{
		if (m_ifs.is_open())
			m_ifs.close();
	}

private:
	std::ifstream m_ifs;
	__int64 m_size;
};

class FileData
{
public:
	FileData()
	{
		m_isLoaded = false;
		m_data = nullptr;
		m_size = 0;
	}
	FileData(FileData&& fd)
	{
		m_isLoaded = fd.m_isLoaded;
		m_data = fd.m_data;
		m_size = fd.m_size;
		fd.m_isLoaded = false;
		fd.m_data = nullptr;
		fd.m_size = 0;
	}
	FileData(const FileData&) = delete;
	const FileData& operator = (const FileData&) = delete;
	FileData(const char* fileName, bool isBinary)
	{
		m_data = nullptr;
		Load(fileName, isBinary);
	}
	FileData(const wchar_t* fileName, bool isBinary)
	{
		m_data = nullptr;
		Load(fileName, isBinary);
	}
	~FileData()
	{
		delete[] m_data;
	}
	bool Load(const char* fileName, bool isBinary)
	{
		Release();
		FileReader fr(fileName, isBinary);
		m_isLoaded = fr.IsOpen();
		if (m_isLoaded && fr.GetSize() > 0)
		{
			m_size = fr.GetSize();
			m_data = new char[m_size];
			fr.Readv(m_data, m_size);
		}
		return m_isLoaded;
	}
	bool Load(const wchar_t* fileName, bool isBinary)
	{
		Release();
		FileReader fr(fileName, isBinary);
		m_isLoaded = fr.IsOpen();
		if (m_isLoaded && fr.GetSize() > 0)
		{
			m_size = fr.GetSize();
			m_data = new char[m_size];
			fr.Readv(m_data, m_size);
		}
		return m_isLoaded;
	}
	bool IsLoaded()
	{
		return m_isLoaded;
	}
	void* GetData()
	{
		return m_data;
	}
	__int64 GetSize()
	{
		return m_size;
	}
	void Release()
	{
		delete[] m_data;
		m_data = nullptr;
		m_size = 0;
		m_isLoaded = false;
	}

private:
	bool m_isLoaded;
	void* m_data;
	__int64 m_size;
};