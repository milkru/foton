#pragma once

namespace FT
{
	class Logger
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
