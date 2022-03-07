#include "Log.h"

FT_BEGIN_NAMESPACE

ImGuiTextBuffer ImGuiLogger::s_TextBuffer;

void ImGuiLogger::Log(const char* inFormat, ...) IM_FMTARGS(2)
{
	int oldSize = s_TextBuffer.size();
	va_list arguments;
	va_start(arguments, inFormat);
	s_TextBuffer.appendfv(inFormat, arguments);
	va_end(arguments);
}

void ImGuiLogger::Clear()
{
	s_TextBuffer.clear();
}

void ImGuiLogger::Draw(const char* title)
{
	if (!ImGui::Begin(title))
	{
		ImGui::End();
		return;
	}

	ImGui::SameLine();
	ImGui::SetWindowFontScale(1.25f);
	ImGui::Text("Output");
	ImGui::SetWindowFontScale(1.0f);

	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false,
		ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::TextUnformatted(s_TextBuffer.begin());
	ImGui::PopStyleVar();

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
	ImGui::End();
}

FT_END_NAMESPACE
