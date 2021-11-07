#pragma once

#define FT_LOG(fmt, ...) do { fprintf(stdout, fmt, __VA_ARGS__); } while(0)

namespace FT
{
	class ImGuiLogger
	{
	public:
		void Log(const char* fmt, ...) IM_FMTARGS(2);
		void Draw(const char* title);
		void Clear();

	private:
		ImGuiTextBuffer m_TextBuffer;
		bool m_ScrollToBottom;
	};
}
