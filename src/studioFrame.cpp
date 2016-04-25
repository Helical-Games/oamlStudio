//-----------------------------------------------------------------------------
// Copyright (c) 2015-2016 Marcelo Fernandez
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "oamlCommon.h"
#include "tinyxml2.h"

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/scrolbar.h>
#include <wx/sizer.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/filename.h>
#include <wx/filehistory.h>
#include <wx/config.h>
#include <wx/statline.h>
#include <archive.h>
#include <archive_entry.h>


wxDEFINE_EVENT(EVENT_ADD_AUDIO, wxCommandEvent);
wxDEFINE_EVENT(EVENT_REMOVE_AUDIO, wxCommandEvent);
wxDEFINE_EVENT(EVENT_SELECT_AUDIO, wxCommandEvent);
wxDEFINE_EVENT(EVENT_ADD_LAYER, wxCommandEvent);
wxDEFINE_EVENT(EVENT_PLAY, wxCommandEvent);
wxDEFINE_EVENT(EVENT_QUIT, wxCommandEvent);
wxDEFINE_EVENT(EVENT_NEW_PROJECT, wxCommandEvent);
wxDEFINE_EVENT(EVENT_LOAD_PROJECT, wxCommandEvent);
wxDEFINE_EVENT(EVENT_LOAD_OTHER, wxCommandEvent);


BEGIN_EVENT_TABLE(StudioFrame, wxFrame)
	EVT_MENU(ID_New, StudioFrame::OnNew)
	EVT_MENU(ID_Load, StudioFrame::OnLoad)
	EVT_MENU(ID_Save, StudioFrame::OnSave)
	EVT_MENU(ID_SaveAs, StudioFrame::OnSaveAs)
	EVT_MENU(ID_Export, StudioFrame::OnExport)
	EVT_MENU(ID_Quit, StudioFrame::OnQuit)
	EVT_MENU(ID_About, StudioFrame::OnAbout)
	EVT_MENU(ID_AddMusicTrack, StudioFrame::OnAddMusicTrack)
	EVT_MENU(ID_AddSfxTrack, StudioFrame::OnAddSfxTrack)
	EVT_MENU(ID_EditMusicTrackName, StudioFrame::OnEditMusicTrackName)
	EVT_MENU(ID_EditSfxTrackName, StudioFrame::OnEditSfxTrackName)
	EVT_MENU(ID_PlaybackPanel, StudioFrame::OnPlaybackPanel)
	EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, StudioFrame::OnRecentFile)
	EVT_COMMAND(wxID_ANY, EVENT_SELECT_AUDIO, StudioFrame::OnSelectAudio)
	EVT_COMMAND(wxID_ANY, EVENT_ADD_AUDIO, StudioFrame::OnAddAudio)
	EVT_COMMAND(wxID_ANY, EVENT_REMOVE_AUDIO, StudioFrame::OnRemoveAudio)
	EVT_COMMAND(wxID_ANY, EVENT_ADD_LAYER, StudioFrame::OnAddLayer)
	EVT_COMMAND(wxID_ANY, EVENT_PLAY, StudioFrame::OnPlay)
	EVT_COMMAND(wxID_ANY, EVENT_QUIT, StudioFrame::OnQuit)
	EVT_COMMAND(wxID_ANY, EVENT_NEW_PROJECT, StudioFrame::OnNew)
	EVT_COMMAND(wxID_ANY, EVENT_LOAD_PROJECT, StudioFrame::OnLoadProject)
	EVT_COMMAND(wxID_ANY, EVENT_LOAD_OTHER, StudioFrame::OnLoad)
END_EVENT_TABLE()

StudioTimer::StudioTimer(StudioFrame* pane) : wxTimer() {
	StudioTimer::pane = pane;
}

void StudioTimer::Notify() {
	wxString str = musicList->GetItemText(labelIndex);
	studioApi->TrackRename(trackName, str.ToStdString());
	pane->UpdateTrackName(trackName, str.ToStdString());
}

void StudioFrame::UpdateTrackName(std::string trackName, std::string newName) {
	if (trackPane) {
		trackPane->UpdateTrackName(trackName, newName);
	}
	if (controlPane) {
		controlPane->UpdateTrackName(trackName, newName);
	}
	trackControl->UpdateTrackName(trackName, newName);

	Layout();
}

StudioFrame::StudioFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(NULL, -1, title, pos, size, style) {
	config = new wxConfig("oamlStudio");
	timer = NULL;
	trackPane = NULL;
	controlPane = NULL;
	rightLine = NULL;

	wxMenuBar *menuBar = new wxMenuBar;
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_New, _("&New..."));
	menuFile->AppendSeparator();
	menuFile->Append(ID_Load, _("&Load..."));
	menuFile->Append(ID_Save, _("&Save..."));
	menuFile->Append(ID_SaveAs, _("&Save As..."));
	menuFile->Append(ID_Export, _("&Export..."));
	menuFile->AppendSeparator();

	fileHistory = new wxFileHistory();

	wxMenu *recent = new wxMenu(long(0));
	menuFile->Append(ID_Recent, "Recent files", recent);
	fileHistory->UseMenu(recent);
	fileHistory->Load(*config);

	menuFile->AppendSeparator();
	menuFile->Append(ID_Quit, _("E&xit"));

	menuBar->Append(menuFile, _("&File"));

	menuFile = new wxMenu;
	menuFile->Append(ID_AddMusicTrack, _("Add &music track"));
	menuFile->Append(ID_AddSfxTrack, _("Add &sfx track"));
	menuFile->AppendSeparator();

	menuBar->Append(menuFile, _("&Tracks"));

/*	menuFile = new wxMenu;
	menuFile->Append(ID_AddAudio, _("&Add audio"));
	menuFile->AppendSeparator();

	menuBar->Append(menuFile, _("&Audios"));*/

	viewMenu = new wxMenu;
	viewMenu->AppendCheckItem(ID_PlaybackPanel, _("&Playback Panel"));
	viewMenu->AppendSeparator();

	menuBar->Append(viewMenu, _("&View"));

	menuFile = new wxMenu;
	menuFile->Append(ID_About, _("A&bout..."));
	menuFile->AppendSeparator();

	menuBar->Append(menuFile, _("&About"));

	SetMenuBar(menuBar);

	CreateStatusBar();
	SetStatusText(_("Ready"));

	mainSizer = new wxBoxSizer(wxHORIZONTAL);

	SetBackgroundColour(wxColour(0x40, 0x40, 0x40));

	// Left panel
	vSizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText *staticText = new wxStaticText(this, wxID_ANY, wxString("-- Music Tracks --"));
	vSizer->Add(staticText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 2);

	musicList = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(240, -1), wxLC_LIST | wxLC_EDIT_LABELS | wxLC_SINGLE_SEL);
	musicList->SetBackgroundColour(wxColour(0x80, 0x80, 0x80));
	musicList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &StudioFrame::OnMusicListActivated, this);
	musicList->Bind(wxEVT_RIGHT_UP, &StudioFrame::OnMusicListMenu, this);
	musicList->Bind(wxEVT_LIST_END_LABEL_EDIT, &StudioFrame::OnMusicEndLabelEdit, this);

	vSizer->Add(musicList, 1, wxALL, 5);

	staticText = new wxStaticText(this, wxID_ANY, wxString("-- Sfx Tracks --"));
	vSizer->Add(staticText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 2);

	sfxList = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(240, -1), wxLC_LIST | wxLC_EDIT_LABELS | wxLC_SINGLE_SEL);
	sfxList->SetBackgroundColour(wxColour(0x80, 0x80, 0x80));
	sfxList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &StudioFrame::OnSfxListActivated, this);
	sfxList->Bind(wxEVT_RIGHT_UP, &StudioFrame::OnSfxListMenu, this);
	sfxList->Bind(wxEVT_LIST_END_LABEL_EDIT, &StudioFrame::OnSfxEndLabelEdit, this);

	vSizer->Add(sfxList, 1, wxALL, 5);

	wxStaticLine *staticLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
	vSizer->Add(staticLine, 0, wxEXPAND | wxALL, 0);

	trackControl = new TrackControl(this, wxID_ANY);
	vSizer->Add(trackControl, 0, wxALL, 5);

	mainSizer->Add(vSizer, 0, wxEXPAND | wxALL, 0);

	staticLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
	mainSizer->Add(staticLine, 0, wxEXPAND | wxALL, 0);

	// Right panel
	vSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(vSizer, 1, wxEXPAND | wxALL, 0);

	SetSizer(mainSizer);
	Layout();

	Centre(wxBOTH);

	playFrame = new PlaybackFrame(this, wxID_ANY);
	playFrame->Show(true);
	viewMenu->Check(ID_PlaybackPanel, playFrame->IsShown());

	StartupFrame *startupFrame = new StartupFrame(this);
	startupFrame->Show(true);
}

void StudioFrame::SelectTrack(std::string name) {
	oamlTrackInfo *track = GetTrackInfo(name);
	if (track == NULL)
		return;

	if (controlPane) controlPane->Destroy();
	if (rightLine) rightLine->Destroy();
	if (trackPane) trackPane->Destroy();

	controlPane = new ControlPanel(this, wxID_ANY);
	controlPane->SetTrackMode(track->musicTrack);
	controlPane->OnSelectAudio("");
	vSizer->Add(controlPane, 0, wxEXPAND | wxALL, 5);

	rightLine = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
	vSizer->Add(rightLine, 0, wxEXPAND | wxALL, 0);

	trackPane = new TrackPanel(this, wxID_ANY, name);
	trackPane->SetTrackMode(track->musicTrack);
	vSizer->Add(trackPane, 1, wxEXPAND | wxALL, 5);

	for (size_t i=0; i<track->audios.size(); i++) {
		trackPane->AddAudio(&track->audios[i]);
	}

	SetSizer(mainSizer);
	Layout();

	controlPane->SetTrack(name);
	controlPane->OnSelectAudio("");

	trackControl->SetTrack(name);
}

void StudioFrame::OnMusicListActivated(wxListEvent& event) {
	int index = event.GetIndex();
	if (index == -1) {
		wxMessageBox(_("You must choose a track!"));
		return;
	}

	wxString str = musicList->GetItemText(index);
	SelectTrack(str.ToStdString());
}

void StudioFrame::OnMusicListMenu(wxMouseEvent& WXUNUSED(event)) {
	wxMenu menu(wxT(""));
	menu.Append(ID_AddMusicTrack, wxT("&Add Track"));
	menu.Append(ID_EditMusicTrackName, wxT("Edit Track &Name"));
	PopupMenu(&menu);
}

void StudioFrame::OnMusicEndLabelEdit(wxListEvent& event) {
	if (timer == NULL) {
		timer = new StudioTimer(this);
	}

	wxString str = musicList->GetItemText(event.GetIndex());
	timer->labelIndex = event.GetIndex();
	timer->musicList = musicList;
	timer->trackName = str.ToStdString();
	timer->StartOnce(10);
}

void StudioFrame::OnSfxListActivated(wxListEvent& event) {
	int index = event.GetIndex();
	if (index == -1) {
		wxMessageBox(_("You must choose a track!"));
		return;
	}

	wxString str = sfxList->GetItemText(index);
	SelectTrack(str.ToStdString());
}

void StudioFrame::OnSfxListMenu(wxMouseEvent& WXUNUSED(event)) {
	wxMenu menu(wxT(""));
	menu.Append(ID_AddSfxTrack, wxT("&Add Track"));
	menu.Append(ID_EditSfxTrackName, wxT("Edit Track &Name"));
	PopupMenu(&menu);
}

void StudioFrame::OnSfxEndLabelEdit(wxListEvent& event) {
	if (timer == NULL) {
		timer = new StudioTimer(this);
	}

	wxString str = sfxList->GetItemText(event.GetIndex());
	timer->labelIndex = event.GetIndex();
	timer->sfxList = sfxList;
	timer->trackName = str.ToStdString();
	timer->StartOnce(10);
}

void StudioFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	fileHistory->Save(*config);

	delete config;
	delete fileHistory;

	Close(TRUE);
}

void StudioFrame::OnNew(wxCommandEvent& WXUNUSED(event)) {
	if (trackPane) {
		trackPane->Destroy();
	}

	musicList->ClearAll();
	sfxList->ClearAll();
}

void StudioFrame::Load(std::string filename) {
	defsPath = filename;
	wxFileName fname(defsPath);
	prjPath = fname.GetPathWithSep();
	InitCallbacks(prjPath);

	if (oaml->Init(fname.GetFullName().ToStdString().c_str()) != OAML_OK) {
		wxMessageBox(_("Error loading project"));

		for (size_t i=0; i<fileHistory->GetCount(); i++) {
			if (fileHistory->GetHistoryFile(i) == fname.GetFullPath()) {
				fileHistory->RemoveFileFromHistory(i);
				break;
			}
		}
		return;
	}

	musicList->ClearAll();
	sfxList->ClearAll();

	oamlTracksInfo *info = oaml->GetTracksInfo();

	for (size_t i=0; i<info->tracks.size(); i++) {
		oamlTrackInfo *track = &info->tracks[i];
		if (track->musicTrack) {
			musicList->InsertItem(musicList->GetItemCount(), wxString(track->name));
		} else if (track->sfxTrack) {
			sfxList->InsertItem(sfxList->GetItemCount(), wxString(track->name));
		}
	}
}

void StudioFrame::OnLoadProject(wxCommandEvent& event) {
	Load(event.GetString().ToStdString());
}

void StudioFrame::OnLoad(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog openFileDialog(this, _("Open oaml.defs"), ".", "oaml.defs", "*.defs", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	wxString path = openFileDialog.GetPath();
	Load(path.ToStdString());
	fileHistory->AddFileToHistory(path);
}

void StudioFrame::OnRecentFile(wxCommandEvent& event) {
	wxString path(fileHistory->GetHistoryFile(event.GetId() - wxID_FILE1));
	if (path.empty() == false) {
		Load(path.ToStdString());
	}
}

void StudioFrame::AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, const char *value) {
	tinyxml2::XMLElement *el = node->GetDocument()->NewElement(name);
	el->SetText(value);
	node->InsertEndChild(el);
}

void StudioFrame::AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, int value) {
	tinyxml2::XMLElement *el = node->GetDocument()->NewElement(name);
	el->SetText(value);
	node->InsertEndChild(el);
}

void StudioFrame::AddSimpleChildToNode(tinyxml2::XMLNode *node, const char *name, float value) {
	tinyxml2::XMLElement *el = node->GetDocument()->NewElement(name);
	el->SetText(value);
	node->InsertEndChild(el);
}

tinyxml2::XMLNode* StudioFrame::CreateAudioDefs(tinyxml2::XMLDocument& xmlDoc, oamlAudioInfo *audio, bool createPkg) {
	tinyxml2::XMLNode *audioEl = xmlDoc.NewElement("audio");
	if (audioEl == NULL)
		return NULL;

	for (std::vector<oamlLayerInfo>::iterator layer=audio->layers.begin(); layer<audio->layers.end(); ++layer) {
		if (createPkg) {
			wxFileName fname(layer->filename);
			AddSimpleChildToNode(audioEl, "filename", fname.GetFullName().ToStdString().c_str());
		} else {
			AddSimpleChildToNode(audioEl, "filename", layer->filename.c_str());
		}
	}

	if (audio->type) AddSimpleChildToNode(audioEl, "type", audio->type);
	if (audio->volume) AddSimpleChildToNode(audioEl, "volume", audio->volume);
	if (audio->bpm) AddSimpleChildToNode(audioEl, "bpm", audio->bpm);
	if (audio->beatsPerBar) AddSimpleChildToNode(audioEl, "beatsPerBar", audio->beatsPerBar);
	if (audio->bars) AddSimpleChildToNode(audioEl, "bars", audio->bars);
	if (audio->minMovementBars) AddSimpleChildToNode(audioEl, "minMovementBars", audio->minMovementBars);
	if (audio->randomChance) AddSimpleChildToNode(audioEl, "randomChance", audio->randomChance);
	if (audio->fadeIn) AddSimpleChildToNode(audioEl, "fadeIn", audio->fadeIn);
	if (audio->fadeOut) AddSimpleChildToNode(audioEl, "fadeOut", audio->fadeOut);
	if (audio->xfadeIn) AddSimpleChildToNode(audioEl, "xfadeIn", audio->xfadeIn);
	if (audio->xfadeOut) AddSimpleChildToNode(audioEl, "xfadeOut", audio->xfadeOut);
	if (audio->condId) AddSimpleChildToNode(audioEl, "condId", audio->condId);
	if (audio->condType) AddSimpleChildToNode(audioEl, "condType", audio->condType);
	if (audio->condValue) AddSimpleChildToNode(audioEl, "condValue", audio->condValue);
	if (audio->condValue2) AddSimpleChildToNode(audioEl, "condValue2", audio->condValue2);

	return audioEl;
}

void StudioFrame::CreateTrackDefs(tinyxml2::XMLDocument& xmlDoc, oamlTrackInfo *track, bool createPkg) {
	tinyxml2::XMLNode *trackEl = xmlDoc.NewElement("track");
	if (track->sfxTrack) {
		trackEl->ToElement()->SetAttribute("type", "sfx");
	} else {
		trackEl->ToElement()->SetAttribute("type", "music");
	}

	AddSimpleChildToNode(trackEl, "name", track->name.c_str());

	if (track->groups.size() > 0) {
		for (std::vector<std::string>::iterator it=track->groups.begin(); it<track->groups.end(); ++it) {
			AddSimpleChildToNode(trackEl, "group", it->c_str());
		}
	}
	if (track->subgroups.size() > 0) {
		for (std::vector<std::string>::iterator it=track->subgroups.begin(); it<track->subgroups.end(); ++it) {
			AddSimpleChildToNode(trackEl, "subgroup", it->c_str());
		}
	}
	if (track->volume) AddSimpleChildToNode(trackEl, "volume", track->volume);
	if (track->fadeIn) AddSimpleChildToNode(trackEl, "fadeIn", track->fadeIn);
	if (track->fadeOut) AddSimpleChildToNode(trackEl, "fadeOut", track->fadeOut);
	if (track->xfadeIn) AddSimpleChildToNode(trackEl, "xfadeIn", track->xfadeIn);
	if (track->xfadeOut) AddSimpleChildToNode(trackEl, "xfadeOut", track->xfadeOut);

	for (std::vector<oamlAudioInfo>::iterator audio=track->audios.begin(); audio<track->audios.end(); ++audio) {
		tinyxml2::XMLNode *el = CreateAudioDefs(xmlDoc, &(*audio), createPkg);
		if (el != NULL) {
			trackEl->InsertEndChild(el);
		}
	}

	xmlDoc.InsertEndChild(trackEl);
}

void StudioFrame::CreateDefs(tinyxml2::XMLDocument& xmlDoc, bool createPkg) {
	xmlDoc.InsertFirstChild(xmlDoc.NewDeclaration());

	oamlTracksInfo *info = oaml->GetTracksInfo();
	for (std::vector<oamlTrackInfo>::iterator track=info->tracks.begin(); track<info->tracks.end(); ++track) {
		CreateTrackDefs(xmlDoc, &(*track), createPkg);
	}
}

void StudioFrame::ReloadDefs() {
/*	tinyxml2::XMLDocument xmlDoc;
	tinyxml2::XMLPrinter printer;

	CreateDefs(xmlDoc);
	xmlDoc.Accept(&printer);
	oaml->InitString(printer.CStr());*/
}

void StudioFrame::OnSave(wxCommandEvent& WXUNUSED(event)) {
	tinyxml2::XMLDocument xmlDoc;

	CreateDefs(xmlDoc);
	xmlDoc.SaveFile(defsPath.c_str());
}

void StudioFrame::OnSaveAs(wxCommandEvent& event) {
	wxFileDialog openFileDialog(this, _("Save oaml.defs"), ".", "oaml.defs", "*.defs", wxFD_SAVE);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	defsPath = openFileDialog.GetPath();
	OnSave(event);
}

int StudioFrame::WriteDefsToZip(struct archive *zip) {
	tinyxml2::XMLDocument xmlDoc;
	tinyxml2::XMLPrinter printer;

	CreateDefs(xmlDoc, true);
	xmlDoc.Accept(&printer);
	const char *buffer = printer.CStr();

	struct archive_entry *entry = archive_entry_new();
	if (entry == NULL) {
		wxMessageBox(_("archive_entry_new error"));
		return -1;
	}

	archive_entry_set_pathname(entry, "oaml.defs");
	archive_entry_set_size(entry, strlen(buffer));
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);
	archive_write_header(zip, entry);
	if (archive_write_data(zip, buffer, strlen(buffer)) != strlen(buffer)) {
		wxMessageBox(_("archive_write_data error"));
		return -1;
	}
	archive_entry_free(entry);

	return 0;
}

int StudioFrame::WriteFileToZip(struct archive *zip, std::string file) {
	const char *filename = file.c_str();
	void *fd = studioCbs.open(filename);
	if (fd == NULL) {
		wxString str;
		str.Printf(wxT("Error creating file %s"), filename);
		wxMessageBox(str);

		return -1;
	}

	studioCbs.seek(fd, 0, SEEK_END);
	size_t size = studioCbs.tell(fd);
	studioCbs.seek(fd, 0, SEEK_SET);

	wxFileName fname(filename);

	struct archive_entry *entry = archive_entry_new();
	if (entry == NULL) {
		wxMessageBox(_("archive_entry_new error"));
		return -1;
	}

	archive_entry_set_pathname(entry, fname.GetFullName().ToStdString().c_str());
	archive_entry_set_size(entry, size);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);
	archive_write_header(zip, entry);

	char buffer[4096];
	while (size > 0) {
		int bytes = studioCbs.read(buffer, 1, 4096, fd);
		if (bytes == 0) break;

		if (archive_write_data(zip, buffer, bytes) != bytes) {
			studioCbs.close(fd);
			wxMessageBox(_("archive_write_data error"));
			return -1;
		}
	}

	studioCbs.close(fd);
	archive_entry_free(entry);

	return 0;
}

int StudioFrame::CreateZip(std::string zfile, std::vector<std::string> files) {
	struct archive *zip;

	zip = archive_write_new();
	if (zip == NULL) {
		wxMessageBox(_("archive_write_new error"));
		return -1;
	}
	archive_write_set_format_zip(zip);
//	archive_write_zip_set_compression_store(zip);
	archive_write_open_filename(zip, zfile.c_str());

	if (WriteDefsToZip(zip)) {
		archive_write_close(zip);
		archive_write_finish(zip);
		return -1;
	}

	for (size_t i=0; i<files.size(); i++) {
		if (WriteFileToZip(zip, files[i])) {
			archive_write_close(zip);
			archive_write_finish(zip);
			return -1;
		}
	}

	archive_write_close(zip);
	archive_write_finish(zip);

	return 0;
}

void StudioFrame::OnExport(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> list;

	oamlTracksInfo* info = oaml->GetTracksInfo();
	for (size_t i=0; i<info->tracks.size(); i++) {
		for (size_t j=0; j<info->tracks[i].audios.size(); j++) {
			for (size_t k=0; k<info->tracks[i].audios[j].layers.size(); k++) {
				list.push_back(info->tracks[i].audios[j].layers[k].filename);
			}
		}
	}

	wxFileDialog openFileDialog(this, _("Save oamlPackage.zip"), ".", "oamlPackage.zip", "*.zip", wxFD_SAVE);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	CreateZip(wxString(openFileDialog.GetPath()).ToStdString(), list);
}

void StudioFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	wxString str;

	// TODO - Make a nice custom dialog

	str.Printf("oamlStudio - Studio for Open Adaptive Music Library\r\n"
		   "oaml and oamlStudio are both licensed under the MIT license.\r\n"
		   "\r\n"
		   "https://github.com/marcelofg55/oaml\r\n"
		   "ttps://github.com/marcelofg55/oamlStudio\r\n"
		   "Copyright (c) 2015-2016 Marcelo Fernandez");

	wxMessageBox(str, _("About oamlStudio"), wxOK | wxICON_INFORMATION, this);
}

void StudioFrame::AddTrack(std::string name) {
	studioApi->TrackNew(name);
}

void StudioFrame::OnAddMusicTrack(wxCommandEvent& WXUNUSED(event)) {
	oamlTracksInfo *info = oaml->GetTracksInfo();
	int index = info ? info->tracks.size() : 0;

	char name[1024];
	snprintf(name, 1024, "Track%d", index);
	AddTrack(std::string(name));

	musicList->InsertItem(index, wxString(name));
	SelectTrack(name);

	musicList->EditLabel(index);
}

void StudioFrame::OnAddSfxTrack(wxCommandEvent& WXUNUSED(event)) {
	oamlTracksInfo *info = oaml->GetTracksInfo();
	int index = info ? info->tracks.size() : 0;

	char name[1024];
	snprintf(name, 1024, "Track%d", index);
	AddTrack(std::string(name));

	sfxList->InsertItem(index, wxString(name));
	SelectTrack(name);

	sfxList->EditLabel(index);
}

void StudioFrame::OnEditMusicTrackName(wxCommandEvent& WXUNUSED(event)) {
	musicList->EditLabel(musicList->GetFirstSelected());
}

void StudioFrame::OnEditSfxTrackName(wxCommandEvent& WXUNUSED(event)) {
	sfxList->EditLabel(sfxList->GetFirstSelected());
}

void StudioFrame::OnSelectAudio(wxCommandEvent& event) {
	if (controlPane) {
		controlPane->OnSelectAudio(event.GetString().ToStdString());
	}
}

void StudioFrame::OnAddAudio(wxCommandEvent& event) {
	if (controlPane == NULL)
		return;

	ReloadDefs();

	oamlAudioInfo info;
	oamlRC rc = GetAudioInfo(controlPane->GetTrackName(), event.GetString().ToStdString(), &info);
	if (rc != OAML_OK)
		return;

	trackPane->AddAudio(&info);
	ReloadDefs();

	SetSizer(mainSizer);
	Layout();
}

void StudioFrame::OnAddLayer(wxCommandEvent& event) {
/*	oamlAudioInfo* audio = GetAudioInfo(controlPane->GetTrackName(), event.GetString().ToStdString());
	if (audio == NULL)
		return;

	trackPane->AddAudio(audio);
	ReloadDefs();

	SetSizer(mainSizer);
	Layout();*/
}

void StudioFrame::OnRemoveAudio(wxCommandEvent& event) {
	if (controlPane) {
		controlPane->OnSelectAudio("");
	}

	trackPane->RemoveAudio(event.GetString().ToStdString());

	SetSizer(mainSizer);
	Layout();
}

void StudioFrame::OnPlay(wxCommandEvent& WXUNUSED(event)) {
	if (controlPane == NULL)
		return;

	if (controlPane->IsMusicMode()) {
		oaml->PlayTrack(controlPane->GetTrack());
	} else {
		oaml->PlaySfx(controlPane->GetSelectedAudioName());
	}
}

void StudioFrame::OnPlaybackPanel(wxCommandEvent& WXUNUSED(event)) {
	bool show = playFrame->IsShown() ? false : true;
	playFrame->Show(show);
	viewMenu->Check(ID_PlaybackPanel, show);
}
