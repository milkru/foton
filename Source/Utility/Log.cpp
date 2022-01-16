#include "Log.h"

FT_BEGIN_NAMESPACE

ImGuiTextBuffer ImGuiLogger::s_TextBuffer;
ImVector<int> ImGuiLogger::s_LineOffsets;
bool ImGuiLogger::s_AutoScroll = true;

void ImGuiLogger::Log(const char* inFormat, ...) IM_FMTARGS(2)
{
	int oldSize = s_TextBuffer.size();
	va_list arguments;
	va_start(arguments, inFormat);
	s_TextBuffer.appendfv(inFormat, arguments);
	va_end(arguments);

	for (int newSize = s_TextBuffer.size(); oldSize < newSize; ++oldSize)
	{
		if (s_TextBuffer[oldSize] == '\n')
		{
			s_LineOffsets.push_back(oldSize + 1);
		}
	}
}

void ImGuiLogger::Clear()
{
	s_TextBuffer.clear();
	s_LineOffsets.clear();
	s_LineOffsets.push_back(0);
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
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	const char* buffer = s_TextBuffer.begin();
	const char* bufferEnd = s_TextBuffer.end();

	ImGuiListClipper clipper;
	clipper.Begin(s_LineOffsets.Size);

	while (clipper.Step())
	{
		for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
		{
			const char* line_start = buffer + s_LineOffsets[line_no];
			const char* line_end = (line_no + 1 < s_LineOffsets.Size) ? (buffer + s_LineOffsets[line_no + 1] - 1) : bufferEnd;
			ImGui::TextUnformatted(line_start, line_end);
		}
	}

	clipper.End();

	ImGui::PopStyleVar();

	if (s_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
	ImGui::End();
}

FT_END_NAMESPACE
