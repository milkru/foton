#include "log.h"

namespace FT
{
	void Logger::Log(const char* fmt, ...) IM_FMTARGS(2)
	{
		va_list args;
		va_start(args, fmt);
		m_TextBuffer.appendfv(fmt, args);
		va_end(args);

		m_ScrollToBottom = true;
	}

	void Logger::Draw(const char* title)
	{
		static const ImVec2 DefaultWindowSize = ImVec2(200, 200);

		ImGui::Begin(title);

		ImGui::SetWindowSize(DefaultWindowSize, ImGuiCond_FirstUseEver);
		ImGui::TextUnformatted(m_TextBuffer.begin());

		if (m_ScrollToBottom)
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::End();

		m_ScrollToBottom = false;
	}

	void Logger::Clear()
	{
		m_TextBuffer.clear();
	}
}
