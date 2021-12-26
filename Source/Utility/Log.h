#pragma once

#define FT_LOG(fmt, ...) do { ImGuiLogger::Log(fmt, __VA_ARGS__); } while(0)

FT_BEGIN_NAMESPACE

class ImGuiLogger
{
public:
	static void Log(const char* inFormat, ...) IM_FMTARGS(2);
	static void Clear();
	static void Draw(const char* inTitle);

private:
	static ImGuiTextBuffer s_TextBuffer;
	static ImVector<int> s_LineOffsets;
	static bool s_AutoScroll;
};

FT_END_NAMESPACE
